#pragma once

#include <math/seadVector.h>

namespace al {
class LiveActor;
}

class ActorRailBrakeMoverParam {
public:
    ActorRailBrakeMoverParam();
    ActorRailBrakeMoverParam(float accel, float brakeLength, float brake, float minSpeed,
                             float shortRailLength, float shortAccel, float shortBrakeLength,
                             float shortBrake);

    float mAccel;
    float mBrakeLength;
    float mBrake;
    float mMinSpeed;
    float mShortRailLength;
    float mShortAccel;
    float mShortBrakeLength;
    float mShortBrake;
};

class ActorRailBrakeMover {
public:
    ActorRailBrakeMover(al::LiveActor* pActor, float maxSpeed,
                        const ActorRailBrakeMoverParam* pParam);
    bool moveSyncRailBrake();
    bool moveSyncRailByTime();
    void getPosition(sead::Vector3f* pPos);
    void getPosition(sead::Vector3f* pPos, float rate);
    bool moveSyncRailByTimeInOut(al::LiveActor* pActor, int* pPartIndex, float* pPartRate,
                                 bool isSyncTrans);
    bool moveSyncRailByPos(al::LiveActor* pActor, float rate);
    void resetSpeed();
    void setSpeedByTime(int time, bool isStop);

    al::LiveActor* mActor;
    float mSpeed;
    float mMaxSpeed;
    float mLength;
    float mRateStep;
    float mRate;
    const ActorRailBrakeMoverParam* mParam;
};
