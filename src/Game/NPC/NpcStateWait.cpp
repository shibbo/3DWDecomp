#include "NPC/NpcStateWait.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Item/AcquireItemFunc.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include <cmath>

#include "Enemy/EnemyStateUtil.hpp"

namespace {
const NpcStateWaitParam sDefaultWaitParam("Wait", nullptr, nullptr, nullptr, nullptr, nullptr,
                                     nullptr, false, nullptr, false);
NERVE_DECL(NpcStateWait, Wait)
NERVE_DECL(NpcStateWait, Stroke)
NERVE_DECL(NpcStateWait, WaitAfter)
NERVE_DECL(NpcStateWait, Turn)
NERVE_DECL(NpcStateWait, Trampled)
NERVE_DECL(NpcStateWait, Reaction)
NERVE_DECL(NpcStateWait, MicReaction)
NERVE_DECL(NpcStateWait, TurnEnd)
const struct {
    NpcStateWaitNrvWait Wait;
    NpcStateWaitNrvStroke Stroke;
    NpcStateWaitNrvWaitAfter WaitAfter;
    NpcStateWaitNrvTurn Turn;
    NpcStateWaitNrvTrampled Trampled;
    NpcStateWaitNrvReaction Reaction;
    NpcStateWaitNrvMicReaction MicReaction;
    NpcStateWaitNrvTurnEnd TurnEnd;
} NrvNpcStateWait;
}

/**
 * @brief Creates the waiting state and its optional touch and rumble support.
 * @param pActor Actor controlled by this state.
 * @param rInitInfo Actor initialization context; unused.
 * @param pWaitParam Waiting and reaction settings, or nullptr for defaults.
 * @param pTurnParam Optional settings for facing nearby players.
 * @param pRumbleParam Optional scale-rumble settings.
 */
NpcStateWait::NpcStateWait(al::LiveActor* pActor, const al::ActorInitInfo& rInitInfo,
                          const NpcStateWaitParam* pWaitParam, const NpcStateTurnParam* pTurnParam,
                          const NpcStateRumbleParam* pRumbleParam)
    : al::ActorStateBase("NPC\u5f85\u6a5f\u72b6\u614b", pActor), mStrokeState(nullptr),
      mWaitParam(pWaitParam), mTurnParam(pTurnParam), mRumbleParam(pRumbleParam),
      mRumbleCalculator(nullptr), mRumbleFrame(-1), mReactionCooldown(0),
      mIsWaitAfter(false), mIsTurnDisabled(false) {
    initNerve(&NrvNpcStateWait.Wait, 1);
    if (!pWaitParam) {
        mWaitParam = &sDefaultWaitParam;
    }
    mStrokeState = new ActorStateSupportStroke(mHostActor);
    al::initNerveState(this, mStrokeState, &NrvNpcStateWait.Stroke, "DRC\u306a\u3067\u306a\u3067");
    if (!mWaitParam->mTrampledAction && mRumbleParam) {
        mRumbleCalculator = new al::RumbleCalculatorCosMultLinear(
            mRumbleParam->mFrequency, mRumbleParam->mAngleOffset,
            mRumbleParam->mAmplitude, mRumbleParam->mFrames);
    }
    al::listenStageSwitchOnOffStart(mHostActor, al::Functor(this, &NpcStateWait::setWaitAfter),
                                    al::Functor(this, &NpcStateWait::setWait));
}

/**
 * @brief Releases the state's nerve executor.
 */
NpcStateWait::~NpcStateWait() = default;

/**
 * @brief Selects the alternate waiting action and changes active waiting nerves.
 */
void NpcStateWait::setWaitAfter() {
    mIsWaitAfter = true;
    if (al::isNerve(this, &NrvNpcStateWait.Wait) || al::isNerve(this, &NrvNpcStateWait.WaitAfter)) {
        al::setNerve(this, &NrvNpcStateWait.WaitAfter);
    }
}

/**
 * @brief Selects the normal waiting action and changes active waiting nerves.
 */
void NpcStateWait::setWait() {
    mIsWaitAfter = false;
    if (al::isNerve(this, &NrvNpcStateWait.Wait) || al::isNerve(this, &NrvNpcStateWait.WaitAfter)) {
        al::setNerve(this, &NrvNpcStateWait.Wait);
    }
}

/**
 * @brief Activates the state and selects the configured waiting nerve.
 */
void NpcStateWait::appear() {
    mIsDead = false;
    startWait();
}

/**
 * @brief Returns to the configured normal or alternate waiting nerve.
 */
void NpcStateWait::startWait() {
    al::setNerve(this, mIsWaitAfter ? static_cast<const al::Nerve*>(&NrvNpcStateWait.WaitAfter)
                                  : static_cast<const al::Nerve*>(&NrvNpcStateWait.Wait));
}

/**
 * @brief Advances the reaction cooldown and optional scale-rumble animation.
 */
void NpcStateWait::control() {
    if (mReactionCooldown - 1 >= 0) {
        --mReactionCooldown;
    }
    if (mRumbleFrame >= 0) {
        if (mRumbleFrame == 0) {
            mRumbleCalculator->start(0);
        }
        if (mRumbleParam->mFrames > mRumbleFrame) {
            mRumbleCalculator->calc();
            const sead::Vector3f& rRumble = mRumbleCalculator->getOutput();
            const float scale = mRumbleParam->mScale;
            al::setScale(mHostActor, sead::Vector3f(scale + rRumble.x, scale + rRumble.y, scale + rRumble.z));
            ++mRumbleFrame;
        } else {
            al::setScaleAll(mHostActor, mRumbleParam->mScale);
            mRumbleCalculator->reset();
            mRumbleFrame = -1;
        }
    }
}

/**
 * @brief Disables player-facing turns and cancels an active turn.
 */
void NpcStateWait::invalidateTurn() {
    mIsTurnDisabled = true;
    if (al::isNerve(this, &NrvNpcStateWait.Turn)) {
        startWait();
    }
}

/**
 * @brief Pushes a player sensor when this actor supplies a map-object sensor.
 * @param pSelf This actor's attacking sensor.
 * @param pOther Sensor contacted by the actor.
 */
void NpcStateWait::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pSelf) && al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}

/**
 * @brief Handles reactions to attacks, tramples, and reflected player contact.
 * @param pMessage Incoming sensor message.
 * @param pOther Sensor sending the message.
 * @param pSelf This actor's receiving sensor.
 * @return Whether a reaction or trample response accepted the message.
 */
bool NpcStateWait::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther, al::HitSensor* pSelf) {
    bool reaction;
    if (mWaitParam->mRestrictReaction) {
        reaction = al::isMsgBallTrample(pMessage) ||
            (EnemyStateUtil::isMsgBlowDown(pMessage) && !al::isMsgPlayerInvincibleAttack(pMessage));
    } else {
        reaction = !al::isMsgKickKouraAttack(pMessage) && !al::isMsgPlayerBodyLanding(pMessage) &&
            !al::isMsgPlayerBoomerangAttack(pMessage) && !al::isMsgPlayerInvincibleAttack(pMessage) &&
            (al::isMsgBallTrample(pMessage) || EnemyStateUtil::isMsgBlowDown(pMessage) ||
             al::isMsgKickKouraReflect(pMessage) || al::isMsgPlayerBoomerangReflect(pMessage));
    }
    if (reaction && tryStartReaction()) {
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        return true;
    }
    if (al::isMsgPlayerTrampleForCrossoverSensor(pMessage, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMessage) || al::isMsgPlayerBodyAttackReflect(pMessage)) {
        if (mRumbleCalculator) {
            mRumbleFrame = 0;
            al::startSe(mHostActor, "PgTrample");
        } else if (mWaitParam->mTrampledAction) {
            al::setNerve(this, &NrvNpcStateWait.Trampled);
        }
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        return true;
    }
    return false;
}

/**
 * @brief Starts a reaction unless its action is absent or its cooldown is active.
 * @return Whether the reaction nerve was started.
 */
bool NpcStateWait::tryStartReaction() {
    if (!mWaitParam->mReactionAction) {
        return false;
    }
    if (al::isNerve(this, &NrvNpcStateWait.Reaction) && mReactionCooldown > 0) {
        mReactionCooldown = 10;
        return false;
    }
    al::setNerve(this, &NrvNpcStateWait.Reaction);
    return true;
}

/**
 * @brief Routes touch support and direct touch reactions.
 * @param pMessage Incoming touch message.
 * @param pPointer Pointer that generated the message.
 * @param pTarget Screen target receiving the touch.
 * @return Whether the touch message was accepted.
 */
bool NpcStateWait::receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                                        al::ScreenPointTarget* pTarget) {
    if (!al::isNerve(this, &NrvNpcStateWait.Reaction) && !al::isNerve(this, &NrvNpcStateWait.Trampled) &&
        mStrokeState->receiveMsgScreenPoint(pMessage, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvNpcStateWait.Stroke)) {
            al::setNerve(this, &NrvNpcStateWait.Stroke);
        }
        return true;
    }
    if (al::isMsgTouchAssistTrigNoPat(pMessage)) {
        return tryStartReaction();
    }
    return al::isMsgTouchAssist(pMessage);
}

/**
 * @brief Starts turning when a nearby player lies beyond the configured facing angle.
 * @param pParam Optional turning settings.
 * @return Whether the turning nerve was started.
 */
bool NpcStateWait::tryStartTurn(const NpcStateTurnParam* pParam) {
    if (!pParam || mIsTurnDisabled) {
        return false;
    }
    if (pParam->mIsEnabled) {
        if (al::isNerve(this, &NrvNpcStateWait.Wait) && pParam->mOnlyAfter) {
            return false;
        }
        const al::LiveActor* pPlayer = rc::tryFindNearestActivePlayerActorInSphere(mHostActor, pParam->mSearchRadius);
        if (pPlayer && pParam->mStartAngle < std::fabs(al::calcAngleToTargetH(mHostActor, al::getTrans(pPlayer)))) {
            al::setNerve(this, &NrvNpcStateWait.Turn);
            return true;
        }
    }
    return false;
}

/**
 * @brief Starts a microphone reaction when the actor is visible on screen.
 * @return Whether the microphone reaction nerve was started.
 */
bool NpcStateWait::tryStartMicReaction() {
    if (mWaitParam->mMicReactionAction && al::isMicInputOn(mHostActor)) {
        sead::Vector2f screenPosition(0.0f, 0.0f);
        al::calcScreenPosFromWorldPos(&screenPosition, mHostActor, al::getTrans(mHostActor), 0);
        if (al::isInScreen(screenPosition, 100.0f)) {
            al::setNerve(this, &NrvNpcStateWait.MicReaction);
            return true;
        }
    }
    return false;
}

/**
 * @brief Plays the normal waiting action and checks player and microphone reactions.
 */
void NpcStateWait::exeWait() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(mHostActor, mWaitParam->mWaitAction);
    }
    if (tryStartTurn(mTurnParam)) {
        return;
    }
    tryStartMicReaction();
}

/**
 * @brief Plays the alternate waiting action and checks player and microphone reactions.
 */
void NpcStateWait::exeWaitAfter() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(mHostActor, mWaitParam->mWaitAfterAction);
    }
    if (tryStartTurn(mTurnParam)) {
        return;
    }
    tryStartMicReaction();
}

/**
 * @brief Turns toward a nearby player, pausing during scale rumble.
 */
void NpcStateWait::exeTurn() {
    if (al::isFirstStep(this) && mWaitParam->mTurnAction) {
        al::tryStartActionIfNotPlaying(mHostActor, mWaitParam->mTurnAction);
    }
    if (mRumbleCalculator && mRumbleCalculator->isRumbling()) {
        return;
    }
    if (tryStartMicReaction()) {
        return;
    }
    const al::LiveActor* pPlayer = rc::tryFindNearestActivePlayerActorInSphere(mHostActor, mTurnParam->mSearchRadius);
    if (!pPlayer || al::turnToTarget(mHostActor, pPlayer, mTurnParam->mTurnDegrees)) {
        startTurnEnd();
        return;
    }
    if (std::fabs(al::calcAngleToTargetH(mHostActor, al::getTrans(pPlayer))) < mTurnParam->mEndAngle) {
        startTurnEnd();
    }
}

/**
 * @brief Selects a turn-ending delay or returns directly to waiting.
 */
void NpcStateWait::startTurnEnd() {
    if (mTurnParam->mEndFrames <= 0) {
        startWait();
        return;
    }
    al::setNerve(this, &NrvNpcStateWait.TurnEnd);
}

/**
 * @brief Waits out the turn-ending delay while accepting further reactions.
 */
void NpcStateWait::exeTurnEnd() {
    if (tryStartTurn(mTurnParam) || tryStartMicReaction()) {
        return;
    }
    if (al::isGreaterEqualStep(this, mTurnParam->mEndFrames)) {
        startWait();
    }
}

/**
 * @brief Plays the hit reaction and starts its retrigger cooldown.
 */
void NpcStateWait::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mWaitParam->mReactionAction);
        mReactionCooldown = 10;
    }
    if (al::isActionEnd(mHostActor)) {
        startWait();
    }
}

/**
 * @brief Plays the microphone reaction before returning to waiting.
 */
void NpcStateWait::exeMicReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mWaitParam->mMicReactionAction);
    }
    if (al::isActionEnd(mHostActor)) {
        startWait();
    }
}

/**
 * @brief Updates touch support and emits the optional item at the stroke trigger.
 */
void NpcStateWait::exeStroke() {
    if (al::isFirstStep(this) && mWaitParam->mStrokeAction) {
        al::startAction(mHostActor, mWaitParam->mStrokeAction);
    }
    al::updateNerveState(this);
    if (mWaitParam->mAppearItem && mStrokeState->shouldAppearItem()) {
        sead::Vector3f front(0.0f, 0.0f, 0.0f);
        al::calcFrontDir(&front, mHostActor);
        al::appearItemTiming(mHostActor, "\u64ab\u3067\u308b", al::getTrans(mHostActor) + mWaitParam->mItemOffset, front);
    }
    if (!mStrokeState->isTouch()) {
        startWait();
    }
}

/**
 * @brief Plays the trample reaction before returning to waiting.
 */
void NpcStateWait::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, mWaitParam->mTrampledAction);
    }
    if (al::isActionEnd(mHostActor)) {
        startWait();
    }
}

/**
 * @brief Stores waiting actions and optional stroke-item settings.
 * @param pWaitAction Normal waiting action.
 * @param pWaitAfterAction Alternate waiting action.
 * @param pTurnAction Optional turning action.
 * @param pReactionAction Optional hit reaction action.
 * @param pMicReactionAction Optional microphone reaction action.
 * @param pStrokeAction Optional touch-support action.
 * @param pTrampledAction Optional trample action.
 * @param appearItem Whether stroking can trigger an item.
 * @param pItemOffset Optional item spawn offset; nullptr selects zero.
 * @param restrictReaction Whether to restrict the accepted attack reactions.
 */
NpcStateWaitParam::NpcStateWaitParam(const char* pWaitAction, const char* pWaitAfterAction,
                                    const char* pTurnAction, const char* pReactionAction,
                                    const char* pMicReactionAction, const char* pStrokeAction,
                                    const char* pTrampledAction, bool appearItem,
                                    const sead::Vector3f* pItemOffset, bool restrictReaction)
    : mWaitAction(pWaitAction), mWaitAfterAction(pWaitAfterAction), mTurnAction(pTurnAction),
      mReactionAction(pReactionAction), mMicReactionAction(pMicReactionAction),
      mStrokeAction(pStrokeAction), mTrampledAction(pTrampledAction), mAppearItem(appearItem),
      mItemOffset(0.0f, 0.0f, 0.0f), mRestrictReaction(restrictReaction) {
    if (pItemOffset) {
        mItemOffset.set(*pItemOffset);
    }
}

/**
 * @brief Stores the angles, speed, and duration used to face a nearby player.
 * @param startAngle Facing-angle threshold that starts a turn.
 * @param endAngle Facing-angle threshold that ends a turn.
 * @param turnDegrees Turn speed in degrees per update.
 * @param searchRadius Radius for finding an active player.
 * @param isEnabled Whether turning is enabled.
 * @param onlyAfter Whether turns are disabled during the normal waiting nerve.
 * @param endFrames Delay after a turn ends.
 */
NpcStateTurnParam::NpcStateTurnParam(float startAngle, float endAngle, float turnDegrees,
                                    float searchRadius, bool isEnabled, bool onlyAfter, int endFrames)
    : mStartAngle(startAngle), mEndAngle(endAngle), mTurnDegrees(turnDegrees),
      mSearchRadius(searchRadius), mIsEnabled(isEnabled), mOnlyAfter(onlyAfter), mEndFrames(endFrames) {}

/**
 * @brief Stores the timing and scale settings for trample rumble.
 * @param frames Rumble duration in frames.
 * @param frequency Oscillation frequency.
 * @param angleOffset Initial oscillation angle.
 * @param amplitude Oscillation amplitude.
 * @param scale Base scale to restore after rumbling.
 */
NpcStateRumbleParam::NpcStateRumbleParam(int frames, float frequency, float angleOffset, float amplitude, float scale)
    : mFrames(frames), mFrequency(frequency), mAngleOffset(angleOffset), mAmplitude(amplitude), mScale(scale) {}
