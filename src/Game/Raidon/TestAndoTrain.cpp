#include "Raidon/TestAndoTrain.hpp"

#include <math/seadQuat.h>
#include <prim/seadSafeString.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/HitSensor/HitSensor.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Obj/PartsModel.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Rail/Rail.hpp"
#include "Library/Rail/RailKeeper.hpp"
#include "Library/Rail/RailRider.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"

namespace {
NERVE_DECL(TestAndoTrain, Wait)
NERVE_DECL(TestAndoTrain, Move)
NERVES_MAKE_NOSTRUCT(TestAndoTrain, Wait, Move)

/// Number of frames a carriage hop takes to reach the next carriage.
constexpr s32 cJumpPropagateFrame = 15;

/// Vertical speed a carriage hops with.
constexpr f32 cJumpSpeed = 20.0f;

/// Gravity applied to a hopping carriage every frame.
constexpr f32 cJumpGravity = 0.5f;

/// Half the length of a carriage, from its centre to its couplings.
constexpr f32 cCarHalfLength = 150.0f;

/// Gap between the couplings of two neighbouring carriages.
constexpr f32 cCouplingLength = 40.0f;

/// Full length of a carriage.
constexpr f32 cCarLength = 300.0f;

/// Maximum speed along the rail.
constexpr f32 cSpeedMax = 50.0f;

/**
 * @brief Rotates the actor so that its front faces the given direction.
 * @param pActor Actor to rotate.
 * @param rFront Direction the actor should face.
 */
inline void setQuatFront(al::LiveActor* pActor, const sead::Vector3f& rFront) {
    sead::Quatf quat;

    if (!quat.makeVectorRotation(sead::Vector3f(0.0f, 0.0f, 1.0f), rFront)) {
        quat = sead::Quatf(-4.371139e-08f, 0.0f, 1.0f, 0.0f);
    }

    al::setQuat(pActor, quat);
}

/**
 * @brief Makes the matrix the puppets are bound with, which sits above the given carriage.
 * @param pOut Puppet matrix.
 * @param rCarMtx Carriage matrix.
 */
inline void calcPuppetMtx(sead::Matrix34f* pOut, const sead::Matrix34f& rCarMtx) {
    const sead::Matrix34f offsetMtx(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 100.0f, 0.0f, 0.0f,
                                    1.0f, 0.0f);
    *pOut = rCarMtx * offsetMtx;
}

/**
 * @brief Scales a vector to the given length, leaving a zero vector unchanged.
 * @param pVec Vector to scale.
 * @param length Length to scale to.
 */
inline void setLength(sead::Vector3f* pVec, f32 length) {
    f32 len = pVec->length();

    if (len > 0.0f) {
        *pVec *= length / len;
    }
}
}  // namespace

/** @param pName Actor name. */
TestAndoTrain::TestAndoTrain(const char* pName) : al::LiveActor(pName) {
    const sead::Matrix34f identity(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                   0.0f);

    mRailNum = 0;
    mRailIndex = 0;
    mSpeed = 0.0f;
    mDriveState = DriveState::None;

    for (s32 i = 0; i < cCarNum; i++) {
        mCarMtx[i] = identity;
        mCars[i] = nullptr;
        mPuppets[i] = nullptr;
        mPuppetMtx[i] = identity;
        mJumpFrame[i] = 0;
        mJumpHeight[i] = 0.0f;
        mJumpSpeed[i] = 0.0f;
    }

    mEffectTrans = {0.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates the rails and the carriages and puts the train on the nearest rail.
 * @param rInfo Actor init info.
 */
void TestAndoTrain::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTestAndoTrainWait, 0);
    mRailNum = al::calcLinkChildNum(rInfo, "Rail");

    for (s32 i = 0; i < mRailNum; i++) {
        al::PlacementInfo railInfo;
        al::getLinksInfoByIndex(&railInfo, al::getPlacementInfo(rInfo), "Rail", i);
        mRailKeepers[i] = new al::RailKeeper(railInfo);
    }

    syncNearestRailPos();
    makeActorAppeared();

    sead::Matrix34f mtx;
    al::makeMtxRT(&mtx, this);
    mCarMtx[0] = mtx;
    mCars[0] = this;
    calcPuppetMtx(&mPuppetMtx[0], mCarMtx[0]);
    mEffectTrans.set(al::getTrans(this));

    for (s32 i = 1; i < cCarNum; i++) {
        f32 offset = -(cCarHalfLength * 2 + cCouplingLength) * i;
        mCarMtx[i] = mtx;
        mCarMtx[i].setTranslation(mtx.getBase(2) * offset + mtx.getTranslation());
        calcPuppetMtx(&mPuppetMtx[i], mCarMtx[i]);
        mCars[i] = al::createPartsModel(this, rInfo, "客車", "TestAndoCarriage", &mCarMtx[i]);
        al::setHitSensorMtxPtr(this, al::StringTmp<128>("Car%d", i).cstr(), &mCarMtx[i]);
    }
}

/** @brief Moves the train onto the nearest point of the closest rail. */
void TestAndoTrain::syncNearestRailPos() {
    sead::Vector3f trans = al::getTrans(this);
    f32 minDistance = sead::Mathf::maxNumber();

    for (s32 i = 0; i < mRailNum; i++) {
        sead::Vector3f railPos;
        mRailKeepers[i]->getRail()->calcNearestRailPos(&railPos, trans, 100.0f);
        f32 distance = (trans - railPos).length();

        if (distance < minDistance) {
            minDistance = distance;
            mRailIndex = i;
        }
    }

    getRailRider()->moveToNearestRail(al::getTrans(this));
    al::setTrans(this, getRailRider()->getPosition());
    setQuatFront(this, getRailRider()->getDirection());
}

/**
 * @brief Does nothing.
 * @param pSelf Own sensor.
 * @param pOther Other sensor.
 */
void TestAndoTrain::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {}

/**
 * @brief Lets players get on and off the carriages.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Own carriage sensor ("Car0" to "Car3").
 * @return Whether the message was handled.
 */
bool TestAndoTrain::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                               al::HitSensor* pSelf) {
    const char* sensorName = pSelf->getName();

    if (al::isMsgBindStart(pMsg)) {
        for (s32 i = 0; i < cCarNum; i++) {
            if (al::isEqualString(sensorName, al::StringTmp<128>("Car%d", i).cstr()) &&
                mPuppets[i] == nullptr) {
                return true;
            }
        }

        return false;
    }

    if (al::isMsgBindInit(pMsg)) {
        s32 index = sensorName[3] - '0';
        mPuppets[index] = rc::startPuppet(pSelf, pOther);
        rc::startPuppetAction(mPuppets[index], "Land");
        rc::setPuppetMtx(mPuppets[index], &mPuppetMtx[index]);

        if (al::isNerve(this, &NrvTestAndoTrainWait)) {
            al::setNerve(this, &NrvTestAndoTrainMove);
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        mPuppets[sensorName[3] - '0'] = nullptr;
        return true;
    }

    return false;
}

/** @brief Waits for a player to get on, puffing a little smoke. */
void TestAndoTrain::exeWait() {
    if (al::isFirstStep(this)) {
        al::emitEffect(this, "SmokeLittle", nullptr);
    }
}

/** @brief Drives along the rail controlled by the first player on board. */
void TestAndoTrain::exeMove() {
    if (al::isFirstStep(this)) {
        al::invalidateClipping(this);
    }

    s32 playerIndex = findControlPlayerIndex();
    updateRailRider(playerIndex);
    updateCarriage(playerIndex);
    updatePuppet();
    updateEffect();
}

/**
 * @brief Finds the player that controls the train.
 * @return Index of the first occupied carriage, or -1 if nobody is on board.
 */
s32 TestAndoTrain::findControlPlayerIndex() const {
    for (s32 i = 0; i < cCarNum; i++) {
        if (mPuppets[i] != nullptr) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Accelerates or brakes from the stick input and moves the locomotive along the rail.
 * @param playerIndex Index of the controlling player, or -1 if nobody is on board.
 */
void TestAndoTrain::updateRailRider(s32 playerIndex) {
    if (playerIndex >= 0) {
        IUsePlayerPuppet* puppet = mPuppets[playerIndex];

        switch (mDriveState) {
        case DriveState::None:
            if (rc::isPuppetStickOn(puppet)) {
                const sead::Vector3f& stick = rc::getPuppetStickWorldWithoutSnap(puppet);
                f32 dot = stick.dot(getRailRider()->getDirection());
                mDriveState = dot < 0.0f ? DriveState::Brake : DriveState::Accel;
            }

            break;
        case DriveState::Accel:
            if (rc::isPuppetStickOn(puppet)) {
                mSpeed += 0.1f;
            } else {
                mDriveState = DriveState::None;
            }

            break;
        case DriveState::Brake:
            if (rc::isPuppetStickOn(puppet)) {
                mSpeed -= 0.5f;
            } else {
                mDriveState = DriveState::None;
            }

            break;
        }
    } else {
        mSpeed *= 0.5f;
    }

    mSpeed = sead::Mathf::clamp(mSpeed, 0.0f, cSpeedMax);
    getRailRider()->setSpeed(mSpeed);
    getRailRider()->move();
    al::setTrans(this, getRailRider()->getPosition() + sead::Vector3f(0.0f, mJumpHeight[0], 0.0f));
    setQuatFront(this, getRailRider()->getDirection());
}

/**
 * @brief Updates the carriage hops and lines the carriages up behind the locomotive.
 * @param playerIndex Index of the controlling player, or -1 if nobody is on board.
 */
void TestAndoTrain::updateCarriage(s32 playerIndex) {
    if (mJumpHeight[0] != 0.0f || mJumpHeight[1] != 0.0f || mJumpHeight[2] != 0.0f ||
        mJumpHeight[3] != 0.0f) {
        for (s32 i = 0; i < cCarNum; i++) {
            if (mJumpFrame[i] > 0 && mJumpFrame[i] < cJumpPropagateFrame) {
                mJumpFrame[i]++;
            }
        }

        for (s32 i = 0; i < cCarNum; i++) {
            if (mJumpFrame[i] != 0) {
                continue;
            }

            if ((i > 0 && mJumpFrame[i - 1] == cJumpPropagateFrame) ||
                (i < cCarNum - 1 && mJumpFrame[i + 1] == cJumpPropagateFrame)) {
                mJumpFrame[i] = 1;
                mJumpSpeed[i] = cJumpSpeed;
            }
        }
    } else {
        for (s32 i = 0; i < cCarNum; i++) {
            mJumpFrame[i] = 0;
        }

        if (playerIndex >= 0 && rc::isPuppetTrigJumpButton(mPuppets[playerIndex])) {
            mJumpFrame[playerIndex]++;
            mJumpSpeed[playerIndex] = cJumpSpeed;
        }
    }

    for (s32 i = 0; i < cCarNum; i++) {
        mJumpSpeed[i] -= cJumpGravity;
        mJumpHeight[i] += mJumpSpeed[i];

        if (mJumpHeight[i] < 0.0f) {
            mJumpSpeed[i] = 0.0f;
            mJumpHeight[i] = 0.0f;
        }
    }

    al::makeMtxRT(&mCarMtx[0], this);

    for (s32 i = 0; i < cCarNum - 1; i++) {
        const sead::Matrix34f& prevMtx = mCarMtx[i];
        sead::Matrix34f& mtx = mCarMtx[i + 1];

        sead::Vector3f prevFront(prevMtx.m[0][2], prevMtx.m[1][2], prevMtx.m[2][2]);
        sead::Vector3f prevBack(prevMtx.m[0][3] - prevFront.x * cCarHalfLength,
                                prevMtx.m[1][3] - prevFront.y * cCarHalfLength,
                                prevMtx.m[2][3] - prevFront.z * cCarHalfLength);
        sead::Vector3f front(mtx.m[0][2], mtx.m[1][2], mtx.m[2][2]);
        sead::Vector3f trans(mtx.m[0][3], mtx.m[1][3], mtx.m[2][3]);
        sead::Vector3f frontPos = trans + front * cCarHalfLength;
        sead::Vector3f backPos = trans - front * cCarHalfLength;

        mRailKeepers[mRailIndex]->getRail()->calcNearestRailPos(&frontPos, frontPos, cSpeedMax);
        mRailKeepers[mRailIndex]->getRail()->calcNearestRailPos(&backPos, backPos, cSpeedMax);
        frontPos += sead::Vector3f(0.0f, mJumpHeight[i + 1], 0.0f);
        backPos += sead::Vector3f(0.0f, mJumpHeight[i + 1], 0.0f);

        sead::Vector3f coupling = frontPos - prevBack;

        if (al::isNearZero(coupling)) {
            frontPos = prevBack - prevFront * cCouplingLength;
        } else {
            setLength(&coupling, cCouplingLength);
            frontPos = prevBack + coupling;
        }

        sead::Vector3f body = backPos - frontPos;

        if (al::isNearZero(body)) {
            backPos = frontPos - front * cCarLength;
        } else {
            setLength(&body, cCarLength);
            backPos = frontPos + body;
        }

        sead::Vector3f center = (frontPos + backPos) * 0.5f;
        sead::Vector3f dir = frontPos - backPos;
        al::normalize(&dir);

        if (dir.y > 0.99f) {
            sead::Quatf quat;
            quat.makeVectorRotation(front, dir);
            sead::Matrix34f rotateMtx;
            rotateMtx.makeQT(quat, sead::Vector3f(0.0f, 0.0f, 0.0f));
            mtx = rotateMtx * mtx;
            mtx.m[0][3] = center.x;
            mtx.m[1][3] = center.y;
            mtx.m[2][3] = center.z;
            al::normalize(&mtx);
        } else {
            sead::Vector3f side;
            side.setCross(sead::Vector3f::ey, dir);
            al::normalize(&side);
            sead::Vector3f up;
            up.setCross(dir, side);
            mtx.setBase(0, side);
            mtx.setBase(1, up);
            mtx.setBase(2, dir);
            mtx.setTranslation(center);
        }
    }
}

/** @brief Lands the riders and moves them along with their carriages. */
void TestAndoTrain::updatePuppet() {
    for (s32 i = 0; i < cCarNum; i++) {
        if (mPuppets[i] == nullptr) {
            continue;
        }

        if (rc::isPuppetAction(mPuppets[i], "Land") && rc::isPuppetActionEnd(mPuppets[i])) {
            rc::startPuppetAction(mPuppets[i], "TestAndoTrainRide");
        }

        calcPuppetMtx(&mPuppetMtx[i], mCarMtx[i]);
        al::normalize(&mPuppetMtx[i]);
        rc::setPuppetMtx(mPuppets[i], &mPuppetMtx[i]);
    }
}

/** @brief Switches the smoke effect by the current speed. */
void TestAndoTrain::updateEffect() {
    mEffectTrans.set(al::getTrans(this));

    if (mSpeed > 30.0f) {
        if (al::isEffectEmitting(this, "SmokeNormal")) {
            al::deleteEffect(this, "SmokeNormal");
        }

        if (al::isEffectEmitting(this, "SmokeLittle")) {
            al::deleteEffect(this, "SmokeLittle");
        }

        if (!al::isEffectEmitting(this, "SmokeMany")) {
            al::emitEffect(this, "SmokeMany", nullptr);
        }
    } else if (mSpeed > 2.0f) {
        if (al::isEffectEmitting(this, "SmokeMany")) {
            al::deleteEffect(this, "SmokeMany");
        }

        if (al::isEffectEmitting(this, "SmokeLittle")) {
            al::deleteEffect(this, "SmokeLittle");
        }

        if (!al::isEffectEmitting(this, "SmokeNormal")) {
            al::emitEffect(this, "SmokeNormal", nullptr);
        }
    } else {
        if (al::isEffectEmitting(this, "SmokeMany")) {
            al::deleteEffect(this, "SmokeMany");
        }

        if (al::isEffectEmitting(this, "SmokeNormal")) {
            al::deleteEffect(this, "SmokeNormal");
        }

        if (!al::isEffectEmitting(this, "SmokeLittle")) {
            al::emitEffect(this, "SmokeLittle", nullptr);
        }
    }
}

/** @return Rider of the rail the train is on. */
al::RailRider* TestAndoTrain::getRailRider() {
    return mRailKeepers[mRailIndex]->getRailRider();
}
