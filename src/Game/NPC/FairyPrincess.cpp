#include "NPC/FairyPrincess.hpp"
#include "NPC/Bottle.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Enemy/ActorJointLookController.hpp"
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
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(FairyPrincess, WaitGoalPole)
NERVE_DECL(FairyPrincess, Stroke)
NERVE_DECL(FairyPrincess, DemoAction)
const struct {
    FairyPrincessNrvWaitGoalPole WaitGoalPole;
    FairyPrincessNrvStroke Stroke;
    FairyPrincessNrvDemoAction DemoAction;
} NrvFairyPrincess;
} // namespace
/** @brief Creates the captive fairy. @param pName Actor name. */
FairyPrincess::FairyPrincess(const char* pName)
    : al::LiveActor(pName), mBottle(nullptr), mStroke(nullptr), mLook(nullptr),
      mHeadBase(sead::Matrix34f::ident),
      mRumble(new al::RumbleCalculatorCosMultLinear(2.5f, 1.5f, 0.03f, 30)),
      mHasAppearedItem(false), mLookEnabled(false) {}
/** @brief Releases the fairy actor. */
FairyPrincess::~FairyPrincess() = default;
/** @brief Initializes the default fairy appearance. @param rInfo Placement initialization. */
void FairyPrincess::init(const al::ActorInitInfo& rInfo) {
    initFairyWithArchiveName(rInfo, "FairyPrincess01", "WaitHappyGoalPole");
}
/** @brief Initializes fairy animation and head tracking. @param rInfo Placement initialization.
 * @param pArchive Model archive. @param pAction Initial action. */
void FairyPrincess::initFairyWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchive,
                                             const char* pAction) {
    al::initActorWithArchiveName(this, rInfo, pArchive, nullptr);
    al::initNerve(this, &NrvFairyPrincess.WaitGoalPole, 1);
    al::startAction(this, pAction);
    mStroke = new ActorStateSupportStroke(this);
    mStroke->disableFeedback();
    al::initNerveState(this, mStroke, &NrvFairyPrincess.Stroke, "DRC\u306a\u3067\u306a\u3067");
    al::initJointControllerKeeper(this, 1);
    auto* pParam = new ActorJointLookControllerParam(
        1.5f, sead::Vector2f(-60, 60), false, al::getJointMtxPtr(this, "JointRoot"), &mHeadBase);
    mLook = new ActorJointLookController(this, 1);
    mLook->appendJoint("JointRoot", sead::Vector3f::ex, pParam);
    makeActorAppeared();
}
/** @brief Updates head tracking and reaction scale. */
void FairyPrincess::control() {
    if (mLookEnabled) {
        mLook->update();
    }
    if (al::isNerve(this, &NrvFairyPrincess.DemoAction) || mBottle) {
        return;
    }
    mRumble->calc();
    al::setScaleY(this, mRumble->getOutput().y + 1.0f);
}
/** @brief Pushes contacting players. @param pSelf Own sensor. @param pOther Contact sensor. */
void FairyPrincess::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorPlayer(pOther)) {
        al::sendMsgPush(pOther, pSelf);
    }
}
/** @brief Checks whether a free fairy can react. @return Whether reactions are enabled. */
bool FairyPrincess::isEnableReaction() const {
    if (al::isNerve(this, &NrvFairyPrincess.DemoAction)) {
        return false;
    }
    if (mRumble->isRumbling()) {
        return false;
    }
    return mBottle == nullptr;
}
/** @brief Handles physical interactions. @param pMessage Incoming message. @param pOther Sender
 * sensor. @param pSelf Receiver sensor. @return Whether the interaction was handled. */
bool FairyPrincess::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                               al::HitSensor* pSelf) {
    if (!isEnableReaction()) {
        return false;
    }
    if (al::isMsgPlayerTrampleForCrossoverSensor(pMessage, pOther, pSelf) ||
        al::isMsgPlayerObjHipDropReflectAll(pMessage) ||
        al::isMsgPlayerBodyAttackReflect(pMessage)) {
        goto react;
    }
    if (al::isMsgKickKouraAttack(pMessage) || al::isMsgPlayerBodyAttack(pMessage) ||
        al::isMsgPlayerBodyLanding(pMessage) || al::isMsgPlayerBoomerangAttack(pMessage) ||
        al::isMsgPlayerInvincibleAttack(pMessage)) {
        return false;
    }
    if (al::isMsgBallTrample(pMessage) || EnemyStateUtil::isMsgBlowDown(pMessage) ||
        al::isMsgKickKouraReflect(pMessage) || al::isMsgPlayerBoomerangReflect(pMessage)) {
        goto react;
    }
    return false;
react:
    mRumble->start(0);
    al::startSe(this, "PgTrampled", nullptr);
    rc::requestHitReactionToAttackerNpc(pSelf, pOther);
    return true;
}
/** @brief Handles pointer interactions. @param pMessage Incoming message. @param pPointer Input
 * pointer. @param pTarget Point target. @return Whether the interaction was handled. */
bool FairyPrincess::receiveMsgScreenPoint(const al::SensorMsg* pMessage,
                                          al::ScreenPointer* pPointer,
                                          al::ScreenPointTarget* pTarget) {
    if (!mBottle && mStroke->receiveMsgScreenPoint(pMessage, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvFairyPrincess.Stroke)) {
            al::setNerve(this, &NrvFairyPrincess.Stroke);
        }
        return true;
    }
    if (!isEnableReaction()) {
        return false;
    }
    if (al::isMsgTouchAssistTrigNoPat(pMessage)) {
        mRumble->start(0);
        al::startSe(this, "PgTouched", nullptr);
        return true;
    }
    return false;
}
/** @brief Updates the head tracking frame after animation. */
void FairyPrincess::calcAnim() {
    al::LiveActor::calcAnim();
    mHeadBase = *getBaseMtx();
    sead::Vector3f pos(0, 0, 0);
    al::calcJointPos(&pos, this, "JointRoot");
    mHeadBase.setTranslation(pos);
    if (mBottle) {
        al::rotateMtxYDirDegree(&mHeadBase, mHeadBase, -30.0f);
    }
}
/** @brief Clips the fairy and its bottle together. */
void FairyPrincess::startClipped() {
    al::LiveActor::startClipped();
    if (mBottle) {
        mBottle->startClipped();
    }
}
/** @brief Restores the fairy and its bottle from clipping. */
void FairyPrincess::endClipped() {
    al::LiveActor::endClipped();
    if (mBottle) {
        mBottle->endClipped();
    }
}
/** @brief Starts a demo at another actor position. @param pAction Demo action. @param pDemo
 * Position source. */
void FairyPrincess::startDemoAction(const char* pAction, const al::LiveActor* pDemo) {
    al::invalidateClipping(this);
    al::resetPosition(this, al::getTrans(pDemo), false);
    al::startAction(this, pAction);
    mRumble->reset();
    al::setScaleY(this, 1.0f);
    al::setNerve(this, &NrvFairyPrincess.DemoAction);
}
/** @brief Creates the bottle at the placement. @param rInfo Placement initialization. */
void FairyPrincess::createBottle(const al::ActorInitInfo& rInfo) {
    mBottle = new Bottle("\u30dc\u30c8\u30eb");
    al::initCreateActorWithPlacementInfo(mBottle, rInfo);
}
/** @brief Breaks an attached bottle and starts the release action. @return Whether a bottle exists.
 */
bool FairyPrincess::tryBreakBottle() {
    if (!mBottle) {
        return false;
    }
    mBottle->setBreak();
    al::startAction(this, "Release");
    al::setNerve(this, &NrvFairyPrincess.DemoAction);
    return true;
}
/** @brief Gets the wing subactor. @return Fairy wings. */
al::LiveActor* FairyPrincess::getWingActor() const {
    return al::getSubActor(this, "\u5996\u7cbe\u7fbd");
}
/** @brief Gets the attached bottle. @return Bottle actor, or nullptr. */
al::LiveActor* FairyPrincess::getBottleActor() const { return mBottle; }
/** @brief Gets the inner bottle subactor. @return Inner bottle actor. */
al::LiveActor* FairyPrincess::getBottleInnerActor() const { return mBottle->getInnerActor(); }
/** @brief Gets the bottle cap subactor. @return Cap actor. */
al::LiveActor* FairyPrincess::getBottleCapActor() const { return mBottle->getCapActor(); }
/** @brief Tracks nearby players and reacts to microphone input. */
void FairyPrincess::exeWaitGoalPole() {
    if (mLookEnabled) {
        mLook->setLookAtNearestPlayer(-1.0f);
    }
    if (isEnableReaction() && al::isMicInputOn(this)) {
        {
            sead::Vector3f pos(0, 0, 0);
            al::calcJointPos(&pos, this, "JointRoot");
            if (al::isJudgedToClipFrustum(this, pos, al::getClippingRadius(this), 300.0f)) {
                return;
            }
        }
        mRumble->start(0);
        al::startSe(this, "PgReactionMic", nullptr);
    }
}
/** @brief Updates stroking feedback and its one-time item reward. */
void FairyPrincess::exeStroke() {
    if (!mRumble->isRumbling()) {
        mRumble->start(0);
    }
    al::holdSe(this, "PgTouchStroked", nullptr);
    al::updateNerveState(this);
    if (!mHasAppearedItem && mStroke->shouldAppearItem()) {
        sead::Vector3f pos(0, 0, 0), front(0, 0, 0);
        al::calcJointPos(&pos, this, "JointRoot");
        al::calcFrontDir(&front, this);
        al::appearItemTiming(this, "\u64ab\u3067\u308b", pos + sead::Vector3f(0, 150, 0), front);
        mHasAppearedItem = true;
    }
    if (!mStroke->isTouch()) {
        mRumble->reset();
        al::setNerve(this, &NrvFairyPrincess.WaitGoalPole);
    }
}
/** @brief Clears head tracking when a demo begins. */
void FairyPrincess::exeDemoAction() {
    if (al::isFirstStep(this)) {
        mLook->stopLook();
    }
}
