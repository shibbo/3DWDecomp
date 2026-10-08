#include "MapObj/TestAndoManyCoin.hpp"
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadQuatCalcCommon.h>
#include <nn/g3d/g3d_ModelObj.h>
#include <nn/g3d/g3d_ResModel.h>
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Item/AcquireItemFunc.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMapUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Shader/DeferredRendering/DeferredRenderingShpUbo.hpp"
#include "Project/Model/SimpleModelG3D.hpp"

namespace sead {

/**
 * @brief Integrates an angular velocity into a rotation and renormalizes it.
 * @param q Rotation to update.
 * @param angularVelocity Angular velocity in radians per frame.
 * @param dt Time step in frames.
 */
template <typename T>
void QuatCalcCommon<T>::applyAngularVelocity(Base& q, const Vec3& angularVelocity, T dt) {
    const T halfDt = dt * 0.5f;
    const T x = halfDt * angularVelocity.x;
    const T y = halfDt * angularVelocity.y;
    const T z = halfDt * angularVelocity.z;
    const T qw = q.w - x * q.x - y * q.y - z * q.z;
    const T qx = x * q.w + q.x + y * q.z - z * q.y;
    const T qy = q.y - x * q.z + y * q.w + z * q.x;
    const T qz = x * q.y + q.z - y * q.x + q.w * z;
    q.w = qw;
    q.x = qx;
    q.y = qy;
    q.z = qz;
    normalize(q);
}

}  // namespace sead

namespace {

/**
 * @brief Coin rings that are shot out one after another, spread outwards and bounce back down.
 */
class TestAndoManyCoinCalcRing : public TestAndoManyCoinCalcBase {
public:
    TestAndoManyCoinCalcRing();
    void update(u32 step) override;
    void sort(const sead::Vector3f& rCameraPos, const sead::Vector3f& rCameraDir) override;
    u32 getMaxNum() const override;
    u32 getNum() const override;
    void getPos(sead::Vector3f* pPos, u32 index) const override;
    f32 getRadius() const override;
    bool tryCollect(const sead::Vector3f& rPos, f32 radius, s32 step) override;
    void drawDebug() const override;

private:
    u32 mRingNum = 16;
    u32 mCoinNumPerRing = 48;
    u32 mMaxNum = mRingNum * mCoinNumPerRing;
    u32 mActiveRingNum = 0;
    sead::Vector3f* mPositions = new sead::Vector3f[mMaxNum];
    sead::PtrArray<sead::Vector3f> mSortedPositions;
};

/**
 * @brief Coins that spawn two per frame at random spots and keep falling.
 */
class TestAndoManyCoinCalcFountain : public TestAndoManyCoinCalcBase {
public:
    TestAndoManyCoinCalcFountain();
    void update(u32 step) override;
    void sort(const sead::Vector3f& rCameraPos, const sead::Vector3f& rCameraDir) override;
    u32 getMaxNum() const override;
    u32 getNum() const override;
    void getPos(sead::Vector3f* pPos, u32 index) const override;
    f32 getRadius() const override;
    bool tryCollect(const sead::Vector3f& rPos, f32 radius, s32 step) override;
    void drawDebug() const override;

private:
    u32 mMaxNum = 300;
    u32 mNum = 0;
    u32 mNewestIndex = 0;
    f32 mFallDistance = 0.0f;
    sead::Vector3f* mPositions = new sead::Vector3f[mMaxNum];
    f32* mFallSpeeds = new f32[mMaxNum];
    sead::PtrArray<sead::Vector3f> mSortedPositions;
};

/**
 * @brief A single coin of the grid formation.
 */
struct TestAndoManyCoinGridCoin {
    sead::Vector3f mPos;
    bool mIsAlive;
};

/**
 * @brief A flat grid of coins that disappear one by one when collected.
 */
class TestAndoManyCoinCalcGrid : public TestAndoManyCoinCalcBase {
public:
    TestAndoManyCoinCalcGrid();
    void update(u32 step) override;
    void sort(const sead::Vector3f& rCameraPos, const sead::Vector3f& rCameraDir) override;
    u32 getMaxNum() const override;
    u32 getNum() const override;
    void getPos(sead::Vector3f* pPos, u32 index) const override;
    f32 getRadius() const override;
    bool tryCollect(const sead::Vector3f& rPos, f32 radius, s32 step) override;
    void drawDebug() const override;

private:
    u32 mWidth = 30;
    u32 mHeight = 30;
    f32 mInterval = 100.0f;
    f32 mRadius;
    s32 mMaxNum = mWidth * mHeight;
    s32 mNum = mMaxNum;
    TestAndoManyCoinGridCoin* mCoins = new TestAndoManyCoinGridCoin[mMaxNum];
    sead::PtrArray<TestAndoManyCoinGridCoin> mSortedCoins;
    s32 mDrawStart = 0;
    s32 mDrawEnd = 0;
};

NERVE_DECL(TestAndoManyCoin, Cute);
NERVES_MAKE_NOSTRUCT(TestAndoManyCoin, Cute)

sead::Vector3f sSortCameraPos;
sead::Vector3f sSortCameraDir;

}  // namespace

/**
 * @brief Constructs the actor.
 * @param pName Actor name.
 */
TestAndoManyCoin::TestAndoManyCoin(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the actor and creates the coin formation selected by "CalcType".
 * @param rInfo Actor init info.
 */
void TestAndoManyCoin::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTestAndoManyCoinCute, 0);
    al::tryAddDisplayOffset(this, rInfo);
    mCenter = al::getTrans(this);
    al::calcFrontDir(&mFront, this);
    al::tryGetArg(&mCalcType, rInfo, "CalcType");

    switch (mCalcType) {
    case CalcType_Grid:
        mCalc = new TestAndoManyCoinCalcGrid();
        break;
    case CalcType_Fountain:
        mCalc = new TestAndoManyCoinCalcFountain();
        al::tryGetArg(&mKillStep, rInfo, "IntParam1");
        al::tryGetArg(&mRotateRadius, rInfo, "FloatParam1");
        al::tryGetArg(&mRotateSpeed, rInfo, "FloatParam2");
        break;
    default:
        mCalc = new TestAndoManyCoinCalcRing();
        break;
    }

    al::setClippingInfo(this, mCalc->getRadius() + 100.0f, nullptr);
    al::trySyncStageSwitchAppear(this);
    mShapeUbos = new al::DeferredRenderingShpUbo**[mModelKeeper->getModelCafe()
                                                       ->getResModel()
                                                       ->GetShapeCount()];
    mCalc->getMaxNum();
}

/**
 * @brief Draw hook; the coins are drawn through the shape uniform blocks instead.
 */
void TestAndoManyCoin::draw() const {
    al::isDead(this);
}

/**
 * @brief Collects the coins the player's item-get sensor touches.
 * @param pMsg Received message.
 * @param pSender Sensor that sent the message.
 * @param pReceiver Sensor of this actor that received it.
 * @return Whether the message was handled.
 */
bool TestAndoManyCoin::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                  al::HitSensor* pReceiver) {
    if (mCollectCooldown != 0) {
        return false;
    }

    if (!al::isMsgPlayerItemGet(pMsg)) {
        return false;
    }

    if (mLastCollector == al::getSensorHost(pSender)) {
        return false;
    }

    if (!mCalc->tryCollect(al::getSensorPos(pSender) - al::getSensorPos(pReceiver),
                           al::getSensorRadius(pSender), al::getNerveStep(this))) {
        return false;
    }

    al::appearItemTiming(this, "タッチ", al::getSensorPos(pSender), sead::Vector3f::ey);
    mLastCollector = al::getSensorHost(pSender);
    return true;
}

/**
 * @brief Main nerve: moves the actor, updates and sorts the coins and uploads their matrices.
 */
void TestAndoManyCoin::exeCute() {
    if (mLastCollector != nullptr) {
        mCollectCooldown = 5;
        mLastCollector = nullptr;
    } else if (mCollectCooldown != 0) {
        mCollectCooldown--;
    }

    if (mRotateRadius > 0.0f) {
        sead::Quatf rotation;
        rotation.setAxisAngle(sead::Vector3f::ey, mRotateSpeed);
        mFront.rotate(rotation);
        sead::Vector3f trans = mCenter + mFront * mRotateRadius;
        al::setTrans(this, trans);
    }

    sead::Matrix34f translation;
    translation.makeIdentity();
    sead::Matrix34f baseMtx;
    baseMtx.makeIdentity();
    baseMtx.setTranslation(al::getTrans(this));
    sead::Matrix34f invBaseMtx;
    invBaseMtx.setInverse(baseMtx);
    mCalc->update(al::getNerveStep(this));

    sead::Vector3f cameraPos = al::getCameraPos(this);
    cameraPos.mul(invBaseMtx);
    sead::Vector3f cameraDir;
    al::calcCameraLookDir(&cameraDir, this);
    cameraDir.rotate(invBaseMtx);
    mCalc->sort(cameraPos, cameraDir);

    nn::g3d::ModelObj* modelObj = mModelKeeper->getModelCafe()->getModelG3D()->getModelObj();
    sead::QuatCalcCommon<f32>::applyAngularVelocity(
        *al::getQuatPtr(this), sead::Vector3f(0.0f, sead::Mathf::deg2rad(1.0f), 0.0f), 5.0f);
    al::makeMtxRT(&baseMtx, this);
    al::setClippingInfo(this, mCalc->getRadius() + 100.0f, nullptr);

    s32 shapeNum = modelObj->GetNumShapes();
    s32 coinNum = mCalc->getNum();
    for (s32 shape = 0; shape < shapeNum; shape++) {
        for (s32 i = 0; i < coinNum; i++) {
            sead::Vector3f pos;
            mCalc->getPos(&pos, i);
            translation.setTranslation(pos);
            sead::Matrix34f mtx;
            mtx.setMul(translation, baseMtx);
            mShapeUbos[shape][i]->setMtx(&mtx);
            mShapeUbos[shape][i]->swap();
        }
    }

    al::setSensorRadius(this, "Body", mCalc->getRadius());

    if (mKillStep > 0 && al::isGreaterEqualStep(this, mKillStep)) {
        kill();
    }
}

namespace {

/**
 * @brief Orders coin positions by their depth along the camera direction.
 * @param pA First position.
 * @param pB Second position.
 * @return -1 if pA is nearer, 1 if it is farther, 0 if both have the same depth.
 */
s32 compareDepth(const sead::Vector3f* pA, const sead::Vector3f* pB) {
    sead::Vector3f offsetA = *pA - sSortCameraPos;
    sead::Vector3f offsetB = *pB - sSortCameraPos;
    f32 depthA = offsetA.dot(sSortCameraDir);
    f32 depthB = offsetB.dot(sSortCameraDir);
    if (depthA < depthB) {
        return -1;
    }

    return depthA != depthB;
}

/**
 * @brief Creates the ring formation.
 */
TestAndoManyCoinCalcRing::TestAndoManyCoinCalcRing() {
    for (u32 i = 0; i < mMaxNum; i++) {
        mPositions[i].set(0.0f, 0.0f, 0.0f);
    }

    mSortedPositions.allocBuffer(mMaxNum, nullptr);
}

/**
 * @brief Shoots out new rings over time and moves every ring along its arc.
 * @param step Nerve step.
 */
void TestAndoManyCoinCalcRing::update(u32 step) {
    f32 time = step / 200.0f;
    u32 prevRingNum = mActiveRingNum;
    s32 ringNum = sead::Mathi::min(mRingNum, s32(time * mRingNum));
    mActiveRingNum = ringNum;
    if (prevRingNum < ringNum) {
        for (u32 ring = prevRingNum; ring < mActiveRingNum; ring++) {
            for (u32 i = 0; i < mCoinNumPerRing; i++) {
                mSortedPositions.pushBack(&mPositions[ring * mCoinNumPerRing + i]);
            }
        }
    }

    for (u32 ring = 0; ring < mRingNum; ring++) {
        f32 rate = al::wrapValue(time + f32(mRingNum - ring) / mRingNum, 1.0f);
        f32 radius = rate * 800.0f;
        f32 wave = sead::Mathf::cos(rate * sead::Mathf::pi2() * 4.0f);
        f32 height = (1.0f - rate) * ((1.0f - rate) * ((1.0f - wave) * 0.5f * 400.0f)) +
                     rate * -130.0f;
        for (u32 i = 0; i < mCoinNumPerRing; i++) {
            f32 angle = s32(i) * sead::Mathf::pi2() / mCoinNumPerRing;
            mPositions[ring * mCoinNumPerRing + i].set(sead::Mathf::sin(angle) * radius, height,
                                                       sead::Mathf::cos(angle) * radius);
        }
    }
}

/**
 * @brief Sorts the coins by depth.
 * @param rCameraPos Camera position in formation space.
 * @param rCameraDir Camera direction in formation space.
 */
void TestAndoManyCoinCalcRing::sort(const sead::Vector3f& rCameraPos,
                                    const sead::Vector3f& rCameraDir) {
    sSortCameraPos.set(rCameraPos);
    sSortCameraDir.set(rCameraDir);
    mSortedPositions.heapSort_<sead::Vector3f>(compareDepth);
}

/**
 * @brief Gets the coin capacity.
 * @return Total number of coins of all rings.
 */
u32 TestAndoManyCoinCalcRing::getMaxNum() const {
    return mRingNum * mCoinNumPerRing;
}

/**
 * @brief Gets the number of shown coins.
 * @return Number of coins in the rings shot out so far.
 */
u32 TestAndoManyCoinCalcRing::getNum() const {
    return mActiveRingNum * mCoinNumPerRing;
}

/**
 * @brief Gets a coin position in draw order.
 * @param pPos Output position.
 * @param index Coin index.
 */
void TestAndoManyCoinCalcRing::getPos(sead::Vector3f* pPos, u32 index) const {
    pPos->set(*mSortedPositions[index]);
}

/**
 * @brief Gets the formation radius.
 * @return Maximum ring radius.
 */
f32 TestAndoManyCoinCalcRing::getRadius() const {
    return 800.0f;
}

/**
 * @brief Checks whether a sensor touches the ring arc.
 * @param rPos Sensor position relative to the formation.
 * @param radius Sensor radius.
 * @param step Nerve step.
 * @return Whether a coin was touched.
 */
bool TestAndoManyCoinCalcRing::tryCollect(const sead::Vector3f& rPos, f32 radius, s32 step) {
    f32 rate = rPos.length() / 800.0f;
    if (rate > 1.0f) {
        return false;
    }

    f32 top = (1.0f - rate) * (1.0f - rate) * 400.0f;
    f32 bottom = rate * -130.0f;
    if (!(rPos.y < top + bottom + radius)) {
        return false;
    }

    return rPos.y > bottom - radius;
}

/**
 * @brief Debug draw of the coins (empty in release).
 */
void TestAndoManyCoinCalcRing::drawDebug() const {
    u32 num = getNum();
    for (u32 i = 0; i < num; i++) {
        sead::Vector3f pos;
        getPos(&pos, i);
    }
}

/**
 * @brief Creates the fountain formation.
 */
TestAndoManyCoinCalcFountain::TestAndoManyCoinCalcFountain() {
    mSortedPositions.allocBuffer(mMaxNum, nullptr);
}

/**
 * @brief Makes the coins fall and spawns two new ones.
 * @param step Nerve step.
 */
void TestAndoManyCoinCalcFountain::update(u32 step) {
    mFallDistance = 0.0f;
    for (u32 i = 0; i < mNum; i++) {
        mFallSpeeds[i] += 0.27f;
        if (mFallSpeeds[i] > 30.0f) {
            mFallSpeeds[i] = 30.0f;
        }

        mPositions[i] += sead::Vector3f(0.0f, -mFallSpeeds[i], 0.0f);
        if (mFallDistance < -mPositions[i].y) {
            mFallDistance = -mPositions[i].y;
        }
    }

    for (s32 i = 0; i < 2; i++) {
        if (mNum < mMaxNum) {
            mSortedPositions.pushBack(&mPositions[mNum]);
            mNum++;
        }

        mNewestIndex = (mNewestIndex + 1) % mMaxNum;
        mFallSpeeds[mNewestIndex] = 0.0f;
        f32 angle = al::getRandom(sead::Mathf::pi2());
        f32 distance = al::getRandom(180.0f);
        mPositions[mNewestIndex].set(sead::Mathf::sin(angle) * distance, 0.0f,
                                     sead::Mathf::cos(angle) * distance);
    }
}

/**
 * @brief Sorts the coins by depth.
 * @param rCameraPos Camera position in formation space.
 * @param rCameraDir Camera direction in formation space.
 */
void TestAndoManyCoinCalcFountain::sort(const sead::Vector3f& rCameraPos,
                                        const sead::Vector3f& rCameraDir) {
    sSortCameraPos.set(rCameraPos);
    sSortCameraDir.set(rCameraDir);
    mSortedPositions.heapSort_<sead::Vector3f>(
        [](const sead::Vector3f* pA, const sead::Vector3f* pB) { return compareDepth(pA, pB); });
}

/**
 * @brief Gets the coin capacity.
 * @return Maximum number of coins.
 */
u32 TestAndoManyCoinCalcFountain::getMaxNum() const {
    return mMaxNum;
}

/**
 * @brief Gets the number of spawned coins.
 * @return Number of coins.
 */
u32 TestAndoManyCoinCalcFountain::getNum() const {
    return mNum;
}

/**
 * @brief Gets a coin position.
 * @param pPos Output position.
 * @param index Coin index.
 */
void TestAndoManyCoinCalcFountain::getPos(sead::Vector3f* pPos, u32 index) const {
    pPos->set(mPositions[index]);
}

/**
 * @brief Gets the formation radius.
 * @return Fall distance of the lowest coin, at least 1000.
 */
f32 TestAndoManyCoinCalcFountain::getRadius() const {
    return sead::Mathf::max(1000.0f, mFallDistance);
}

/**
 * @brief Checks whether a sensor touches the falling coin column.
 * @param rPos Sensor position relative to the formation.
 * @param radius Sensor radius.
 * @param step Nerve step.
 * @return Whether a coin was touched.
 */
bool TestAndoManyCoinCalcFountain::tryCollect(const sead::Vector3f& rPos, f32 radius, s32 step) {
    if (rPos.y > radius) {
        return false;
    }

    return sead::Mathf::sqrt(rPos.x * rPos.x + rPos.z * rPos.z) < radius + 180.0f;
}

/**
 * @brief Debug draw of the coins (empty in release).
 */
void TestAndoManyCoinCalcFountain::drawDebug() const {
    u32 num = getNum();
    for (u32 i = 0; i < num; i++) {
        sead::Vector3f pos;
        getPos(&pos, i);
    }
}

/**
 * @brief Creates the grid formation and places every coin.
 */
TestAndoManyCoinCalcGrid::TestAndoManyCoinCalcGrid() {
    mRadius = sead::Vector2f((mWidth - 1) * 0.5f * mInterval, (mHeight - 1) * 0.5f * mInterval)
                  .length();
    mSortedCoins.allocBuffer(mMaxNum, nullptr);
    for (s32 z = 0; z < mHeight; z++) {
        for (s32 x = 0; x < mWidth; x++) {
            s32 index = z * mWidth + x;
            mCoins[index].mPos.set((x - (mWidth - 1) * 0.5f) * mInterval, 0.0f,
                                   (z - (mHeight - 1) * 0.5f) * mInterval);
            mCoins[index].mIsAlive = true;
            mSortedCoins.pushBack(&mCoins[index]);
        }
    }
}

/**
 * @brief Resets the draw range to all coins.
 * @param step Nerve step.
 */
void TestAndoManyCoinCalcGrid::update(u32 step) {
    mDrawStart = 0;
    mDrawEnd = mMaxNum - 1;
}

/**
 * @brief Sorts the coins by depth, collected coins last.
 * @param rCameraPos Camera position in formation space.
 * @param rCameraDir Camera direction in formation space.
 */
void TestAndoManyCoinCalcGrid::sort(const sead::Vector3f& rCameraPos,
                                    const sead::Vector3f& rCameraDir) {
    sSortCameraPos.set(rCameraPos);
    sSortCameraDir.set(rCameraDir);
    mSortedCoins.heapSort_<TestAndoManyCoinGridCoin>(
        [](const TestAndoManyCoinGridCoin* pA, const TestAndoManyCoinGridCoin* pB) {
            if (!pA->mIsAlive) {
                return 1;
            }

            if (!pB->mIsAlive) {
                return -1;
            }

            return compareDepth(&pA->mPos, &pB->mPos);
        });
}

/**
 * @brief Gets the coin capacity.
 * @return Number of grid cells.
 */
u32 TestAndoManyCoinCalcGrid::getMaxNum() const {
    return mMaxNum;
}

/**
 * @brief Gets the number of remaining coins.
 * @return Number of coins not collected yet.
 */
u32 TestAndoManyCoinCalcGrid::getNum() const {
    return mNum;
}

/**
 * @brief Gets a coin position in draw order.
 * @param pPos Output position.
 * @param index Coin index.
 */
void TestAndoManyCoinCalcGrid::getPos(sead::Vector3f* pPos, u32 index) const {
    pPos->set(mSortedCoins[index]->mPos);
}

/**
 * @brief Gets the formation radius.
 * @return Distance from the center to a corner coin.
 */
f32 TestAndoManyCoinCalcGrid::getRadius() const {
    return mRadius;
}

/**
 * @brief Collects every coin within reach of a sensor.
 * @param rPos Sensor position relative to the formation.
 * @param radius Sensor radius.
 * @param step Nerve step.
 * @return Whether any coin was collected.
 */
bool TestAndoManyCoinCalcGrid::tryCollect(const sead::Vector3f& rPos, f32 radius, s32 step) {
    if (mNum == 0) {
        return false;
    }

    if (rPos.y > 150.0f) {
        return false;
    }

    f32 offsetX = mInterval * 0.5f + mInterval * ((mWidth - 1) * 0.5f);
    f32 offsetZ = mInterval * 0.5f + mInterval * ((mHeight - 1) * 0.5f);
    s32 maxX = sead::Mathi::min(mWidth - 1, s32((rPos.x + radius + offsetX) / mInterval));
    s32 minZ = sead::Mathi::max(0, s32((rPos.z - radius + offsetZ) / mInterval));
    s32 maxZ = sead::Mathi::min(mHeight - 1, s32((rPos.z + radius + offsetZ) / mInterval));
    if (minZ > maxZ) {
        return false;
    }

    s32 minX = sead::Mathi::max(0, s32((rPos.x - radius + offsetX) / mInterval));
    if (minX > maxX) {
        return false;
    }

    f32 reach = (radius + 50.0f) * (radius + 50.0f);
    bool isCollected = false;
    for (s32 z = minZ; z <= maxZ; z++) {
        for (s32 x = minX; x <= maxX; x++) {
            s32 index = z * mWidth + x;
            TestAndoManyCoinGridCoin& coin = mCoins[index];
            if (coin.mIsAlive && (coin.mPos - rPos).squaredLength() < reach) {
                coin.mIsAlive = false;
                mNum--;
                isCollected = true;
            }
        }
    }

    return isCollected;
}

/**
 * @brief Debug draw of the coins (empty in release).
 */
void TestAndoManyCoinCalcGrid::drawDebug() const {}

}  // namespace
