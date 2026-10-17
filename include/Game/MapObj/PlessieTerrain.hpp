#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Util/AttachObjectList.hpp"
class PlessieTerrain : public al::LiveActor {
public:
    explicit PlessieTerrain(const char*);
    ~PlessieTerrain() override;
    void init(const al::ActorInitInfo&) override;
    void startClipped() override;
    void endClipped() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void startRise();
    void hideInstant();
    void syncAttachments(bool);
    void startHide();
    void exeHidden();
    void exeAppear();
    void exeStay();
    void exeHide();
private:
    rc::AttachObjectList mAttachments;
    sead::Vector3f mBasePos = sead::Vector3f::zero;
    bool mIsPlessieChase = false;
};
static_assert(sizeof(PlessieTerrain) == 0x178);
