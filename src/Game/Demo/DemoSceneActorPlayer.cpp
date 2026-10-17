#include "Demo/DemoSceneActorPlayer.hpp"

#include "Demo/DemoActionList.hpp"
#include "Demo/DemoScenePlayerModel.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Player/Normal/PlayerModel.hpp"
#include "Project/Play/Actor/ActorAlphaCtrl.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataConst.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace DemoSceneActorFunction {
void calcPlacementBaseMtx(sead::Matrix34f* pOut, const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pBaseMtx, sead::Matrix34f* pLocalMtx);
}

namespace {
NERVE_DECL(DemoSceneActorPlayer, Wait);
NERVE_DECL(DemoSceneActorPlayer, Play);
NERVES_MAKE_NOSTRUCT(DemoSceneActorPlayer, Wait, Play)
}

/** @brief Creates a demo player with identity placement transforms. @param pName Actor name.
 * @param pHolder Owning demo scene. @param pSelector Retargeting configuration. */
DemoSceneActorPlayer::DemoSceneActorPlayer(const char* pName, DemoSceneActorHolder* pHolder,
    PlayerRetargettingSelector* pSelector)
    : al::LiveActor(pName), mHolder(pHolder), mSelector(pSelector) {
    mPlacementMtx.makeIdentity();
    mLocalMtx.makeIdentity();
}

/** @brief Initializes the player and action list. @param rInfo Actor placement.
 * @param rDemoInfo Parent demo placement. @param pMtx Optional parent transform. */
void DemoSceneActorPlayer::initDemoSceneActor(const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pMtx) {
    init(rInfo);
    mActions = new DemoActionList(rInfo, "Action");
    DemoSceneActorFunction::calcPlacementBaseMtx(&mPlacementMtx, rInfo, rDemoInfo, pMtx, &mLocalMtx);
}

/** @brief Updates placement relative to the demo. @param pMtx Parent transform. */
void DemoSceneActorPlayer::setPlacementBaseMtx(const sead::Matrix34f* pMtx) {
    mPlacementMtx.setMul(*pMtx, mLocalMtx);
}

/** @brief Loads the character, figure, and model-sharing settings. @param rInfo Placement data. */
void DemoSceneActorPlayer::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorUpdate(this, rInfo, "デモプレイヤー前処理");
    al::initNerve(this, &NrvDemoSceneActorPlayerWait, 0);
    const char* modelName = nullptr;
    if (alPlacementFunction::tryGetModelName(&modelName, rInfo)) {
        mCharacter = GameDataConst::getPlayerCharacterTypeFromName(modelName);
        const char* suffix = nullptr;
        al::tryGetStringArg(&suffix, rInfo, "SuffixName");
        mSingleMode = al::isSingleMode(rInfo);
        if (mSingleMode) {
            bool noShadow = false;
            al::tryGetArg(&noShadow, rInfo, "IsNoShadowMaskInSingleMode");
            mShowShadow = !noShadow;
        }
        al::tryGetArg(&mUniqueModel, rInfo, "IsUniquePlayerModel");
        al::tryGetArg(&mIgnoreSingleModeCharID, rInfo, "IsIgnoreSingleModeCharID");
        if (mAlphaCtrl)
            mAlphaCtrl->setOn(false);
        int figure = 0;
        al::tryGetArg(&figure, rInfo, "FigureType");
        if (mSingleMode && figure == 1 &&
            SingleModeDataFunction::isMeowserJrAvailable(GameDataHolderAccessor(this)))
            figure = 2;
        if (figure == 1) {
            mFigure = 3;
            if (!mUniqueModel)
                rc::requestCreateFigureClimb(this, mCharacter, suffix);
        } else if (figure == 2) {
            mFigure = 9;
            if (!mUniqueModel)
                rc::requestCreateFigureClimbGiga(this, mCharacter, suffix);
        } else if (!mUniqueModel) {
            rc::requestCreateFigureSuper(this, mCharacter, suffix);
        }
        if (mUniqueModel) {
            mModel = DemoScenePlayerModel::createSingle(rInfo,
                GameDataConst::getPlayerCharacterName(mCharacter), mFigure, mSelector, this, suffix);
            mModel->kill();
        } else {
            mModel = nullptr;
        }
    } else {
        al::initActorPoseTRSV(this);
    }
    makeActorDead();
}

/** @brief No additional placement initialization is required. */
void DemoSceneActorPlayer::initAfterPlacement() {}

/** @brief Acquires a player model and starts the demo. @param rMtx Demo transform. */
void DemoSceneActorPlayer::startDemo(const sead::Matrix34f& rMtx) {
    sead::Matrix34f mtx;
    mtx.setMul(rMtx, mPlacementMtx);
    if (!mUniqueModel) {
        mModel = rc::getDemoPlayerModel(this, mCharacter);
        if (mSingleMode && !mIgnoreSingleModeCharID) {
            al::LiveActor* player = rc::getActivePlayer(this);
            if (rc::isReallyPlayerActor(player)) {
                int figure = rc::getPlayerFigureType(player);
                mModel->setFigure(figure);
            } else {
                mModel->setFigure(mFigure);
            }
        } else if (mIgnoreSingleModeCharID) {
            mModel->setFigure(mFigure);
        }
    }
    if (mSingleMode)
        al::setShadowDropLength(mModel->getPlayerModel(), 90.0f);
    mModel->startDemo(mtx);
    makeActorAppeared();
    if (mShowShadow)
        mModel->showShadow();
}

/** @brief Starts an indexed action. @param index Action list index. */
void DemoSceneActorPlayer::startAction(int index) {
    const char* action = mActions->getActionName(index);
    if (action)
        mModel->startAction(action);
    al::setNerve(this, &NrvDemoSceneActorPlayerPlay);
    if (mShowShadow)
        mModel->showShadow();
}

/** @brief Starts an available action or hides the model. @param pName Requested action.
 * @return Whether the action exists and started. */
bool DemoSceneActorPlayer::tryStartActionByName(const char* pName) {
    if (pName && al::isExistAction(mModel->getModel(), pName)) {
        mModel->startAction(pName);
        al::setNerve(this, &NrvDemoSceneActorPlayerPlay);
        al::showModelIfHide(mModel->getModel());
        if (mShowShadow)
            mModel->showShadow();
        return true;
    }
    al::hideModelIfShow(mModel->getModel());
    if (mShowShadow)
        mModel->hideShadow();
    return false;
}

/** @brief Checks model animation completion. @return Whether the current action ended. */
bool DemoSceneActorPlayer::isModelActionEnd() const { return al::isActionEnd(mModel->getModel()); }

/** @brief Releases the demo model and returns to waiting. */
void DemoSceneActorPlayer::endDemo() {
    if (mUniqueModel)
        mModel->kill();
    else
        rc::releaseDemoPlayerModel(mModel);
    makeActorDead();
    al::setNerve(this, &NrvDemoSceneActorPlayerWait);
}

/** @brief Kills every effect on the active demo model. */
void DemoSceneActorPlayer::killAllEffect() {
    if (mModel)
        mModel->killAllEffect();
}

/** @brief Updates the demo model from this actor. */
void DemoSceneActorPlayer::exePlay() { mModel->update(this); }
/** @brief Waits for playback to begin. */
void DemoSceneActorPlayer::exeWait() {}
