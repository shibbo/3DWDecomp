#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
template <class T> class DeriveActorGroup;
class AreaObjGroup;
} // namespace al
class Fish;
class FishColony : public al::LiveActor {
  public:
    explicit FishColony(const char*);
    void init(const al::ActorInitInfo&) override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*,
                               al::ScreenPointTarget*) override;
    void exeGroup();

  private:
    al::DeriveActorGroup<Fish>* mFish = nullptr;
    al::AreaObjGroup* mSwimArea = nullptr;
    sead::Vector3f mPreviousPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
    float mAvoidDist = 60.0f;
    float mSpeed = 2.5f;
    int mSpeedTimer = 0;
    bool mForward = true;
};
static_assert(sizeof(FishColony) == 0x180);
