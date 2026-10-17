#include "MapObj/TestBoxQuestionLongRot.hpp"

#include <math/seadQuatCalcCommon.h>
#include <prim/seadSafeString.h>

#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(TestBoxQuestionLongRot, Wait);
NERVE_DECL(TestBoxQuestionLongRot, BindWait);
NERVE_DECL(TestBoxQuestionLongRot, BindJump);
NERVE_DECL(TestBoxQuestionLongRot, BindMove);
NERVE_DECL(TestBoxQuestionLongRot, Kill);
NERVES_MAKE_NOSTRUCT(TestBoxQuestionLongRot, Wait, BindWait, BindJump, BindMove, Kill)

/// Bind sensors of the box, one per player slot.
const char* const cBindSensorNames[] = {"CenterBind", "RightBind", "LeftBind"};

/// Number of players that can carry the box.
constexpr s32 cBindMax = 3;

/// Frames a bind sensor stays disabled after its player let go.
constexpr s32 cInvalidSensorTime = 30;

/// Frames a jump input is remembered while waiting for the other players to jump too.
constexpr s32 cJumpInputTime = 20;

/// Highest rotation speed of the box (10 degrees per frame).
constexpr f32 cMaxAngularSpeed = 0.17453292f;

/**
 * @brief Place a puppet below the bind sensor holding it and stop it.
 * @param pPuppet The puppet.
 * @param pSensor The bind sensor holding the puppet.
 */
void setPuppetBindTrans(IUsePlayerPuppet* pPuppet, const al::HitSensor* pSensor) {
    sead::Vector3f offset(0.0f, -100.0f, 0.0f);
    rc::setPuppetTrans(pPuppet, al::getSensorPos(pSensor) + offset);
    rc::setPuppetVelocity(pPuppet, sead::Vector3f(0.0f, 0.0f, 0.0f));
}

/**
 * @brief Invert a 3x3 matrix, leaving the output untouched if it is singular.
 * @param pOut Receives the inverse.
 * @param rMtx The matrix.
 */
void invertMatrix(sead::Matrix33f* pOut, const sead::Matrix33f& rMtx) {
    f32 cofactor0 = rMtx.m[1][1] * rMtx.m[2][2] - rMtx.m[2][1] * rMtx.m[1][2];
    f32 cofactor1 = rMtx.m[1][2] * rMtx.m[2][0] - rMtx.m[2][2] * rMtx.m[1][0];
    f32 cofactor2 = rMtx.m[2][1] * rMtx.m[1][0] - rMtx.m[1][1] * rMtx.m[2][0];
    f32 det = rMtx.m[0][0] * cofactor0 + rMtx.m[0][1] * cofactor1 + rMtx.m[0][2] * cofactor2;
    if (det == 0.0f) {
        return;
    }

    f32 invDet = 1.0f / det;
    pOut->m[0][0] = cofactor0 * invDet;
    pOut->m[0][1] = (rMtx.m[2][1] * rMtx.m[0][2] - rMtx.m[2][2] * rMtx.m[0][1]) * invDet;
    pOut->m[0][2] = (rMtx.m[1][2] * rMtx.m[0][1] - rMtx.m[1][1] * rMtx.m[0][2]) * invDet;
    pOut->m[1][0] = cofactor1 * invDet;
    pOut->m[1][1] = (rMtx.m[0][0] * rMtx.m[2][2] - rMtx.m[2][0] * rMtx.m[0][2]) * invDet;
    pOut->m[1][2] = (rMtx.m[1][0] * rMtx.m[0][2] - rMtx.m[0][0] * rMtx.m[1][2]) * invDet;
    pOut->m[2][0] = cofactor2 * invDet;
    pOut->m[2][1] = (rMtx.m[0][1] * rMtx.m[2][0] - rMtx.m[0][0] * rMtx.m[2][1]) * invDet;
    pOut->m[2][2] = (rMtx.m[0][0] * rMtx.m[1][1] - rMtx.m[0][1] * rMtx.m[1][0]) * invDet;
}

/**
 * @brief Grow a pair of bounds to include a push vector.
 * @param pMin The lower bound.
 * @param pMax The upper bound.
 * @param rPush The push vector.
 */
void addPushBounds(sead::Vector3f* pMin, sead::Vector3f* pMax, const sead::Vector3f& rPush) {
    pMin->x = sead::Mathf::min(rPush.x, pMin->x);
    pMin->y = sead::Mathf::min(rPush.y, pMin->y);
    pMin->z = sead::Mathf::min(rPush.z, pMin->z);
    pMax->x = sead::Mathf::max(rPush.x, pMax->x);
    pMax->y = sead::Mathf::max(rPush.y, pMax->y);
    pMax->z = sead::Mathf::max(rPush.z, pMax->z);
}
}  // namespace

/**
 * @brief Construct the box.
 * @param pName Name of the actor.
 */
TestBoxQuestionLongRot::TestBoxQuestionLongRot(const char* pName) : al::LiveActor(pName) {
    for (s32 i = 0; i < cBindMax; i++) {
        mPuppets[i] = nullptr;
        mIsFirstBind[i] = false;
        mInvalidSensorTimer[i] = 0;
        mJumpTimer[i] = 0;
        mIsOnGround[i] = false;
    }

    mAngularVelocity = {0.0f, 0.0f, 0.0f};
    mCenterOffset = {0.0f, 0.0f, 0.0f};
    mIsCollided = false;
}

/**
 * @brief Initialize the box.
 * @param rInfo Placement info.
 */
void TestBoxQuestionLongRot::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TestBoxQuestionLong", nullptr);
    al::initNerve(this, &NrvTestBoxQuestionLongRotWait, 0);
    makeActorAppeared();
}

/**
 * @brief Handle the bind messages of the players grabbing the box.
 * @param pMsg The message.
 * @param pOther The sensor of the sender.
 * @param pSelf The bind sensor of the box receiving the message.
 * @return Whether the message was handled.
 */
bool TestBoxQuestionLongRot::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                        al::HitSensor* pSelf) {
    if (al::isMsgBindStart(pMsg)) {
        if (al::isNerve(this, &NrvTestBoxQuestionLongRotWait) ||
            al::isNerve(this, &NrvTestBoxQuestionLongRotBindWait) ||
            al::isNerve(this, &NrvTestBoxQuestionLongRotBindJump)) {
            return mPuppets[findBindIndex(pSelf)] == nullptr;
        }

        return false;
    }

    if (al::isMsgBindInit(pMsg)) {
        u32 index = findBindIndex(pSelf);
        IUsePlayerPuppet* puppet = rc::startPuppet(pSelf, pOther);
        mPuppets[index] = puppet;
        rc::startPuppetAction(puppet, "RouteDokanBazookaFly");
        rc::validatePlayerMash(rc::getPuppetSensor(puppet));

        sead::Quatf quat;
        al::calcQuat(&quat, al::getSensorHost(pSelf));
        rc::setPuppetQuat(puppet, quat);
        setPuppetBindTrans(puppet, pSelf);

        mBindNum++;
        mIsFirstBind[index] = true;
        if (al::isNerve(this, &NrvTestBoxQuestionLongRotWait)) {
            al::setNerve(this, &NrvTestBoxQuestionLongRotBindWait);
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        u32 index = findBindIndex(pSelf);
        rc::invalidatePlayerMash(rc::getPuppetSensor(mPuppets[index]));
        mPuppets[index] = nullptr;
        mBindNum--;
        if (mBindNum != 0) {
            initRigidBody();
        }

        return true;
    }

    return false;
}

/**
 * @brief Find the player slot of a bind sensor.
 * @param pSensor The bind sensor.
 * @return The slot index.
 */
u32 TestBoxQuestionLongRot::findBindIndex(const al::HitSensor* pSensor) const {
    for (u32 i = 0; i < cBindMax; i++) {
        if (al::isSensorName(pSensor, cBindSensorNames[i])) {
            return i;
        }
    }

    return 0;
}

/**
 * @brief Reset the rigid body for the current set of players: center of mass, inertia and the
 * offset of the actor from the center of mass.
 */
void TestBoxQuestionLongRot::initRigidBody() {
    mAngularVelocity = {0.0f, 0.0f, 0.0f};
    mVelocity = {0.0f, 0.0f, 0.0f};
    al::calcQuat(&mQuat, this);

    s32 firstIndex = 0;
    for (; firstIndex < cBindMax; firstIndex++) {
        if (mPuppets[firstIndex] != nullptr) {
            break;
        }
    }

    s32 lastIndex = -1;
    f32 mass = 0.0f;
    for (s32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] != nullptr) {
            mass += 40.0f;
            lastIndex = i;
        }
    }

    f32 inertia = mass / 12.0f;
    f32 span = lastIndex - firstIndex;
    f32 inertiaSide = inertia * (span * span + 1.0f);
    mInertia = sead::Matrix33f(inertia + inertia, 0.0f, 0.0f, 0.0f, inertiaSide, 0.0f, 0.0f, 0.0f,
                               inertiaSide);

    mCenter = {0.0f, 0.0f, 0.0f};
    u32 bindNum = 0;
    for (s32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] != nullptr) {
            mCenter += al::getSensorPos(al::getHitSensor(this, cBindSensorNames[i]));
            bindNum++;
        }
    }

    mCenter *= 1.0f / bindNum;

    sead::Vector3f offset = mCenter - al::getTrans(this);
    sead::Vector3f up;
    sead::Vector3f front;
    sead::Vector3f side;
    al::calcUpDir(&up, this);
    al::calcFrontDir(&front, this);
    al::calcSideDir(&side, this);
    mCenterOffset.set(offset.dot(side), offset.dot(up), offset.dot(front));
}

/**
 * @brief Nobody holds the box.
 */
void TestBoxQuestionLongRot::exeWait() {}

/**
 * @brief Players hold the box and wait for the others to grab it too.
 */
void TestBoxQuestionLongRot::exeBindWait() {
    if (al::isFirstStep(this)) {
        mCoinTrans.e = al::getTrans(this).e;
    }

    for (u32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] == nullptr) {
            continue;
        }

        if (mIsFirstBind[i]) {
            rc::validateSubActionPuppet(mPuppets[i]);
            rc::invalidateUpperSubActionPuppet(mPuppets[i]);
            mIsFirstBind[i] = false;
        }

        setPuppetBindTrans(mPuppets[i], al::getHitSensor(this, cBindSensorNames[i]));
        if (rc::isPuppetHoldSquatButton(mPuppets[i])) {
            releasePuppet(i);
        }
    }

    if (isAllBinded()) {
        initRigidBody();
        al::setNerve(this, &NrvTestBoxQuestionLongRotBindJump);
    } else if (isNoBinded()) {
        al::setNerve(this, &NrvTestBoxQuestionLongRotWait);
    } else if (al::isGreaterEqualStep(this, 300)) {
        initRigidBody();
        al::setNerve(this, &NrvTestBoxQuestionLongRotBindMove);
    }
}

/**
 * @brief Let a player drop off the box.
 * @param index The player slot.
 */
inline void TestBoxQuestionLongRot::releasePuppet(u32 index) {
    rc::invalidatePlayerMash(rc::getPuppetSensor(mPuppets[index]));
    rc::endBindAndPuppetNull(&mPuppets[index], nullptr);
    invalidateBindSensor(index);
    mBindNum--;
}

/**
 * @brief Disable a bind sensor for a while so its player isn't grabbed again right away.
 * @param index The player slot.
 */
void TestBoxQuestionLongRot::invalidateBindSensor(u32 index) {
    mInvalidSensorTimer[index] = cInvalidSensorTime;
    al::invalidateHitSensor(this, cBindSensorNames[index]);
}

/**
 * @brief Check whether nobody holds the box.
 * @return Whether every player slot is empty.
 */
bool TestBoxQuestionLongRot::isNoBinded() const {
    for (s32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] != nullptr) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Check whether every player slot is taken.
 * @return Whether three players hold the box.
 */
bool TestBoxQuestionLongRot::isAllBinded() const {
    for (s32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] == nullptr) {
            return false;
        }
    }

    return true;
}

/**
 * @brief The players carry the box around.
 */
void TestBoxQuestionLongRot::exeBindMove() {
    if (checkBindEnd()) {
        return;
    }

    if (al::isFirstStep(this)) {
        resetJumpPow();
    }

    updateRigidBody();
    applyPose();
    applyCollision();
    updateCoin();

    u32 jumpNum = calcJumpPow();
    if (jumpNum != 0) {
        if (mVelocity.dot(sead::Vector3f::ey) < 0.0f) {
            al::verticalizeVec(&mVelocity, sead::Vector3f::ey, mVelocity);
        }

        f32 jumpSpeed = jumpNum * (0.25f - 0.2f) / 3.0f + 0.2f;
        mVelocity += sead::Vector3f(0.0f, jumpSpeed, 0.0f);
        al::setNerve(this, &NrvTestBoxQuestionLongRotBindJump);
    }
}

/**
 * @brief Go away once every player let go of the box.
 * @return Whether nobody holds the box anymore.
 */
bool TestBoxQuestionLongRot::checkBindEnd() {
    if (!isNoBinded()) {
        return false;
    }

    al::setNerve(this, &NrvTestBoxQuestionLongRotKill);
    return true;
}

/**
 * @brief Forget the remembered jump inputs.
 */
void TestBoxQuestionLongRot::resetJumpPow() {
    for (s32 i = 0; i < cBindMax; i++) {
        mJumpTimer[i] = 0;
    }
}

/**
 * @brief Push the box with the players' sticks and integrate the rigid body for one frame.
 */
void TestBoxQuestionLongRot::updateRigidBody() {
    sead::Vector3f totalForce = {0.0f, 0.0f, 0.0f};
    sead::Vector3f torque = {0.0f, 0.0f, 0.0f};
    bool isMoving = false;

    for (s32 i = 0; i < cBindMax; i++) {
        IUsePlayerPuppet* puppet = mPuppets[i];
        if (puppet == nullptr) {
            continue;
        }

        const char* sensorName = cBindSensorNames[i];
        setPuppetBindTrans(puppet, al::getHitSensor(this, sensorName));

        sead::Vector3f force;
        if (rc::isPuppetStickOn(mPuppets[i])) {
            if (!rc::isPuppetAction(mPuppets[i], "Move")) {
                rc::startPuppetAction(mPuppets[i], "Move");
                rc::setPuppetBlendAnimWeight(mPuppets[i], 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                rc::setPuppetActionRate(mPuppets[i], 3.0f);
            }

            sead::Vector3f stick = rc::getPuppetStickWorldWithSnap(mPuppets[i]);
            sead::Vector3f front = stick;
            al::normalizeOrZero(&front);
            rc::setPuppetFrontVec(mPuppets[i], front);
            force = stick * 0.005f;
            isMoving = true;
        } else {
            if (!rc::isPuppetAction(mPuppets[i], "Wait")) {
                rc::startPuppetAction(mPuppets[i], "Wait");
            }

            if (mIsOnGround[i]) {
                force.set(mVelocity.x * -0.1f, 0.0f, mVelocity.z * -0.1f);
            } else {
                force.set(0.0f, 0.0f, 0.0f);
            }
        }

        totalForce += force;

        const sead::Vector3f& sensorPos = al::getSensorPos(al::getHitSensor(this, sensorName));
        sead::Vector3f arm(sensorPos.x - mCenter.x, 0.0f, sensorPos.z - mCenter.z);
        arm *= 0.01f;
        torque += arm.cross(force);
    }

    mVelocity -= sead::Vector3f::ey * 0.005f;

    if (mIsCollided) {
        if (isMoving) {
            mVelocity += totalForce;

            sead::Matrix33f rotMtx;
            rotMtx.fromQuat(mQuat);
            sead::Matrix33f invRotMtx;
            invertMatrix(&invRotMtx, rotMtx);
            sead::Matrix33f inertia;
            inertia.setMul(rotMtx, mInertia);
            inertia.setMul(inertia, invRotMtx);
            sead::Matrix33f invInertia;
            invertMatrix(&invInertia, inertia);

            sead::Vector3f angularAccel = invInertia * torque;
            if (angularAccel.dot(mAngularVelocity) < 0.0f) {
                mAngularVelocity *= 0.8f;
            }

            mAngularVelocity += angularAccel;
            if (mAngularVelocity.length() > cMaxAngularSpeed) {
                al::normalizeOrZero(&mAngularVelocity);
                mAngularVelocity *= cMaxAngularSpeed;
            }
        } else {
            mVelocity = {0.0f, 0.0f, 0.0f};
            mAngularVelocity = {0.0f, 0.0f, 0.0f};
        }
    }

    integrate();
}

/**
 * @brief Move the center of mass and rotate the box by its velocities.
 */
inline void TestBoxQuestionLongRot::integrate() {
    constrainVelocity();
    mCenter += mVelocity * 100.0f;
    sead::QuatCalcCommon<f32>::applyAngularVelocity(mQuat, mAngularVelocity, 1.0f);
}

/**
 * @brief Place the actor according to the rigid body.
 */
void TestBoxQuestionLongRot::applyPose() {
    sead::Vector3f trans;
    trans.setRotated(mQuat, -mCenterOffset);
    trans += mCenter;
    al::setTrans(this, trans);
    al::updatePoseQuat(this, mQuat);
}

/**
 * @brief Push the box out of the ground and walls and check which players stand on ground.
 */
void TestBoxQuestionLongRot::applyCollision() {
    sead::Matrix33f rotMtx;
    rotMtx.fromQuat(mQuat);
    sead::Vector3f side = rotMtx.getBase(0);
    sead::Vector3f up = rotMtx.getBase(1) * 50.0f;
    mIsCollided = false;

    sead::Vector3f minPush = {0.0f, 0.0f, 0.0f};
    sead::Vector3f maxPush = {0.0f, 0.0f, 0.0f};
    for (s32 i = 0; i < cBindMax; i++) {
        sead::Vector3f pos = al::getTrans(this) + side * (i * 100.0f - 100.0f) + up;
        mIsOnGround[i] = false;

        u32 hitNum = alCollisionUtil::checkStrikeSphere(this, pos, 50.0f, nullptr, nullptr);
        for (u32 j = 0; j < hitNum; j++) {
            const al::SphereHitInfo* hitInfo = alCollisionUtil::getStrikeSphereInfo(this, j);
            addPushBounds(&minPush, &maxPush, *hitInfo->mTriangle.getNormal(0) * hitInfo->_70);
            if (mVelocity.dot(*hitInfo->mTriangle.getNormal(0)) < 0.0f) {
                al::verticalizeVec(&mVelocity, *hitInfo->mTriangle.getNormal(0), mVelocity);
            }
        }

        if (mPuppets[i] == nullptr) {
            continue;
        }

        const al::ArrowHitInfo* arrowInfo;
        if (!alCollisionUtil::getFirstPolyOnArrow(this, &arrowInfo, pos,
                                                  sead::Vector3f::ey * -100.0f, nullptr, nullptr)) {
            continue;
        }

        if (arrowInfo->mTriangle.getNormal(0)->dot(sead::Vector3f::ey) < 0.70710677f) {
            continue;
        }

        addPushBounds(&minPush, &maxPush, sead::Vector3f::ey * (100.0f - arrowInfo->_70));
        if (mVelocity.dot(sead::Vector3f::ey) < 0.0f) {
            al::verticalizeVec(&mVelocity, sead::Vector3f::ey, mVelocity);
        }

        mIsCollided = true;
        mIsOnGround[i] = true;
    }

    sead::Vector3f push = maxPush + minPush;
    mCenter += push;
    *al::getTransPtr(this) += push;
}

/**
 * @brief Drop a coin for every player each time the box traveled far enough.
 */
void TestBoxQuestionLongRot::updateCoin() {
    if ((mCoinTrans - al::getTrans(this)).length() < 200.0f) {
        return;
    }

    mCoinTrans.e = al::getTrans(this).e;
    for (s32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] == nullptr) {
            continue;
        }

        al::StringTmp<128> timingName("Puppet%d", i);
        al::appearItemTiming(this, timingName.cstr(),
                             al::getSensorPos(al::getHitSensor(this, cBindSensorNames[i])) +
                                 sead::Vector3f::ey * 100.0f,
                             sead::Vector3f::ey);
    }
}

/**
 * @brief Count the players jumping. A jump input is remembered for a few frames so the players
 * don't have to press the button on the very same frame.
 * @return The number of jumping players if every player jumps (or a remembered input runs out),
 * 0 otherwise.
 */
u32 TestBoxQuestionLongRot::calcJumpPow() {
    u32 bindNum = 0;
    u32 jumpNum = 0;
    bool isTimeOut = false;
    for (s32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] == nullptr) {
            continue;
        }

        bindNum++;
        if (rc::isPuppetTrigJumpButton(mPuppets[i])) {
            mJumpTimer[i] = cJumpInputTime;
            jumpNum++;
        } else if (mJumpTimer[i] != 0) {
            mJumpTimer[i]--;
            jumpNum++;
            if (mJumpTimer[i] == 0) {
                isTimeOut = true;
            }
        }
    }

    if (bindNum == jumpNum || isTimeOut) {
        return jumpNum;
    }

    return 0;
}

/**
 * @brief The box flies through the air after the players jumped.
 */
void TestBoxQuestionLongRot::exeBindJump() {
    bool isBindChanged = false;
    for (u32 i = 0; i < cBindMax; i++) {
        if (mPuppets[i] == nullptr) {
            continue;
        }

        if (mIsFirstBind[i]) {
            rc::validateSubActionPuppet(mPuppets[i]);
            rc::invalidateUpperSubActionPuppet(mPuppets[i]);
            mIsFirstBind[i] = false;
            isBindChanged = true;
        } else if (rc::isPuppetHoldSquatButton(mPuppets[i])) {
            releasePuppet(i);
            isBindChanged = true;
        }
    }

    if (checkBindEnd()) {
        return;
    }

    if (isBindChanged) {
        initRigidBody();
    }

    for (s32 i = 0; i < cBindMax; i++) {
        IUsePlayerPuppet* puppet = mPuppets[i];
        if (puppet == nullptr) {
            continue;
        }

        setPuppetBindTrans(puppet, al::getHitSensor(this, cBindSensorNames[i]));
        if (!rc::isPuppetAction(mPuppets[i], "Jump")) {
            rc::startPuppetAction(mPuppets[i], "Jump");
        }
    }

    mVelocity -= sead::Vector3f::ey * 0.005f;
    integrate();
    applyPose();

    f32 fallSpeed = mVelocity.dot(sead::Vector3f::ey);
    applyCollision();
    updateCoin();
    if (fallSpeed < 0.0f && mIsCollided) {
        al::setNerve(this, &NrvTestBoxQuestionLongRotBindMove);
    }
}

/**
 * @brief Limit the horizontal speed and the falling speed of the box.
 */
void TestBoxQuestionLongRot::constrainVelocity() {
    sead::Vector3f horizontal;
    al::verticalizeVec(&horizontal, sead::Vector3f::ey, mVelocity);
    if (horizontal.length() > 0.1f) {
        mVelocity -= horizontal;
        al::normalizeOrZero(&horizontal);
        horizontal *= 0.1f;
        mVelocity += horizontal;
    }

    sead::Vector3f vertical;
    al::parallelizeVec(&vertical, sead::Vector3f::ey, mVelocity);
    if (vertical.dot(sead::Vector3f::ey) < 0.0f && vertical.length() > 0.2f) {
        mVelocity -= vertical;
        al::normalizeOrZero(&vertical);
        vertical *= 0.2f;
        mVelocity += vertical;
    }
}

/**
 * @brief Disappear.
 */
void TestBoxQuestionLongRot::exeKill() {
    kill();
}

/**
 * @brief Re-enable the bind sensors once their cooldown ran out.
 */
void TestBoxQuestionLongRot::control() {
    for (s32 i = 0; i < cBindMax; i++) {
        if (mInvalidSensorTimer[i] != 0) {
            mInvalidSensorTimer[i]--;
            if (mInvalidSensorTimer[i] == 0) {
                al::validateHitSensor(this, cBindSensorNames[i]);
            }
        }
    }
}

/**
 * @brief Move the puppets with the box and let them collide.
 * @return Whether any of the players stands on the ground.
 */
bool TestBoxQuestionLongRot::updatePuppetCollider() {
    bool isOnFloor = false;
    for (s32 i = 0; i < cBindMax; i++) {
        IUsePlayerPuppet* puppet = mPuppets[i];
        if (puppet == nullptr) {
            continue;
        }

        setPuppetBindTrans(puppet, al::getHitSensor(this, cBindSensorNames[i]));
        rc::solveAirPuppet(mPuppets[i]);
        isOnFloor |= rc::isOnFloorPuppet(mPuppets[i]);
    }

    return isOnFloor;
}

/**
 * @brief Check whether every player pressed jump this frame.
 * @return Whether all three players jumped.
 */
bool TestBoxQuestionLongRot::isAllJump() const {
    return rc::isPuppetTrigJumpButton(mPuppets[0]) && rc::isPuppetTrigJumpButton(mPuppets[1]) &&
           rc::isPuppetTrigJumpButton(mPuppets[2]);
}
