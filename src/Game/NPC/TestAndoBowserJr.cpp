#include "NPC/TestAndoBowserJr.hpp"
#include "Player/Normal/PlayerKeyConfig.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(TestAndoBowserJr, Wait)
NERVE_DECL(TestAndoBowserJr, Hide)
NERVE_DECL(TestAndoBowserJr, AttackSpin)
const struct {
    TestAndoBowserJrNrvWait Wait;
    TestAndoBowserJrNrvHide Hide;
    TestAndoBowserJrNrvAttackSpin AttackSpin;
} NrvTestAndoBowserJr;
} // namespace
/** @brief Creates the assist-controller test actor. @param pName Actor name. */
TestAndoBowserJr::TestAndoBowserJr(const char* pName)
    : al::LiveActor(pName), mKeyConfig(nullptr), mInput(nullptr) {}
/** @brief Releases the test actor. */
TestAndoBowserJr::~TestAndoBowserJr() = default;
/** @brief Initializes the assist player's input and visibility. @param rInfo Placement
 * initialization. */
void TestAndoBowserJr::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTestAndoBowserJr.Wait, 0);
    makeActorAppeared();
    if (!isAssistPadConnected()) {
        al::hideModel(this);
        al::setNerve(this, &NrvTestAndoBowserJr.Hide);
    }
    mKeyConfig = new PlayerKeyConfig(rc::getPadPortByUserId(1));
    mInput = new PlayerInput(mKeyConfig);
    mInput->setActorForAreaSearch(this);
    al::invalidateClipping(this);
}
/** @brief Checks the assist player's controller. @return Whether its pad is connected. */
bool TestAndoBowserJr::isAssistPadConnected() const {
    return al::isPadConnected(rc::getPadPortByUserId(1));
}
/** @brief Appears beside the nearest player when the assist pad connects. */
void TestAndoBowserJr::exeHide() {
    if (isAssistPadConnected()) {
        const al::LiveActor* pPlayer = al::tryFindNearestPlayerActor(this);
        if (pPlayer) {
            al::resetPosition(this, al::getTrans(pPlayer), false);
        }
        al::showModel(this);
        al::setNerve(this, &NrvTestAndoBowserJr.Wait);
    }
}
/** @brief Reads camera-relative movement and starts spin attacks. */
void TestAndoBowserJr::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
    sead::Matrix34f camera;
    camera.setInverse(al::getCameraViewMtx(this));
    mInput->update(camera);
    *al::getVelocityPtr(this) *= 0.95f;
    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    *pVelocity += mInput->getMoveVecNoArrange();
    if (mInput->isJumpButtonOn()) {
        *al::getVelocityPtr(this) += sead::Vector3f(0.0f, 1.0f, 0.0f);
    } else if (mInput->isDashButtonOn()) {
        *al::getVelocityPtr(this) += sead::Vector3f(0.0f, -1.0f, 0.0f);
    }
    if (al::getVelocity(this).length() > 0.5f) {
        sead::Vector3f direction = al::getVelocity(this);
        al::normalizeOrDirZ(&direction);
        al::turnToDirection(this, direction, 5.0f);
    }
    if (!isAssistPadConnected()) {
        al::hideModel(this);
        al::setNerve(this, &NrvTestAndoBowserJr.Hide);
    } else if (mInput->isFireBallTrigOn()) {
        al::setNerve(this, &NrvTestAndoBowserJr.AttackSpin);
    }
}
/** @brief Plays a spin attack, damping movement and checking controller loss. */
void TestAndoBowserJr::exeAttackSpin() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackSpin");
    }
    *al::getVelocityPtr(this) *= 0.95f;
    if (!isAssistPadConnected()) {
        al::hideModel(this);
        al::setNerve(this, &NrvTestAndoBowserJr.Hide);
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTestAndoBowserJr.Wait);
    }
}
/** @brief Leaves movement control to the active nerve. */
void TestAndoBowserJr::control() {}
