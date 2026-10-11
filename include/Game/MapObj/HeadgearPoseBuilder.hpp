#pragma once
#include "Library/Nerve/NerveExecutor.hpp"
namespace al { class LiveActor; class HitSensor; }
struct HeadgearPoseBuilderParam;
class HeadgearPoseBuilder : public al::NerveExecutor {
public:
    HeadgearPoseBuilder(al::LiveActor*, const HeadgearPoseBuilderParam*);
    void start(al::HitSensor*);
    void update();
    void updateAndCalcAnimDirect();
    bool isPlayerRaidonActionPlaying() const;
    al::HitSensor* getPlayerSensor() const { return mPlayerSensor; }
    void clearPlayerSensor() { mPlayerSensor = nullptr; }
private:
    al::LiveActor* mHost;
    void* mUnreconstructed18;
    al::HitSensor* mPlayerSensor;
    u8 mUnreconstructed28[0x20];
};
static_assert(sizeof(HeadgearPoseBuilder) == 0x48);
