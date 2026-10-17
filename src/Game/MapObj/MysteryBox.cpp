#include "MapObj/MysteryBox.hpp"

#include <attributes.h>

#include "Layout/CounterMysteryBox.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/MysteryBoxConnector.hpp"
#include "MapObj/MysteryHouseChecker.hpp"
#include "MapObj/WarpCubeBindPuppeteer.hpp"
#include "MapObj/WarpObjUtil.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(MysteryBox, Wait)
NERVE_DECL(MysteryBox, AppearWait)
NERVE_DECL(MysteryBox, PlayerOut)
NERVE_DECL(MysteryBox, WarpReturn)
NERVE_DECL(MysteryBox, AllBindForceInit)
NERVE_DECL(MysteryBox, PlayerIn)
NERVE_DECL(MysteryBox, WaitWarpReturn)
NERVE_DECL(MysteryBox, CountDownIntro)
NERVE_DECL(MysteryBox, CountDown)
NERVE_DECL(MysteryBox, Appear)
NERVE_DECL(MysteryBox, WaitWarpBegin)
NERVE_DECL(MysteryBox, WarpBegin)
NERVE_DECL(MysteryBox, AllBindForce)
NERVE_DECL(MysteryBox, AllBindForceIn)

NERVES_MAKE_NOSTRUCT(MysteryBox, Wait, AppearWait, PlayerOut, WarpReturn, AllBindForceInit,
                     PlayerIn, WaitWarpReturn, CountDownIntro, CountDown, Appear, WaitWarpBegin,
                     WarpBegin, AllBindForce, AllBindForceIn)

/// How the bind of the players ends when they come out in the challenge room.
PlayerBindEndParam sWarpBeginEndParam = {{}, 0, 30, true, true, true, 0, 1.2f, 30};

/// How the bind of the players ends when they come back out of the entrance box.
PlayerBindEndParam sWarpReturnEndParam = {{}, 0, 30, true, true, true, 0, 1.2f, 30};

/// Local velocity of the players jumping out of the entrance box on their return.
const sead::Vector3f cReturnVelocity(0.0f, 10.0f, 0.0f);

/// Shadow mask size of the box while players go in or come out of it.
const sead::Vector3f cShadowMaskSizeWarp(170.0f, 500.0f, 170.0f);

/// Number of steps of the count down (ten seconds).
constexpr s32 cCountDownStep = 600;

/**
 * @brief Blend the shadow mask size of an actor over a range of action frames.
 * @param pActor The actor whose shadow is updated.
 * @param rFrom Size at the start of the range.
 * @param rTo Size at the end of the range.
 * @param actionFrame Current action frame.
 * @param start First frame of the range.
 * @param end Last frame of the range.
 */
ALWAYS_INLINE void lerpShadowMaskSize(al::LiveActor* pActor, const sead::Vector3f& rFrom,
                                      const sead::Vector3f& rTo, f32 actionFrame, s32 start,
                                      s32 end) {
    s32 frame = static_cast<s32>(actionFrame);

    if (frame >= start && frame <= end) {
        sead::Vector3f size = {1.0f, 1.0f, 1.0f};
        f32 rate = al::normalize(static_cast<f32>(frame), static_cast<f32>(start),
                                 static_cast<f32>(end));
        al::lerpVec(&size, rFrom, rTo, rate);
        al::setShadowMaskSize(pActor, "シャドウマスク", size);
    }
}

/**
 * @brief Compute the side offset of a player lined up with the others in front of a box.
 * @param index Index of the player in the line.
 * @param num Number of players in the line.
 * @return Offset along the side axis of the box.
 */
ALWAYS_INLINE f32 calcPlayerSideOffset(s32 index, s32 num) {
    return ((num - 1) * 0.5f - index) * 150.0f;
}

/**
 * @brief Compute where a player is placed when it comes out of a box.
 * @param index Index of the player among the warped players.
 * @param num Number of warped players.
 * @return Offset from the box, in its local space.
 */
ALWAYS_INLINE sead::Vector3f calcPlayerLocalOffset(s32 index, s32 num) {
    return {calcPlayerSideOffset(index, num), 400.0f, 0.0f};
}
}  // namespace

/**
 * @brief Construct a mystery box.
 * @param pName Name of the actor.
 */
MysteryBox::MysteryBox(const char* pName)
    : al::LiveActor(pName), mPlacementId(new al::PlacementId()) {}

/**
 * @brief Initialize the box, or the destination box of the challenge room when it holds the
 * green star.
 * @param rInfo The actor init info.
 */
void MysteryBox::init(const al::ActorInitInfo& rInfo) {
    s32 effectType = 0;
    al::tryGetArg(&effectType, rInfo, "EffectType");
    const char* suffix = effectType == 1 ? "TeresaMansion" : nullptr;
    al::tryGetArg(&mSeType, rInfo, "SeType");

    if (al::calcLinkChildNum(rInfo, "GreenStar") >= 1) {
        mGreenStar = new GreenStar("グリーンスター");
        al::initLinksActor(mGreenStar, rInfo, "GreenStar", 0);
        al::initActorSceneInfo(this, rInfo);
        al::initActorPoseTRSV(this);
        al::initActorSRT(this, rInfo);
        al::initStageSwitch(this, rInfo);
        al::listenStageSwitchOn(this, "SwitchEnd", al::Functor(this, &MysteryBox::endSwitchOn));
        makeActorDead();

        if (!mIsDest) {
            MysteryBoxConnectorFunction::registerDestMysteryBox(this);
            al::tryGetArg(reinterpret_cast<s32*>(&mPlayerAppearPos), rInfo, "PlayerAppearPos");
        }

        return;
    }

    al::initActorWithArchiveName(this, rInfo, "MysteryBox", suffix);
    al::initNerve(this, &NrvMysteryBoxWait, 0);
    al::calcShadowMaskSize(&mShadowMaskSize, this, "シャドウマスク");
    al::tryAddDisplayOffset(this, rInfo);
    mPuppeteerGroup = new BindPuppeteerGroup("ミステリーボックスバインド操作グループ",
                                             al::getPlayerNumMax(this));

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNumMax(); i++) {
        mPuppeteerGroup->registerPuppeteer(
            new WarpCubeBindPuppeteer("ミステリーボックスバインド操作", rInfo));
    }

    mBindPuppeteers.allocBuffer(mPuppeteerGroup->getPuppeteerNumMax(), nullptr);
    s32 destNum = al::calcLinkChildNum(rInfo, "DestMysteryBox");

    if (destNum > 0) {
        auto* destBox = new MysteryBox("ミステリーボックス");
        destBox->mIsDest = true;
        al::initLinksActor(destBox, rInfo, "DestMysteryBox", 0);
        destBox->makeActorDead();
        setDestMysteryBox(destBox);
    } else if (!mIsDest) {
        MysteryBoxConnectorFunction::registerSrcMysteryBox(this);
    }

    if (al::calcLinkChildNum(rInfo, "EndMysteryBox") >= 1) {
        mEndMtx = new sead::Matrix34f();
        al::getLinksMatrix(mEndMtx, rInfo, "EndMysteryBox");
    }

    if (!al::tryGetPlacementID(mPlacementId, rInfo)) {
        makeActorDead();
        return;
    }

    if (destNum >= 1) {
        al::PlacementInfo destInfo;
        al::getLinksInfo(&destInfo, al::getPlacementInfo(rInfo), "DestMysteryBox");
        al::tryGetArg(reinterpret_cast<s32*>(&mPlayerAppearPos), destInfo, "PlayerAppearPos");
    }

    mCounter = new CounterMysteryBox(al::getLayoutInitInfo(rInfo));
    mMtxConnector = al::tryCreateMtxConnector(this, rInfo);
    al::trySyncStageSwitchAppear(this);
    al::listenStageSwitchOn(this, "SwitchEnd", al::Functor(this, &MysteryBox::endSwitchOn));
    MysteryHouseCheckerFunction::tryRegisterMysteryBox(this);
}

/**
 * @brief Hide the counter and stop the count down music.
 */
inline void MysteryBox::endCountDown() {
    mCounter->kill();
    al::stopBgm(this, "Porter", -1, -1);
}

/**
 * @brief Bring the players back when the end switch of the challenge room turns on.
 */
void MysteryBox::endSwitchOn() {
    if (isCountDown()) {
        endCountDown();
        al::setNerve(this, &NrvMysteryBoxWaitWarpReturn);
    }
}

/**
 * @brief Set the destination box of the challenge room.
 * @param pDestBox The destination box.
 */
void MysteryBox::setDestMysteryBox(const MysteryBox* pDestBox) {
    mDestMtx = new sead::Matrix34f();
    al::makeMtxRT(mDestMtx, pDestBox);
    mGreenStar = pDestBox->mGreenStar;
    mPlayerAppearPos = pDestBox->mPlayerAppearPos;
}

/**
 * @brief Attach the box to the collision below it, if it is connected to one.
 */
void MysteryBox::initAfterPlacement() {
    if (mMtxConnector != nullptr) {
        al::attachMtxConnectorToCollision(mMtxConnector, this, false);
    }
}

/**
 * @brief Appear hidden, then show up after a moment.
 */
void MysteryBox::appear() {
    al::hideModel(this);
    al::invalidateHitSensors(this);
    al::setNerve(this, &NrvMysteryBoxAppearWait);
    al::LiveActor::appear();
}

/**
 * @brief Disappear with the hit reaction.
 */
void MysteryBox::kill() {
    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
}

/**
 * @brief Follow the collision the box is connected to.
 */
void MysteryBox::control() {
    if (mMtxConnector != nullptr) {
        al::connectPoseQT(this, mMtxConnector);
    }
}

/**
 * @brief Handle the binds of the players entering the box.
 * @param pMsg The received message.
 * @param pOther The sensor of the sender.
 * @param pSelf The sensor of the box.
 * @return Whether the message was handled.
 */
bool MysteryBox::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (al::isMsgBindGiant(pMsg)) {
        return true;
    }

    if (al::isMsgBindStart(pMsg)) {
        if (al::isNerve(this, &NrvMysteryBoxPlayerOut)) {
            return false;
        }

        // The result is unused in the game as well.
        al::isNerve(this, &NrvMysteryBoxWarpReturn);
        return true;
    }

    if (al::isMsgBindInit(pMsg)) {
        if (al::isNerve(this, &NrvMysteryBoxWait)) {
            WarpObjUtil::stopStageTimer(this);
        }

        auto* puppeteer =
            mPuppeteerGroup->getPuppeteerByPlayerIndex<WarpCubeBindPuppeteer>(pOther);

        if (al::isNerve(this, &NrvMysteryBoxAllBindForceInit)) {
            puppeteer->startBindForce(pOther, pSelf, al::getTrans(this));
            mBindForceNum++;
        } else {
            if (!al::isNerve(this, &NrvMysteryBoxWarpReturn)) {
                al::setNerve(this, &NrvMysteryBoxPlayerIn);
            }

            puppeteer->startBindInStart(pOther, pSelf);
        }

        al::sendMsgWarpStart(pOther, pSelf);
        s32 userId = rc::findControlUserId(rc::getPuppetSensor(puppeteer->getPlayerPuppet()));

        // Keep the bound players sorted by user id.
        for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
            IUsePlayerPuppet* puppet = mBindPuppeteers[i]->getPlayerPuppet();

            if (userId < rc::findControlUserId(rc::getPuppetSensor(puppet))) {
                mBindPuppeteers.insert(i, puppeteer);
                return true;
            }
        }

        mBindPuppeteers.pushBack(puppeteer);
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
            if (mBindPuppeteers[i]->tryCancelBind(pMsg, pOther)) {
                mBindPuppeteers.erase(i);

                if (mBindPuppeteers.isEmpty()) {
                    al::setNerve(this, &NrvMysteryBoxWait);
                    WarpObjUtil::restartStageTimer(this);
                    rc::cancelRequestBindAndResetDisableReviveBubbleForAllPlayer(this, pSelf);
                }

                return true;
            }
        }

        return false;
    }

    return false;
}

/**
 * @brief Stop the count down.
 * @param isKill Whether the box is killed and the music areas are enabled again.
 * @return Whether the count down was running.
 */
bool MysteryBox::tryCancelCountDown(bool isKill) {
    if (!isCountDown()) {
        return false;
    }

    endCountDown();

    if (isKill) {
        al::enableBgmChangeArea(this);
        kill();
    }

    return true;
}

/**
 * @brief Check whether the count down of the challenge room is running.
 * @return Whether the box counts down.
 */
bool MysteryBox::isCountDown() const {
    return al::isNerve(this, &NrvMysteryBoxCountDownIntro) ||
           al::isNerve(this, &NrvMysteryBoxCountDown);
}

/**
 * @brief Check whether the count down of the challenge room is over.
 * @return Whether the time ran out.
 */
bool MysteryBox::isCountDownEnd() const {
    return al::isNerve(this, &NrvMysteryBoxCountDown) &&
           al::isGreaterEqualStep(this, cCountDownStep + 60);
}

/**
 * @brief Release a bound player at a place, making it jump out.
 * @param pPuppeteer The puppeteer of the player.
 * @param rTrans Where the player is placed.
 * @param rFront Front direction of the player.
 * @param rLocalVelocity Velocity of the player, relative to its front direction.
 * @param pEndParam How the bind ends.
 */
void MysteryBox::warpPlayerPuppet(WarpCubeBindPuppeteer* pPuppeteer, const sead::Vector3f& rTrans,
                                  const sead::Vector3f& rFront,
                                  const sead::Vector3f& rLocalVelocity,
                                  const PlayerBindEndParam* pEndParam) {
    IUsePlayerPuppet* puppet = pPuppeteer->getPlayerPuppet();
    al::sendMsgWarpEnd(rc::getPuppetSensor(puppet), al::getHitSensor(this, nullptr));
    sead::Vector3f velocity = rLocalVelocity;
    sead::Matrix34f mtx = sead::Matrix34f::ident;
    al::makeMtxFrontUp(&mtx, rFront, sead::Vector3f::ey);
    velocity.setMul(mtx, velocity);
    rc::showPuppet(puppet);
    rc::setPuppetFrontVec(puppet, rFront);
    rc::setPuppetUpVec(puppet, sead::Vector3f::ey);
    rc::setPuppetVelocity(puppet, velocity);
    rc::setPuppetTrans(puppet, rTrans);
    rc::resetPuppetDynamics(puppet);
    pPuppeteer->endBind(pEndParam);
}

/**
 * @brief Bring the players back once the green star of the challenge room was collected.
 * @return Whether the green star was collected.
 */
bool MysteryBox::tryCancelGreenStarGetOrSwitchOn() {
    if (mGreenStar == nullptr || !rc::isAcquiredGreenStarInScene(mGreenStar)) {
        return false;
    }

    if (isCountDown()) {
        endCountDown();
    }

    al::setNerve(this, &NrvMysteryBoxWaitWarpReturn);
    return true;
}

/**
 * @brief Grow the shadow while the players go in.
 */
void MysteryBox::updateShadowScalePlayerIn() {
    lerpShadowMaskSize(this, mShadowMaskSize, cShadowMaskSizeWarp, al::getActionFrame(this), 10,
                       25);
}

/**
 * @brief Wait for players.
 */
void MysteryBox::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/**
 * @brief Stay hidden for a moment before appearing.
 */
void MysteryBox::exeAppearWait() {
    if (al::isStep(this, 10)) {
        al::showModel(this);
        al::validateHitSensors(this);
        al::setNerve(this, &NrvMysteryBoxAppear);
    }
}

/**
 * @brief Play the appear animation.
 */
void MysteryBox::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");

        if (mSeType != 0) {
            al::startSe(this, "PgAppearKarakuri");
        } else {
            al::startSe(this, "PgAppear");
        }
    }

    al::setNerveAtActionEnd(this, &NrvMysteryBoxWait);
}

/**
 * @brief Swallow the players.
 */
void MysteryBox::exePlayerIn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "In");
        al::invalidateClipping(this);
        al::disableBgmChangeArea(this);
    }

    mWarpStep++;
    updateShadowScalePlayerIn();

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvMysteryBoxWaitWarpBegin);
    }
}

/**
 * @brief Wait a moment, then warp if every player is bound or bind the missing ones by force.
 */
void MysteryBox::exeWaitWarpBegin() {
    if (mWarpStep++ >= 60) {
        if (rc::checkAllPlayerBindedAndDisableReviveBubble(this, al::getHitSensor(this, nullptr))) {
            al::setNerve(this, &NrvMysteryBoxWarpBegin);
        } else {
            al::setNerve(this, &NrvMysteryBoxAllBindForceInit);
        }
    }
}

/**
 * @brief Request every player to be bound by the box.
 */
void MysteryBox::exeAllBindForceInit() {
    if (al::isFirstStep(this)) {
        mBindForceNum = 0;
    }

    rc::requestBindAllPlayer(this, al::getHitSensor(this, nullptr));

    if (rc::isAllPlayerBinded(this)) {
        al::setNerve(this, &NrvMysteryBoxAllBindForce);
    }
}

/**
 * @brief Wait for the forced binds to be ready.
 */
void MysteryBox::exeAllBindForce() {
    mPuppeteerGroup->update();

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        auto* puppeteer = mPuppeteerGroup->getPuppeteer<WarpCubeBindPuppeteer>(i);

        if (puppeteer->isBind() && !puppeteer->isEnableStart()) {
            return;
        }
    }

    if (mBindForceNum > 0) {
        al::setNerve(this, &NrvMysteryBoxAllBindForceIn);
    } else {
        al::setNerve(this, &NrvMysteryBoxWarpBegin);
    }
}

/**
 * @brief Swallow the players that were bound by force.
 */
void MysteryBox::exeAllBindForceIn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "In");
    }

    updateShadowScalePlayerIn();

    if (al::isActionEnd(this) && al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvMysteryBoxWarpBegin);
    }
}

/**
 * @brief Cover the screen, then carry the players to the challenge room.
 */
void MysteryBox::exeWarpBegin() {
    if (al::isFirstStep(this)) {
        rc::releaseAllTouchPointerHoldItem(this);
        al::requestCaptureScreenCover(this, 4);
        return;
    }

    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    mDestMtx->getBase(front, 2);

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        IUsePlayerPuppet* puppet = mBindPuppeteers[i]->getPlayerPuppet();
        sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
        s32 num = mBindPuppeteers.size();
        sead::Vector3f offset = calcPlayerLocalOffset(i, num);

        if (mPlayerAppearPos == PlayerAppearPos_Side) {
            offset.x = calcPlayerSideOffset(num - 1 - i, num);
            offset.x += (rc::getControlUserNumMax() - num) * -0.5f * 150.0f;
        }

        al::calcTransLocalOffsetByMtx(&trans, *mDestMtx, offset);

        if (rc::isPuppetHidden(puppet)) {
            rc::showPuppet(puppet);
        }

        rc::showPuppetSilhouette(puppet);
        rc::setPuppetUpVec(puppet, sead::Vector3f::ey);
        warpPlayerPuppet(mBindPuppeteers[i], trans, front, sead::Vector3f::zero,
                         &sWarpBeginEndParam);
    }

    al::requestCancelInterpole(this);
    rc::setPlacementIdObjGhostPlayerRecorder(this, mPlacementId, false);
    rc::tryStartFromObjGhostPlayerRecorder(this, mPlacementId);

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        mPuppeteerGroup->getPuppeteer(i)->setNullPlayerPuppet();
    }

    mBindPuppeteers.clear();
    al::setNerve(this, &NrvMysteryBoxCountDownIntro);
}

/**
 * @brief Start the music and show the counter of the challenge room.
 */
void MysteryBox::exeCountDownIntro() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::stopSequenceBgm(this, "MysteryHouseEntrance", -1);
        al::startBgm(this, "Porter", -1, 0, -1, -1);
        WarpObjUtil::restartStageTimer(this);
        rc::resetDisableReviveBubbleForAllPlayer(this);
        mCounter->appear();

        if (mEndMtx != nullptr) {
            al::updatePoseMtx(this, mEndMtx);
        }
    }

    mCounter->setCount(10);

    if (tryCancelGreenStarGetOrSwitchOn()) {
        return;
    }

    if (al::isStep(this, 50)) {
        al::setNerve(this, &NrvMysteryBoxCountDown);
    }
}

/**
 * @brief Count down the time left in the challenge room.
 */
void MysteryBox::exeCountDown() {
    s32 count = 0;

    if (al::isLessStep(this, cCountDownStep)) {
        count = (cCountDownStep - 1 - al::getNerveStep(this)) / 60 + 1;
    }

    mCounter->setCount(count);

    if (al::isStep(this, 540) || al::isStep(this, 480) || al::isStep(this, 420)) {
        al::startSe(this, "CountDown");
    }

    if (al::isStep(this, cCountDownStep)) {
        al::startSe(this, "CountUp");
    }

    if (!mAutoCountDownCancel) {
        return;
    }

    if (tryCancelGreenStarGetOrSwitchOn()) {
        return;
    }

    if (isCountDownEnd()) {
        al::setNerve(this, &NrvMysteryBoxWarpReturn);
    }
}

/**
 * @brief Wait a moment, then cover the screen before bringing the players back.
 */
void MysteryBox::exeWaitWarpReturn() {
    if (al::isGreaterEqualStep(this, 120)) {
        al::requestCaptureScreenCover(this, 4);
        al::setNerve(this, &NrvMysteryBoxWarpReturn);
    }
}

/**
 * @brief Bind every player and bring them back to the entrance box.
 */
void MysteryBox::exeWarpReturn() {
    rc::requestBindAllPlayer(this, al::getHitSensor(this, nullptr));

    if (!rc::isAllPlayerBinded(this)) {
        return;
    }

    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    al::calcFrontDir(&front, this);

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
        sead::Vector3f offset = calcPlayerLocalOffset(i, mBindPuppeteers.size());
        al::calcTransLocalOffset(&trans, this, offset);
        warpPlayerPuppet(mBindPuppeteers[i], trans, front, cReturnVelocity,
                         &sWarpReturnEndParam);
    }

    mCounter->kill();
    al::requestCancelInterpole(this);

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        mPuppeteerGroup->getPuppeteer(i)->setNullPlayerPuppet();
    }

    mBindPuppeteers.clear();
    al::startAction(this, "Out");
    al::setNerve(this, &NrvMysteryBoxPlayerOut);
}

/**
 * @brief Let the players jump out of the entrance box, then disappear.
 */
void MysteryBox::exePlayerOut() {
    if (al::isFirstStep(this)) {
        rc::resetDisableReviveBubbleForAllPlayer(this);
        al::stopBgm(this, "Porter", -1, -1);
        al::enableBgmChangeArea(this);
    }

    lerpShadowMaskSize(this, cShadowMaskSizeWarp, mShadowMaskSize, al::getActionFrame(this), 0,
                       10);

    if (al::isActionEnd(this)) {
        kill();
    }
}
