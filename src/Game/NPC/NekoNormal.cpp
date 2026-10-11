#include "NPC/NekoNormal.hpp"

#include <attributes.h>
#include <math/seadMathCalcCommon.h>
#include <prim/seadBitFlag.h>
#include <prim/seadSafeString.h>

#include "MapObj/DisasterModeController.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePlayerHoldParam.hpp"
#include "MapObj/SePlayObj.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "NPC/NekoBindPuppeteer.hpp"
#include "NPC/NekoParent.hpp"
#include "NPC/NekoStateWaitParam.hpp"
#include "NPC/NpcFunction.hpp"
#include "NPC/NpcHeadController.hpp"
#include "NPC/NpcStateChase.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "NPC/NpcStateParam.hpp"
#include "NPC/NpcStateRunAway.hpp"
#include "NPC/NpcStateWander.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Effect/Effect.hpp"
#include "Project/Effect/EffectInfo.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/Math/MatrixUtil.hpp"

namespace {
NERVE_DECL(NekoNormal, Wait)
NERVE_DECL(NekoNormal, Wander)
NERVE_DECL(NekoNormal, Chase)
NERVE_DECL(NekoNormal, RunAway)
NERVE_DECL(NekoNormal, RunAwayFast)
NERVE_DECL(NekoNormal, Stroke)
NERVE_DECL(NekoNormal, Hold)
NERVE_DECL(NekoNormal, BindRideEnd)
NERVE_DECL(NekoNormal, Pounce)
NERVE_DECL(NekoNormal, HitReact)
NERVE_DECL(NekoNormal, Fall)
NERVE_DECL(NekoNormal, Appear)
NERVE_DECL(NekoNormal, Trampled)
NERVE_DECL(NekoNormal, Purr)
NERVE_DECL(NekoNormal, TargetWait)
NERVE_DECL(NekoNormal, Attack)
NERVE_DECL(NekoNormal, RunAwayWait)
NERVE_DECL(NekoNormal, RunAwayAvoid)
NERVE_DECL(NekoNormal, SeekTarget)
NERVE_DECL(NekoNormal, WaitPush)
NERVE_DECL(NekoNormal, Release)
NERVE_DECL(NekoNormal, WaitEnd)
NERVE_DECL(NekoNormal, Startle)
NERVE_DECL(NekoNormal, RunAwayEnd)
NERVE_DECL(NekoNormal, Alert)
NERVE_DECL(NekoNormal, ChaseEnd)
NERVE_DECL(NekoNormal, AlertEnd)
NERVE_DECL(NekoNormal, FollowStart)
NERVE_DECL(NekoNormal, Follow)
NERVE_DECL(NekoNormal, FollowWait)
NERVE_DECL(NekoNormal, FollowJump)
NERVE_DECL(NekoNormal, FollowJumpToPlayer)
NERVE_DECL(NekoNormal, FollowJumpEnd)
NERVE_DECL(NekoNormal, BindEnd)

/**
 * @brief Nerve that ends a binding but keeps the cat in place; shares exeBindEnd.
 */
class NekoNormalNrvBindEndStay : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<NekoNormal>()->exeBindEnd();
    }
};

NERVE_DECL(NekoNormal, GoalWait)
NERVE_DECL(NekoNormal, WaitStart)
NERVE_DECL(NekoNormal, BindRide)
NERVE_DECL(NekoNormal, BindStart)
NERVE_DECL(NekoNormal, FaceTarget)
NERVE_DECL(NekoNormal, StartleWait)

// Non-const nerve objects: the game merges them into one block.
NekoNormalNrvWait NrvNekoNormalWait;
NekoNormalNrvWander NrvNekoNormalWander;
NekoNormalNrvChase NrvNekoNormalChase;
NekoNormalNrvRunAway NrvNekoNormalRunAway;
NekoNormalNrvRunAwayFast NrvNekoNormalRunAwayFast;
NekoNormalNrvStroke NrvNekoNormalStroke;
NekoNormalNrvHold NrvNekoNormalHold;
NekoNormalNrvBindRideEnd NrvNekoNormalBindRideEnd;
NekoNormalNrvPounce NrvNekoNormalPounce;
NekoNormalNrvHitReact NrvNekoNormalHitReact;
NekoNormalNrvFall NrvNekoNormalFall;
NekoNormalNrvAppear NrvNekoNormalAppear;
NekoNormalNrvTrampled NrvNekoNormalTrampled;
NekoNormalNrvPurr NrvNekoNormalPurr;
NekoNormalNrvTargetWait NrvNekoNormalTargetWait;
NekoNormalNrvAttack NrvNekoNormalAttack;
NekoNormalNrvRunAwayWait NrvNekoNormalRunAwayWait;
NekoNormalNrvRunAwayAvoid NrvNekoNormalRunAwayAvoid;
NekoNormalNrvSeekTarget NrvNekoNormalSeekTarget;
NekoNormalNrvWaitPush NrvNekoNormalWaitPush;
NekoNormalNrvRelease NrvNekoNormalRelease;
NekoNormalNrvWaitEnd NrvNekoNormalWaitEnd;
NekoNormalNrvStartle NrvNekoNormalStartle;
NekoNormalNrvRunAwayEnd NrvNekoNormalRunAwayEnd;
NekoNormalNrvAlert NrvNekoNormalAlert;
NekoNormalNrvChaseEnd NrvNekoNormalChaseEnd;
NekoNormalNrvAlertEnd NrvNekoNormalAlertEnd;
NekoNormalNrvFollowStart NrvNekoNormalFollowStart;
NekoNormalNrvFollow NrvNekoNormalFollow;
NekoNormalNrvFollowWait NrvNekoNormalFollowWait;
NekoNormalNrvFollowJump NrvNekoNormalFollowJump;
NekoNormalNrvFollowJumpToPlayer NrvNekoNormalFollowJumpToPlayer;
NekoNormalNrvFollowJumpEnd NrvNekoNormalFollowJumpEnd;
NekoNormalNrvBindEnd NrvNekoNormalBindEnd;
NekoNormalNrvBindEndStay NrvNekoNormalBindEndStay;
NekoNormalNrvGoalWait NrvNekoNormalGoalWait;
NekoNormalNrvWaitStart NrvNekoNormalWaitStart;
NekoNormalNrvBindRide NrvNekoNormalBindRide;
NekoNormalNrvBindStart NrvNekoNormalBindStart;
NekoNormalNrvFaceTarget NrvNekoNormalFaceTarget;
NekoNormalNrvStartleWait NrvNekoNormalStartleWait;

/** @brief Horizontal and vertical speed of the cat when dropped in front of the player. */
const sead::Vector2f sDropFrontSpeed(10.0f, 10.0f);
/** @brief Horizontal and vertical speed of the cat when dropped behind the player. */
const sead::Vector2f sDropBackSpeed(3.0f, 5.0f);
/** @brief Distance rate and vertical speed of the jump at the end of a ride. */
const sead::Vector2f sRideEndJumpNear(0.05f, 35.0f);
/** @brief Same as sRideEndJumpNear, when the player is far below or above. */
const sead::Vector2f sRideEndJumpHigh(0.04f, 60.0f);
/** @brief Same as sRideEndJumpNear, when the player is far away. */
const sead::Vector2f sRideEndJumpFar(0.05f, 50.0f);
NpcStateTurnParam sTurnParam(0.0f, 0.0f, 5.0f, -1.0f, true, true, 0);
NpcStateRumbleParam sRumbleParam(30, 2.0f, 1.5f, 0.2f, 1.0f);
NpcStateParam sJumpStateParam(2.25f, 0.98f, 0.89f, 300.0f, 800.0f, 80.0f, 40.0f, 110.0f, 5, 0.3f);
NpcStateParam sFallStateParam(1.5f, 0.98f, 0.89f, 300.0f, 800.0f, 80.0f, 40.0f, 110.0f, 5, 0.3f);
NpcStateParam sStateParam(0.5f, 0.98f, 0.89f, 300.0f, 800.0f, 80.0f, 40.0f, 110.0f, 5, 0.3f);
/** @brief Targets the cat is not afraid of. */
const sead::BitFlag32 sFriendlyTargetTypes(
    npc::NpcFindTargetType_PlayerClimb | npc::NpcFindTargetType_Cursor |
    npc::NpcFindTargetType_Ball | npc::NpcFindTargetType_Koura | npc::NpcFindTargetType_Bird);
/** @brief Targets the cat looks for. */
const sead::BitFlag32 sSearchTargetTypes(
    npc::NpcFindTargetType_Player | npc::NpcFindTargetType_Cursor | npc::NpcFindTargetType_Ball |
    npc::NpcFindTargetType_Koura | npc::NpcFindTargetType_Bird);
ItemStatePlayerHoldParam sPlayerHoldParam(
    sead::Vector3f(-25.0f, 0.0f, 25.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
    sead::Vector3f(-30.0f, 0.0f, 30.0f), sead::Vector3f(-25.0f, 0.0f, 50.0f),
    sead::Vector3f(-30.0f, 0.0f, 30.0f), sead::Vector3f(-50.0f, 0.0f, 25.0f),
    sead::Vector3f(-25.0f, 0.0f, 50.0f), sead::Vector3f(-30.0f, 0.0f, 45.0f),
    sead::Vector3f(-25.0f, 0.0f, 50.0f), sead::Vector3f(-30.0f, 0.0f, 45.0f),
    sead::Vector3f(0.0f, 0.0f, 0.0f));
NekoStateWaitParam sSleepParam("WaitSleep", "WaitSleepStart", "WaitSleepEnd", nullptr, nullptr);
NekoStateWaitParam sLayDownParam("WaitLayDown", nullptr, nullptr, nullptr, nullptr);
NekoStateWaitParam sPlayParam("WaitPlay", nullptr, nullptr, nullptr, nullptr);
NekoStateWaitParam sSitParam("WaitSit", nullptr, nullptr, "WaitSit01", "WaitSit02");
NekoStateWaitParam sWaitParam("Wait", nullptr, nullptr, nullptr, nullptr);

/** @brief Minimum duration of the idle behaviors, from Sit to Sleep. */
const s32 cMinIdleTimes[] = {700, 400, 240, 400, 600};

/**
 * @brief Check whether a behavior is one of the idle behaviors with wait parameters.
 * @param behavior The behavior.
 * @return Whether the behavior is an idle one.
 */
inline bool isIdleBehavior(al::param::NekoBehavior behavior) {
    return behavior >= al::param::NekoBehavior_Sit && behavior <= al::param::NekoBehavior_Sleep;
}

/**
 * @brief Get the wait parameters of an idle behavior.
 * @param behavior The idle behavior.
 * @return Wait parameters of the behavior, the plain wait ones for a non idle behavior.
 */
inline const NekoStateWaitParam* getWaitParam(al::param::NekoBehavior behavior) {
    switch (behavior) {
    case al::param::NekoBehavior_Sit:
        return &sSitParam;
    case al::param::NekoBehavior_Play:
        return &sPlayParam;
    case al::param::NekoBehavior_LayDown:
        return &sLayDownParam;
    case al::param::NekoBehavior_Sleep:
        return &sSleepParam;
    case al::param::NekoBehavior_Wait:
    default:
        return &sWaitParam;
    }
}

/**
 * @brief Pick one of the variation actions of an idle behavior.
 * @param pParam Wait parameters of the behavior.
 * @return Name of the variation action, nullptr if the behavior has none.
 */
inline const char* getRandomSubAction(const NekoStateWaitParam* pParam) {
    if (pParam->mSubAction1 == nullptr) {
        return nullptr;
    }

    if (pParam->mSubAction2 == nullptr) {
        return pParam->mSubAction1;
    }

    return al::getRandom() > 0.5f ? pParam->mSubAction1 : pParam->mSubAction2;
}

/**
 * @brief Emit the effect of an imminent disaster, synchronized with the disaster cycle.
 * @param pActor The cat.
 */
void tryEmitDisasterAnticipationEffect(al::LiveActor* pActor) {
    DisasterModeController* controller = DisasterModeController::tryGetController(pActor);

    if (controller == nullptr ||
        controller->getState() != DisasterModeController::State::DisasterStart) {
        return;
    }

    s32 frame = controller->getStateFrame();

    if (frame >= 120) {
        return;
    }

    al::Effect* effect = pActor->getEffectKeeper()->findEffect("DisasterAnticipation");

    if (effect->isEmitterActive()) {
        return;
    }

    const_cast<al::EffectInfo*>(effect->getEffectInfo())->mParam.mForceCalcFrame = frame;
    al::tryEmitEffect(pActor, "DisasterAnticipation", nullptr);
}

/**
 * @brief Kill the player hold state when the player lets go of the cat.
 * @param pState The player hold state.
 * @param pMsg Received message.
 * @param pOther Sensor of the player.
 */
void tryKillPlayerHoldState(ItemStatePlayerHold* pState, const al::SensorMsg* pMsg,
                            al::HitSensor* pOther) {
    if (al::isMsgPlayerRelease(pMsg) || al::isMsgPlayerReleaseDamage(pMsg) ||
        al::isMsgPlayerReleaseDead(pMsg) || al::isMsgHoldCancel(pMsg) ||
        al::isMsgWarpStart(pMsg)) {
        if (al::isMsgWarpStart(pMsg)) {
            rc::requestPlayerRelease(pOther);
        }

        pState->kill();
    }
}

/**
 * @brief Check whether the cat plays a standing action it can sit down from.
 * @param pActor The cat.
 * @return Whether the cat is standing.
 */
bool isActionStanding(const al::LiveActor* pActor) {
    return al::isActionPlaying(pActor, "Wait") || al::isActionPlaying(pActor, "Walk") ||
           al::isActionPlaying(pActor, "Run") || al::isActionPlaying(pActor, "WaitLookLeftEnd") ||
           al::isActionPlaying(pActor, "WaitLookRightEnd") ||
           al::isActionPlaying(pActor, "ThrowEnd") || al::isActionPlaying(pActor, "Pounce") ||
           al::isActionPlaying(pActor, "Purr") || al::isActionPlaying(pActor, "Stroke");
}
}  // namespace

/**
 * @brief Construct the regular cat mode.
 * @param pHost Host cat of the mode.
 */
NekoNormal::NekoNormal(Neko* pHost) : IUseNekoModeActor("NekoNormal"), mHost(pHost) {
    mParam = new NekoNormalParam();
    mBindPuppeteers = new NekoBindPuppeteer*[2];
    mBindPuppeteers[0] = new NekoBindPuppeteer(this);
    mBindPuppeteers[1] = new NekoBindPuppeteer(this);
    mBindPuppeteer = mBindPuppeteers[0];
}

/**
 * @brief Initialize the regular cat and its states.
 * @param rInfo Placement information.
 * @param colorType Coat color of the cat.
 * @param pTargetFinder Target finder shared with the other modes of the cat.
 */
void NekoNormal::init(const al::ActorInitInfo& rInfo, neko::ColorType colorType,
                      NpcTargetFinder* pTargetFinder) {
    NekoNormalParam* param = mParam;
    {
        s32 startBehavior;

        if (al::tryGetArg(&startBehavior, rInfo, "StartBehavior")) {
            param->mStartBehavior = static_cast<al::param::NekoBehavior>(startBehavior);
        }
    }

    al::tryGetArg(&param->mWanderRange, rInfo, "WanderRange");
    al::tryGetArg(&param->mChaseRange, rInfo, "ChaseRange");
    al::tryGetArg(&param->mIsEnableCliffCheck, rInfo, "IsEnableCliffCheck");
    al::tryGetArg(&param->mIsEnableShoreCheck, rInfo, "IsEnableShoreCheck");
    al::tryGetArg(&param->mIsDisabledPR, rInfo, "isDisabledPR");
    al::tryGetArg(&param->mIsDisablePlessieChase, rInfo, "isDisablePlessieChase");
    al::tryGetStringArg(&param->mComment, rInfo, "Comment");
    mColorType = colorType;

    if (colorType == 4) {
        al::initActorWithArchiveName(this, rInfo, "NekoCollect", nullptr);
        SePlayObj* collectSe = new SePlayObj("NekoCollectSePlayObj");
        mCollectSe = collectSe;
        collectSe->initAttached(rInfo, "NekoCollect");
    } else if (colorType <= 3) {
        al::initActorWithArchiveName(this, rInfo, "Neko", nullptr);

        if (al::tryStartMtpAnimIfExist(this, "NekoColor")) {
            al::setMtpAnimFrame(this, colorType);
            al::setMtpAnimFrameRate(this, 0.0f);
        }
    } else {
        al::initActorChangeModel(this, rInfo);
    }

    mTargetFinder = pTargetFinder;
    pTargetFinder->setSearchTypes(sSearchTargetTypes.getDirect());
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Player, 1);

    const al::Resource* modelResource = al::getModelResource(this);
    al::ByamlIter initIter;

    if (al::tryGetActorInitFileIter(&initIter, modelResource, "InitNeko", nullptr)) {
        al::ByamlIter headIter;

        if (initIter.tryGetIterByKey(&headIter, "NpcHeadControl")) {
            mHeadController = neko::makeHeadController(this, headIter, mTargetFinder,
                                                       npc::NpcFindTargetType_All);
        }
    }

    al::initNerve(this, &NrvNekoNormalWait, 8);
    mWanderParam = new NpcStateWanderParam(
        120, 300, 0.3f, 1.5f, 50.0f, mParam->mWanderRange > 0.0f ? mParam->mWanderRange : 500.0f,
        1.5f, 150.0f, 1.0f, mParam->mIsEnableCliffCheck,
        mParam->mIsEnableShoreCheck, "Walk", "Wait", true, 60, 3000.0f);
    mChaseParam = new NpcStateChaseParam(1.2f, 130.0f, 500.0f, 3.5f, -1.0f, false,
                                         mParam->mIsEnableCliffCheck, mParam->mIsEnableShoreCheck,
                                         "Run", "Wait", mTargetFinder->getParam()->getChaseRange());
    mRunAwayParam = new NpcStateRunAwayParam(0.5f, 30, 1.0f, 60.0f, 4.0f, 50.0f, -1.0f, false,
                                             mParam->mIsEnableCliffCheck,
                                             mParam->mIsEnableShoreCheck, "Walk", "Wait", 300.0f,
                                             100);
    mRunAwayFastParam = new NpcStateRunAwayParam(2.0f, 12, 8.0f, 20.0f, 8.0f, 75.0f, -1.0f, false,
                                                 mParam->mIsEnableCliffCheck,
                                                 mParam->mIsEnableShoreCheck, "Run", "WaitStartle",
                                                 450.0f, 30);
    mStateWander = new NpcStateWander(this, al::getFrontPtr(this), &sStateParam, mWanderParam);
    mStateChase = new NpcStateChase(this, al::getFrontPtr(this), mTargetFinder, &sStateParam,
                                    mChaseParam, false, &mIsOnGround);
    mStateRunAway = new NpcStateRunAway(this, al::getFrontPtr(this), mTargetFinder, &sStateParam,
                                        mRunAwayParam, false, &mIsOnGround);
    mStateRunAwayFast = new NpcStateRunAway(this, al::getFrontPtr(this), mTargetFinder,
                                            &sStateParam, mRunAwayFastParam, false, &mIsOnGround);
    mStateSupportStroke = new ActorStateSupportStroke(this);
    al::initNerveState(this, mStateWander, &NrvNekoNormalWander, "[state]Wander");
    al::initNerveState(this, mStateChase, &NrvNekoNormalChase, "[state]Chase");
    al::initNerveState(this, mStateRunAway, &NrvNekoNormalRunAway, "[state]RunAway");
    al::initNerveState(this, mStateRunAwayFast, &NrvNekoNormalRunAwayFast, "[state]RunAwayFast");
    al::initNerveState(this, mStateSupportStroke, &NrvNekoNormalStroke, "[state]Stroke");
    mStatePlayerHold = new ItemStatePlayerHold(this, &sPlayerHoldParam, false, true);
    al::initNerveState(this, mStatePlayerHold, &NrvNekoNormalHold, "[state]PlayerHold");
    mStatePlayerHold->initColliderControl();
    mCollisionController = al::createActorCollisionController(this);
    al::createAndSetColliderSpecialPurpose(this, "NekoMoveLimit");

    if (mParam->mStartBehavior == al::param::NekoBehavior_Goal) {
        neko::Target* target = new neko::Target();
        target->mTrans = al::getTrans(this);
        mGoalTarget = target;
        mIsAtGoal = true;
    }

    mChaseRange = mParam->mChaseRange;
    al::tryStartActionIfNotPlaying(this, "WaitSit");
    tryStartDefaultBehavior(al::param::NekoBehavior_Default);
}

/**
 * @brief Start the idle behavior of the cat.
 * @param behavior Behavior to start, NekoBehavior_Default for the one of the placement.
 * @return Whether a new behavior was started.
 */
bool NekoNormal::tryStartDefaultBehavior(al::param::NekoBehavior behavior) {
    if (mIsAtGoal || mParam->mStartBehavior == al::param::NekoBehavior_Goal) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvNekoNormalGoalWait);
        al::showModelIfHide(this);
        return true;
    }

    al::param::NekoBehavior prevBehavior = mBehavior;

    if (behavior >= al::param::NekoBehavior_Default) {
        behavior = mParam->mStartBehavior;

        if (behavior == al::param::NekoBehavior_Random) {
            s32 random = al::getRandom(0, 100);

            switch (prevBehavior) {
            case al::param::NekoBehavior_Sleep:
                behavior = al::param::NekoBehavior_LayDown;
                break;
            case al::param::NekoBehavior_LayDown:
                behavior =
                    random < 50 ? al::param::NekoBehavior_Sleep : al::param::NekoBehavior_Sit;
                break;
            case al::param::NekoBehavior_Sit:
                if (random < 20) {
                    behavior = al::param::NekoBehavior_Play;
                } else {
                    behavior = random < 70 ? al::param::NekoBehavior_LayDown :
                                             al::param::NekoBehavior_Wait;
                }
                break;
            default:
                behavior = al::param::NekoBehavior_Sit;
                break;
            }
        }
    }

    mBehavior = behavior;
    s32 minTime = isIdleBehavior(behavior) ? cMinIdleTimes[behavior - al::param::NekoBehavior_Sit] :
                                             240;
    mBehaviorTime =
        al::getRandom(minTime, behavior == al::param::NekoBehavior_Sleep ? 2400 : 1200);

    if (mBehavior == al::param::NekoBehavior_Wait && neko::isActive(this, 3000.0f) &&
        !rc::isActiveDemo(this)) {
        if (al::isNerve(this, &NrvNekoNormalWander)) {
            return false;
        }

        al::setNerve(this, &NrvNekoNormalWander);
        al::showModelIfHide(this);
        return true;
    }

    if (isWait() && prevBehavior == mBehavior) {
        return false;
    }

    bool isStart;

    switch (mBehavior) {
    case al::param::NekoBehavior_Sit:
        isStart = isActionStanding(this) || al::isActionPlaying(this, "WaitLayDown");
        break;
    case al::param::NekoBehavior_LayDown:
        isStart = true;
        break;
    default:
        isStart = getWaitParam(mBehavior)->mStartAction != nullptr;
        break;
    }

    al::setNerve(this, isStart ? static_cast<const al::Nerve*>(&NrvNekoNormalWaitStart) :
                                 &NrvNekoNormalWait);
    al::showModelIfHide(this);
    return true;
}

/**
 * @brief Update the cool times, the bindings and the head of the cat.
 */
void NekoNormal::control() {
    neko::setOnGroundFlag(this, mIsOnGround);
    mIsPushed = false;

    if (mCoolTime > 0) {
        mCoolTime--;
    }

    if (mReactCoolTime > 0.0f) {
        mReactCoolTime -= 1.0f;
    }

    if (mPackunEatCoolTime > 0) {
        mPackunEatCoolTime--;
    }

    if (mIsAtGoal) {
        return;
    }

    // The result is not used.
    isHold();
    bool isInteractive = this->isInteractive();
    al::updateActorCollisionController(mCollisionController);
    mBindPuppeteers[0]->update();
    mBindPuppeteers[1]->update();

    if (mBindPuppeteers[0]->isBinding() || mBindPuppeteers[1]->isBinding()) {
        al::invalidateClipping(this);
    }

    if (isInteractive && !al::isNerve(this, &NrvNekoNormalRelease) &&
        !al::isNerve(this, &NrvNekoNormalBindRideEnd)) {
        if (!mIsOnGround && !al::isNoCollide(this)) {
            if (!al::isNerve(this, &NrvNekoNormalPounce) &&
                !al::isNerve(this, &NrvNekoNormalHitReact)) {
                neko::trySetNerve(this, &NrvNekoNormalFall);
            }

            return;
        }

        if (mIsOnGround) {
            al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);

            if (groundSensor != nullptr) {
                al::HitSensor* bodySensor = al::getHitSensor(this, "Body");
                al::sendMsgEnemyFloorTouch(groundSensor, bodySensor);

                if (rc::sendMsgEnemyFloorTouchTrampoline(groundSensor, bodySensor) &&
                    tryBounce(30.0f)) {
                    return;
                }
            }
        }
    }

    if (!isRunAway()) {
        mRunAwayTime = 0;
    }

    mHeadController->update();
    mHeadController->setLookAtTarget(nullptr);
    DisasterModeController* controller = DisasterModeController::tryGetController(this);

    if (controller != nullptr &&
        controller->getState() == DisasterModeController::State::DisasterStart &&
        controller->getStateFrame() < 120) {
        al::tryHoldSe(this, "PgDisasterAnticipation", nullptr);
    }
}

/**
 * @brief Check whether the cat was thrown or dropped by the player.
 * @return Whether the cat was released.
 */
bool NekoNormal::isThrown() const {
    return al::isNerve(this, &NrvNekoNormalRelease);
}

/**
 * @brief Bounce the cat upwards, keeping its horizontal velocity.
 * @param speed Vertical speed of the bounce.
 * @return Whether the cat started bouncing.
 */
bool NekoNormal::tryBounce(f32 speed) {
    if (!neko::trySetNerve(this, &NrvNekoNormalFall)) {
        return false;
    }

    sead::Vector3f up;
    al::calcUpDir(&up, this);
    sead::Vector3f velocity;
    al::verticalizeVec(&velocity, up, al::getVelocity(this));
    velocity = up * speed + velocity;
    al::setVelocity(this, velocity);
    return true;
}

/**
 * @brief Check whether the cat is running away.
 * @return Whether the cat is running away or reacting to a hit.
 */
bool NekoNormal::isRunAway() const {
    return al::isNerve(this, &NrvNekoNormalRunAway) ||
           al::isNerve(this, &NrvNekoNormalRunAwayFast) ||
           al::isNerve(this, &NrvNekoNormalRunAwayEnd) ||
           al::isNerve(this, &NrvNekoNormalRunAwayWait) ||
           al::isNerve(this, &NrvNekoNormalRunAwayAvoid) ||
           (al::isNerve(this, &NrvNekoNormalAlert) && mActionTime > 0) ||
           al::isNerve(this, &NrvNekoNormalHitReact) || al::isNerve(this, &NrvNekoNormalTrampled);
}

/**
 * @brief Update the animations, using the puppet matrix while the cat is bound.
 */
void NekoNormal::calcAnim() {
    al::LiveActor::calcAnim();

    if (mIsValidPuppetMtx) {
        al::setBaseMtxAndCalcAnim(this, mPuppetMtx, al::getScale(this));
    }
}

/**
 * @brief Update the actor while its movement is paused.
 * @param isPaused Whether the movement is paused.
 */
void NekoNormal::movementPaused(bool isPaused) {
    al::LiveActor::movementPaused(isPaused);

    if (mIsValidPuppetMtx) {
        al::setBaseMtxAndCalcAnim(this, mPuppetMtx, al::getScale(this));
    }
}

/**
 * @brief Clip the cat, and its host if this mode is the active one.
 */
void NekoNormal::startClipped() {
    if (mHost->getModeActor() == this) {
        mHost->tryStartClipped();
    }

    if (mCollectSe != nullptr && !mIsAtGoal) {
        al::updatePoseMtx(mCollectSe, getBaseMtx());
        al::invalidateClipping(mCollectSe);
        al::tryStartSeByName(mCollectSe, "WaitMeow", nullptr);
    }

    al::LiveActor::startClipped();
}

/**
 * @brief Unclip the cat, and its host if this mode is the active one.
 */
void NekoNormal::endClipped() {
    if (mHost->getModeActor() == this) {
        mHost->tryEndClipped();
    }

    if (mCollectSe != nullptr && !mIsAtGoal) {
        al::tryStopSeByName(mCollectSe, "WaitMeow");
    }

    tryEmitDisasterAnticipationEffect(this);
    al::LiveActor::endClipped();
}

/**
 * @brief Update the collider, following the player while the cat is carried.
 */
void NekoNormal::updateCollider() {
    if (mStatePlayerHold->isDead()) {
        al::LiveActor::updateCollider();
        return;
    }

    mStatePlayerHold->updateCollider(al::getHitSensor(this, "Body"));
}

/**
 * @brief Make this mode the active one of the host cat.
 * @param rReason Why the mode gets attached.
 */
void NekoNormal::startAttach(const NekoAttachReason& rReason) {
    al::onCollide(this);
    mTargetFinder->clearTarget();
    mTargetFinder->setSearchTypes(sSearchTargetTypes.getDirect());
    mTargetFinder->setTargetTypePriority(npc::NpcFindTargetType_Player, 1);

    if (rReason.mType == NekoAttachReason::Type_AppearAtHost) {
        al::resetPosition(this, al::getTrans(mHost), false);
        al::faceToDirection(this, al::getFront(mHost));
        al::hideModelIfShow(this);
        mCoolTime = 5;
        al::setNerve(this, &NrvNekoNormalAppear);
    } else if (rReason.mType == NekoAttachReason::Type_Hide && mIsAtGoal) {
        mHost->tryStartHide();
    } else {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    }

    if (rReason.mType == NekoAttachReason::Type_Hide) {
        tryEmitDisasterAnticipationEffect(this);
    }
}

/**
 * @brief Kill the cat.
 * @param isDeleteParticle Whether the anticipation particles are deleted immediately.
 */
void NekoNormal::startKill(bool isDeleteParticle) {
    if (isHold()) {
        endHold();

        if (mHolderSensor != nullptr) {
            rc::requestPlayerRelease(mHolderSensor);
        }

        al::resetActorCollisionController(mCollisionController, 1);
        al::validateClipping(this);
    }

    if (isRide()) {
        if (mRideSensor != nullptr) {
            rc::sendMsgNpcBindCancel(mRideSensor, al::getHitSensor(this, "Body"));
        } else {
            endBindNpc(static_cast<NpcPuppetBindEndType>(4));
        }
    }

    if (isDeleteParticle) {
        al::tryDeleteEffectAndParticle(this, "DisasterAnticipation");
    } else {
        al::tryDeleteEffect(this, "DisasterAnticipation");
    }

    kill();
}

/**
 * @brief Put the cat down at the position of the player holding it.
 */
void NekoNormal::endHold() {
    f32 transY = al::getTrans(this).y;
    sead::Vector3f pos = al::getActorTrans(mHolderSensor);
    pos.y = transY;
    al::resetPosition(this, pos, false);
    al::onCollide(this);
    updateCollider();
    al::setVelocityZero(this);
    mCoolTime = 30;
    alPadRumbleFunction::stopPadRumbleLoop(this, "NekoCarry", al::getTransPtr(this), -1);
}

/**
 * @brief Push, touch and attack the actors the cat collides with.
 * @param pSelf Sensor of the cat.
 * @param pOther Sensor of the other actor.
 */
void NekoNormal::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isHold()) {
        if (!al::isSensorNpc(pSelf)) {
            return;
        }

        if (!al::isSensorPlayer(pOther) && !al::isSensorRide(pOther) &&
            neko::isHitSensorRadius(35.0f, pSelf, pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        if (npc::isSensorBall(pOther) || al::isSensorKickKoura(pOther)) {
            al::sendMsgBallAttack(pOther, pSelf, nullptr);
            return;
        }

        if (neko::isSensorEnemyReactAttack(pOther)) {
            if (!al::isSensorHostName(pOther, "サーチキラー")) {
                if (!al::sendMsgNekoAttack(pOther, pSelf)) {
                    al::sendMsgBallAttack(pOther, pSelf, nullptr);
                }

                return;
            }

            if (mCoolTime > 0) {
                return;
            }

            sead::Vector3f dir = al::getSensorPos(pOther) - al::getSensorPos(pSelf);

            if (al::isNearAngleDegreeHV(dir, al::getFront(this), al::getGravity(this), 60.0f,
                                        60.0f) &&
                al::sendMsgKillerReflect(pOther, pSelf)) {
                mCoolTime = 30;
            }

            return;
        }

        if (al::isSensorEnemy(pOther) || al::isSensorNpc(pOther) || al::isSensorKoopaJr(pOther) ||
            al::isSensorRide(pOther)) {
            if (!al::sendMsgNekoAttack(pOther, pSelf)) {
                al::sendMsgBallAttack(pOther, pSelf, nullptr);
            }

            return;
        }

        if (rc::isSensorKinopioBrigadeNpc(pOther)) {
            al::sendMsgNekoAttack(pOther, pSelf);
            return;
        }

        if (neko::isSensorMapObjReactAttack(pOther)) {
            if (!al::isSensorHostName(pOther, "木箱") &&
                !al::isSensorHostName(pOther, "SignBoardCat")) {
                if (!al::sendMsgNekoAttack(pOther, pSelf)) {
                    al::sendMsgBallAttack(pOther, pSelf, nullptr);
                }

                return;
            }

            sead::Vector3f dir;
            al::calcDirBetweenSensors(&dir, pSelf, pOther);

            if (!(dir.y > -0.4f)) {
                return;
            }

            sead::Vector3f diff = al::getSensorPos(pOther) - al::getSensorPos(pSelf);

            if (al::isNearAngleDegreeHV(diff, al::getFront(this), al::getGravity(this), 45.0f,
                                        60.0f) &&
                !al::sendMsgNekoAttack(pOther, pSelf)) {
                al::sendMsgBallAttack(pOther, pSelf, nullptr);
            }

            return;
        }

        if (!al::isSensorMapObj(pOther) || al::sendMsgBallItemGet(pOther, pSelf) ||
            mCoolTime > 0) {
            return;
        }

        sead::Vector3f dir;
        al::calcDirBetweenSensors(&dir, pSelf, pOther);

        if (!(dir.y > -0.4f) || !al::sendMsgBallAttackCollide(pOther, pSelf)) {
            return;
        }

        if (al::isSensorHostName(pOther, "ブロックスイッチ")) {
            mCoolTime = 60;
        } else if (al::isSensorHostName(pOther, "ビックリボックス★")) {
            mCoolTime = 30;
        } else if (al::isSensorHostName(pOther, "レンガブロック[壊れる]★")) {
            mCoolTime = 5;
        } else {
            mCoolTime = 20;
        }

        return;
    }

    if (isRide() || al::isNerve(this, &NrvNekoNormalBindRideEnd)) {
        if (al::isSensorNpc(pSelf) && al::isNerve(this, &NrvNekoNormalBindRideEnd) &&
            al::isGreaterStep(this, 15) && !al::isSensorPlayer(pOther) &&
            neko::isHitSensorRadius(35.0f, pSelf, pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        return;
    }

    if (isInteractive()) {
        mTargetFinder->attackSensor(pSelf, pOther);
    }

    if (!al::isSensorNpc(pSelf)) {
        return;
    }

    if (neko::isHitSensorRadius(35.0f, pSelf, pOther)) {
        al::sendMsgNpcTouch(pOther, pSelf);

        if (al::isSensorNpc(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }

        if (mIsAtGoal) {
            if (al::isSensorPlayer(pOther)) {
                al::sendMsgPush(pOther, pSelf);
            }

            if (al::isSensorRide(pOther)) {
                al::sendMsgNekoPush(pOther, pSelf);
            }

            return;
        }

        if (al::isSensorRide(pOther) && al::isSensorPlessie(pOther)) {
            al::sendMsgPush(pSelf, pOther);
        }
    }

    if (al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pSelf, pOther);

        if (al::isNerve(this, &NrvNekoNormalPurr) ||
            (al::isNerve(this, &NrvNekoNormalSeekTarget) && mIsAtGoal) ||
            al::isNerve(this, &NrvNekoNormalGoalWait) || mCoolTime > 0) {
            return;
        }

        if (rc::isPlayerClimbOrClimbSpecial(pOther) && mFollowPlayer != nullptr && !isRunAway() &&
            rc::isPlayerOnGround(mFollowPlayer) && !al::isNerve(this, &NrvNekoNormalFall) &&
            !al::isNerve(this, &NrvNekoNormalHitReact) &&
            !al::isNerve(this, &NrvNekoNormalStroke) &&
            !al::isNerve(this, &NrvNekoNormalTrampled) &&
            !al::isNerve(this, &NrvNekoNormalAppear) &&
            neko::trySetNerve(this, &NrvNekoNormalPurr)) {
            return;
        }
    }

    if (al::isNerve(this, &NrvNekoNormalRelease) && al::isSensorBindableNpc(pOther)) {
        rc::sendMsgNpcBindInit(pOther, pSelf);
    }

    if (al::isNerve(this, &NrvNekoNormalRelease) || al::isNerve(this, &NrvNekoNormalChase) ||
        al::isNerve(this, &NrvNekoNormalFollow) || al::isNerve(this, &NrvNekoNormalChaseEnd) ||
        al::isNerve(this, &NrvNekoNormalTargetWait)) {
        if (al::isSensorKickKoura(pOther)) {
            if (al::isNerve(this, &NrvNekoNormalRelease) ||
                (mTargetFinder->getTargetType() & npc::NpcFindTargetType_Player) == 0) {
                if (al::sendMsgBallAttack(pOther, pSelf, nullptr)) {
                    neko::trySetNerve(this, &NrvNekoNormalPounce);
                }
            } else if (neko::isHitSensorRadius(35.0f, pSelf, pOther)) {
                al::sendMsgPushStrong(pOther, pSelf);
            }

            return;
        }

        if (npc::isSensorBall(pOther)) {
            if (al::isNerve(this, &NrvNekoNormalRelease)) {
                if (!neko::trySetNerve(this, &NrvNekoNormalPounce)) {
                    return;
                }

                al::sendMsgBallAttack(pOther, pSelf, nullptr);
            } else if ((mTargetFinder->getTargetType() & npc::NpcFindTargetType_Player) == 0) {
                if (al::sendMsgNekoAttack(pOther, pSelf)) {
                    al::faceToTarget(this, al::getSensorPos(pOther));
                    neko::trySetNerve(this, &NrvNekoNormalTargetWait);
                }
            } else if (neko::isHitSensorRadius(35.0f, pSelf, pOther)) {
                al::sendMsgPushStrong(pOther, pSelf);
            }

            return;
        }

        if (npc::isSensorBird(pOther)) {
            if (!(al::calcDistanceH(this, al::getSensorPos(pOther)) < 200.0f)) {
                return;
            }

            al::sendMsgNekoAttack(pOther, pSelf);

            if (tryStartReactToTarget()) {
                return;
            }

            neko::trySetNerve(this, &NrvNekoNormalPounce);
            return;
        }
    }

    if (neko::isSensorEnemyReactAttack(pOther)) {
        al::sendMsgNekoAttack(pOther, pSelf);
        tryStartHitReact(pSelf, pOther, nullptr);
        return;
    }

    if ((al::isSensorEnemyBody(pOther) || al::isSensorEnemy(pOther) ||
         al::isSensorKoopaJr(pOther)) &&
        (al::isNerve(this, &NrvNekoNormalRelease) || al::isNerve(this, &NrvNekoNormalAttack) ||
         (isFollow() &&
          al::isFaceToTargetDegreeH(this, al::getSensorPos(pOther), al::getFront(this), 20.0f))) &&
        (al::sendMsgNekoAttack(pOther, pSelf) || al::sendMsgBallAttack(pOther, pSelf, nullptr))) {
        neko::trySetNerve(this, &NrvNekoNormalAttack);
        return;
    }

    if (al::isNerve(this, &NrvNekoNormalRelease) &&
        (neko::isSensorMapObjReactAttack(pOther) || al::isSensorNpc(pOther)) &&
        al::sendMsgNekoAttack(pOther, pSelf)) {
        neko::trySetNerve(this, &NrvNekoNormalAttack);
        return;
    }

    if ((al::isSensorBindableGoal(pOther) || al::isSensorBindableGoalItem(pOther)) &&
        al::isNerve(this, &NrvNekoNormalRelease)) {
        al::sendMsgNekoAttack(pOther, pSelf);
    }

    if (al::isNerve(this, &NrvNekoNormalRelease) && rc::isSensorKinopioBrigadeNpc(pOther)) {
        tryStartHitReact(pSelf, pOther, nullptr);
        al::sendMsgNekoAttack(pOther, pSelf);
        return;
    }

    if (al::isNerve(this, &NrvNekoNormalRelease) && al::isSensorMapObj(pOther) &&
        al::sendMsgBallItemGet(pOther, pSelf)) {
        return;
    }

    if (!al::isSensorNpcAvoid(pOther) || !mIsOnGround || !isInteractive() ||
        al::isNerve(this, &NrvNekoNormalRelease) || isFollow() ||
        al::isNerve(this, &NrvNekoNormalRunAwayWait) || al::isNerve(this, &NrvNekoNormalFall) ||
        al::isNerve(this, &NrvNekoNormalHitReact) || al::isNerve(this, &NrvNekoNormalTrampled) ||
        al::isNerve(this, &NrvNekoNormalStroke)) {
        return;
    }

    mAvoidSensor = pOther;
    neko::trySetNerve(this, &NrvNekoNormalRunAwayAvoid);
}

/**
 * @brief Check whether the cat can attack.
 * @return Whether the cat is neither purring, at its goal nor cooling down.
 */
bool NekoNormal::isEnableAttack() const {
    if (al::isNerve(this, &NrvNekoNormalPurr)) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalSeekTarget) && mIsAtGoal) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalGoalWait)) {
        return false;
    }

    return mCoolTime < 1;
}

/**
 * @brief Check whether the cat is chasing a target.
 * @return Whether the cat is chasing or following a target.
 */
bool NekoNormal::isChase() const {
    return al::isNerve(this, &NrvNekoNormalChase) || al::isNerve(this, &NrvNekoNormalFollow) ||
           al::isNerve(this, &NrvNekoNormalChaseEnd);
}

/**
 * @brief Start following, chasing or fleeing the current target.
 * @return Whether a reaction was started.
 */
bool NekoNormal::tryStartReactToTarget() {
    if (mIsAtGoal) {
        return false;
    }

    if (mFollowPlayer != nullptr && isFollowPlayerNear()) {
        al::setNerve(this, &NrvNekoNormalFollow);
        return true;
    }

    if (mTargetFinder->getTarget() == nullptr || mCoolTime > 0) {
        return false;
    }

    u32 targetType = mTargetFinder->getTargetType();

    if (targetType & npc::NpcFindTargetType_PlayerClimb) {
        if (isRunAway() || isChase()) {
            return false;
        }

        if (al::isNerve(this, &NrvNekoNormalAlert)) {
            return neko::trySetNerve(this, &NrvNekoNormalAlertEnd);
        }

        mFollowPlayer = mTargetFinder->getTarget();

        if (mTargetFinder->getTarget() != nullptr && mTargetFinder->isTargetValid()) {
            al::setNerve(this, &NrvNekoNormalFollowStart);
        } else {
            al::setNerve(this, &NrvNekoNormalFaceTarget);
        }

        return true;
    }

    if (targetType & npc::NpcFindTargetType_Koura) {
        if (isRunAway() || isChase()) {
            return false;
        }

        if (al::isNerve(this, &NrvNekoNormalAlert)) {
            al::setNerve(this, &NrvNekoNormalAlertEnd);
        } else {
            al::setNerve(this, &NrvNekoNormalChase);
        }

        return true;
    }

    if (mTargetFinder->isTargetValid()) {
        if ((sFriendlyTargetTypes.getDirect() & targetType) != 0) {
            if (isRunAway() || isChase()) {
                return false;
            }

            if (al::isNerve(this, &NrvNekoNormalAlert)) {
                al::setNerve(this, &NrvNekoNormalAlertEnd);
            } else {
                al::setNerve(this, &NrvNekoNormalChase);
            }

            return true;
        } else if (isRunAway()) {
            if (neko::trySetNerve(this, &NrvNekoNormalRunAway)) {
                return true;
            }
        } else if (neko::trySetNerve(this, &NrvNekoNormalStartleWait)) {
            return true;
        }
    } else if (isRunAway()) {
        if (neko::trySetNerve(this, &NrvNekoNormalRunAway)) {
            return true;
        }
    } else if (neko::trySetNerve(this, &NrvNekoNormalStartleWait)) {
        return true;
    }

    return false;
}

/**
 * @brief Get knocked back by a hit.
 * @param pSelf Sensor of the cat.
 * @param pOther Sensor of the attacker.
 * @param pMsg Received message, nullptr for a hit detected by the cat itself.
 * @return Whether the cat reacted to the hit.
 */
bool NekoNormal::tryStartHitReact(const al::HitSensor* pSelf, const al::HitSensor* pOther,
                                  const al::SensorMsg* pMsg) {
    if (al::isNerve(this, &NrvNekoNormalSeekTarget) ||
        (al::isNerve(this, &NrvNekoNormalHitReact) && mCoolTime > 0)) {
        return false;
    }

    al::setNerve(this, &NrvNekoNormalHitReact);
    sead::Vector3f* velocity = &mHitReactVelocity;

    if (pOther != nullptr) {
        f32 speed = al::isSensorRide(pOther) ? 25.0f : 10.0f;
        al::calcDirBetweenSensorsH(velocity, pOther, pSelf);
        velocity->y = 0.1f;
        al::normalize(velocity);
        *velocity *= speed;
    } else {
        al::calcDirBetweenSensorsH(velocity, pOther, pSelf);
        velocity->y = 0.1f;
        al::normalize(velocity);
        *velocity *= 10.0f;
    }

    if (pMsg != nullptr && neko::isMsgNpcAttackerHitReaction(this, pMsg, pOther, pSelf)) {
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
    } else if (pMsg != nullptr && rc::isMsgPackunEat(pMsg)) {
        sead::Vector3f dir;
        al::calcDirBetweenSensors(&dir, pSelf, pOther);
        dir *= al::getSensorRadius(pSelf);
        al::startHitReactionHitEffect(this, "ＮＰＣヒット", al::getSensorPos(pSelf) + dir);
    } else if (pSelf != nullptr && pOther != nullptr) {
        al::startHitReactionHitEffect(this, "ＮＰＣヒット", pOther, pSelf);
    }

    if (pMsg != nullptr && al::isMsgPlayerFireBallAttack(pMsg)) {
        mCoolTime = 8;
    } else if (pMsg != nullptr && al::isMsgPlayerClimbAttack(pMsg)) {
        mCoolTime = 30;
    } else if (pMsg != nullptr && (al::isMsgNekoAttack(pMsg) || al::isMsgExplosion(pMsg))) {
        mCoolTime = 60;
    } else if (pOther != nullptr && al::isSensorRide(pOther)) {
        mCoolTime = 60;
    } else {
        mCoolTime = 20;
    }

    return true;
}

/**
 * @brief Check whether the cat follows the player.
 * @return Whether the cat follows or purrs at the player.
 */
bool NekoNormal::isFollow() const {
    return al::isNerve(this, &NrvNekoNormalFollowStart) ||
           al::isNerve(this, &NrvNekoNormalFollow) || al::isNerve(this, &NrvNekoNormalFollowWait) ||
           al::isNerve(this, &NrvNekoNormalFollowJumpToPlayer) ||
           al::isNerve(this, &NrvNekoNormalFollowJump) ||
           al::isNerve(this, &NrvNekoNormalFollowJumpEnd) || al::isNerve(this, &NrvNekoNormalPurr);
}

/**
 * @brief React to pushes, attacks, the player carrying the cat and other messages.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the cat.
 * @return Whether the message was handled.
 */
bool NekoNormal::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvNekoNormalSeekTarget)) {
        return false;
    }

    if ((neko::isMsgMeraWanwanTrackAttack(this, pMsg, pOther, nullptr) ||
         al::isMsgDisasterSpikeAttack(pMsg) || al::isMsgGigaEnemyAttack(pMsg) ||
         rc::isMsgNeedleRollerAttack(pMsg)) &&
        mHost != nullptr && mHost->tryStartHide()) {
        return true;
    }

    if (rc::isMsgPackunEatStart(pMsg)) {
        if (mPackunEatCoolTime > 0) {
            return false;
        }

        mPackunEatCoolTime = 120;
        return true;
    }

    if (mIsAtGoal) {
        if (al::isNerve(this, &NrvNekoNormalAppear)) {
            return false;
        }

        if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
            al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg) ||
            al::isMsgPlayerObjStatueDrop(pMsg)) {
            if (mCoolTime > 0 || isRunAway()) {
                return false;
            }

            al::startSe(this, "PgTrample", nullptr);
            rc::requestHitReactionToAttackerNpc(pSelf, pOther);
            mCoolTime = 30;
            al::setNerve(this, &NrvNekoNormalTrampled);
            return true;
        }

        if (neko::isMsgHitReaction(this, pMsg, pOther, pSelf) || al::isMsgKickKouraReflect(pMsg) ||
            al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerObjHipDropAll(pMsg)) {
            return tryStartHitReact(pSelf, pOther, pMsg);
        }

        return false;
    }

    if (mBindPuppeteers[0]->isTargetSensor(pOther) &&
        mBindPuppeteers[0]->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    if (mBindPuppeteers[1]->isTargetSensor(pOther) &&
        mBindPuppeteers[1]->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    if (rc::isMsgAskControlUserId(pMsg, mHolderSensor)) {
        return true;
    }

    if (isEnableHold() && al::isSensorPlayer(pOther) && al::isMsgPlayerCarryFrontNeko(pMsg)) {
        if (!mStatePlayerHold->tryStartCarryFront(pMsg, pOther, false)) {
            return false;
        }

        mHolderSensor = pOther;
        al::setNerve(this, &NrvNekoNormalHold);
        return true;
    }

    if (isHold()) {
        if (al::isMsgPlayerHideItem(pMsg)) {
            al::hideModelIfShow(this);
            al::tryKillEmitterAndParticleAll(this);
        } else if (al::isMsgPlayerShowItem(pMsg)) {
            al::showModelIfHide(this);
        }

        if (al::isMsgKickKouraBlow(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);
            bool isHolding = al::isNerve(this, &NrvNekoNormalHold);
            releaseDropBack();

            if (!isHolding) {
                mBindPuppeteer->stopBind();
            }

            return true;
        }

        if (al::isMsgPlayerRelease(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);

            if (rc::isPlayerBinded(mHolderSensor, pSelf)) {
                tryStartDefaultBehavior(al::param::NekoBehavior_Default);
            }

            if (al::isNerve(this, &NrvNekoNormalHold)) {
                tryKillPlayerHoldState(mStatePlayerHold, pMsg, pOther);
                releaseThrow();
            } else {
                releaseDrop();
                mBindPuppeteer->stopBind();
            }

            return true;
        }

        if (al::isMsgPlayerReleaseDamage(pMsg) || al::isMsgPlayerReleaseDead(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);

            if (al::isNerve(this, &NrvNekoNormalHold)) {
                tryKillPlayerHoldState(mStatePlayerHold, pMsg, pOther);
                releaseDamage(pSelf, pOther);
            } else {
                releaseDamage(pSelf, pOther);
                mBindPuppeteer->stopBind();
            }

            return true;
        }

        if (al::isMsgHoldCancel(pMsg) || al::isMsgWarpStart(pMsg)) {
            al::resetActorCollisionController(mCollisionController, 1);

            if (al::isNerve(this, &NrvNekoNormalHold)) {
                tryKillPlayerHoldState(mStatePlayerHold, pMsg, pOther);
                releaseRide();
            } else {
                releaseRide();
                mBindPuppeteer->stopBind();
            }

            rc::sendMsgNpcBindInit(pOther, pSelf);
            return true;
        }

        return false;
    }

    if (isRide()) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalBindRideEnd)) {
        if (!al::isGreaterStep(this, 15)) {
            return false;
        }

        if (npc::isSensorNeko(pOther) && al::getVelocity(this).y < 0.0f) {
            neko::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 0.3f, 35.0f,
                                                   mIsOnGround, false);
        } else {
            neko::tryReceiveMsgPushAndAddVelocityH(this, pMsg, pOther, pSelf, 2.0f, 35.0f,
                                                   mIsOnGround, false);
        }

        return false;
    }

    if (rc::isMsgJumpPanelAction(pMsg) &&
        tryBounce(rc::isMsgJumpPanelActionAndSuperJump(pMsg) ? 100.0f : 60.0f)) {
        return true;
    }

    if (neko::tryReceiveMsgPushAndAddVelocityH(
            this, pMsg, pOther, pSelf, 2.0f, 35.0f,
            mIsOnGround ? mParam->mIsEnableCliffCheck : false, mParam->mIsEnableShoreCheck)) {
        if (isWait()) {
            al::setNerve(this, &NrvNekoNormalWaitPush);
        }

        mIsPushed = true;
        return true;
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg) ||
        al::isMsgPlayerObjStatueDrop(pMsg)) {
        if (al::isSensorRide(pOther)) {
            return tryStartHitReact(pSelf, pOther, pMsg);
        }

        if (mCoolTime > 0 || isRunAway()) {
            return false;
        }

        al::startSe(this, "PgTrample", nullptr);
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        mCoolTime = 30;
        al::setNerve(this, &NrvNekoNormalTrampled);
        return true;
    }

    if (neko::isMsgHitReaction(this, pMsg, pOther, pSelf) || al::isMsgBlockUpperPunch(pMsg)) {
        if (al::isMsgBallAttack(pMsg) &&
            ((mTargetFinder->getTargetType() & npc::NpcFindTargetType_Ball) ||
             al::isNerve(this, &NrvNekoNormalPounce))) {
            return false;
        }

        if (al::isSensorHostName(pOther, "ファイアーバー") && mHost != nullptr &&
            mHost->tryStartHide()) {
            return true;
        }

        if (isFollow() && al::isMsgNekoAttack(pMsg)) {
            return true;
        }

        return tryStartHitReact(pSelf, pOther, pMsg);
    }

    if (al::isMsgGigaBellPush(pMsg) && mHost != nullptr && mHost->tryStartHide()) {
        return true;
    }

    return false;
}

/**
 * @brief Check whether the player can pick the cat up.
 * @return Whether the cat is free and not cooling down.
 */
bool NekoNormal::isEnableHold() const {
    if (al::isNerve(this, &NrvNekoNormalBindRide)) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalBindRideEnd)) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalHold)) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalRelease)) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalSeekTarget) && mIsAtGoal) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalGoalWait)) {
        return false;
    }

    return mCoolTime < 1;
}

/**
 * @brief Drop the cat behind the player holding it.
 */
void NekoNormal::releaseDropBack() {
    startRelease(-rc::getPlayerFront(mHolderSensor), sDropBackSpeed.x,
                 sDropBackSpeed.y);
}

/**
 * @brief Throw the cat in front of the player holding it.
 */
void NekoNormal::releaseThrow() {
    sead::Vector3f front = rc::getPlayerFront(mHolderSensor);
    const sead::Vector3f& holderVelocity = al::getActorVelocity(mHolderSensor);
    // The result is not used.
    sead::Mathf::sqrt(holderVelocity.x * holderVelocity.x + holderVelocity.z * holderVelocity.z);
    startRelease(front, rc::getPlayerSpeedH(mHolderSensor) + 30.0f, 30.0f);
}

/**
 * @brief Drop the cat, in front of the player if it stands still, behind it otherwise.
 */
void NekoNormal::releaseDrop() {
    f32 speed = rc::getPlayerSpeedH(mHolderSensor);
    const sead::Vector3f& front = rc::getPlayerFront(mHolderSensor);

    if (speed < 0.5f) {
        startRelease(front, sDropFrontSpeed.x, sDropFrontSpeed.y);
    } else {
        startRelease(-front, sDropBackSpeed.x, sDropBackSpeed.y);
    }
}

/**
 * @brief Drop the cat away from what hurt the player holding it.
 * @param pSelf Sensor of the cat.
 * @param pOther Sensor of the player.
 */
void NekoNormal::releaseDamage(const al::HitSensor* pSelf, const al::HitSensor* pOther) {
    sead::Vector3f dir;
    al::calcDirBetweenSensorsH(&dir, pSelf, pOther);
    startRelease(-dir, 10.3f, 27.5f);
}

/**
 * @brief Drop the cat behind the player when it stops holding it to ride something.
 */
void NekoNormal::releaseRide() {
    startRelease(-rc::getPlayerFront(mHolderSensor), rc::getPlayerSpeedH(mHolderSensor), 2.0f);
}

/**
 * @brief Check whether the cat is idle.
 * @return Whether the cat plays an idle behavior.
 */
bool NekoNormal::isWait() const {
    return al::isNerve(this, &NrvNekoNormalWaitStart) || al::isNerve(this, &NrvNekoNormalWait) ||
           al::isNerve(this, &NrvNekoNormalWaitEnd);
}

/**
 * @brief React to the touch screen pointer.
 * @param pMsg Received message.
 * @param pPointer The screen pointer.
 * @param pTarget The touched screen point target.
 * @return Whether the cat got stroked.
 */
bool NekoNormal::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvNekoNormalSeekTarget) && mIsAtGoal) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalGoalWait)) {
        return false;
    }

    if (isHold()) {
        return false;
    }

    if (isRide()) {
        return false;
    }

    mTargetFinder->receiveMsgScreenPoint(pMsg, pPointer, pTarget);

    if (al::isScreenPointTargetName(pTarget, "Eye")) {
        return false;
    }

    if (mStateSupportStroke->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvNekoNormalStroke)) {
            al::setNerve(this, &NrvNekoNormalStroke);
        }

        return true;
    }

    return false;
}

/**
 * @brief Check whether the cat reached its goal.
 * @return Whether the cat is at its goal.
 */
bool NekoNormal::isEnableGoal() const {
    return (al::isNerve(this, &NrvNekoNormalSeekTarget) && mIsAtGoal) ||
           al::isNerve(this, &NrvNekoNormalGoalWait);
}

/**
 * @brief Bounce the cat back when the actor it was bound to cancels the binding.
 * @param pPuppeteer Puppeteer whose binding got cancelled.
 * @param pSensor Sensor of the binding actor.
 */
void NekoNormal::receivedBindCancel(NekoBindPuppeteer* pPuppeteer, al::HitSensor* pSensor) {
    if (mBindPuppeteer != pPuppeteer) {
        return;
    }

    al::setVelocityZero(this);
    const sead::Vector3f& velocity = al::getVelocity(this);
    sead::Vector3f dir = {-velocity.x, 0.0f, -velocity.z};
    al::normalizeOrDirZ(&dir);
    dir.x *= 30.0f;
    dir.z *= 30.0f;
    dir.y = 30.0f;
    al::setVelocity(this, dir);
    al::setNerve(this, &NrvNekoNormalRelease);
}

/**
 * @brief Start an idle behavior.
 */
void NekoNormal::exeWaitStart() {
    const NekoStateWaitParam* param = getWaitParam(mBehavior);

    if (al::isFirstStep(this)) {
        if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
            al::validateClipping(this);
        }

        const char* action;

        if (mBehavior == al::param::NekoBehavior_LayDown) {
            action = "WaitLayDownStart";
        } else if (mBehavior == al::param::NekoBehavior_Sit) {
            if (isActionStanding(this)) {
                action = "WaitSitStart";
            } else {
                action = al::isActionPlaying(this, "WaitLayDown") ? "WaitLayDownEnd" : nullptr;
            }
        } else {
            action = param->mStartAction;
        }

        al::tryStartActionIfNotPlaying(this, action);
        mFollowPlayer = nullptr;

        if (rc::isInAreaObj(this, rc::AreaObjType::OffCollideArea)) {
            al::offCollide(this);
        } else {
            al::setVelocityToGravity(this, sStateParam.getGravity());
        }
    }

    updatePassiveMovement();
    mTargetFinder->update();

    if (tryStartReactToTarget()) {
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNekoNormalWait);
    }
}

/**
 * @brief Play an idle behavior until its time is up.
 */
void NekoNormal::exeWait() {
    const NekoStateWaitParam* param = getWaitParam(mBehavior);

    if (al::isFirstStep(this)) {
        if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
            al::validateClipping(this);
        }

        const char* action = getRandomSubAction(param);

        if (action == nullptr || al::getRandom() < 0.5f) {
            action = param->mWaitAction;
        }

        al::tryStartActionIfNotPlaying(this, action);
        mActionTime = 240;
        mFollowPlayer = nullptr;

        if (rc::isInAreaObj(this, rc::AreaObjType::OffCollideArea)) {
            al::offCollide(this);
        } else {
            al::setVelocityToGravity(this, sStateParam.getGravity());
        }
    }

    updatePassiveMovement();
    mTargetFinder->update();

    if (tryStartReactToTarget()) {
        return;
    }

    if (mBehavior == al::param::NekoBehavior_Wait && neko::isActive(this, 2000.0f)) {
        al::setNerve(this, &NrvNekoNormalWaitEnd);
        return;
    }

    if (al::isActionOneTime(this, al::getActionName(this))) {
        if (al::isActionEnd(this)) {
            al::tryStartActionIfNotPlaying(this, param->mWaitAction);
            mActionTime = 240;
        }
    } else {
        if (al::isGreaterStep(this, mBehaviorTime)) {
            al::setNerve(this, &NrvNekoNormalWaitEnd);
            return;
        }

        if (mActionTime <= 0) {
            const char* action = getRandomSubAction(param);

            if (action != nullptr && al::getRandom() > 0.5f) {
                al::tryStartActionIfNotPlaying(this, action);
            }

            mActionTime = 240;
        }
    }

    mActionTime--;
}

/**
 * @brief End an idle behavior, then start the next one.
 */
void NekoNormal::exeWaitEnd() {
    mTargetFinder->update();

    if (tryStartReactToTarget()) {
        return;
    }

    const NekoStateWaitParam* param = getWaitParam(mBehavior);

    if (param->mEndAction != nullptr) {
        if (al::isFirstStep(this)) {
            al::tryStartActionIfNotPlaying(this, param->mEndAction);
        }

        updatePassiveMovement();

        if (!al::isActionEnd(this)) {
            return;
        }
    }

    tryStartDefaultBehavior(al::param::NekoBehavior_Default);
}

/**
 * @brief Get pushed around while idle.
 */
void NekoNormal::exeWaitPush() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::tryStartActionIfNotPlaying(this, "Walk");
    }

    updatePassiveMovement();
    tryLimitMove();
    mTargetFinder->update();

    if (tryStartReactToTarget()) {
        return;
    }

    if (neko::isInMotion(this)) {
        return;
    }

    tryStartDefaultBehavior(al::param::NekoBehavior_Default);
}

/**
 * @brief Wander around.
 */
void NekoNormal::exeWander() {
    if (al::isFirstStep(this)) {
        if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
            al::validateClipping(this);
        }

        al::onCollide(this);
        mFollowPlayer = nullptr;
    }

    mTargetFinder->update();

    if (tryStartReactToTarget()) {
        return;
    }

    al::updateNerveState(this);

    if (!neko::isActive(this, 3000.0f)) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    if (al::isGreaterStep(this, mBehaviorTime) &&
        tryStartDefaultBehavior(al::param::NekoBehavior_Default)) {
        return;
    }

    mLookAtPos = mStateWander->getTargetPos();
    mHeadController->setLookAtTarget(&mLookAtPos);
    neko::setAnimationRate(this);
}

/**
 * @brief Fall until the cat lands on the ground.
 */
void NekoNormal::exeFall() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::onCollide(this);
    }

    if (mIsOnGround) {
        updatePassiveMovement();
        tryLimitMove();

        if (al::isActionPlaying(this, "Fall")) {
            al::startAction(this, "ThrowEnd");
        }

        if (!mIsPushed) {
            al::setVelocityZeroH(this);
        }

        mTargetFinder->update();

        if (al::isActionPlaying(this, "ThrowEnd") && !al::isActionEnd(this)) {
            return;
        }

        if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
            al::validateClipping(this);
        }

        if (tryStartReactToTarget()) {
            return;
        }

        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    if (al::isNoCollide(this)) {
        al::setVelocityZero(this);
    } else {
        NpcStateFunction::calcPassiveMovement(this, &sFallStateParam, mIsOnGround);
    }

    if (al::getVelocity(this).y > 0.0f) {
        al::tryStartActionIfNotPlaying(this, "Up");
    } else if (al::isGreaterStep(this, 8)) {
        al::tryStartActionIfNotPlaying(this, "Fall");
    }
}

/**
 * @brief Get startled by a target, then flee from it.
 */
void NekoNormal::exeStartle() {
    mTargetFinder->update();

    if (al::isFirstStep(this)) {
        al::startAction(this, "Startle");
        al::setVelocityZeroH(this);
    }

    updatePassiveMovement();
    bool isTurnEnd = false;

    if (al::isActionPlaying(this, "Startle")) {
        if (al::isActionEnd(this) &&
            (mTargetFinder->getTarget() != nullptr || mTargetFinder->getLastTarget() != nullptr) &&
            al::isActionPlaying(this, "Startle")) {
            sead::Vector3f lastTargetPos = mTargetFinder->getLastTargetPos();
            sead::Vector3f dir = lastTargetPos - al::getTrans(this);
            al::normalizeOrDirZ(&dir);
            sead::Vector3f side;
            al::calcSideDir(&side, this);

            if (side.dot(dir) > 0.0f) {
                al::startAction(this, "TurnRightStartle");
            } else {
                al::startAction(this, "TurnLeftStartle");
            }
        }
    } else if (mTargetFinder->getTarget() == nullptr &&
               mTargetFinder->getLastTarget() == nullptr) {
        isTurnEnd = true;
    } else {
        isTurnEnd = al::turnDirectionFromTargetDegree(this, al::getFrontPtr(this),
                                                      mTargetFinder->getLastTargetPos(), 12.0f);
    }

    if ((mTargetFinder->getTargetType() & sFriendlyTargetTypes.getDirect()) == 0 &&
        isNearPlayer(200.0f)) {
        al::setNerve(this, &NrvNekoNormalRunAwayFast);
    } else if (isTurnEnd) {
        al::setNerve(this, &NrvNekoNormalRunAwayFast);
    }
}

/**
 * @brief Check whether the current target is near the cat.
 * @param range Distance under which the target is near.
 * @return Whether the target is near.
 */
bool NekoNormal::isNearPlayer(f32 range) const {
    if (!(mTargetFinder->getParam()->getChaseRange() < range) &&
        mTargetFinder->getTarget() != nullptr &&
        al::calcDistance(this, mTargetFinder->getTargetPos()) < range) {
        return true;
    }

    return false;
}

/**
 * @brief Walk carefully while watching a target that startled the cat.
 */
void NekoNormal::exeStartleWait() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Walk");
        mActionTime = al::getRandom(100, 180);
        al::onCollide(this);
    }

    if (mIsOnGround) {
        al::addVelocityToDirection(this, al::getFront(this), 0.5f);
        neko::setAnimationRate(this);
    }

    updatePassiveMovement();
    tryLimitMove();

    if (al::isLessStep(this, 5)) {
        mHeadController->requestResetLook();
    }

    mTargetFinder->update();
    bool isTurnEnd = false;
    al::LiveActor* target = mTargetFinder->getTarget();

    if (target != nullptr) {
        isTurnEnd = al::turnDirectionToTargetDegree(this, al::getFrontPtr(this),
                                                    al::getTrans(target), 5.0f);
    }

    if ((mTargetFinder->getTargetType() & sFriendlyTargetTypes.getDirect()) != 0) {
        if (tryStartReactToTarget()) {
            return;
        }
    } else {
        if (isTurnEnd ||
            (mTargetFinder->getTarget() != nullptr && mTargetFinder->isTargetValid())) {
            al::setNerve(this, &NrvNekoNormalStartle);
            return;
        }

        if (isNearPlayer(400.0f)) {
            al::setNerve(this, &NrvNekoNormalRunAway);
            return;
        }

        if (isNearPlayer(200.0f)) {
            al::setNerve(this, &NrvNekoNormalRunAwayFast);
            return;
        }
    }

    if (mTargetFinder->getTarget() == nullptr || !mTargetFinder->isTargetInChaseRange()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    if (al::isGreaterEqualStep(this, mActionTime)) {
        al::setNerve(this, &NrvNekoNormalRunAway);
    }
}

/**
 * @brief Get knocked back by a hit.
 */
void NekoNormal::exeHitReact() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::startAction(this, "HitReact");
        al::showModelIfHide(this);

        if (!mIsAtGoal) {
            al::addVelocity(this, mHitReactVelocity);
            al::faceToDirection(this, mHitReactVelocity);
        }
    }

    updatePassiveMovement();
    tryLimitMove();
    mTargetFinder->update();

    if (!al::isActionEnd(this)) {
        return;
    }

    if (mIsAtGoal) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    if (mFollowPlayer != nullptr && tryStartReactToTarget()) {
        return;
    }

    al::setNerve(this, &NrvNekoNormalRunAwayFast);
}

/**
 * @brief Get trampled by the player.
 */
void NekoNormal::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Trampled");
        al::onCollide(this);
        al::showModelIfHide(this);
    }

    updatePassiveMovement();
    tryLimitMove();

    if (!al::isActionEnd(this)) {
        return;
    }

    if (mIsAtGoal) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    al::setNerve(this, &NrvNekoNormalRunAwayFast);
}

/**
 * @brief Run away from the current target.
 */
void NekoNormal::exeRunAway() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvNekoNormalRunAwayEnd);
        return;
    }

    lookAtRunAwayDir(false);

    if (isNearPlayer(200.0f)) {
        al::setNerve(this, &NrvNekoNormalRunAwayFast);
        return;
    }

    if (mTargetFinder->isTargetChanged() && tryStartReactToTarget()) {
        return;
    }

    if (mStateRunAway->isRunStraight() && mRunAwayTime++ >= 110) {
        al::setNerve(this, &NrvNekoNormalRunAwayWait);
        mRunAwayTime = 0;
        return;
    }

    neko::setAnimationRate(this);
}

/**
 * @brief Run away from the current target as fast as possible.
 */
void NekoNormal::exeRunAwayFast() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        mRunAwayTime = 0;
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvNekoNormalRunAwayEnd);
        return;
    }

    lookAtRunAwayDir(true);
    neko::setAnimationRate(this);
}

/**
 * @brief Run away from an area the cat must avoid.
 */
void NekoNormal::exeRunAwayAvoid() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Run");
        al::onCollide(this);
    }

    if (mAvoidSensor == nullptr) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    sead::Vector3f avoidPos = al::getSensorPos(mAvoidSensor);

    if (mIsOnGround &&
        al::calcDistanceH(this, avoidPos) < al::getSensorRadius(mAvoidSensor) + 100.0f) {
        al::turnDirectionFromTargetDegree(this, al::getFrontPtr(this), avoidPos, 0.7f);
        al::addVelocity(this, sead::Vector3f(al::getFront(this)));
    } else if (al::isGreaterEqualStep(this, 20) || al::isVelocitySlow(this, 1.0f)) {
        mTargetFinder->update();

        if (!tryStartReactToTarget()) {
            tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        }

        mAvoidSensor = nullptr;
        return;
    }

    lookAwayFrom(avoidPos);
    updatePassiveMovement();

    if ((tryLimitMove() || al::isCollidedWallFace(this)) && mRunAwayTime++ >= 110) {
        al::setNerve(this, &NrvNekoNormalRunAwayWait);
        mAvoidSensor = nullptr;
        mRunAwayTime = 0;
        return;
    }

    neko::setAnimationRate(this);
}

/**
 * @brief Wait after running away until the threat is gone.
 */
void NekoNormal::exeRunAwayWait() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::tryStartActionIfNotPlaying(this, "Wait");
        mRunAwayTime = 0;
    }

    mTargetFinder->update();
    updatePassiveMovement();
    tryLimitMove();

    if (isNearPlayer(350.0f)) {
        al::setNerve(this, &NrvNekoNormalRunAwayFast);
    } else if (mAvoidSensor != nullptr || mTargetFinder->getTarget() != nullptr) {
        return;
    } else {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    }

    mAvoidSensor = nullptr;
}

/**
 * @brief Stop running away, then keep an eye on the player or start the default behavior.
 */
void NekoNormal::exeRunAwayEnd() {
    mTargetFinder->update();

    if (tryStartReactToTarget()) {
        return;
    }

    if (isNearPlayer(300.0f) || isLimitMoveNext()) {
        al::setVelocityToGravity(this, 0.1f);
        al::setNerve(this, &NrvNekoNormalRunAway);
        return;
    }

    if (al::isGreaterEqualStep(this, 20) || al::isVelocitySlow(this, 1.0f)) {
        if (mTargetFinder->getLastTargetType() & npc::NpcFindTargetType_Player) {
            al::setNerve(this, &NrvNekoNormalAlert);
            return;
        }

        if (tryStartReactToTarget()) {
            return;
        }

        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    updatePassiveMovement();
    neko::setAnimationRate(this);
}

/**
 * @brief Chase the current target.
 */
void NekoNormal::exeChase() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Run");
        al::onCollide(this);
    }

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvNekoNormalChaseEnd);
        return;
    }

    if (mTargetFinder->isTargetChanged() && tryStartReactToTarget()) {
        return;
    }

    if ((mTargetFinder->getTargetType() & sFriendlyTargetTypes.getDirect()) == 0) {
        al::setNerve(this, &NrvNekoNormalChaseEnd);
        return;
    }

    neko::setAnimationRate(this);
}

/**
 * @brief Stop chasing, then start wandering.
 */
void NekoNormal::exeChaseEnd() {
    // The first step is checked, but nothing happens on it.
    al::isFirstStep(this);

    if (al::isGreaterEqualStep(this, 20)) {
        al::setNerve(this, &NrvNekoNormalWander);
    }

    if (isLimitMoveNext()) {
        al::setVelocityToGravity(this, 0.1f);
        al::setNerve(this, &NrvNekoNormalWander);
        return;
    }

    updatePassiveMovement();
    neko::setAnimationRate(this);
}

/**
 * @brief Look around for the player that scared the cat.
 */
void NekoNormal::exeAlert() {
    if (al::isFirstStep(this)) {
        mLookAtPos = al::findNearestPlayerPos(this);
        al::setVelocityZeroH(this);
        sead::Vector3f lookAtPos = mLookAtPos;
        sead::Vector3f dir = lookAtPos - al::getTrans(this);
        al::normalizeOrDirZ(&dir);
        sead::Vector3f side;
        al::calcSideDir(&side, this);

        if (side.dot(dir) > 0.0f) {
            al::startAction(this, "WaitLookRightStart");
        } else {
            al::startAction(this, "WaitLookLeftStart");
        }

        mActionTime = 999;
    }

    mTargetFinder->update();

    if (al::isNoCollide(this)) {
        al::setVelocityZero(this);
    } else {
        NpcStateFunction::calcPassiveMovement(this, &sStateParam, mIsOnGround);
    }

    if (al::isActionPlaying(this, "WaitLookRightStart") && al::isActionEnd(this)) {
        al::startAction(this, "WaitLookRight");
    } else if (al::isActionPlaying(this, "WaitLookLeftStart") && al::isActionEnd(this)) {
        al::startAction(this, "WaitLookLeft");
    }

    if (al::isGreaterStep(this, 80)) {
        mActionTime = 0;

        if (!tryStartReactToTarget() &&
            !al::isNear(this, al::findNearestPlayerPos(this), 1000.0f)) {
            al::setNerve(this, &NrvNekoNormalAlertEnd);
        }
    }
}

/**
 * @brief Stop looking around, then sit down.
 */
void NekoNormal::exeAlertEnd() {
    if (al::isFirstStep(this)) {
        if (al::isActionPlaying(this, "WaitLookRight")) {
            al::startAction(this, "WaitLookRightEnd");
            mLookAtPos = al::getFront(this);
            al::rotateVectorDegree(&mLookAtPos, mLookAtPos, al::getGravity(this), 179.0f);
        } else if (al::isActionPlaying(this, "WaitLookLeft")) {
            al::startAction(this, "WaitLookLeftEnd");
            mLookAtPos = al::getFront(this);
            al::rotateVectorDegree(&mLookAtPos, mLookAtPos, al::getGravity(this), -179.0f);
        }
    }

    mTargetFinder->update();

    if (al::isNoCollide(this)) {
        al::setVelocityZero(this);
    } else {
        NpcStateFunction::calcPassiveMovement(this, &sStateParam, mIsOnGround);
    }

    al::turnDirectionDegree(this, al::getFrontPtr(this), mLookAtPos,
                            180.0f / al::getActionFrameMax(this, "WaitLookRightEnd"));

    if (al::isActionEnd(this) && !tryStartReactToTarget()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Sit);
    }
}

/**
 * @brief Turn towards the current target.
 */
void NekoNormal::exeFaceTarget() {
    if (al::isFirstStep(this)) {
        mLookAtPos = mFollowPlayer != nullptr ? al::getTrans(mFollowPlayer) :
                                                mTargetFinder->getTargetPos();
        sead::Vector3f lookAtPos = mLookAtPos;
        sead::Vector3f dir = lookAtPos - al::getTrans(this);
        al::normalizeOrDirZ(&dir);
        sead::Vector3f side;
        al::calcSideDir(&side, this);

        if (side.dot(dir) > 0.0f) {
            al::startAction(this, "TurnLeft");
        } else {
            al::startAction(this, "TurnRight");
        }
    }

    mTargetFinder->update();

    if (mFollowPlayer == nullptr && mTargetFinder->getTarget() == nullptr) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        return;
    }

    updatePassiveMovement();

    if (!al::turnDirectionToTargetDegree(this, al::getFrontPtr(this), mLookAtPos, 5.0f)) {
        return;
    }

    if (mFollowPlayer != nullptr) {
        al::setNerve(this, &NrvNekoNormalFollowStart);
        return;
    }

    if (!tryStartReactToTarget()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    }
}

/**
 * @brief Notice the player and start following it.
 */
void NekoNormal::exeFollowStart() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, "Find");
        al::onCollide(this);
    }

    updatePassiveMovement();

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvNekoNormalFollow);
    }

    if (isFollowPlayerNear()) {
        return;
    }

    mFollowPlayer = nullptr;
    mTargetFinder->update();

    if (tryStartReactToTarget()) {
        return;
    }

    tryStartDefaultBehavior(al::param::NekoBehavior_Default);
}

/**
 * @brief Follow the player.
 */
void NekoNormal::exeFollow() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        mFollowSpeed = al::calcSpeedH(this) / 10.0f;
        mActionTime = 20;

        if (al::calcDistanceH(this, mFollowPlayer) < 400.0f) {
            al::tryStartActionIfNotPlaying(this, "Walk");
        } else {
            al::tryStartActionIfNotPlaying(this, "Run");
        }
    }

    f32 distanceH = al::calcDistanceH(this, mFollowPlayer);
    updatePassiveMovement();

    if (mIsOnGround) {
        mFollowSpeed *= 0.97f;
        bool isRun = al::isActionPlaying(this, "Run");

        if (distanceH > 500.0f) {
            al::tryStartActionIfNotPlaying(this, "Run");
            isRun = true;
        } else if (distanceH < 400.0f) {
            f32 playerSpeed = al::calcSpeedH(mFollowPlayer) / 10.0f;

            if (playerSpeed < 0.4f) {
                al::tryStartActionIfNotPlaying(this, "Walk");
                isRun = false;
            }

            mFollowSpeed = sead::Mathf::clamp(playerSpeed, 0.2f, 1.5f);
        }

        f32 maxSpeed = isRun ? 1.5f : 0.4f;
        f32 turnDegree = isRun ? 4.0f : 5.0f;
        mFollowSpeed = sead::Mathf::min(mFollowSpeed + 0.05f, maxSpeed);
        sead::Vector3f* front = al::getFrontPtr(this);
        al::turnDirectionToTargetDegree(this, front, al::getTrans(mFollowPlayer), turnDegree);
        al::addVelocity(this, {mFollowSpeed * front->x, mFollowSpeed * front->y,
                               mFollowSpeed * front->z});
    }

    if ((mChaseParam->isEnableCliffCheck() &&
         NpcStateFunction::isFallNextMove(this, 100.0f, 150.0f, 150.0f, false)) ||
        (mParam->mIsEnableShoreCheck &&
         NpcStateFunction::isNPCAvoidAreaNextMove(this, 150.0f, 20.0f, 100.0f, false)) ||
        (al::isCollidedWall(this) && NpcStateFunction::isWallNextMove(this, 100.0f, 20.0f))) {
        al::setVelocityToGravity(this, 0.1f);

        if (mActionTime-- <= 0) {
            al::setNerve(this, &NrvNekoNormalFollowWait);
            return;
        }
    } else {
        mActionTime = 20;
    }

    if (!isFollowPlayerNear()) {
        mFollowPlayer = nullptr;

        if (!tryStartReactToTarget()) {
            tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        }

        return;
    }

    neko::setAnimationRate(this);

    if ((distanceH < 400.0f && !rc::isPlayerOnGround(mFollowPlayer)) ||
        (al::calcDistanceV(this, mFollowPlayer) > 150.0f &&
         al::calcDistanceH(this, mFollowPlayer) < 100.0f)) {
        al::setNerve(this, &NrvNekoNormalFollowWait);
    }
}

/**
 * @brief Wait for the player to come back within reach while following it.
 */
void NekoNormal::exeFollowWait() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, isActionStanding(this) ? "WaitSitStart" : "WaitSit");
    }

    if (al::isActionOneTime(this, al::getActionName(this)) && al::isActionEnd(this)) {
        al::startAction(this, "WaitSit");
    }

    updatePassiveMovement();
    sead::Vector3f dir;
    al::calcDirH(&dir, al::getTrans(this), al::getTrans(mFollowPlayer));
    sead::Vector3f trans = al::getTrans(this);
    sead::Vector3f gravity = al::getGravity(this);
    bool isBlocked;

    if ((mChaseParam->isEnableCliffCheck() &&
         NpcStateFunction::isFallNextMove(this, trans, dir, gravity, 150.0f, 150.0f, 110.0f,
                                          false)) ||
        (mParam->mIsEnableShoreCheck &&
         NpcStateFunction::isNPCAvoidAreaNextMove(this, trans, dir, gravity, 150.0f, 20.0f, 100.0f,
                                                  false)) ||
        NpcStateFunction::isWallNextMove(this, trans, dir, gravity, 100.0f, 20.0f)) {
        al::setVelocityToGravity(this, 0.1f);
        isBlocked = true;
    } else {
        isBlocked = false;
    }

    if (al::isGreaterStep(this, 70)) {
        al::LiveActor* player = mFollowPlayer;

        if (!(al::calcDistanceH(this, player) > 1000.0f) &&
            !(al::calcDistanceV(this, player) > 800.0f) &&
            (al::calcDistanceV(this, mFollowPlayer) < 150.0f ||
             al::calcDistanceH(this, mFollowPlayer) > 100.0f)) {
            if (!isBlocked) {
                al::setNerve(this, &NrvNekoNormalFollow);
                return;
            }
        } else {
            mFollowPlayer = nullptr;
            mTargetFinder->update();

            if (!tryStartReactToTarget()) {
                tryStartDefaultBehavior(al::param::NekoBehavior_Default);
            }

            return;
        }
    }

    al::LiveActor* player = mFollowPlayer;

    if (rc::isPlayerDeadOrBubble(player) || !rc::isPlayerClimbOrClimbSpecial(player)) {
        mFollowPlayer = nullptr;

        if (!tryStartReactToTarget()) {
            tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        }
    }
}

/**
 * @brief Jump while following the player.
 */
void NekoNormal::exeFollowJump() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "Pounce");
        sead::Vector3f front = al::getFront(this);

        if (al::calcDistance(this, mFollowPlayer) > 100.0f) {
            front.x *= 10.0f;
            front.z *= 10.0f;
        } else {
            front.x = -front.x;
            front.z = -front.z;
        }

        front.y = 40.0f;
        al::addVelocity(this, front);
    }

    NpcStateFunction::calcPassiveMovement(this, &sJumpStateParam, mIsOnGround);

    if (al::isActionPlaying(this, "Pounce") && al::isStep(this, 20)) {
        al::stopAction(this);
    }

    if (al::getVelocity(this).y < 0.0f) {
        al::tryStartActionIfNotPlaying(this, "Fall");
    }

    if (al::isGreaterStep(this, 10) && mIsOnGround) {
        al::setNerve(this, &NrvNekoNormalFollowJumpEnd);
    }
}

/**
 * @brief Jump to the player.
 */
void NekoNormal::exeFollowJumpToPlayer() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::startAction(this, "Pounce");
        sead::Vector3f playerTrans = al::getTrans(mFollowPlayer);
        sead::Vector3f dir;
        al::calcDirH(&dir, al::getTrans(this), playerTrans);
        dir.normalize();
        f32 speed = sead::Mathf::clamp(al::calcDistanceH(this, playerTrans) * 0.03f, 0.0f, 20.0f);
        dir.x = speed * dir.x;
        dir.z = speed * dir.z;
        dir.y = 40.0f;
        al::addVelocity(this, dir);
    }

    if (al::isActionPlaying(this, "Pounce") && al::isStep(this, 20)) {
        al::stopAction(this);
    }

    if (al::getVelocity(this).y < 0.0f) {
        al::tryStartActionIfNotPlaying(this, "Fall");
    }

    NpcStateFunction::calcPassiveMovement(this, &sJumpStateParam, mIsOnGround);
    sead::Vector3f* front = al::getFrontPtr(this);
    al::turnDirectionToTargetDegree(this, front, al::getTrans(mFollowPlayer), 5.0f);
    f32 playerSpeed = al::calcSpeedH(mFollowPlayer);
    al::addVelocity(this, {playerSpeed * front->x * 0.05f, playerSpeed * front->y * 0.05f,
                           playerSpeed * front->z * 0.05f});

    if (al::isGreaterStep(this, 10) && mIsOnGround) {
        al::setNerve(this, &NrvNekoNormalFollowJumpEnd);
    }
}

/**
 * @brief Land after a jump while following the player.
 */
void NekoNormal::exeFollowJumpEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ThrowEnd");
        al::setVelocityZeroH(this);
    }

    NpcStateFunction::calcPassiveMovement(this, &sStateParam, mIsOnGround);
    mTargetFinder->update();

    if (!al::isActionEnd(this)) {
        return;
    }

    if (!isFollowPlayerNear()) {
        mFollowPlayer = nullptr;
    }

    if (!tryStartReactToTarget()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    }

    if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
        al::validateClipping(this);
    }
}

/**
 * @brief Get stroked by the touch screen pointer.
 */
void NekoNormal::exeStroke() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Stroke");
        al::onCollide(this);
        al::showModelIfHide(this);
    }

    al::updateNerveState(this);
    updatePassiveMovement();

    if (!mIsPushed) {
        al::setVelocityZeroH(this);
    }

    mTargetFinder->update();

    if (mStateSupportStroke->isTouch() || !al::isGreaterStep(this, 60)) {
        return;
    }

    if (tryStartReactToTarget()) {
        return;
    }

    tryStartDefaultBehavior(al::param::NekoBehavior_Default);
}

/**
 * @brief Purr at the player.
 */
void NekoNormal::exePurr() {
    if (al::isFirstStep(this)) {
        al::setVelocityZeroH(this);
        al::startAction(this, "Purr");
        al::onCollide(this);
    }

    updatePassiveMovement();
    mTargetFinder->update();

    if (!isFollowPlayerNear()) {
        mFollowPlayer = nullptr;

        if (tryStartReactToTarget()) {
            return;
        }

        tryStartDefaultBehavior(al::param::NekoBehavior_Sit);
        return;
    }

    if (!al::isGreaterStep(this, 30)) {
        return;
    }

    if (al::calcDistanceH(this, mFollowPlayer) > 100.0f ||
        al::calcDistanceV(this, mFollowPlayer) > 200.0f) {
        if (tryStartReactToTarget()) {
            return;
        }

        tryStartDefaultBehavior(al::param::NekoBehavior_Sit);
    }
}

/**
 * @brief Start a binding to the actor holding the cat.
 */
void NekoNormal::exeBindStart() {
    if (al::isFirstStep(this) && !mBindPuppeteer->startEnter(mHolderSensor)) {
        al::setNerve(this, &NrvNekoNormalRelease);
    }
}

/**
 * @brief End a binding, then start the default behavior.
 */
void NekoNormal::exeBindEnd() {
    if (!al::isFirstStep(this)) {
        return;
    }

    if (al::isNerve(this, &NrvNekoNormalBindEnd)) {
        mBindPuppeteer->startExit();
    } else if (al::isNerve(this, &NrvNekoNormalBindEndStay)) {
        mBindPuppeteer->startExitStay();
    } else {
        mBindPuppeteer->startDizzy();
        mBindPuppeteer =
            mBindPuppeteer == mBindPuppeteers[1] ? mBindPuppeteers[0] : mBindPuppeteers[1];

        if (mBindPuppeteer->isBinding()) {
            mBindPuppeteer->endBindForce();
        }
    }

    if (al::isNerve(this, &NrvNekoNormalBindEnd) && al::isCollidedGround(this)) {
        al::setVelocity(this, sead::Vector3f(0.0f, 0.0f, 0.0f));
    } else {
        al::setVelocityZero(this);
    }

    tryStartDefaultBehavior(al::param::NekoBehavior_Default);
}

/**
 * @brief Get ridden by an NPC puppeteer.
 */
void NekoNormal::exeBindRide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitRideStart");
        mPuppetInputStick.x = 0.0f;
        mPuppetInputStick.y = 0.0f;
    }

    if (mIsValidPuppetMtx) {
        sead::Vector3f front;
        mPuppetMtx.getBase(front, 2);
        sead::Vector3f trans;
        mPuppetMtx.getTranslation(trans);
        al::normalizeOrDirZ(&front);
        setFrontVec(front);
        setTransVec(trans);
    }

    if (al::isActionPlaying(this, "WaitRideStart")) {
        if (al::isActionEnd(this)) {
            al::startAction(this, "WaitRide");
        }

        return;
    }

    if (mPuppetInputStick.x > 0.1f) {
        al::tryStartActionIfNotPlaying(this, "WaitRideRight");
    } else if (mPuppetInputStick.x < -0.1f) {
        al::tryStartActionIfNotPlaying(this, "WaitRideLeft");
    } else {
        al::tryStartActionIfNotPlaying(this, "WaitRide");
    }
}

/**
 * @brief Jump off after an NPC puppeteer stopped riding the cat.
 */
void NekoNormal::exeBindRideEnd() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
        al::onCollide(this);
        al::LiveActor* player = rc::findNearestActivePlayerActor(this);
        al::LiveActor* rider = al::getSensorHost(mRideSensor);
        al::startAction(this, "WaitRideEnd");
        sead::Vector2f jump = sRideEndJumpNear;
        f32 distanceH = al::calcDistanceH(rider, player);
        f32 distanceV = al::calcDistanceV(rider, player);

        if (distanceH > 800.0f) {
            jump = sRideEndJumpFar;
        } else if (distanceV > 300.0f) {
            jump = sRideEndJumpHigh;
        }

        sead::Vector3f dir;
        al::calcDirH(&dir, al::getTrans(this), al::getTrans(player));
        dir.normalize();
        f32 speed = distanceH * jump.x;
        f32 maxSpeed = mBindEndType == 0 ? 100.0f : 20.0f;
        speed = sead::Mathf::clamp(speed, 10.0f, maxSpeed);
        dir.x = speed * dir.x;
        dir.z = speed * dir.z;
        dir.y = jump.y;
        al::setVelocity(this, dir);
        al::faceToDirection(this, dir);
        mRideSensor = nullptr;
    }

    if (al::isNoCollide(this)) {
        al::setVelocityZero(this);
    } else {
        NpcStateFunction::calcPassiveMovement(this, &sJumpStateParam, mIsOnGround);
    }

    if (!mIsOnGround) {
        return;
    }

    al::tryStartActionIfNotPlaying(this, "ThrowEnd");

    if (!mIsPushed) {
        al::setVelocityZeroH(this);
    }

    mTargetFinder->update();

    if (!al::isActionEnd(this)) {
        return;
    }

    if (!tryStartReactToTarget()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    }

    if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
        al::validateClipping(this);
    }
}

/**
 * @brief Get carried by the player.
 */
void NekoNormal::exeHold() {
    if (al::isFirstStep(this)) {
        al::setColliderRadius(mCollisionController, 30.0f);
        al::setColliderOffsetY(mCollisionController, 40.0f);
        al::startAction(this, "Carry");
        al::startSe(this, "PgHoldStart", nullptr);
        startHold();
        al::setVelocityZero(this);
        al::invalidateClipping(this);
        alPadRumbleFunction::startPadRumbleLoopNo3D(this, "NekoCarry", al::getTransPtr(this), -1,
                                                    false);
        mActionTime = 80;

        if (mHost != nullptr && getNekoType() == 4) {
            mHost->setRequestHide(true);
        }
    }

    if (al::isOnGround(this, 0, 0.0f) && !rc::isPlayerOnGround(mHolderSensor)) {
        al::sendMsgPush(mHolderSensor, al::getHitSensor(this, "Body"));
    }

    if (mActionTime-- <= 0) {
        if (alPadRumbleFunction::checkIsAlivePadRumbleLoop(this, "NekoCarry",
                                                           al::getTransPtr(this), -1)) {
            alPadRumbleFunction::stopPadRumbleLoop(this, "NekoCarry", al::getTransPtr(this), -1);
            mActionTime = 200;
        } else {
            alPadRumbleFunction::startPadRumbleLoopNo3D(this, "NekoCarry",
                                                        al::getTransPtr(this), -1, false);
            mActionTime = 80;
        }
    }

    al::updateNerveState(this);
    auto* holder = static_cast<PlayerActor*>(al::getSensorHost(mHolderSensor));

    if (holder == nullptr) {
        return;
    }

    if (!rc::isPlayerBinded(mHolderSensor) && holder->getInput()->isSquatTrigOn()) {
        rc::requestPlayerRelease(mHolderSensor);
        al::resetActorCollisionController(mCollisionController, 1);

        if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
            al::validateClipping(this);
        }

        releaseDrop();
        return;
    }

    // The result is not used.
    static_cast<void>(rc::isPlayerWait(al::getSensorHost(mHolderSensor)) ||
                      rc::isPlayerDash(al::getSensorHost(mHolderSensor)) ||
                      rc::isPlayerDashFast(al::getSensorHost(mHolderSensor)));
}

/**
 * @brief Prepare the cat to be carried.
 */
void NekoNormal::startHold() {
    al::offCollide(this);
}

/**
 * @brief Fly through the air after the player threw or dropped the cat.
 */
void NekoNormal::exeRelease() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Throw");

        if (mHolderSensor != nullptr) {
            rc::requestPlayerRelease(mHolderSensor);
        }

        mHolderSensor = nullptr;
        al::invalidateClipping(this);
        al::onCollide(this);
    }

    if (al::isActionPlaying(this, "Throw") && al::isActionEnd(this)) {
        al::startAction(this, "Fall");
    }

    if (al::isNoCollide(this)) {
        al::setVelocityZero(this);
    } else {
        NpcStateFunction::calcPassiveMovement(this, &sJumpStateParam, mIsOnGround);
    }

    if (al::isGreaterStep(this, 7) && mIsOnGround) {
        al::tryStartActionIfNotPlaying(this, "ThrowEnd");

        if (!mIsPushed) {
            al::setVelocityToGravity(this, 0.1f);
        }

        if (al::isActionEnd(this)) {
            if (!rc::isInAreaObj(this, rc::AreaObjType::InvalidateClippingArea)) {
                al::validateClipping(this);
            }

            if (mFollowPlayer != nullptr && !isFollowPlayerNear()) {
                mFollowPlayer = nullptr;
            }

            if (!tryStartReactToTarget()) {
                tryStartDefaultBehavior(al::param::NekoBehavior_Wait);
            }

            return;
        }

        al::HitSensor* groundSensor = al::tryGetCollidedGroundSensor(this);

        if (groundSensor != nullptr) {
            al::sendMsgEnemyFloorTouch(groundSensor, al::getHitSensor(this, "Body"));

            if (rc::sendMsgBoundTrampoline(groundSensor, al::getHitSensor(this, "Body")) &&
                tryBounce(30.0f)) {
                return;
            }
        }
    }

    mCoolTime = 30;
}

/**
 * @brief Attack an enemy.
 */
void NekoNormal::exeAttack() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::startAction(this, "Pounce");
    }

    updatePassiveMovement();
    tryLimitMove();
    mTargetFinder->update();

    if (al::isActionEnd(this) && !tryStartReactToTarget()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    }
}

/**
 * @brief Watch a toy the cat wants to play with.
 */
void NekoNormal::exeTargetWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitPlay");
    }

    mTargetFinder->update();
    updatePassiveMovement();

    if ((mTargetFinder->getTargetType() &
         (npc::NpcFindTargetType_Ball | npc::NpcFindTargetType_Koura |
          npc::NpcFindTargetType_Bird)) &&
        mIsOnGround) {
        al::turnDirectionToTargetDegree(this, al::getFrontPtr(this),
                                        mTargetFinder->getTargetPos(),
                                        mRunAwayParam->getTurnDegree());
    }

    if (al::isGreaterStep(this, 120) || mTargetFinder->getTarget() == nullptr) {
        if (!tryStartReactToTarget()) {
            tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        }
    }
}

/**
 * @brief Pounce on a toy.
 */
void NekoNormal::exePounce() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Pounce");
        al::onCollide(this);
    }

    updatePassiveMovement();
    tryLimitMove();
    mTargetFinder->update();

    if (al::isActionEnd(this) && !tryStartReactToTarget()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Wait);
    }
}

/**
 * @brief Appear at the position of the host cat.
 */
void NekoNormal::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        al::onCollide(this);
        al::validateClipping(this);
        al::showModelIfHide(this);
    }

    updatePassiveMovement();

    if (al::isActionEnd(this) && !tryStartReactToTarget()) {
        tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    }
}

/**
 * @brief Walk to the drop target the cat was brought to.
 */
void NekoNormal::exeSeekTarget() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        stopCollectSe();
    }

    const neko::Target* target = mGoalTarget;

    if (target->mRange > 0.0f && !neko::isInRange(this, target->mTrans, target->mRange)) {
        if (mIsAtGoal && mHost->tryStartHide()) {
            return;
        }

        mGoalTarget = nullptr;

        if (!tryStartReactToTarget()) {
            tryStartDefaultBehavior(al::param::NekoBehavior_Default);
        }

        return;
    }

    if (mIsOnGround) {
        al::tryStartActionIfNotPlaying(this, "Walk");
        mLookAtPos = *mGoalTarget->tryGetHostTrans();
        mHeadController->setLookAtTarget(&mLookAtPos);
        mHeadController->update();
        f32 distanceH = al::calcDistanceH(this, mGoalTarget->mTrans);
        sead::Vector3f* front = al::getFrontPtr(this);

        if (distanceH < 25.0f) {
            if (al::turnDirectionToTargetDegree(this, front, mLookAtPos, 8.0f)) {
                if (tryStartDefaultBehavior(al::param::NekoBehavior_Default)) {
                    mHost->setActivePosition(mGoalTarget->mTrans);
                }

                return;
            }
        } else {
            al::turnDirectionToTargetDegree(this, front, mGoalTarget->mTrans, 8.0f);
            al::setVelocityToDirection(this, *front, 2.5f);

            if (mGoalTarget->mHost != nullptr && al::isNear(this, mGoalTarget->mHost, 120.0f)) {
                const sead::Vector3f& trans = al::getTrans(this);
                const sead::Vector3f& parentTrans = al::getTrans(mGoalTarget->mHost);
                sead::Vector3f dir = {trans.x - parentTrans.x, 0.0f, trans.z - parentTrans.z};
                al::normalizeOrDirZ(&dir);
                f32 push = 2.0f - al::getVelocity(this).dot(dir);

                if (push > 0.0f) {
                    sead::Vector3f* velocity = al::getVelocityPtr(this);
                    *velocity =
                        sead::Vector3f(push * dir.x, push * dir.y, push * dir.z) + *velocity;
                }
            }
        }
    }

    updatePassiveMovement();
    tryLimitMove();
    neko::setAnimationRate(this);
}

/**
 * @brief Wait at the drop target, the cat is back home.
 */
void NekoNormal::exeGoalWait() {
    if (al::isFirstStep(this)) {
        mHost->setActivePosition(mGoalTarget->mTrans);
        al::validateClipping(this);
        mIsAtGoal = true;
        stopCollectSe();

        if (rc::isAnyActiveDemo(this)) {
            al::startAction(this, "WaitGoalStart");
        } else if (isActionStanding(this)) {
            al::startAction(this, "WaitSitStart");
        } else {
            al::startAction(this, "WaitGoal");
        }

        al::offCollide(this);

        if (mGoalTarget->mHost != nullptr) {
            sead::Vector3f dir;
            al::calcDirH(&dir, al::getTrans(this), *mGoalTarget->tryGetHostTrans());
            al::faceToDirection(this, dir);
            mHost->setActiveFace(dir);
        }
    }

    if (al::isActionOneTime(this, al::getActionName(this)) && al::isActionEnd(this)) {
        al::startAction(this, "WaitGoal");
    }

    mHeadController->update();
    al::setVelocityZero(this);
}

/**
 * @brief Jump out of the player's hands towards a drop target of a cat parent.
 * @param pTarget The drop target.
 * @param isForce Whether the host cat must stop asking to be hidden.
 * @return Whether the cat started seeking the target.
 */
bool NekoNormal::startSeekTarget(const neko::Target* pTarget, bool isForce) {
    if (al::isNerve(this, &NrvNekoNormalSeekTarget)) {
        return false;
    }

    if (isHold()) {
        endHold();
        rc::requestPlayerRelease(mHolderSensor);
        al::resetActorCollisionController(mCollisionController, 1);
        al::startAction(this, "CollectJump");
        sead::Vector3f front = al::getFront(this);
        front.x *= 6.0f;
        front.z *= 6.0f;
        front.y = 10.0f;
        al::setVelocity(this, front);
    }

    al::invalidateClipping(this);
    mIsAtGoal = isForce;
    mGoalTarget = const_cast<neko::Target*>(pTarget);

    if (isForce && mHost != nullptr && getNekoType() == 4) {
        mHost->setRequestHide(false);
    }

    al::setNerve(this, &NrvNekoNormalSeekTarget);
    return true;
}

/**
 * @brief Check whether the cat waits at its goal.
 * @return Whether the cat waits at its goal.
 */
bool NekoNormal::isAtGoal() const {
    return al::isNerve(this, &NrvNekoNormalGoalWait);
}

/**
 * @brief Check whether the cat reacts to its surroundings.
 * @return Whether the cat is free.
 */
bool NekoNormal::isInteractive() const {
    if (al::isNerve(this, &NrvNekoNormalSeekTarget) && mIsAtGoal) {
        return false;
    }

    if (al::isNerve(this, &NrvNekoNormalGoalWait)) {
        return false;
    }

    if (isHold()) {
        return false;
    }

    if (isRide()) {
        return false;
    }

    return !al::isNerve(this, &NrvNekoNormalAppear);
}

/**
 * @brief Check whether the player carries the cat.
 * @return Whether the cat is carried.
 */
bool NekoNormal::isHold() const {
    if (!al::isNerve(this, &NrvNekoNormalHold) && !al::isNerve(this, &NrvNekoNormalBindStart)) {
        return false;
    }

    if (mHolderSensor == nullptr) {
        return false;
    }

    return rc::isPlayerHolding(mHolderSensor, this);
}

/**
 * @brief Check whether an NPC puppeteer rides the cat.
 * @return Whether the cat is ridden.
 */
bool NekoNormal::isRide() const {
    return al::isNerve(this, &NrvNekoNormalBindRide);
}

/**
 * @brief Check whether a cat parent can take the cat back.
 * @return Whether the cat is a lost kitten brought close to its parent.
 */
bool NekoNormal::canCollect() const {
    if (getNekoType() != 4) {
        return false;
    }

    if (isHold()) {
        return true;
    }

    if (al::isNerve(this, &NrvNekoNormalRelease) &&
        (al::isGreaterStep(this, 15) || mIsOnGround)) {
        return true;
    }

    if ((al::isNerve(this, &NrvNekoNormalFollow) || al::isNerve(this, &NrvNekoNormalFollowWait)) &&
        mFollowPlayer != nullptr && al::calcDistanceH(this, mFollowPlayer) < 400.0f &&
        al::isGreaterStep(this, 30)) {
        return true;
    }

    if (al::isNerve(this, &NrvNekoNormalPurr) && al::isGreaterStep(this, 30)) {
        return true;
    }

    return false;
}

/**
 * @brief Put the cat down and make it fly away from the player.
 * @param dir Horizontal direction of the flight.
 * @param speedH Horizontal speed of the flight.
 * @param speedV Vertical speed of the flight.
 */
void NekoNormal::startRelease(sead::Vector3f dir, f32 speedH, f32 speedV) {
    endHold();
    sead::Vector3f velocity = dir;

    if (al::isNearZero(dir, 0.001f)) {
        velocity.x = 0.0f;
        velocity.z = 30.0f;
        speedV = 30.0f;
    } else {
        velocity.x *= speedH;
        velocity.z *= speedH;
    }

    velocity.y = speedV;
    al::setVelocity(this, velocity);
    al::setNerve(this, &NrvNekoNormalRelease);
}

/**
 * @brief Drop the cat in front of the player holding it.
 */
void NekoNormal::releaseDropFront() {
    startRelease(rc::getPlayerFront(mHolderSensor), sDropFrontSpeed.x,
                 sDropFrontSpeed.y);
}

/**
 * @brief Warn that Fury Bowser is about to show up.
 * @param isEmitEffect Whether the anticipation effect is emitted.
 */
void NekoNormal::startDisasterAnticipation(bool isEmitEffect) {
    if (!isEmitEffect || al::isClipped(this)) {
        return;
    }

    tryEmitDisasterAnticipationEffect(this);
}

/**
 * @brief Stop the carry rumble when the disaster demo starts.
 * @return Always true.
 */
bool NekoNormal::startDisasterDemo() {
    if (isHold()) {
        alPadRumbleFunction::stopPadRumbleLoop(this, "NekoCarry", al::getTransPtr(this), -1);
    }

    return true;
}

/**
 * @brief Set the parent the cat belongs to.
 * @param pParent The cat parent.
 */
void NekoNormal::setNekoParent(const NekoParent* pParent) {
    mChaseRange = pParent->getChaseRange();
}

/**
 * @brief Get ridden by an NPC puppeteer.
 * @param pSelf Sensor of the cat.
 * @param pOther Sensor of the rider.
 */
void NekoNormal::startBindNpc(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mRideSensor != nullptr) {
        rc::sendMsgNpcBindCancel(mRideSensor, al::getHitSensor(this, "Body"));
    }

    mRideSensor = pSelf;
    al::setNerve(this, &NrvNekoNormalBindRide);
    al::offCollide(this);
    al::invalidateShadow(this);
}

/**
 * @brief Stop getting ridden by an NPC puppeteer.
 * @param type How the binding ended.
 */
void NekoNormal::endBindNpc(NpcPuppetBindEndType type) {
    al::onCollide(this);
    al::validateShadow(this);
    mBindEndType = type;
    mIsValidPuppetMtx = false;

    switch (type) {
    case 0:
    case 1:
        al::setNerve(this, &NrvNekoNormalBindRideEnd);
        return;
    default:
        break;
    }

    tryStartDefaultBehavior(al::param::NekoBehavior_Default);
    mHost->tryStartHide();
    mRideSensor = nullptr;
}

/**
 * @brief Move the cat with its NPC puppeteer.
 * @param rTrans New position.
 */
void NekoNormal::setTransVec(const sead::Vector3f& rTrans) {
    al::setTrans(this, rTrans);
}

/**
 * @return Position of the cat.
 */
const sead::Vector3f& NekoNormal::getTransVec() const {
    return al::getTrans(this);
}

/**
 * @brief Tilt the cat with its NPC puppeteer.
 * @param rUp New up direction.
 */
void NekoNormal::setUpVec(const sead::Vector3f& rUp) {
    sead::Matrix34f mtx;
    al::makeMtxFrontUpPos(&mtx, getFrontVec(), rUp, getTransVec());
    al::setBaseMtxAndCalcAnim(this, mtx, al::getScale(this));
}

/**
 * @return Up direction of the cat.
 * @note Returns a reference to a temporary, like the original code.
 */
const sead::Vector3f& NekoNormal::getUpVec() const {
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    return up;
}

/**
 * @brief Turn the cat with its NPC puppeteer.
 * @param rFront New front direction.
 */
void NekoNormal::setFrontVec(const sead::Vector3f& rFront) {
    al::setFront(this, rFront);
}

/**
 * @return Front direction of the cat.
 */
const sead::Vector3f& NekoNormal::getFrontVec() const {
    return al::getFront(this);
}

/**
 * @brief Pose the cat with the matrix of its NPC puppeteer.
 * @param pMtx The pose matrix.
 */
void NekoNormal::setMtx(const sead::Matrix34f* pMtx) {
    mPuppetMtx = *pMtx;
    mIsValidPuppetMtx = true;
}

/**
 * @brief Does nothing.
 */
void NekoNormal::onStartHide() {}

/**
 * @brief Move the cat with the passive movement of its idle states.
 */
inline void NekoNormal::updatePassiveMovement() {
    if (al::isNoCollide(this)) {
        al::setVelocityZero(this);
    } else {
        NpcStateFunction::calcPassiveMovement(this, &sStateParam, mIsOnGround);
    }
}

/**
 * @brief Stop the cat when it is about to fall off a cliff or enter an avoid area.
 * @return Whether the cat got stopped.
 */
inline bool NekoNormal::tryLimitMove() {
    if (NpcStateFunction::isFallNextMove(this, 150.0f, 150.0f, 110.0f, false) ||
        NpcStateFunction::isNPCAvoidAreaNextMove(this, 150.0f, 20.0f, 100.0f, false)) {
        al::setVelocityToGravity(this, 0.1f);
        return true;
    }

    return false;
}

/**
 * @brief Check whether the next move of the cat leads off a cliff or into the water.
 * @return Whether the next move must be avoided.
 */
inline bool NekoNormal::isLimitMoveNext() {
    return (mChaseParam->isEnableCliffCheck() &&
            NpcStateFunction::isFallNextMove(this, 150.0f, 150.0f, 110.0f, false)) ||
           (mParam->mIsEnableShoreCheck &&
            NpcStateFunction::isNPCAvoidAreaNextMove(this, 150.0f, 20.0f, 100.0f, false));
}

/**
 * @brief Check whether the followed player can still be followed.
 * @return Whether the followed player is alive, a cat and close enough.
 */
inline bool NekoNormal::isFollowPlayerNear() const {
    al::LiveActor* player = mFollowPlayer;

    if (rc::isPlayerDeadOrBubble(player) || !rc::isPlayerClimbOrClimbSpecial(player)) {
        return false;
    }

    player = mFollowPlayer;

    if (al::calcDistanceH(this, player) > 1000.0f) {
        return false;
    }

    return !(al::calcDistanceV(this, player) > 800.0f);
}

/**
 * @brief Stop and kill the meowing of a lost kitten.
 */
inline void NekoNormal::stopCollectSe() {
    if (mCollectSe != nullptr) {
        al::tryStopSeByName(mCollectSe, "WaitMeow");
        mCollectSe->kill();
        mCollectSe = nullptr;
    }
}

/**
 * @brief Look where the cat runs to.
 * @param isFast Whether the cat runs away fast.
 */
inline void NekoNormal::lookAtRunAwayDir(bool isFast) {
    if (!al::isGreaterStep(this, 5)) {
        mHeadController->requestResetLook();
        return;
    }

    const NpcStateRunAway* state = isFast ? mStateRunAwayFast : mStateRunAway;

    if (state->isRunStraight()) {
        const sead::Vector3f& trans = al::getTrans(this);
        mLookAtPos = al::getFront(this) * 300.0f + trans;
    } else {
        sead::Vector3f threatPos = state->getThreatPos();
        al::calcDirH(&mLookAtPos, threatPos, al::getTrans(this));
        const sead::Vector3f& trans = al::getTrans(this);
        mLookAtPos = mLookAtPos * 300.0f + trans;
    }

    mHeadController->setLookAtTarget(&mLookAtPos);
}

/**
 * @brief Look away from a position.
 * @param rPos The position to look away from.
 */
inline void NekoNormal::lookAwayFrom(const sead::Vector3f& rPos) {
    al::calcDirH(&mLookAtPos, rPos, al::getTrans(this));
    const sead::Vector3f& trans = al::getTrans(this);
    mLookAtPos = mLookAtPos * 300.0f + trans;
    mHeadController->setLookAtTarget(&mLookAtPos);
}
