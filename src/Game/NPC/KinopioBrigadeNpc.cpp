#include "NPC/KinopioBrigadeNpc.hpp"

#include "Camera/NpcGoalCameraTarget.hpp"
#include "Enemy/ActorMicRumbler.hpp"
#include "Layout/GuideBalloonBrigade.hpp"
#include "Layout/IslandMap.hpp"
#include "MapObj/ActorStateDemoCamera.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/KinopioBrigadeWatcher.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/AnimScaleController.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Clipping/ClippingAreaActorInfo.hpp"

namespace {
NERVE_DECL(KinopioBrigadeNpc, Wait)
NERVE_DECL(KinopioBrigadeNpc, TakeOut)
NERVE_DECL(KinopioBrigadeNpc, Move)
NERVE_DECL(KinopioBrigadeNpc, RouteDokan)
NERVE_DECL(KinopioBrigadeNpc, Appear)
NERVE_DECL(KinopioBrigadeNpc, StandBy)
NERVE_DECL(KinopioBrigadeNpc, MicReaction)
NERVE_DECL(KinopioBrigadeNpc, SpinReaction)
NERVE_DECL(KinopioBrigadeNpc, Trampled)
NERVE_DECL(KinopioBrigadeNpc, Reaction)
NERVE_DECL(KinopioBrigadeNpc, Touch)
NERVE_DECL(KinopioBrigadeNpc, Relief)
NERVE_DECL(KinopioBrigadeNpc, ReliefStart)
NERVE_DECL(KinopioBrigadeNpc, DisasterAfraid)
NERVE_DECL(KinopioBrigadeNpc, TurnToCamera)
NERVE_DECL(KinopioBrigadeNpc, WaitTurn)

class KinopioBrigadeNpcNrvWave : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KinopioBrigadeNpc>()->exeWait();
    }
};

class KinopioBrigadeNpcNrvWaveTurn : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KinopioBrigadeNpc>()->exeWaitTurn();
    }
};

NERVES_MAKE_NOSTRUCT(KinopioBrigadeNpc, Wait, TakeOut, Move, RouteDokan, Appear, StandBy,
                     MicReaction, SpinReaction, Trampled, Reaction, Touch, Relief, ReliefStart,
                     DisasterAfraid, TurnToCamera, Wave, WaitTurn, WaveTurn)

/**
 * @brief Creates the wobble parameters used while the microphone picks up input.
 * @return The wobble parameters.
 */
al::AnimScaleParam createMicRumbleParam() {
    al::AnimScaleParam param;
    param._24 = 1.0f;
    param._28 = 10.0f;
    param._2c = 0.05f;
    return param;
}

const al::AnimScaleParam sMicRumbleParam = createMicRumbleParam();
const sead::Vector3f sGuideBalloonOffset(0.0f, 120.0f, 0.0f);

/**
 * @brief Checks whether the member is in a calm state in which it can react to attacks.
 * @param pNpc The brigade member.
 * @return Whether a reaction can be started.
 */
bool isEnableReaction(const KinopioBrigadeNpc* pNpc) {
    return al::isNerve(pNpc, &NrvKinopioBrigadeNpcWait) ||
           al::isNerve(pNpc, &NrvKinopioBrigadeNpcWaitTurn) ||
           al::isNerve(pNpc, &NrvKinopioBrigadeNpcWave) ||
           al::isNerve(pNpc, &NrvKinopioBrigadeNpcWaveTurn) ||
           al::isNerve(pNpc, &NrvKinopioBrigadeNpcTrampled) ||
           al::isNerve(pNpc, &NrvKinopioBrigadeNpcReaction) ||
           al::isNerve(pNpc, &NrvKinopioBrigadeNpcMicReaction) ||
           al::isNerve(pNpc, &NrvKinopioBrigadeNpcTouch);
}

/**
 * @brief Checks whether the member can react to the microphone or to touches.
 * @param pNpc The brigade member.
 * @return Whether a microphone or touch reaction can be started.
 */
bool isEnableMicReaction(const KinopioBrigadeNpc* pNpc) {
    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcWait)) {
        return true;
    }

    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcWaitTurn)) {
        return true;
    }

    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcWave)) {
        return true;
    }

    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcWaveTurn)) {
        return true;
    }

    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcTrampled)) {
        return true;
    }

    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcMicReaction)) {
        return true;
    }

    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcTouch)) {
        return true;
    }

    if (al::isNerve(pNpc, &NrvKinopioBrigadeNpcReaction) && al::isGreaterStep(pNpc, 25)) {
        return true;
    }

    return false;
}

/**
 * @brief Plays the sound of a player crossing the route dokan.
 * @param pActor Actor playing the sound.
 */
void startSeCross(al::LiveActor* pActor) {
    al::startSe(pActor, "PgCross");
}
}  // namespace

/**
 * @brief Checks whether the scenario of the member's shine has been completed.
 * @return Whether the scenario is complete.
 */
inline bool KinopioBrigadeNpc::isGoalItemScenarioComplete() const {
    return SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this),
                                                      mGoalItem->getIslandId() - 1,
                                                      mGoalItem->getShineId() - 1);
}

/**
 * @brief Checks whether the member is busy handing out its reward.
 * @return Whether the reward is being handed out.
 */
inline bool KinopioBrigadeNpc::isGivingReward() const {
    if (al::isNerve(this, &NrvKinopioBrigadeNpcRelief)) {
        return true;
    }

    if (al::isNerve(this, &NrvKinopioBrigadeNpcReliefStart)) {
        return true;
    }

    if (al::isNerve(this, &NrvKinopioBrigadeNpcTakeOut)) {
        return true;
    }

    return al::isNerve(this, &NrvKinopioBrigadeNpcTurnToCamera);
}

/**
 * @brief Checks whether the start switch is on outside of single mode.
 * @return Whether the start switch is on in multiplayer.
 */
inline bool KinopioBrigadeNpc::isOnSwitchStartMultiMode() {
    return !mIsSingleMode && al::isOnSwitchStart(this);
}

/**
 * @brief Starts a reaction, or plays the small "no" reaction (scale wobble and sound) if busy.
 * @param pNerve Reaction nerve to start.
 */
inline void KinopioBrigadeNpc::startReactionOrNegative(const al::Nerve* pNerve) {
    al::AnimScaleController* pAnimScale = mAnimScaleController;
    if (isEnableReaction(this)) {
        al::setNerve(this, pNerve);
        return;
    }

    pAnimScale->startHitReaction();
    al::startSe(this, "PgReactionNegative");
}

/**
 * @brief Starts the reaction to an attack, or the small "no" reaction if not already reacting.
 * @return Whether a reaction was started.
 */
inline bool KinopioBrigadeNpc::tryStartReaction() {
    al::AnimScaleController* pAnimScale = mAnimScaleController;
    if (isEnableReaction(this)) {
        al::setNerve(this, &NrvKinopioBrigadeNpcReaction);
        return true;
    }

    if (al::isNerve(this, &NrvKinopioBrigadeNpcReaction) || pAnimScale->isHitReaction(25)) {
        return false;
    }

    pAnimScale->startHitReaction();
    al::startSe(this, "PgReactionNegative");
    return true;
}

/**
 * @brief Starts the spin reaction and remembers the nerve to return to.
 * @return Whether the spin reaction was started.
 */
inline bool KinopioBrigadeNpc::tryStartSpinReaction() {
    if (!canPlaySingleModeReaction() || al::isNerve(this, &NrvKinopioBrigadeNpcSpinReaction)) {
        return false;
    }

    mNerveAfterSpinReaction = al::getNerve(this);
    al::setNerve(this, &NrvKinopioBrigadeNpcSpinReaction);
    return true;
}

/**
 * @brief Waves when a player is near, otherwise just waits.
 */
inline void KinopioBrigadeNpc::setNerveWaitOrWave() {
    al::LiveActor* pPlayer = al::tryFindNearestPlayerActor(this);
    if (pPlayer != nullptr && al::calcDistanceH(this, pPlayer) < 1000.0f) {
        al::setNerve(this, &NrvKinopioBrigadeNpcWave);
        return;
    }

    al::setNerve(this, &NrvKinopioBrigadeNpcWait);
}

/**
 * @brief Creates the balloon showing which brigade members were found.
 * @param rInfo Actor init info.
 */
inline void KinopioBrigadeNpc::initGuideBalloon(const al::ActorInitInfo& rInfo) {
    mGuideBalloon = new GuideBalloonBrigade("guide", al::getLayoutInitInfo(rInfo),
                                            al::getTransPtr(this), sGuideBalloonOffset,
                                            getSceneInfo()->demoDirector);
}

/**
 * @brief Constructs a brigade member.
 * @param pName Actor name.
 */
KinopioBrigadeNpc::KinopioBrigadeNpc(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the member, or only the bare actor for a discovered location marker.
 * @param rInfo Actor init info.
 * @param isDiscoveredLocation Whether this actor only marks the location of a found member.
 */
void KinopioBrigadeNpc::init(const al::ActorInitInfo& rInfo, bool isDiscoveredLocation) {
    al::initActor(this, rInfo);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    if (isDiscoveredLocation) {
        return;
    }

    al::tryGetArg(&mMoveSpeed, rInfo, "MoveSpeed");
    al::tryGetArg(&mState, rInfo, "State");
    mSpeed = mMoveSpeed;
    mMicRumbler = new ActorMicRumbler(this, &sMicRumbleParam);
    mStateSupportStroke = new ActorStateSupportStroke(this);
    mAnimScaleController = new al::AnimScaleController(nullptr);
    mIsInWaveArea = al::isExistAreaObj(this, "KinopioBrigadeWaveArea");
    bool isUseDemo = false;
    al::tryGetArg(&isUseDemo, rInfo, "IsUseDemo");
    al::initNerve(this, &NrvKinopioBrigadeNpcWait, 1);
    if (mIsSingleMode) {
        if (al::calcLinkChildNum(rInfo, "GoalItem") != 0) {
            mGoalItem = new GoalItem("GoalItem");
            al::initLinksActor(mGoalItem, rInfo, "GoalItem", 0);
            al::copyPose(mGoalItem, this);
            GameDataHolderAccessor accessor(this);
            mIsScenarioComplete = SingleModeDataFunction::isScenarioComplete(
                accessor, mGoalItem->getIslandId() - 1, mGoalItem->getShineId() - 1);
            mGoalItem->makeActorDead();
            if (al::isValidStageSwitch(this, "SwitchAllowGoalItemCollect") &&
                !mIsScenarioComplete) {
                mIsWaitGoalItemCollectSwitch = true;
            }

            mCameraTicket = al::initProgramableCamera_RS(this, rInfo, "KinopioBrigadeNpc",
                                                         &mCameraPos, &mCameraAt, nullptr);
            if (al::calcLinkChildNum(rInfo, "KinopioBrigadeWatcher") != 0) {
                ProjectActorFactory factory;
                mWatcher = static_cast<KinopioBrigadeWatcher*>(
                    al::createLinksActorFromFactory(factory, rInfo, "KinopioBrigadeWatcher", 0));
                mWatcher->setScenario(mGoalItem->getShineId(), mGoalItem->getIslandId());
                initGuideBalloon(rInfo);
            }
        } else if (al::calcLinkChildNum(rInfo, "KinopioBrigadeWatcher") != 0) {
            ProjectActorFactory factory;
            mWatcher = static_cast<KinopioBrigadeWatcher*>(
                al::createLinksActorFromFactory(factory, rInfo, "KinopioBrigadeWatcher", 0));
            initGuideBalloon(rInfo);
        }

        if (mIsSingleMode && al::calcLinkChildNum(rInfo, "DiscoveredLocation") != 0) {
            al::PlacementInfo placementInfo;
            al::ActorInitInfo locationInfo;
            al::getLinksInfoByIndex(&placementInfo, rInfo, "DiscoveredLocation", 0);
            mDiscoveredLocation = new KinopioBrigadeNpc("DiscoveredBrigadeLocation");
            locationInfo.initViewIdHost(&placementInfo, rInfo);
            mDiscoveredLocation->init(locationInfo, true);
            al::getTrans(&mDiscoveredTrans, placementInfo);
            mDiscoveredClippingInfo = mDiscoveredLocation->getClippingInfoNode()->mInfo;
            mDiscoveredLocation->kill();
        } else {
            mDiscoveredTrans = al::getTrans(this);
        }

        if (mIsScenarioComplete) {
            moveToDiscoveredLocation(true);
        }
    }

    bool isSyncAppear = al::trySyncStageSwitchAppear(this);
    if (al::calcLinkChildNum(rInfo, "GreenStar") != 0) {
        mGreenStar = new GreenStar("グリーンスター");
        al::initLinksActor(mGreenStar, rInfo, "GreenStar", 0);
        al::copyPose(mGreenStar, this);
        if (isSyncAppear | al::isValidSwitchStart(this)) {
            mGreenStar->makeActorDead();
        } else {
            mGreenStar->makeActorAppeared();
        }

        if (isUseDemo) {
            mDemoCameraParam = new ActorStateDemoCameraParam(
                al::getActionFrameMax(this, "TakeOut"), 0, 30, nullptr, nullptr);
            mStateDemoCamera =
                new ActorStateDemoCamera(this, rInfo, "TakeOut", mDemoCameraParam, false);
            rc::addDemoActor(mGreenStar);
            al::initNerveState(this, mStateDemoCamera, &NrvKinopioBrigadeNpcTakeOut,
                               "グリーンスター取り出しデモ");
        }
    }

    if (al::isExistRail(this) && !mIsScenarioComplete) {
        al::setSyncRailToNearestPos(this);
        switch (mState) {
        case State::Move:
        case State::MoveCheckEnemy:
            al::startAction(this, "Run");
            al::setNerve(this, &NrvKinopioBrigadeNpcMove);
            break;
        case State::RouteDokan:
            al::startAction(this, "RouteDokanMove");
            al::setNerve(this, &NrvKinopioBrigadeNpcRouteDokan);
            break;
        default:
            makeActorDead();
            return;
        }
    } else if (isSyncAppear) {
        al::setNerve(this, &NrvKinopioBrigadeNpcAppear);
    } else if (mGreenStar != nullptr || mGoalItem != nullptr) {
        al::setNerve(this, &NrvKinopioBrigadeNpcStandBy);
    }

    if (!mIsSingleMode) {
        return;
    }

    al::tryGetArg(&mMemberType, rInfo, "MemberType");
    if (mMemberType == 0) {
        return;
    }

    al::tryStartMtpAnimIfNotPlaying(this, "Color");
    switch (mMemberType) {
    case 1:
        al::setMtpAnimFrameAndStop(this, 0.0f);
        break;
    case 2:
        al::setMtpAnimFrameAndStop(this, 2.0f);
        al::startAction(this, "Glasses");
        break;
    case 3:
        al::setMtpAnimFrameAndStop(this, 1.0f);
        al::startAction(this, "Glasses");
        break;
    default:
        break;
    }
}

/**
 * @brief Moves the member to the location where it waits once it was discovered.
 * @param isRegisterClipping Whether to also move the member into the clipping group of that
 * location and mark it as discovered.
 */
void KinopioBrigadeNpc::moveToDiscoveredLocation(bool isRegisterClipping) {
    if (mIsDiscovered) {
        return;
    }

    al::setTrans(this, mDiscoveredTrans);
    if (isRegisterClipping) {
        al::ClippingAreaActorInfoNode* pNode = getClippingInfoNode();
        pNode->mInfo->removeActor(pNode);
        mDiscoveredClippingInfo->registerActor(pNode, false);
        mIsDiscovered = true;
    }
}

/**
 * @brief Initializes the member.
 * @param rInfo Actor init info.
 */
void KinopioBrigadeNpc::init(const al::ActorInitInfo& rInfo) {
    init(rInfo, false);
}

/**
 * @brief Registers the member's shine on the island map.
 */
void KinopioBrigadeNpc::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    if (mWatcher != nullptr || mGoalItem == nullptr) {
        return;
    }

    auto* pIslandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
    if (pIslandMap == nullptr) {
        return;
    }

    pIslandMap->addSpecialShineLocation(
        this, al::getTrans(mGoalItem), {mGoalItem->getIslandId() - 1, mGoalItem->getShineId() - 1});
    pIslandMap->setSpecialShineIconComplete(this, isGoalItemScenarioComplete());
}

/**
 * @brief Moves an already rescued member to its discovered location once it is clipped.
 */
void KinopioBrigadeNpc::startClipped() {
    al::LiveActor::startClipped();
    if (mIsSingleMode && mIsScenarioComplete) {
        moveToDiscoveredLocation(true);
    }
}

/**
 * @brief Updates the timers, the wobble animation and the stroke and microphone reactions.
 */
void KinopioBrigadeNpc::control() {
    if (mReactionCoolTime > 0) {
        mReactionCoolTime--;
    }

    if (mUnusedTimer > 0) {
        mUnusedTimer--;
    }

    mAnimScaleController->update();
    if (mAnimScaleController->isHitReaction(-1)) {
        al::setScale(this, mAnimScaleController->getScale());
    } else {
        mStateSupportStroke->update();
        if (mStateSupportStroke->isTrigStroke()) {
            sead::Vector3f front;
            al::calcFrontDir(&front, this);
            al::appearItemTiming(this, "撫でる",
                                 al::getTrans(this) + sead::Vector3f(0.0f, 180.0f, 0.0f), front);
        }
    }

    if (!isEnableReaction(this) &&
        al::isNearZero(al::getScale(this) - sead::Vector3f::ones, 0.001f)) {
        mMicRumbler->update();
    }

    if (isEnableMicReaction(this) && al::isMicInputOn(this) &&
        !al::isNerve(this, &NrvKinopioBrigadeNpcMicReaction)) {
        al::setNerve(this, &NrvKinopioBrigadeNpcMicReaction);
    }
}

/**
 * @brief Notices enemies in sight, pushes the route dokan rider and pushes back other actors.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void KinopioBrigadeNpc::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEye(pSelf)) {
        if (mState == State::MoveCheckEnemy && al::isSensorEnemyBody(pOther)) {
            mIsEnemyNear = true;
        }

        return;
    }

    if (al::isSensorRide(pOther) && al::isNerve(this, &NrvKinopioBrigadeNpcRouteDokan)) {
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        rc::sendMsgRouteDokanPlayerTouch(pOther, pSelf, front);
        if (al::isGreaterStep(this, 60)) {
            al::setNerve(this, &NrvKinopioBrigadeNpcRouteDokan);
            startSeCross(this);
        }
    }

    if (al::isSensorPlayer(pOther) || al::isSensorEnemy(pOther) || al::isSensorNpc(pOther) ||
        al::isSensorKoopaJr(pOther) || al::isSensorDoorKey(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (!al::isSensorPlessie(pOther) || !al::isSensorName(pOther, "Body")) {
        return;
    }

    al::sendMsgPush(pOther, pSelf);
    if (canPlaySingleModeReaction()) {
        if (!al::isNerve(this, &NrvKinopioBrigadeNpcSpinReaction)) {
            mReactionCoolTime = 30;
            rc::requestHitReactionToAttackerNpc(pSelf, pOther);
            mNerveAfterSpinReaction = al::getNerve(this);
            al::setNerve(this, &NrvKinopioBrigadeNpcSpinReaction);
        }
    } else if (mReactionCoolTime == 0) {
        mReactionCoolTime = 30;
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
    }
}

/**
 * @brief Checks whether the member plays the single mode (brigade) reactions.
 * @return Whether the member belongs to a brigade.
 */
bool KinopioBrigadeNpc::canPlaySingleModeReaction() const {
    return mMemberType > 0 || mWatcher != nullptr;
}

/**
 * @brief Reacts to attacks and trampling.
 * @param pMsg Received message.
 * @param pSelf Own sensor.
 * @param pOther Sender sensor.
 * @return Whether the message was handled.
 */
bool KinopioBrigadeNpc::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                   al::HitSensor* pOther) {
    if (!canReceiveSingleModeMsg()) {
        return false;
    }

    if (al::isMsgPlayerTrampleForCrossoverSensor(pMsg, pSelf, pOther) ||
        al::isMsgPlayerObjHipDropReflectAll(pMsg)) {
        startReactionOrNegative(&NrvKinopioBrigadeNpcTrampled);

        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        return true;
    }

    if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgBallAttack(pMsg) ||
        al::isMsgBallTrample(pMsg)) {
        startReactionOrNegative(&NrvKinopioBrigadeNpcReaction);

        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        return true;
    }

    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgKickKouraReflect(pMsg)) {
        if (!tryStartReaction()) {
            return false;
        }

        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        return true;
    }

    if (al::isMsgLaserAttack(pMsg) || rc::isMsgBobsledBodyAttack(pMsg) ||
        rc::isMsgSkateShoesAttack(pMsg)) {
        if (rc::isMsgSkateShoesAttack(pMsg) && mReactionCoolTime == 0) {
            mReactionCoolTime = 30;
            rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        }

        return tryStartSpinReaction();
    }

    if (al::isMsgNekoAttack(pMsg) || rc::isMsgPackunEat(pMsg)) {
        if (al::isNerve(this, &NrvKinopioBrigadeNpcSpinReaction)) {
            return false;
        }

        rc::requestHitReactionToAttackerNpc(pOther, pSelf);
        return tryStartSpinReaction();
    }

    if (rc::isMsgPackunEatStart(pMsg)) {
        if (mReactionCoolTime != 0 || al::isNerve(this, &NrvKinopioBrigadeNpcSpinReaction)) {
            return false;
        }

        mReactionCoolTime = 180;
        return true;
    }

    return false;
}

/**
 * @brief Checks whether the member is free to react (not busy handing out its reward).
 * @return Whether messages can be received.
 */
bool KinopioBrigadeNpc::canReceiveSingleModeMsg() const {
    return !mIsSingleMode || !isGivingReward();
}

/**
 * @brief Reacts to touch screen pokes and strokes.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Touched target.
 * @return Whether the message was handled.
 */
bool KinopioBrigadeNpc::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                              al::ScreenPointer* pPointer,
                                              al::ScreenPointTarget* pTarget) {
    if (!canReceiveSingleModeMsg()) {
        return false;
    }

    if (al::isMsgTouchAssistTrigNoPat(pMsg)) {
        return tryStartReaction();
    }

    if (al::isMsgTouchAssist(pMsg) && isEnableMicReaction(this)) {
        if (!al::isNerve(this, &NrvKinopioBrigadeNpcTouch)) {
            al::setNerve(this, &NrvKinopioBrigadeNpcTouch);
        }

        mIsTouched = true;
    }

    return mStateSupportStroke->receiveMsgScreenPoint(pMsg, pPointer, pTarget);
}

/**
 * @brief Gets the shine handed out by the member.
 * @return The shine, or nullptr.
 */
GoalItem* KinopioBrigadeNpc::getGoalItem() {
    return mGoalItem;
}

/**
 * @brief Checks whether the member was found and moved to its discovered location.
 * @return Whether the member was discovered.
 */
bool KinopioBrigadeNpc::isDiscovered() const {
    return mIsDiscovered;
}

/**
 * @brief Checks whether Fury Bowser's rampage is going on.
 * @return Whether the member should be afraid.
 */
bool KinopioBrigadeNpc::checkDisaster() {
    if (mIsSingleMode) {
        DisasterModeController* pController = DisasterModeController::tryGetController(this);
        if (pController != nullptr &&
            pController->getState() == DisasterModeController::State(9)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Plays the appear animation.
 */
void KinopioBrigadeNpc::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }

    if (!al::isActionEnd(this)) {
        return;
    }

    if (mGreenStar == nullptr && mGoalItem == nullptr) {
        al::setNerve(this, &NrvKinopioBrigadeNpcWait);
        return;
    }

    if (al::isValidSwitchStart(this) || mIsSingleMode) {
        al::setNerve(this, &NrvKinopioBrigadeNpcStandBy);
        return;
    }

    al::setNerve(this, &NrvKinopioBrigadeNpcRelief);
}

/**
 * @brief Waits for the start switch before handing out the reward.
 */
void KinopioBrigadeNpc::exeStandBy() {
    if (al::isFirstStep(this)) {
        if (mState == State::Afraid) {
            al::startAction(this, "Afraid");
        } else if (mIsSingleMode) {
            al::setNerve(this, &NrvKinopioBrigadeNpcWait);
        } else {
            al::startAction(this, "Wait");
        }
    }

    if (isOnSwitchStartMultiMode()) {
        al::setNerve(this, &NrvKinopioBrigadeNpcRelief);
        return;
    }

    if (isOnSwitchStartSingleMode()) {
        al::setNerve(this, &NrvKinopioBrigadeNpcReliefStart);
        return;
    }

    if (checkDisaster()) {
        al::setNerve(this, &NrvKinopioBrigadeNpcDisasterAfraid);
    }
}

/**
 * @brief Checks whether the start switch was turned on before the shine was collected.
 * @return Whether the member should start handing out its shine.
 */
bool KinopioBrigadeNpc::isOnSwitchStartSingleMode() {
    if (mIsSingleMode && al::isOnSwitchStart(this) && !mIsScenarioComplete) {
        return true;
    }

    return false;
}

/**
 * @brief Disables island warps and starts the relief.
 */
void KinopioBrigadeNpc::exeReliefStart() {
    if (al::isFirstStep(this)) {
        IslandMap::setIslandWarpEnable(this, false);
        al::setNerve(this, &NrvKinopioBrigadeNpcRelief);
    }
}

/**
 * @brief Starts the reward cutscene and the relief animation.
 */
void KinopioBrigadeNpc::exeRelief() {
    if (al::isFirstStep(this)) {
        if (mIsSingleMode) {
            if (!al::isOnSwitchStart(this) || mIsScenarioComplete) {
                IslandMap::setIslandWarpEnable(this, true);
                al::setNerve(this, &NrvKinopioBrigadeNpcWait);
                return;
            }

            if (!rc::isAllPlayerOnGround(this) || !rc::requestStartDemoCutscene(this)) {
                al::setNerve(this, &NrvKinopioBrigadeNpcRelief);
                al::enableChangeSituation(this);
                return;
            }

            rc::addDemoActor(this);
            rc::setDemoAudioType(this, alSeFunction::DemoType(3));
            al::disableChangeSituation(this);
            al::changeBgmVolume(this, 0.5f, 60);
            al::LiveActor* pPlayer = al::getPlayerActor(getSceneInfo()->playerHolder, 0);
            rc::invalidatePlayerInput(pPlayer, 210);
            sead::Vector3f npcPos = al::getTrans(this);
            sead::Vector3f playerPos = al::getTrans(pPlayer);
            sead::Vector3f front = npcPos - playerPos;
            front.y = 0.0f;
            front.normalize();
            rc::setPlayerFrontVec(pPlayer, front);
            NpcGoalCameraTarget cameraTarget(&mCameraAt, &mCameraPos);
            cameraTarget.setMinAlignmentAngle(15.0f);
            cameraTarget.setPlayerMustBeVisible(true);
            sead::Vector3f lookAt = (npcPos + playerPos) * 0.5f;
            lookAt.y = npcPos.y + 50.0f;
            f32 height = npcPos.y + 250.0f;
            if (!cameraTarget.calcSafeAngle(npcPos, playerPos, this, &lookAt, 1000.0f,
                                            &height)) {
                mCameraAt = al::getCameraAt_RS(this, 0);
                mCameraPos = al::getCameraPos_RS(this, 0);
            }

            al::startCamera_RS(this, mCameraTicket, -1);
        }

        al::startAction(this, "Relief");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvKinopioBrigadeNpcTurnToCamera);
    }
}

/**
 * @brief Turns towards the camera before taking out the reward.
 */
void KinopioBrigadeNpc::exeTurnToCamera() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Turn");
    }

    if (al::turnQuatFrontToPosDegreeH(this, al::getCameraPos(this), 6.0f)) {
        al::setNerve(this, &NrvKinopioBrigadeNpcTakeOut);
    }
}

/**
 * @brief Takes out the green star or the shine.
 */
void KinopioBrigadeNpc::exeTakeOut() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TakeOut");
        al::invalidateClipping(this);
    }

    if (al::isStep(this, 120)) {
        if (mGreenStar != nullptr) {
            sead::Vector3f appearPos = al::getTrans(this) + sead::Vector3f(0.0f, 300.0f, 0.0f);
            mGreenStar->appearWithPos(appearPos);
        } else if (mGoalItem != nullptr) {
            al::endCamera_RS(this, mCameraTicket, -1, false);
            if (mStateDemoCamera == nullptr) {
                rc::requestEndDemoCutscene(this);
                al::changeBgmVolume(this, 1.0f, 12);
            }

            rc::requestStartDemoInGameCutscene(this);
            rc::addDemoActor(mGoalItem);
            rc::invalidatePlayerInput(this, 4);
            mGoalItem->appearCollect(true);
            IslandMap::setIslandWarpEnable(this, true);
            mIsScenarioComplete = true;
            if (mWatcher == nullptr) {
                auto* pIslandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
                if (pIslandMap != nullptr) {
                    pIslandMap->setSpecialShineIconComplete(this, true);
                }
            }
        }
    }

    bool isEnd = mStateDemoCamera != nullptr ? al::updateNerveState(this) : al::isActionEnd(this);
    if (isEnd) {
        mMicRumbler->stopAndReset();
        al::validateClipping(this);
        al::enableChangeSituation(this);
        al::setNerve(this, &NrvKinopioBrigadeNpcWait);
    }
}

/**
 * @brief Runs along the rail until the start switch is turned on.
 */
void KinopioBrigadeNpc::exeMove() {
    if (mState == State::MoveCheckEnemy) {
        if (mIsEnemyNear) {
            mSpeed += 0.3f;
        } else {
            mSpeed -= 0.1f;
        }

        mSpeed = sead::Mathf::clamp(mSpeed, 0.0f, mMoveSpeed);
        mIsEnemyNear = false;
    }

    al::moveSyncRailTurn(this, mSpeed);
    al::faceToDirection(this, al::getRailDir(this));
    bool isCollectAllowed = false;
    if (mIsSingleMode && mIsWaitGoalItemCollectSwitch) {
        if (!al::isOnStageSwitch(this, "SwitchAllowGoalItemCollect")) {
            return;
        }

        isCollectAllowed = true;
    }

    if (isOnSwitchStartMultiMode()) {
        if (mGreenStar != nullptr) {
            al::setNerve(this, &NrvKinopioBrigadeNpcRelief);
        } else {
            al::setNerve(this, &NrvKinopioBrigadeNpcWait);
        }

        return;
    }

    if (isOnSwitchStartSingleMode()) {
        if (mGoalItem != nullptr) {
            al::setNerve(this, &NrvKinopioBrigadeNpcReliefStart);
        } else {
            al::setNerve(this, &NrvKinopioBrigadeNpcWait);
        }

        return;
    }

    if (isCollectAllowed) {
        al::setNerve(this, &NrvKinopioBrigadeNpcWait);
    }
}

/**
 * @brief Rides along the route dokan rail, carrying the reward in front of the member.
 */
void KinopioBrigadeNpc::exeRouteDokan() {
    al::moveSyncRailTurn(this, mSpeed);
    al::faceToDirection(this, al::getRailDir(this));
    if (mGreenStar != nullptr) {
        if (al::isDead(mGreenStar)) {
            if (al::isOnSwitchStart(this)) {
                mGreenStar->makeActorAppeared();
            }
        } else if (!mGreenStar->isAcquiredInScene()) {
            al::calcRailPosAtCoord(al::getTransPtr(mGreenStar), this,
                                   al::getRailCoord(this) + 1000.0f);
        }
    }

    if (mGoalItem != nullptr) {
        if (al::isDead(mGoalItem)) {
            if (al::isOnSwitchStart(this)) {
                mGoalItem->makeActorAppeared();
            }
        } else if (!isGoalItemScenarioComplete()) {
            al::calcRailPosAtCoord(al::getTransPtr(mGoalItem), this,
                                   al::getRailCoord(this) + 1000.0f);
        }
    }
}

/**
 * @brief Waits (or waves at a nearby player) and turns towards players out of sight.
 */
void KinopioBrigadeNpc::exeWait() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvKinopioBrigadeNpcWait)) {
            al::startAction(this, "Wait");
        } else {
            al::startAction(this, "Wave");
        }

        if (mGuideBalloon != nullptr && mWatcher != nullptr) {
            if ((al::isNerve(this, &NrvKinopioBrigadeNpcWave) ||
                 al::isNerve(this, &NrvKinopioBrigadeNpcWaveTurn)) &&
                mWatcher->getDiscoveredMemberCount() != mWatcher->getMemberCount()) {
                if (!mGuideBalloon->isAlive()) {
                    al::invalidateClipping(this);
                    mGuideBalloon->startShowBrigade(mWatcher->getBrigadeCollected());
                    al::startSe(this, "GuidMessageAppear");
                }
            } else {
                al::validateClipping(this);
                mGuideBalloon->endShow();
            }
        }

        mTurnCounter = 0;
    }

    if (checkDisaster()) {
        al::setNerve(this, &NrvKinopioBrigadeNpcDisasterAfraid);
        return;
    }

    al::LiveActor* pPlayer = al::tryFindNearestPlayerActor(this);
    if (pPlayer == nullptr) {
        return;
    }

    if (mIsInWaveArea &&
        !al::isInAreaObjPlayerAnyOne(
            this, rc::tryFindAreaObjGroup(this, rc::AreaObjType::KinopioBrigadeWaveArea))) {
        if (!al::isNerve(this, &NrvKinopioBrigadeNpcWait)) {
            al::setNerve(this, &NrvKinopioBrigadeNpcWait);
        }

        return;
    }

    f32 distance = al::calcDistanceH(this, pPlayer);
    bool isNear = distance < 1000.0f;
    if (al::isNerve(this, &NrvKinopioBrigadeNpcWait)) {
        if (isNear) {
            al::setNerve(this, &NrvKinopioBrigadeNpcWave);
            return;
        }
    } else if (!isNear) {
        al::setNerve(this, &NrvKinopioBrigadeNpcWait);
        return;
    }

    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    f32 sightAngle = al::isNerve(this, &NrvKinopioBrigadeNpcWait) ? 25.0f : 1.0f;
    if (!al::isInSightFan(this, al::getTrans(pPlayer), front, 100000.0f, sightAngle, 90.0f) &&
        mTurnCounter++ >= 19) {
        if (al::isNerve(this, &NrvKinopioBrigadeNpcWait)) {
            al::setNerve(this, &NrvKinopioBrigadeNpcWaitTurn);
        } else {
            al::setNerve(this, &NrvKinopioBrigadeNpcWaveTurn);
        }

        return;
    }

    if (mIsSingleMode && al::isValidSwitchStart(this) && isOnSwitchStartSingleMode()) {
        al::setNerve(this, &NrvKinopioBrigadeNpcReliefStart);
    }
}

/**
 * @brief Turns towards the nearest player, then goes back to waiting or waving.
 */
void KinopioBrigadeNpc::exeWaitTurn() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvKinopioBrigadeNpcWaitTurn)) {
            al::startAction(this, "Turn");
        } else {
            al::startAction(this, "WaveTurn");
        }

        mTurnCounter = 0;
    }

    al::LiveActor* pPlayer = al::tryFindNearestPlayerActor(this);
    if (pPlayer == nullptr ||
        (al::turnQuatFrontToPosDegreeH(this, al::getTrans(pPlayer), 3.0f) &&
         mTurnCounter++ >= 19)) {
        if (al::isNerve(this, &NrvKinopioBrigadeNpcWaitTurn)) {
            al::setNerve(this, &NrvKinopioBrigadeNpcWait);
        } else {
            al::setNerve(this, &NrvKinopioBrigadeNpcWave);
        }

        return;
    }

    if (checkDisaster()) {
        al::setNerve(this, &NrvKinopioBrigadeNpcDisasterAfraid);
    }
}

/**
 * @brief Plays the trampled animation.
 */
void KinopioBrigadeNpc::exeTrampled() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Trampled");
    }

    if (al::isActionEnd(this)) {
        setNerveWaitOrWave();
    }
}

/**
 * @brief Plays the reaction to an attack.
 */
void KinopioBrigadeNpc::exeReaction() {
    if (al::isFirstStep(this)) {
        al::LiveActor* pPlayer = al::tryFindNearestPlayerActor(this);
        if (pPlayer != nullptr && al::calcDistanceH(this, pPlayer) < 1000.0f) {
            al::startAction(this, "ReactionWave");
        } else {
            al::startAction(this, "Reaction");
        }
    }

    if (al::isActionEnd(this)) {
        setNerveWaitOrWave();
    }
}

/**
 * @brief Plays the reaction to a spinning attack, then resumes the previous nerve.
 */
void KinopioBrigadeNpc::exeSpinReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, mNerveAfterSpinReaction != nullptr ? mNerveAfterSpinReaction :
                                                                &NrvKinopioBrigadeNpcWait);
        mNerveAfterSpinReaction = nullptr;
    }
}

/**
 * @brief Plays the reaction to microphone input.
 */
void KinopioBrigadeNpc::exeMicReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ReactionMic");
    }

    if (al::isActionEnd(this)) {
        setNerveWaitOrWave();
    }
}

/**
 * @brief Plays the joy animation while being touched.
 */
void KinopioBrigadeNpc::exeTouch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TouchJoy");
    }

    if (mIsTouched) {
        mIsTouched = false;
        return;
    }

    setNerveWaitOrWave();
}

/**
 * @brief Cowers while Fury Bowser is rampaging.
 */
void KinopioBrigadeNpc::exeDisasterAfraid() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Afraid");
    }

    if (mIsSingleMode && al::isValidSwitchStart(this) && isOnSwitchStartSingleMode()) {
        al::setNerve(this, &NrvKinopioBrigadeNpcReliefStart);
        return;
    }

    if (checkDisaster()) {
        return;
    }

    setNerveWaitOrWave();
}
