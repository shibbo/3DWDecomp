#include "Library/Bgm/BgmLineFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Boss/KoopaLastDemoAppear.hpp"
#include "Boss/BossStateDemoStart.hpp"
#include "Boss/KoopaLastFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
namespace {
NERVE_DECL(KoopaLastDemoAppear, Hide);
NERVE_DECL(KoopaLastDemoAppear, Demo);
NERVE_DECL(KoopaLastDemoAppear, Wait);
NERVE_DECL(KoopaLastDemoAppear, JumpStart);
NERVE_DECL(KoopaLastDemoAppear, Jump);
NERVE_DECL(KoopaLastDemoAppear, JumpEnd);
NERVES_MAKE_STRUCT(KoopaLastDemoAppear, Hide, Demo)
NERVES_MAKE_NOSTRUCT(KoopaLastDemoAppear, Wait, JumpStart, Jump, JumpEnd)
}
/** @brief Creates the opening demo actor. @param pName Actor name. */
KoopaLastDemoAppear::KoopaLastDemoAppear(const char* pName) : al::LiveActor(pName) {}
/** @brief Destroys the opening demo actor. */
KoopaLastDemoAppear::~KoopaLastDemoAppear() = default;
/** @brief Loads the camera, target pose, subactors, and start switch. @param rInfo Initialization data. */
void KoopaLastDemoAppear::init(const al::ActorInitInfo& rInfo) {
    KoopaLastFunction::initActorKoopaLastCommon(this, rInfo, "DemoAppear", nullptr);
    mDemoInfo = new BossDemoStartInfo(al::initAnimCamera(this, rInfo), this, "DemoBattleLastKoopaStart", 120, nullptr);
    mDemoState = new BossStateDemoStart(this, rInfo, mDemoInfo);
    al::initNerve(this, &NrvKoopaLastDemoAppear.Hide, 1);
    al::initNerveState(this, mDemoState, &NrvKoopaLastDemoAppear.Demo, "登場デモ");
    mInitPos = al::getTrans(this);
    mTargetPos = mInitPos;
    if (al::calcLinkChildNum(rInfo, "TargetPos") >= 1) {
        al::PlacementInfo placement;
        al::getLinksInfo(&placement, al::getPlacementInfo(rInfo), "TargetPos");
        sead::Vector3f pos(0.0f, 0.0f, 0.0f);
        sead::Quatf quat = sead::Quatf::unit;
        al::tryGetTrans(&pos, placement);
        al::tryGetQuat(&quat, placement);
        mTargetPos = pos;
    }
    al::invalidateShadow(al::getSubActor(this, "鈴"));
    auto* koopa = al::getSubActor(this, "クッパ(変身前)");
    al::changeEnvTextureWetObj(koopa);
    al::initJointControllerKeeper(koopa, 12);
    al::initJointSpringController(koopa, "HairFront1");
    al::initJointSpringController(koopa, "HairFront2");
    al::initJointSpringController(koopa, "HairFront3");
    al::initJointSpringController(koopa, "HairL1");
    al::initJointSpringController(koopa, "HairL2");
    al::initJointSpringController(koopa, "HairL3");
    al::initJointSpringController(koopa, "HairR1");
    al::initJointSpringController(koopa, "HairR2");
    al::initJointSpringController(koopa, "HairR3");
    al::initJointSpringController(koopa, "HairMiddle1");
    al::initJointSpringController(koopa, "HairMiddle2");
    al::initJointSpringController(koopa, "HairMiddle3");
    al::listenStageSwitchOnStart(this, al::Functor(this, &KoopaLastDemoAppear::start));
    if (!al::tryListenStageSwitchAppear(this)) makeActorDead();
}
/** @brief Starts the demo when the actor is alive. */
void KoopaLastDemoAppear::start() {
    if (al::isDead(this)) return;
    al::invalidateClipping(this);
    al::setNerve(this, &NrvKoopaLastDemoAppear.Demo);
}
/** @brief Resets the hidden actor at its initial pose. */
void KoopaLastDemoAppear::appear() {
    if (!al::isNerve(this, &NrvKoopaLastDemoAppear.Hide)) return;
    al::LiveActor::appear();
    al::hideModelIfShow(this);
    int count = al::getSubActorNum(this);
    for (int i = 0; i < count; i++) al::hideModelIfShow(al::getSubActor(this, i));
    al::resetPosition(this, mInitPos, false);
    al::setNerve(this, &NrvKoopaLastDemoAppear.Hide);
}
/** @brief Handles player contact outside the demo. @param pSelf Own sensor. @param pOther Target sensor. */
void KoopaLastDemoAppear::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvKoopaLastDemoAppear.Demo)) return;
    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) && !al::sendMsgEnemyAttack(pOther, pSelf)) al::sendMsgPush(pOther, pSelf);
}
/** @brief Handles common Bowser messages. @param pMsg Message. @param pSelf Own sensor. @param pOther Sender sensor. @return Whether handled. */
bool KoopaLastDemoAppear::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) { return KoopaLastFunction::receiveMsgCommon(pMsg, pSelf, pOther); }
/** @brief Handles common pointer messages. @param pMsg Message. @param pPointer Pointer. @param pTarget Target. @return Whether handled. */
bool KoopaLastDemoAppear::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) { return KoopaLastFunction::receiveMsgScreenPointCommon(pMsg, pPointer, pTarget); }
/** @brief Waits for the demo start switch. */
void KoopaLastDemoAppear::exeHide() {}
/** @brief Plays the opening demo and cleans up skipped effects. */
void KoopaLastDemoAppear::exeDemo() {
    if (al::isStep(this, 1)) {
        al::showModelIfHide(this);
        al::startAction(this, "DemoBattleLastKoopaStart");
        int count = al::getSubActorNum(this);
        for (int i = 0; i < count; i++) {
            al::showModelIfHide(al::getSubActor(this, i));
            al::startAction(al::getSubActor(this, i), "DemoBattleLastKoopaStart");
        }
        al::startBgm(this, "DemoBattleLastKoopaStart", -1, 0, -1, -1);
    }
    if (al::updateNerveState(this)) {
        if (mDemoState->isSkipped()) {
            al::clearSklAnimInterpole(this);
            al::tryDeleteEmitterAndParticleAll(this);
            int count = al::getSubActorNum(this);
            for (int i = 0; i < count; i++) al::tryDeleteEmitterAndParticleAll(al::getSubActor(this, i));
        }
        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvKoopaLastDemoAppearWait);
    }
}
/** @brief Restores the battle pose and music before jumping. */
void KoopaLastDemoAppear::exeWait() {
    if (al::isFirstStep(this)) {
        al::setTrans(this, mTargetPos);
        int count = al::getSubActorNum(this);
        for (int i = 0; i < count; i++) al::getSubActor(this, i)->kill();
        al::startAction(this, "StartDemoWait");
        al::startBgm(this, "Koopa", -1, 0, -1, -1);
    }
    if (al::isLessStep(this, 30)) return;
    al::setNerve(this, &NrvKoopaLastDemoAppearJumpStart);
}
/** @brief Plays the departure jump anticipation. */
void KoopaLastDemoAppear::exeJumpStart() {
    if (al::isFirstStep(this)) al::startAction(this, "StartDemoJumpStart");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvKoopaLastDemoAppearJump);
}
/** @brief Moves Bowser upward and slows the departure jump. */
void KoopaLastDemoAppear::exeJump() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "StartDemoJumpLoop");
        al::setVelocity(this, sead::Vector3f::ey * 60.0f);
        return;
    }
    al::setVelocity(this, al::getVelocity(this) - sead::Vector3f::ey);
    if (!al::isLessStep(this, 60)) al::setNerve(this, &NrvKoopaLastDemoAppearJumpEnd);
}
/** @brief Restores clipping and removes the actor. */
void KoopaLastDemoAppear::exeJumpEnd() {
    al::validateClipping(this);
    kill();
}
