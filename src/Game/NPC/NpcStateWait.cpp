#include "NPC/NpcStateWait.hpp"

#include <cmath>

#include "Enemy/EnemyStateUtil.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "NPC/NpcStateParam.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(NpcStateWait, Wait)
NERVE_DECL(NpcStateWait, Stroke)
NERVE_DECL(NpcStateWait, WaitAfter)
NERVE_DECL(NpcStateWait, Turn)
NERVE_DECL(NpcStateWait, Trampled)
NERVE_DECL(NpcStateWait, Reaction)
NERVE_DECL(NpcStateWait, MicReaction)
NERVE_DECL(NpcStateWait, TurnEnd)

// Not const: the compiler merges them into one block and addresses them relative to each other.
NpcStateWaitNrvWait NrvNpcStateWaitWait;
NpcStateWaitNrvStroke NrvNpcStateWaitStroke;
NpcStateWaitNrvWaitAfter NrvNpcStateWaitWaitAfter;
NpcStateWaitNrvTurn NrvNpcStateWaitTurn;
NpcStateWaitNrvTrampled NrvNpcStateWaitTrampled;
NpcStateWaitNrvReaction NrvNpcStateWaitReaction;
NpcStateWaitNrvMicReaction NrvNpcStateWaitMicReaction;
NpcStateWaitNrvTurnEnd NrvNpcStateWaitTurnEnd;

const NpcStateWaitParam sDefaultWaitParam("Wait", nullptr, nullptr, nullptr, nullptr, nullptr,
                                          nullptr, false, nullptr, false);

typedef al::FunctorV0M<NpcStateWait*, void (NpcStateWait::*)()> NpcStateWaitFunctor;
}  // namespace

/**
 * @brief Constructs the wait state.
 * @param pHost Actor that waits.
 * @param rInfo Init info of the actor (unused).
 * @param pWaitParam Action names and reaction settings, or nullptr for the default ones.
 * @param pTurnParam Turn settings, or nullptr to never turn to the player.
 * @param pRumbleParam Squash settings when trampled, or nullptr to play a trampled action.
 */
NpcStateWait::NpcStateWait(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                           const NpcStateWaitParam* pWaitParam,
                           const NpcStateTurnParam* pTurnParam,
                           const NpcStateRumbleParam* pRumbleParam)
    : al::ActorStateBase("NPC待機状態", pHost), mWaitParam(pWaitParam), mTurnParam(pTurnParam),
      mRumbleParam(pRumbleParam) {
    initNerve(&NrvNpcStateWaitWait, 1);

    if (pWaitParam == nullptr) {
        mWaitParam = &sDefaultWaitParam;
    }

    mStateStroke = new ActorStateSupportStroke(mHostActor);
    al::initNerveState(this, mStateStroke, &NrvNpcStateWaitStroke, "DRCなでなで");

    if (mWaitParam->mTrampledActionName == nullptr) {
        const NpcStateRumbleParam* rumbleParam = mRumbleParam;
        if (rumbleParam != nullptr) {
            mRumble = new al::RumbleCalculatorCosMultLinear(
                rumbleParam->mFrequency, rumbleParam->mAngleOffset, rumbleParam->mAmplitude,
                rumbleParam->mFrames);
        }
    }

    al::listenStageSwitchOnOffStart(mHostActor,
                                    NpcStateWaitFunctor(this, &NpcStateWait::setWaitAfter),
                                    NpcStateWaitFunctor(this, &NpcStateWait::setWait));
}

/**
 * @brief Switches to the after-wait action (stage switch on).
 */
void NpcStateWait::setWaitAfter() {
    mIsWaitAfter = true;
    if (al::isNerve(this, &NrvNpcStateWaitWait) || al::isNerve(this, &NrvNpcStateWaitWaitAfter)) {
        al::setNerve(this, &NrvNpcStateWaitWaitAfter);
    }
}

/**
 * @brief Switches back to the normal wait action (stage switch off).
 */
void NpcStateWait::setWait() {
    mIsWaitAfter = false;
    if (al::isNerve(this, &NrvNpcStateWaitWait) || al::isNerve(this, &NrvNpcStateWaitWaitAfter)) {
        al::setNerve(this, &NrvNpcStateWaitWait);
    }
}

/**
 * @brief Activates the state and starts waiting.
 */
void NpcStateWait::appear() {
    al::ActorStateBase::appear();
    startWait();
}

/**
 * @brief Starts the wait nerve matching the current stage switch state.
 */
void NpcStateWait::startWait() {
    al::setNerve(this, mIsWaitAfter ? static_cast<const al::Nerve*>(&NrvNpcStateWaitWaitAfter) :
                                      &NrvNpcStateWaitWait);
}

/**
 * @brief Counts down the reaction cooldown and plays the trampled squash.
 */
void NpcStateWait::control() {
    if (mReactionCooldown > 0) {
        mReactionCooldown--;
    }

    if (mRumbleFrame < 0) {
        return;
    }

    if (mRumbleFrame == 0) {
        mRumble->start(0);
    }

    if (mRumbleParam->mFrames > mRumbleFrame) {
        mRumble->calc();
        f32 baseScale = mRumbleParam->mBaseScale;
        const sead::Vector3f& rumble = mRumble->getValue();
        al::setScale(mHostActor, {baseScale + rumble.x, baseScale + rumble.y, baseScale + rumble.z});
        mRumbleFrame++;
    } else {
        al::setScaleAll(mHostActor, mRumbleParam->mBaseScale);
        mRumble->reset();
        mRumbleFrame = -1;
    }
}

/**
 * @brief Prevents the NPC from turning to the player, ending a turn in progress.
 */
void NpcStateWait::invalidateTurn() {
    mIsInvalidTurn = true;
    if (al::isNerve(this, &NrvNpcStateWaitTurn)) {
        startWait();
    }
}

/**
 * @brief Pushes players away from the NPC's body.
 * @param pSelf Sensor of the NPC.
 * @param pOther Sensor that was hit.
 */
void NpcStateWait::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Reacts to attacks and trampling.
 * @param pMsg Received message.
 * @param pSelf Sensor of the NPC.
 * @param pOther Sensor that sent the message.
 * @return Whether the message was handled.
 */
bool NpcStateWait::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                              al::HitSensor* pOther) {
    bool isReactionMsg;
    if (mWaitParam->mIsReactOnlyBlowDown) {
        isReactionMsg = al::isMsgBallTrample(pMsg) || (EnemyStateUtil::isMsgBlowDown(pMsg) &&
                                                       !al::isMsgPlayerInvincibleAttack(pMsg));
    } else {
        isReactionMsg = !al::isMsgKickKouraAttack(pMsg) && !al::isMsgPlayerBodyLanding(pMsg) &&
                        !al::isMsgPlayerBoomerangAttack(pMsg) &&
                        !al::isMsgPlayerInvincibleAttack(pMsg) &&
                        (al::isMsgBallTrample(pMsg) || EnemyStateUtil::isMsgBlowDown(pMsg) ||
                         al::isMsgKickKouraReflect(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg));
    }

    if (isReactionMsg && tryStartReaction()) {
        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        return true;
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pSelf, pOther) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg)) {
        if (mRumble != nullptr) {
            mRumbleFrame = 0;
            al::startSe(mHostActor, "PgTrample");
        } else if (mWaitParam->mTrampledActionName != nullptr) {
            al::setNerve(this, &NrvNpcStateWaitTrampled);
        }

        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        return true;
    }

    return false;
}

/**
 * @brief Starts the reaction action, unless it is already playing (then it is extended).
 * @return Whether the reaction was started.
 */
bool NpcStateWait::tryStartReaction() {
    if (mWaitParam->mReactionActionName == nullptr) {
        return false;
    }

    if (al::isNerve(this, &NrvNpcStateWaitReaction) && mReactionCooldown > 0) {
        mReactionCooldown = 10;
        return false;
    }

    al::setNerve(this, &NrvNpcStateWaitReaction);
    return true;
}

/**
 * @brief Handles touch screen strokes and taps.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that sent the message.
 * @param pTarget Screen point target of the NPC.
 * @return Whether the message was handled.
 */
bool NpcStateWait::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    if (!al::isNerve(this, &NrvNpcStateWaitReaction) &&
        !al::isNerve(this, &NrvNpcStateWaitTrampled) &&
        mStateStroke->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvNpcStateWaitStroke)) {
            al::setNerve(this, &NrvNpcStateWaitStroke);
        }

        return true;
    }

    if (al::isMsgTouchAssistTrigNoPat(pMsg)) {
        return tryStartReaction();
    }

    return al::isMsgTouchAssist(pMsg);
}

namespace {
/**
 * @brief Starts turning to the nearest player when they stand outside the facing angle.
 * @param pState Wait state.
 * @param pParam Turn settings, or nullptr to never turn.
 * @return Whether the turn was started.
 */
bool tryStartTurn(NpcStateWait* pState, const NpcStateTurnParam* pParam) {
    if (pParam == nullptr || pState->isInvalidTurn() || !pParam->mIsEnableTurn) {
        return false;
    }

    if (al::isNerve(pState, &NrvNpcStateWaitWait) && pParam->mIsTurnOnlyWaitAfter) {
        return false;
    }

    al::LiveActor* player =
        rc::tryFindNearestActivePlayerActorInSphere(pState->mHostActor, pParam->mSearchRadius);
    if (player == nullptr) {
        return false;
    }

    f32 startTurnAngle = pParam->mStartTurnAngle;
    f32 angle = al::calcAngleToTargetH(pState->mHostActor, al::getTrans(player));
    if (startTurnAngle < std::fabs(angle)) {
        al::setNerve(pState, &NrvNpcStateWaitTurn);
        return true;
    }

    return false;
}
}  // namespace

/**
 * @brief Starts the microphone reaction when the player blows into the mic with the NPC on screen.
 * @return Whether the reaction was started.
 */
inline bool NpcStateWait::tryStartMicReaction() {
    if (mWaitParam->mReactionMicActionName == nullptr || !al::isMicInputOn(mHostActor)) {
        return false;
    }

    sead::Vector2f screenPos = {0.0f, 0.0f};
    al::calcScreenPosFromWorldPos(&screenPos, mHostActor, al::getTrans(mHostActor), 0);
    if (!al::isInScreen(screenPos, 100.0f)) {
        return false;
    }

    al::setNerve(this, &NrvNpcStateWaitMicReaction);
    return true;
}

/**
 * @brief Waits, turning to or reacting at the player.
 */
void NpcStateWait::exeWait() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(mHostActor, mWaitParam->mWaitActionName);
    }

    if (tryStartTurn(this, mTurnParam)) {
        return;
    }

    tryStartMicReaction();
}

/**
 * @brief Waits after the stage switch turned on, turning to or reacting at the player.
 */
void NpcStateWait::exeWaitAfter() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(mHostActor, mWaitParam->mWaitAfterActionName);
    }

    if (tryStartTurn(this, mTurnParam)) {
        return;
    }

    tryStartMicReaction();
}

/**
 * @brief Turns to face the nearest player.
 */
void NpcStateWait::exeTurn() {
    if (al::isFirstStep(this) && mWaitParam->mTurnActionName != nullptr) {
        al::tryStartActionIfNotPlaying(mHostActor, mWaitParam->mTurnActionName);
    }

    if (mRumble != nullptr && !mRumble->isEnd()) {
        startWait();
        return;
    }

    if (tryStartMicReaction()) {
        return;
    }

    al::LiveActor* player =
        rc::tryFindNearestActivePlayerActorInSphere(mHostActor, mTurnParam->mSearchRadius);
    if (player == nullptr) {
        startTurnEnd();
        return;
    }

    if (al::turnToTarget(mHostActor, player, mTurnParam->mTurnDegree)) {
        startTurnEnd();
        return;
    }

    if (std::fabs(al::calcAngleToTargetH(mHostActor, al::getTrans(player))) <
        mTurnParam->mEndTurnAngle) {
        startTurnEnd();
    }
}

/**
 * @brief Ends the turn, waiting a few steps first if the turn settings ask for it.
 */
void NpcStateWait::startTurnEnd() {
    if (mTurnParam->mTurnEndStep <= 0) {
        startWait();
        return;
    }

    al::setNerve(this, &NrvNpcStateWaitTurnEnd);
}

/**
 * @brief Holds still after a turn before waiting again.
 */
void NpcStateWait::exeTurnEnd() {
    if (tryStartTurn(this, mTurnParam)) {
        return;
    }

    if (tryStartMicReaction()) {
        return;
    }

    if (al::isGreaterEqualStep(this, mTurnParam->mTurnEndStep)) {
        startWait();
    }
}

/**
 * @brief Plays the reaction action, then waits again.
 */
void NpcStateWait::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mWaitParam->mReactionActionName);
        mReactionCooldown = 10;
    }

    if (al::isActionEnd(mHostActor)) {
        startWait();
    }
}

/**
 * @brief Plays the microphone reaction action, then waits again.
 */
void NpcStateWait::exeMicReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mWaitParam->mReactionMicActionName);
    }

    if (al::isActionEnd(mHostActor)) {
        startWait();
    }
}

/**
 * @brief Gets stroked by the touch screen, dropping an item on each stroke if enabled.
 */
void NpcStateWait::exeStroke() {
    if (al::isFirstStep(this) && mWaitParam->mTouchActionName != nullptr) {
        al::startAction(mHostActor, mWaitParam->mTouchActionName);
    }

    al::updateNerveState(this);

    if (mWaitParam->mIsAppearItemOnStroke && mStateStroke->isTrigStroke()) {
        sead::Vector3f front = {0.0f, 0.0f, 0.0f};
        al::calcFrontDir(&front, mHostActor);
        al::LiveActor* host = mHostActor;
        sead::Vector3f pos = al::getTrans(host) + mWaitParam->mItemOffset;
        al::appearItemTiming(host, "撫でる", pos, front);
    }

    if (!mStateStroke->isTouch()) {
        startWait();
    }
}

/**
 * @brief Plays the trampled action, then waits again.
 */
void NpcStateWait::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mWaitParam->mTrampledActionName);
    }

    if (al::isActionEnd(mHostActor)) {
        startWait();
    }
}

/**
 * @brief Constructs the wait parameters.
 * @param pWaitActionName Action played while waiting.
 * @param pWaitAfterActionName Action played while waiting after the stage switch turned on.
 * @param pTurnActionName Action played while turning, or nullptr.
 * @param pReactionActionName Action played when attacked or tapped, or nullptr.
 * @param pReactionMicActionName Action played on microphone input, or nullptr.
 * @param pTouchActionName Action played while stroked, or nullptr.
 * @param pTrampledActionName Action played when trampled, or nullptr to squash instead.
 * @param isAppearItemOnStroke Whether each stroke drops an item.
 * @param pItemOffset Offset of the dropped item from the NPC, or nullptr for none.
 * @param isReactOnlyBlowDown Whether only blow-down attacks start the reaction.
 */
NpcStateWaitParam::NpcStateWaitParam(const char* pWaitActionName, const char* pWaitAfterActionName,
                                     const char* pTurnActionName, const char* pReactionActionName,
                                     const char* pReactionMicActionName,
                                     const char* pTouchActionName, const char* pTrampledActionName,
                                     bool isAppearItemOnStroke, const sead::Vector3f* pItemOffset,
                                     bool isReactOnlyBlowDown)
    : mWaitActionName(pWaitActionName), mWaitAfterActionName(pWaitAfterActionName),
      mTurnActionName(pTurnActionName), mReactionActionName(pReactionActionName),
      mReactionMicActionName(pReactionMicActionName), mTouchActionName(pTouchActionName),
      mTrampledActionName(pTrampledActionName), mIsAppearItemOnStroke(isAppearItemOnStroke),
      mItemOffset(0.0f, 0.0f, 0.0f), mIsReactOnlyBlowDown(isReactOnlyBlowDown) {
    if (pItemOffset != nullptr) {
        mItemOffset.e = pItemOffset->e;
    }
}

/**
 * @brief Constructs the turn parameters.
 * @param startTurnAngle Angle to the player beyond which the NPC starts turning, in degrees.
 * @param endTurnAngle Angle to the player under which the turn ends, in degrees.
 * @param turnDegree Turn per step, in degrees.
 * @param searchRadius Radius in which players are looked for.
 * @param isEnableTurn Whether the NPC turns at all.
 * @param isTurnOnlyWaitAfter Whether the NPC only turns after the stage switch turned on.
 * @param turnEndStep Steps to hold still after a turn.
 */
NpcStateTurnParam::NpcStateTurnParam(f32 startTurnAngle, f32 endTurnAngle, f32 turnDegree,
                                     f32 searchRadius, bool isEnableTurn,
                                     bool isTurnOnlyWaitAfter, s32 turnEndStep)
    : mStartTurnAngle(startTurnAngle), mEndTurnAngle(endTurnAngle), mTurnDegree(turnDegree),
      mSearchRadius(searchRadius), mIsEnableTurn(isEnableTurn),
      mIsTurnOnlyWaitAfter(isTurnOnlyWaitAfter), mTurnEndStep(turnEndStep) {}

/**
 * @brief Constructs the rumble parameters.
 * @param frames Length of the squash, in frames.
 * @param frequency Frequency of the squash wave.
 * @param angleOffset Phase offset of the squash wave.
 * @param amplitude Amplitude of the squash wave.
 * @param baseScale Scale of the NPC the squash is added to.
 */
NpcStateRumbleParam::NpcStateRumbleParam(s32 frames, f32 frequency, f32 angleOffset,
                                         f32 amplitude, f32 baseScale)
    : mFrames(frames), mFrequency(frequency), mAngleOffset(angleOffset), mAmplitude(amplitude),
      mBaseScale(baseScale) {}
