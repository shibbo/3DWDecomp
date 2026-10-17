#pragma once

#include "Library/MapObj/FallMapParts.hpp"

class TestChikuwaBlockGold : public al::FallMapParts {
public:
    explicit TestChikuwaBlockGold(const char* pName);
    ~TestChikuwaBlockGold() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;

private:
    int mTouchFrames = -1;
    float mStartHeight = 0.0f;
    float mLastCoinHeight = 0.0f;
    float mCoinAppearBaseDistance = 30.0f;
    float mCoinAppearDistance = 30.0f;
    float mCoinAppearDistanceRate = 0.75f;
};

static_assert(sizeof(TestChikuwaBlockGold) == 0x170);
