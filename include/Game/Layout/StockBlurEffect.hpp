#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

class ItemStockLayoutBase;

/**
 * @brief Blur trail that flies from a stocked item to its slot in the item stock layout.
 * @note Only what reconstructed code needs is declared so far.
 */
class StockBlurEffect {
public:
    StockBlurEffect(ItemStockLayoutBase* pLayout, s32 index);

    void start(sead::Vector3f startPos, sead::Vector3f endPos);
    void update();
    void stop(bool isForce);

    static s32 getEffectStartOffsetY();
    static s32 getEffectStartPosZ();
    static s32 getEffectFrameNum();
    static s32 getEffectDelayFrames();

    /**
     * @brief Get the remaining frames of the effect.
     * @return The remaining frames, negative while the effect is not playing.
     */
    s32 getStep() const { return mStep; }

private:
    ItemStockLayoutBase* mLayout;     // 0x00
    u8 _8[0x30 - 0x8];
    s32 mStep;                        // 0x30
    u8 _34[0x70 - 0x34];
};

static_assert(sizeof(StockBlurEffect) == 0x70);
