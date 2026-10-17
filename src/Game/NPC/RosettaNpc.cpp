#include "NPC/RosettaNpc.hpp"
#include "Player/Normal/PlayerRetargettingSelector.hpp"
namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pHolder);
}
#include "Player/Normal/PlayerAnimFunc.hpp"
#include "Project/Base/StringUtil.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/Joint/JointLookAtController.hpp"
#include "Library/Model/ModelShapeUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace al {
/** @brief Tests microphone input for an actor. @param pActor Actor using audio. @return Whether
 * microphone input is active. */
inline bool isMicInputOn(const LiveActor* pActor) {
    return isMicInputOn(static_cast<const IUseAudioKeeper*>(pActor));
}
} // namespace al
namespace {
NERVE_DECL(RosettaNpc, Wait)
NERVE_DECL(RosettaNpc, BarrierLoop)
NERVE_DECL(RosettaNpc, Barrier)
NERVE_DECL(RosettaNpc, GoalPose)
NERVE_DECL(RosettaNpc, Guard)
const struct {
    RosettaNpcNrvWait Wait;
    RosettaNpcNrvBarrierLoop BarrierLoop;
    RosettaNpcNrvBarrier Barrier;
    RosettaNpcNrvGoalPose GoalPose;
    RosettaNpcNrvGuard Guard;
} NrvRosettaNpc;
const sead::Vector2f sRosettaJointRange(-10, 20);
const char* sRosettaGoalActions[] = {"DemoRosettaAppearance1", "DemoRosettaAppearance2",
                                     "DemoRosettaAppearance3", "DemoRosettaAppearance4"};
/** @brief Starts a retargeted action on Rosetta and her subactors. @param pModel Player model.
 * @param pAction Requested action. @param pSelector Retargeting selector. */
void startRosettaAction(PlayerModel* pModel, const char* pAction,
                        PlayerRetargettingSelector* pSelector) {
    al::StringTmp<128> actionName;
    auto type = PlayerAnimFunc::convertToRegularName(&actionName, pModel, pAction);
    PlayerAnimFunc::controlRetargetting(pModel, pSelector, type);
    al::startAction(pModel, actionName.cstr());
    for (int i = 0; i < al::getSubActorNum(pModel); ++i) {
        if (!al::tryStartAction(al::getSubActor(pModel, i), actionName.cstr())) {
            al::tryStartAction(al::getSubActor(pModel, i), "Wait");
        }
    }
}
} // namespace
/** @brief Creates Rosetta and her animation and interaction controllers. @param pName Actor name.
 * @param rInfo Placement initialization. */
RosettaNpc::RosettaNpc(const char* pName, const al::ActorInitInfo& rInfo)
    : PlayerModel(pName, "Rosetta", rInfo, "Npc"), mSelector(nullptr), mStroke(nullptr),
      mInitialFront(0, 0, 0), mJointFlag(false), mQuat(sead::Quatf::unit), mGoalRank(1),
      mGuardCooldown(0) {
    al::initNerve(this, &NrvRosettaNpc.Wait, 1);
    initJointController(sRosettaJointRange, &mJointFlag, &mQuat, 0.05f);
    setNpcNames("Rosetta", "");
    mStroke = new ActorStateSupportStroke(this);
    mStroke->disableFeedback();
    al::initNerveState(this, mStroke, &NrvRosettaNpc.BarrierLoop, "DRC\u306a\u3067\u306a\u3067");
    mSelector = rc::createPlayerRetargettingSelector(this);
    auto* pSelector = mSelector;
    int id = pSelector->createRetargettingInfo("Rosetta", EPlayerFigure::Super, nullptr);
    pSelector->applyDefaultRetargettingInfo(this, id);
    al::calcFrontDir(&mInitialFront, this);
    makeActorAppeared();
}
/** @brief Releases Rosetta. */
RosettaNpc::~RosettaNpc() = default;
/** @brief Counts down the guard retrigger delay. */
void RosettaNpc::control() {
    if (mGuardCooldown - 1 >= 0) {
        --mGuardCooldown;
    }
}
/** @brief Pushes nearby players. @param pSelf Own sensor. @param pOther Contact sensor. */
void RosettaNpc::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}
/** @brief Reacts to player attacks. @param pMessage Attack message. @param pOther Attacker sensor.
 * @param pSelf Own sensor. @return Whether the attack was handled. */
bool RosettaNpc::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (!al::isMsgPlayerBodyAttack(pMessage) && !al::isMsgPlayerBodyLanding(pMessage) &&
        !al::isMsgPlayerBoomerangAttack(pMessage) && !al::isMsgPlayerInvincibleAttack(pMessage) &&
        (EnemyStateUtil::isMsgBlowDown(pMessage) || al::isMsgPlayerBoomerangReflect(pMessage)) &&
        tryStartGuard()) {
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        return true;
    }
    if (al::isMsgPlayerTrampleForCrossoverSensor(pMessage, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMessage) ||
        al::isMsgPlayerBodyAttackReflect(pMessage)) {
        al::setNerve(this, &NrvRosettaNpc.Barrier);
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        return true;
    }
    return false;
}
/** @brief Starts guarding when the retrigger delay has expired. @return Whether guarding began. */
bool RosettaNpc::tryStartGuard() {
    int cooldown = mGuardCooldown;
    mGuardCooldown = 10;
    if (cooldown > 0) {
        return false;
    }
    al::setNerve(this, &NrvRosettaNpc.Guard);
    return true;
}
/** @brief Responds to touch assistance. @param pMessage Touch message. @param pPointer Touch
 * pointer.
 * @param pTarget Touched target. @return Whether the message was handled. */
bool RosettaNpc::receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (!al::isNerve(this, &NrvRosettaNpc.GoalPose) &&
        mStroke->receiveMsgScreenPoint(pMessage, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvRosettaNpc.BarrierLoop)) {
            al::setNerve(this, &NrvRosettaNpc.BarrierLoop);
        }
        return true;
    }
    if (al::isMsgTouchAssistTrigNoPat(pMessage)) {
        return tryStartGuard();
    }
    return false;
}
/** @brief Looks toward the nearest player and responds to microphone input. */
void RosettaNpc::exeWait() {
    if (al::isFirstStep(this)) {
        startRosettaAction(this, "Wait", mSelector);
    }
    getJointLookController()->requestJointLookAt(al::findNearestPlayerPos(this));
    getJointLookController()->update();
    if (al::isMicInputOn(this) &&
        !al::isJudgedToClipFrustum(this, al::getTrans(this), al::getClippingRadius(this), 300)) {
        tryStartGuard();
    }
}
/** @brief Plays the assigned goal pose and extends its shadow. */
void RosettaNpc::exeGoalPose() {
    if (al::isFirstStep(this)) {
        startRosettaAction(this, sRosettaGoalActions[mGoalRank - 1], mSelector);
    }
    if (al::isStep(this, 1)) {
        al::showModelIfHide(this);
    }
    setShadowLength(500);
}
/** @brief Plays the attack barrier before returning to idle. */
void RosettaNpc::exeBarrier() {
    if (al::isFirstStep(this)) {
        startRosettaAction(this, "Barrier", mSelector);
    }
    al::setNerveAtActionEnd(this, &NrvRosettaNpc.Wait);
}
/** @brief Maintains the touch barrier and emits earned items. */
void RosettaNpc::exeBarrierLoop() {
    if (al::isFirstStep(this)) {
        startRosettaAction(this, "BarrierLoop", mSelector);
    }
    al::updateNerveState(this);
    if (mStroke->shouldAppearItem()) {
        sead::Vector3f front(0, 0, 0);
        al::calcFrontDir(&front, this);
        al::appearItem(this, al::getTrans(this) + sead::Vector3f(0, 150, 0), front);
    }
    if (!mStroke->isTouch()) {
        al::setNerve(this, &NrvRosettaNpc.Wait);
    }
}
/** @brief Plays the guard reaction before returning to idle. */
void RosettaNpc::exeGuard() {
    if (al::isFirstStep(this)) {
        startRosettaAction(this, "Guard", mSelector);
    }
    al::setNerveAtActionEnd(this, &NrvRosettaNpc.Wait);
}
/** @brief Prepares the goal appearance animation. @param rank One-based goal pose index. */
void RosettaNpc::startGoalPose(int rank) {
    mGoalRank = rank;
    al::invalidateClipping(this);
    getJointLookController()->invalidJoint(al::getJointIndex(getModelKeeper(), "Head"));
    al::faceToDirection(this, mInitialFront);
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvRosettaNpc.GoalPose);
}
