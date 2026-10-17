#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>

class FloatingTerrain : public al::LiveActor {
public:
    explicit FloatingTerrain(const char* name);
    void init(const al::ActorInitInfo& info) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void startAppear();
    void startRotate();
    void startHide();
    void syncAttachments(bool syncLife);
    void exeHidden();
    void exeAppear();
    void exeStay();
    void exeHide();
    void exeRotate();
private:
    struct Attachment {
        sead::Vector3f offset;
        al::LiveActor* actor;
    };
    Attachment* mAttachments = nullptr;
    s32 mAttachmentCount = 0;
    sead::Vector3f mHome = sead::Vector3f::zero;
    sead::Quatf mHomeQuat;
};
static_assert(sizeof(FloatingTerrain) == 0x170);
