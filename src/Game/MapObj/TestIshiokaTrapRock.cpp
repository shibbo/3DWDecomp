#include "MapObj/TestIshiokaTrapRock.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
NERVE_DECL(TestIshiokaTrapRock, Wait)
NERVE_DECL(TestIshiokaTrapRock, Move)
NERVES_MAKE_STRUCT(TestIshiokaTrapRock, Wait, Move)
}

TestIshiokaTrapRock::TestIshiokaTrapRock(const char* name) : al::LiveActor(name) {}
void TestIshiokaTrapRock::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::invalidateClipping(this);
    if (al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &TestIshiokaTrapRock::start)))
        al::initNerve(this, &NrvTestIshiokaTrapRock.Wait, 0);
    else
        al::initNerve(this, &NrvTestIshiokaTrapRock.Move, 0);
    makeActorAppeared();
}
void TestIshiokaTrapRock::start() { al::setNerve(this, &NrvTestIshiokaTrapRock.Move); }
void TestIshiokaTrapRock::appear() { al::LiveActor::appear(); }
void TestIshiokaTrapRock::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    const sead::Vector3f& receiverPos = al::getSensorPos(receiver);
    sead::Vector3f dir = receiverPos - al::getSensorPos(sender);
    if (al::normalizeOrZero(&dir)) return;
    if (dir.dot(al::getVelocity(this)) < 1.0f) return;
    if (dir.dot(sead::Vector3f::ey) > 0.0f) return;
    al::sendMsgPressureDeath(receiver, sender);
}
void TestIshiokaTrapRock::exeWait() {}
void TestIshiokaTrapRock::exeMove() {
    if (rc::isInDeathArea(this)) { kill(); return; }
    const sead::Vector3f& trans = al::getTrans(this);
    sead::Vector3f pos = trans + al::getVelocity(this);
    int hits = alCollisionUtil::checkStrikeSphere(this, pos, 500.0f, nullptr, nullptr);
    if (hits != 0) {
        sead::Vector3f min(0, 0, 0), max(0, 0, 0);
        for (unsigned int i = 0; i < static_cast<unsigned int>(hits); ++i) {
            const auto* hit = alCollisionUtil::getStrikeSphereInfo(this, i);
            const sead::Vector3f* hitNormal = hit->mTriangle.getNormal(0);
            sead::Vector3f fix = hit->_70 * *hitNormal;
            if (min.x > fix.x) min.x = fix.x;
            if (min.y > fix.y) min.y = fix.y;
            if (min.z > fix.z) min.z = fix.z;
            if (max.x < fix.x) max.x = fix.x;
            if (max.y < fix.y) max.y = fix.y;
            if (max.z < fix.z) max.z = fix.z;
        }
        sead::Vector3f normal = max + min;
        pos += normal;
        al::normalizeOrZero(&normal);
        sead::Vector3f velocity = al::getVelocity(this);
        float dot = velocity.dot(normal);
        if (dot < 0.0f) *al::getVelocityPtr(this) = velocity - (dot * 1.25f) * normal;
        if (!al::isNearZero(normal, 0.001f)) {
            sead::Vector3f tangent;
            al::verticalizeVec(&tangent, normal, al::getVelocity(this));
            mAngularVelocity = normal.cross(tangent);
            if (!al::normalizeOrZero(&mAngularVelocity)) {
                float speed = tangent.length();
                float length = mAngularVelocity.length();
                if (length > 0.0f) mAngularVelocity *= (speed / 500.0f) / length;
            }
        }
    }
    al::getTransPtr(this)->set(pos);
    *al::getVelocityPtr(this) += sead::Vector3f(0, -0.2f, 0);
    if (al::getVelocity(this).length() > 30.0f) {
        sead::Vector3f* velocity = al::getVelocityPtr(this);
        float length = velocity->length();
        if (length > 0.0f) *velocity *= 30.0f / length;
    }
    sead::Quatf angular(0, mAngularVelocity.x, mAngularVelocity.y, mAngularVelocity.z);
    *al::getQuatPtr(this) += (angular * al::getQuat(this)) * 0.5f;
    float length = al::getQuat(this).length();
    sead::Quatf* quat = al::getQuatPtr(this);
    if (length < 0.001f) quat->makeUnit();
    else quat->normalize();
}
void TestIshiokaTrapRock::updateCollider() {}
