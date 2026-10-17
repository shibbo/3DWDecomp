#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; class PoseHistoryPath; }
class ChorobonCube;

/** @brief Moves a chain of cube Fuzzies along key poses. */
class ChorobonCubeHolder : public al::LiveActor {
public:
    explicit ChorobonCubeHolder(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void start();
    void exeMove();
    void exeDelay();
    void exeWait();
    void exeStop();
private:
    ChorobonCube** mCubes = nullptr;
    s32 mCubeCount = 0;
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    al::PoseHistoryPath* mPath;
    s32 mMoveTime = 0;
    s32 mDelayTime = 0;
    s32 mWaitTime = 0;
    f32 mCubeDistance = 375.0f;
    sead::Vector3f mClippingCenter = {0.0f, 0.0f, 0.0f};
};
static_assert(sizeof(ChorobonCubeHolder) == 0x188);
