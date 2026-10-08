#pragma once

#include <math/seadQuat.h>

#include "Library/LiveActor/LiveActor.hpp"

/** @brief Tuning parameters shared by Fury Bowser's ring beams. */
class DarkBowserRingBeamParam {
public:
    DarkBowserRingBeamParam(f32 speed, s32 lifeTime, const char* pEffectName);

private:
    u8 _0[0x10];
};

/** @brief Expanding ring shockwave fired by Fury Bowser. */
class DarkBowserRingBeam : public al::LiveActor {
public:
    DarkBowserRingBeam(const char* pName, al::LiveActor* pHost);

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
    u8 _148[0x164 - 0x148];
    s32 mBeamMode;  // 0x164
    u8 _168[0x180 - 0x168];
};
static_assert(sizeof(DarkBowserRingBeam) == 0x180);
