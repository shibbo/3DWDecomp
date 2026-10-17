#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class SoundKeyMoveParts : public al::LiveActor {
public:
    explicit SoundKeyMoveParts(const char*);
    ~SoundKeyMoveParts() override;
    void init(const al::ActorInitInfo&) override;
    void changeBgmTrackVolumeByRate(const al::IUseAudioKeeper*, float);
    void exeDelay();
    void exeWait();
    void exeMoveSign();
    void exeMove();
    void exeStopSign();
    void exeStop();
private:
    al::KeyPoseKeeper* mKeyPoses = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    int mWaitTime = 30;
    int mMoveTime = 0;
    int mDelayTime = 0;
    int mBgmLevel = 0;
};
static_assert(sizeof(SoundKeyMoveParts) == 0x170);
