#pragma once
#include "Game/MapObj/FlingPole.hpp"
namespace al {
class PlacementId;
class CameraTicket;
} // namespace al
class GoalItem;
class IslandFlag : public FlingPole {
  public:
    explicit IslandFlag(const char*);
    void init(const al::ActorInitInfo&) override;
    void initJointKeeper() override;
    void control() override;
    virtual void updateLod();
    bool isLod() const;
    void changeFlag();
    void setScenarioAnim();
    void exeCameraIn();
    void exeCameraOut();
    bool isCameraSequenceStarted();
    bool isCameraSequenceDone();

  private:
    al::PlacementId* mPlacementId;
    int _250 = 0;
    int mCompletedScenario = -1;
    sead::PtrArray<GoalItem> mGoalItems;
    al::CameraTicket* mCamera = nullptr;
    int mCameraInStep[5];
    int mCameraHoldStep[5];
    int mCameraOutStep[5];
    int mCameraScenario = 0;
    bool mCameraStarted = false;
    bool mCameraDone = false;
    sead::PtrArray<al::LiveActor> _2b8;
    unsigned mPreviousFlag = 0;
    unsigned mFlag = 1;
    bool mLod[5] = {};
};
static_assert(sizeof(IslandFlag) == 0x2d8);
