#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

class ItemStockLayoutBase;

/**
 * @brief Blur trail that flies from a stocked item to its slot in the item stock layout.
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
    sead::Vector3f mStartPos;
    sead::Vector3f mPos;
    sead::Vector3f mEndPos;
    s32 mIndex;
    s32 mStep;                        // 0x30
    sead::FixedSafeString<32> mEffectName;
};

static_assert(sizeof(StockBlurEffect) == 0x70);
