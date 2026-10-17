#include "MapObj/TestMatsudaFlashlight.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/RigidBodyCore.hpp"
#include "Util/RigidBodyUtil.hpp"
#include "Util/RigidBodyCollisionImpl.hpp"
namespace {
NERVE_DECL(TestMatsudaFlashlight, Fall);
NERVE_DECL(TestMatsudaFlashlight, Hold);
NERVES_MAKE_STRUCT(TestMatsudaFlashlight, Fall, Hold)
}
TestMatsudaFlashlight::TestMatsudaFlashlight(const char* name) : al::LiveActor(name) {}
TestMatsudaFlashlight::~TestMatsudaFlashlight() {}
void TestMatsudaFlashlight::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "TestMatsudaFlashlight", nullptr);
    al::initNerve(this, &NrvTestMatsudaFlashlight.Fall, 0);
    auto* spheres = RigidBodyUtil::createCollisionSphereEveryJoints(this);
    initHitSensor(spheres->size());
    for (int i = 0; i < spheres->size(); ++i) {
        float radius = sead::Mathf::max(20.0f, spheres->unsafeAt(i)->mRadius);
        auto* name = new sead::FixedSafeString<32>;
        name->format("Flashlight%02d", i);
        al::addHitSensorMapObj(this, info, name->cstr(), radius, 8, spheres->at(i)->mPos);
    }
    sead::Vector3f center;
    RigidBodyUtil::adjustCenterOfGravity(spheres->at(0), spheres->size(), &center);
    mRigidBody = RigidBodyUtil::createRigidBody(this, new RigidBodyCollisionImpl(nullptr),
                                              spheres->at(0), spheres->size(), center, 2.0f);
    mRigidBody->setMaxSpeed(10.0f);
    mRigidBody->setRepulsion(2.0f);
    mRigidBody->setRotDamping(0.995f);
    mRigidBody->setPosCorrection(1.05f);
    mRigidBody->setFriction(0.99f);
    mRigidBody->setGravity(sead::Vector3f::ey * -0.35f);
    mRigidBody->initPose(getBaseMtx());
    makeActorAppeared();
}
void TestMatsudaFlashlight::exeFall() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::validateClipping(this);
        al::killPrePassLight(this, "懐中電灯スポットライト", -1);
    }
    mRigidBody->update();
    sead::Matrix34f matrix;
    mRigidBody->calcPoseMtx(&matrix);
    al::updatePoseMtx(this, &matrix);
}
void TestMatsudaFlashlight::exeHold() {
    if (al::isFirstStep(this)) {
        al::offCollide(this);
        al::invalidateClipping(this);
        al::appearPrePassLight(this, "懐中電灯スポットライト", -1);
    }
    sead::Vector3f position;
    calcHoldPos(&position);
    sead::Matrix34f matrix;
    rc::calcPlayerHoldMtx(&matrix, mHolder);
    sead::Vector3f up(matrix(0, 1), matrix(1, 1), matrix(2, 1));
    sead::Vector3f front(rc::getPlayerFront(mHolder));
    if (al::isParallelDirection(front, up, 0.01f)) {
        sead::Vector3f side(matrix(0, 0), matrix(1, 0), matrix(2, 0));
        up = front.cross(side);
        al::normalizeOrZero(&up);
    }
    al::makeMtxUpFrontPos(&matrix, front, up, position);
    al::updatePoseMtx(this, &matrix);
    mRigidBody->initPose(getBaseMtx());
}
void TestMatsudaFlashlight::calcHoldPos(sead::Vector3f* position) {
    sead::Vector3f left;
    sead::Vector3f right;
    rc::calcPlayerModelJointPos(&left, mHolder, "HandL");
    rc::calcPlayerModelJointPos(&right, mHolder, "HandR");
    *position = (left + right) * 0.5f;
}
bool TestMatsudaFlashlight::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                                      al::HitSensor*) {
    if (al::isNerve(this, &NrvTestMatsudaFlashlight.Fall) && al::isMsgPlayerCarryFront(msg)) {
        mHolder = sender;
        al::setNerve(this, &NrvTestMatsudaFlashlight.Hold);
        return true;
    }
    if (al::isNerve(this, &NrvTestMatsudaFlashlight.Hold)) {
        if (al::isMsgPlayerRelease(msg)) {
            mRigidBody->initPose(getBaseMtx());
            sead::Vector3f force(rc::getPlayerFront(mHolder));
            force *= rc::getPlayerFront(mHolder).dot(rc::getPlayerVelocity(mHolder)) + 15.0f;
            force += sead::Vector3f(0.0f, 10.0f, 0.0f);
            sead::Vector3f position;
            calcHoldPos(&position);
            mRigidBody->requestPower(position + sead::Vector3f::zero - rc::getPlayerVelocity(mHolder), force);
        } else if (al::isMsgPlayerReleaseDead(msg)) {
            mRigidBody->initPose(getBaseMtx());
        } else {
            return false;
        }
        mHolder = nullptr;
        al::setNerve(this, &NrvTestMatsudaFlashlight.Fall);
        return true;
    }
    return false;
}
