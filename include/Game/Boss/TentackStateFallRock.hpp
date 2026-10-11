#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
class TentackHead;

struct TentackStateFallRockParam {
    TentackStateFallRockParam();
    constexpr TentackStateFallRockParam(int startWait, int interval, int count, int followInterval,
        float angle, float minDistance, float maxDistance, bool isRepeat, bool isPlayAction)
        : mStartWait(startWait), mInterval(interval), mCount(count), mFollowInterval(followInterval),
          mAngle(angle), mMinDistance(minDistance), mMaxDistance(maxDistance),
          mIsRepeat(isRepeat), mIsPlayAction(isPlayAction) {}

    int mStartWait;
    int mInterval;
    int mCount;
    int mFollowInterval;
    float mAngle;
    float mMinDistance;
    float mMaxDistance;
    bool mIsRepeat;
    bool mIsPlayAction;
};
static_assert(sizeof(TentackStateFallRockParam) == 0x20);

class TentackStateFallRock : public al::NerveStateBase {
public:
    TentackStateFallRock(TentackHead* pHead, const TentackStateFallRockParam* pParam);
    void appear() override;
    void kill() override;
    void exeWaitStart();
    void exeAttackStart();
    void exeShoot();
    void shoot(bool isFollow);
    void exeWait();

    TentackHead* mHead;
    const TentackStateFallRockParam* mParam;
    int mShotCount = 0;
    unsigned char mIsDisableFollow = false;
    unsigned char mIsValidFollowForce = false;
};
static_assert(sizeof(TentackStateFallRock) == 0x30);
