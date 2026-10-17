#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
class OneSideStep : public al::LiveActor {
public:
    explicit OneSideStep(const char*);
    void appearAndSetStart();
    const al::KeyPoseKeeper* getKeyPoseKeeper() const { return mKeyPoseKeeper; }
    void setGenerated() { mGenerated = true; }
private:
    al::KeyPoseKeeper* mKeyPoseKeeper;
    u8 mUnreconstructed[0x100];
    bool mGenerated;
    u8 mPadding[7];
};
static_assert(sizeof(OneSideStep) == 0x258);
