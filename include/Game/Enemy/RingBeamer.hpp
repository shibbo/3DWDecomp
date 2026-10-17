#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class RingBeamerBeam;
class RingBeamerParam;
namespace al { class MtxConnector; }
/** @brief Switchable emitter of expanding ring beams. */
class RingBeamer : public al::LiveActor {
public:
    RingBeamer(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void control() override;
    void stop();
    bool isActive() const;
    void exeActive();
    void exeBeginActive();
    void exeInterval();
    void exeEndInterval();
    void exeEmitBeam();
    void exeDeactive();
    void exeBeginDeactive();
    void exeStop();
private:
    RingBeamerBeam* mBeam = nullptr;
    al::MtxConnector* mConnector = nullptr;
    s32 mCooldown = 15;
    RingBeamerParam* mParam;
    s32 mInterval = 10;
    f32 mSignFrames = 0.0f;
};
static_assert(sizeof(RingBeamer) == 0x170);
