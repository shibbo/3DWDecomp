#pragma once

#include <container/seadPtrArray.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class FlashingCtrl;
}

// One ring of coins; defined in CoinBlowConcentric.cpp with internal linkage.
namespace {
class CoinConcentricCircle;
}

/**
 * Bowser's Fury object that pops out concentric rings of blown coins, one ring after another,
 * and makes them flash and vanish after a time limit.
 */
class CoinBlowConcentric : public al::LiveActor {
public:
    /** How the hover height above the floor is measured. */
    enum class FloorCheckType : s32 {
        None = 0,
        AfterPlacement = 1,
        Appear = 2,
    };

    explicit CoinBlowConcentric(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void calcHeightOffset(f32 checkDistance);
    void appear() override;
    void kill() override;
    void exeAppear();
    void exeWait();
    void setTimerFrame(s32 frames);
    void control() override;

    f32 getHeightOffset() const { return mHeightOffset; }

private:
    typedef sead::PtrArray<CoinConcentricCircle> CircleArray;

    /** End iterator of mCircles, recomputed on every loop check. */
    CircleArray::iterator getCirclesEnd() const {
        return CircleArray::iterator(mCircles.size() + mCircles.data());
    }

    CircleArray mCircles;                                    // 0x148
    al::FlashingCtrl* mFlashingCtrl = nullptr;               // 0x158
    s32 mTimer = 0;                                          // 0x160
    bool mIsCoinHidden = false;                              // 0x164
    s32 mAppearInterval = 0;                                 // 0x168
    s32 mAppearFrame = 0;                                    // 0x16c
    FloorCheckType mFloorCheckType = FloorCheckType::None;  // 0x170
    f32 mHeightOffset = 0.0f;                                // 0x174
    f32 mHoverHeight = 100.0f;                               // 0x178
    bool _17c = false;                                       // 0x17c
};

static_assert(sizeof(CoinBlowConcentric) == 0x180);
