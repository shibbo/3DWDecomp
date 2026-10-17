#include "Demo/DemoScenePlayer.hpp"

#include "Demo/DemoActionList.hpp"
#include "Demo/DemoPlayerLocator.hpp"
#include "Demo/DemoSceneActor.hpp"
#include "Demo/DemoScenePlayerModel.hpp"
#include "Demo/IDemoScenePlayerInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <prim/seadEnum.h>

namespace {
NERVE_DECL(DemoScenePlayer, Wait);
NERVE_DECL(DemoScenePlayer, Play);
NERVES_MAKE_NOSTRUCT(DemoScenePlayer, Wait, Play)
SEAD_ENUM(SeMaterialCode, NoCode, Stone, Sand, Carpet, Soil)
}

/** @brief Captures the active users and their current power-up forms. */
class DemoScenePlayerInfo : public IDemoScenePlayerInfo {
public:
    void setup(DemoScenePlayer* pPlayer) override;
    /** @brief Gets participant count. @return Active player count. */
    int getActivePlayerNum() const override { return mCount; }
    /** @brief Gets a participant's character. @param index Participant index. @return Character ID. */
    int getCharacterType(int index) const override { return mCharacters[index]; }
    /** @brief Gets a participant's power-up. @param index Participant index. @return Figure ID. */
    int getFigureType(int index) const override { return mFigures[index]; }
private:
    int mCount = 0;
    int mCharacters[4];
    int mFigures[4];
};

/** @brief Creates the player coordinator. @param pName Actor name. @param pHolder Demo owner.
 * @param pSelector Player retargeting selector. */
DemoScenePlayer::DemoScenePlayer(const char* pName, DemoSceneActorHolder* pHolder,
    PlayerRetargettingSelector* pSelector) : al::LiveActor(pName), mHolder(pHolder), mSelector(pSelector) {
    mDelayFrames = new int[3];
    for (int i = 0; i < 3; i++)
        mDelayFrames[i] = 0;
    mPlacementMtx.makeIdentity();
    mLocalMtx.makeIdentity();
    mModels = new DemoScenePlayerModel*[rc::getPlayerCharacterNumMax()];
    for (int i = 0; i < rc::getPlayerCharacterNumMax(); i++)
        mModels[i] = nullptr;
}

/** @brief Overrides participant selection. @param pInfo Participant provider. */
void DemoScenePlayer::setDemoScenePlayerInfo(IDemoScenePlayerInfo* pInfo) { mPlayerInfo = pInfo; }

/** @brief Initializes locators, actions, and placement. @param rInfo Actor placement.
 * @param rDemoInfo Demo placement. @param pMtx Optional parent transform.
 * @param onlySuper Whether to use only the Super figure. */
void DemoScenePlayer::initDemoSceneActor(const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pMtx, bool onlySuper) {
    mOnlySuper = onlySuper;
    mLocator = new DemoPlayerLocator("デモプレイヤー位置制御");
    mLocator->initDemoSceneActor(rInfo, rDemoInfo, pMtx);
    init(rInfo);
    mActions = new DemoActionList(rInfo, "Action");
    DemoSceneActorFunction::calcPlacementBaseMtx(&mPlacementMtx, rInfo, rDemoInfo, pMtx, &mLocalMtx);
}

/** @brief Updates parent-relative placement. @param pMtx Parent transform. */
void DemoScenePlayer::setPlacementBaseMtx(const sead::Matrix34f* pMtx) { mPlacementMtx.setMul(*pMtx, mLocalMtx); }

/** @brief Reads action delays and requests the needed player models. @param rInfo Placement data. */
void DemoScenePlayer::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorUpdate(this, rInfo, "デモプレイヤー前処理");
    al::initNerve(this, &NrvDemoScenePlayerWait, 0);
    mDelayFrames[0] = 0;
    al::tryGetArg(&mDelayFrames[0], rInfo, "DelayFrame1");
    int delay = mDelayFrames[0];
    for (int i = 1; i < 3; i++) {
        mDelayFrames[i] = delay;
        if (al::tryGetArg(&mDelayFrames[i], rInfo, al::StringTmp<32>("DelayFrame%d", i + 1).cstr()))
            delay = mDelayFrames[i];
    }
    mSeMaterialCode = 0;
    al::tryGetArg(&mSeMaterialCode, rInfo, "SeMaterialCode");
    const char* suffix = nullptr;
    al::tryGetStringArg(&suffix, rInfo, "PlayerModelSuffixName");
    if (mOnlySuper) {
        for (int i = 0; i < rc::getPlayerCharacterNumMax(); i++)
            rc::requestCreateFigureSuper(this, i, suffix);
    } else {
        rc::requestCreateAllPlayer(this, suffix);
    }
    makeActorAppeared();
}

/** @brief Completes placement without additional work. */
void DemoScenePlayer::initAfterPlacement() {}
/** @brief Leaves playback updates to the active nerve. */
void DemoScenePlayer::control() {}

/** @brief Records active characters and figures. @param pPlayer Demo player actor. */
void DemoScenePlayerInfo::setup(DemoScenePlayer* pPlayer) {
    int users[4];
    mCount = rc::findActiveUserIdList(users, GameDataHolderAccessor(pPlayer));
    for (int i = 0; i < mCount; i++) {
        int user = users[i];
        al::LiveActor* player = rc::tryFindAlivePlayerActorFirstByUserId(pPlayer, user);
        mCharacters[i] = rc::getControlUserCharacterType(GameDataHolderAccessor(pPlayer), user);
        mFigures[i] = player ? rc::getPlayerFigureTypeWithNext(player) : 0;
    }
}

/** @brief Selects player models and starts them at the demo placement. @param rMtx Demo transform. */
void DemoScenePlayer::startDemo(const sead::Matrix34f& rMtx) {
    sead::Matrix34f mtx;
    mtx.setMul(rMtx, mPlacementMtx);
    for (int i = 0; i < rc::getPlayerCharacterNumMax(); i++)
        mModels[i] = nullptr;
    IDemoScenePlayerInfo* info = mPlayerInfo;
    DemoScenePlayerInfo defaultInfo;
    if (!info)
        info = &defaultInfo;
    info->setup(this);
    mPlayerCount = info->getActivePlayerNum();
    if (mLocator)
        mLocator->startDemo(rMtx, mPlayerCount);
    for (int i = 0; i < mPlayerCount; i++) {
        int character = info->getCharacterType(i);
        int figure = mOnlySuper ? 0 : info->getFigureType(i);
        mModels[i] = rc::getDemoPlayerModel(this, character);
        mModels[i]->setFigure(figure);
        mModels[i]->setFollowMtx(mLocator->getPlayerLocatorMtxPtr(i));
        if (mSeMaterialCode)
            mModels[i]->setSeMaterialCode(SeMaterialCode::text(mSeMaterialCode));
    }
    for (int i = 0; i < mPlayerCount; i++)
        if (mModels[i])
            mModels[i]->startDemo(mtx);
}

/** @brief Starts an action with staggered participant delays. @param index Action index. */
void DemoScenePlayer::startAction(int index) {
    const char* action = mActions->getActionName(index);
    if (DemoSceneActorFunction::isHideAction(action)) {
        if (!al::isHideModel(this))
            al::hideModel(this);
    } else if (DemoSceneActorFunction::isShowAction(action)) {
        if (al::isHideModel(this))
            al::showModel(this);
    } else {
        if (mLocator)
            mLocator->startAction(index);
        if (action) {
            if (al::isHideModel(this))
                al::showModel(this);
            for (int i = 0; i < mPlayerCount; i++) {
                if (mModels[i]) {
                    mModels[i]->mStartDelay = mDelayFrames[index] * i;
                    mModels[i]->startAction(action);
                }
            }
        }
        al::setNerve(this, &NrvDemoScenePlayerPlay);
    }
}

/** @brief Stops locators and releases all acquired models. */
void DemoScenePlayer::endDemo() {
    if (mLocator)
        mLocator->endDemo();
    for (int i = 0; i < mPlayerCount; i++) {
        if (mModels[i]) {
            rc::releaseDemoPlayerModel(mModels[i]);
            mModels[i] = nullptr;
        }
    }
    al::setNerve(this, &NrvDemoScenePlayerWait);
}

/** @brief Removes effects from active player models. */
void DemoScenePlayer::killAllEffect() {
    for (int i = 0; i < mPlayerCount; i++)
        if (mModels[i])
            mModels[i]->killAllEffect();
}

/** @brief Gets a participant model. @param index Participant index. @return Model or null beyond the active count. */
al::LiveActor* DemoScenePlayer::getPlayerModel(int index) {
    return index < mPlayerCount ? mModels[index]->getModel() : nullptr;
}
/** @brief Finds a participant model. @param index Participant index. @return Model or null beyond the active count. */
al::LiveActor* DemoScenePlayer::tryGetPlayerModel(int index) const {
    return index < mPlayerCount ? mModels[index]->getModel() : nullptr;
}
/** @brief Waits for playback to begin. */
void DemoScenePlayer::exeWait() {}
/** @brief Advances each active player model. */
void DemoScenePlayer::exePlay() {
    for (int i = 0; i < mPlayerCount; i++)
        if (mModels[i])
            mModels[i]->update(this);
}
