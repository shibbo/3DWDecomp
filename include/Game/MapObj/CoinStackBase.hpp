#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
namespace al { class MtxConnector; }
class CoinStackBase : public al::LiveActor {
public:
    CoinStackBase(const char*, bool);
    ~CoinStackBase() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void initPosition(const sead::Vector3f&);
    void initConnector(const al::MtxConnector*);
    void requestFall(int);
    void exeWait();
    void exeFloatWait();
    void exeFall();
    void exeLand();
    void slide();
    float getStackHeight() const { return mStackHeight; }
    bool isCollected() const { return mIsCollected; }
    void clearCollected() { mIsCollected = false; }
private:
    bool mIsCollected = false;
    bool mIsMoving;
    float mStackHeight = 0.0f;
    sead::Vector3f mLocalPosition = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mVelocity = {0.0f, 0.0f, 0.0f};
    float mLandingY = 0.0f;
    int mFallDelay = 0;
    sead::Vector3f mInitialPosition = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mSlideOffset = {0.0f, 0.0f, 0.0f};
    const al::MtxConnector* mConnector = nullptr;
    sead::Quatf mRotation = sead::Quatf::unit;
};
static_assert(sizeof(CoinStackBase) == 0x1a0);
