#pragma once

#include <math/seadQuat.h>

#include "Library/LiveActor/LiveActor.hpp"

class SePlayObj;

/** @brief Tuning parameters shared by Fury Bowser's ring beams. */
class DarkBowserRingBeamParam {
public:
    DarkBowserRingBeamParam();
    DarkBowserRingBeamParam(f32 speed, s32 lifeTime, const char* pEffectName);

    f32 mSpeed;
    s32 mLifeTime;
    const char* mEffectName;
};

/** @brief Expanding ring shockwave fired by Fury Bowser. */
class DarkBowserRingBeam : public al::LiveActor {
public:
    DarkBowserRingBeam(const char* pName, al::LiveActor* pHost);

    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void setActive();
    void setDeactive();
    bool isFadeOut() const;
    void exeActive();
    void exeFadeOut();
    void exeDeactive();
    void updateSoundEffects();
    void setRingBeamParam(const DarkBowserRingBeamParam* pParam);
    void cutSoundEffects();
    void setActiveWithPosture(const sead::Vector3f& rTrans, const sead::Quatf& rQuat);
    bool isActive() const;

    /**
     * @brief Sets how the ring beam behaves when it is emitted.
     * @param mode Beam mode.
     */
    void setBeamMode(s32 mode) { mBeamMode = mode; }

private:
    void stopSoundEffects();
    f32 mSensorRadius = 0.0f;
    f32 mRadius = 0.0f;
    al::LiveActor* mHost;
    const DarkBowserRingBeamParam* mParam = nullptr;
    u32 mLifeStep = 0;
    s32 mBeamMode = 0;  // 0x164
    bool mFollowHost = false;
    al::LiveActor* mShadow;
    SePlayObj* mSoundActor = nullptr;
};
static_assert(sizeof(DarkBowserRingBeam) == 0x180);
