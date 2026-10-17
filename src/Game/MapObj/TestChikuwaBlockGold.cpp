#include "MapObj/TestChikuwaBlockGold.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Item/ItemUtil.hpp"
#include <cmath>

TestChikuwaBlockGold::TestChikuwaBlockGold(const char* pName) : al::FallMapParts(pName) {}

TestChikuwaBlockGold::~TestChikuwaBlockGold() {}

void TestChikuwaBlockGold::init(const al::ActorInitInfo& rInfo) {
    al::FallMapParts::init(rInfo);
    mLastCoinHeight = mStartHeight = al::getTrans(this).y;
    al::tryGetArg(&mCoinAppearBaseDistance, rInfo, "CoinAppearBaseDistance");
    al::tryGetArg(&mCoinAppearDistanceRate, rInfo, "CoinAppearDistanceRate");
}

void TestChikuwaBlockGold::control() {
    if (mTouchFrames - 1 >= 0)
        --mTouchFrames;
    if (mTouchFrames > 0) {
        if (std::fabs(mLastCoinHeight - al::getTrans(this).y) > mCoinAppearDistance) {
            sead::Vector3f pos = al::getTrans(this) + 300.0f * sead::Vector3f::ey;
            al::appearItemTiming(this, "移動", pos, sead::Vector3f::ey);
            mCoinAppearDistance *= mCoinAppearDistanceRate;
            mLastCoinHeight = al::getTrans(this).y;
        }
    } else {
        mCoinAppearDistance = mCoinAppearBaseDistance;
        mLastCoinHeight = mStartHeight;
    }
}

bool TestChikuwaBlockGold::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                     al::HitSensor* pReceiver) {
    if (al::isMsgFloorTouch(pMsg))
        mTouchFrames = 10;
    al::FallMapParts::receiveMsg(pMsg, pSender, pReceiver);
    return false;
}
