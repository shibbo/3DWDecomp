#include "Layout/StockBlurEffect.hpp"
#include "Layout/ItemStockLayoutBase.hpp"
#include "System/GameDataFunction.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
namespace {
const sead::Vector3f sMultiplayerEndPos(-548.0f, -145.0f, 10.0f);
}
/** @brief Names the stock trail and binds its particle position.
 * @param pLayout Owner of the item stock layout.
 * @param index Effect index within the stock layout. */
StockBlurEffect::StockBlurEffect(ItemStockLayoutBase* pLayout, s32 index)
    : mLayout(pLayout), mIndex(index), mStep(-1) {
    mEffectName.format("%s%d", "StockBlur", mIndex);
    al::setEffectFollowPosPtr(pLayout->getParentLayout(), mEffectName.cstr(), &mPos);
}
/** @brief Starts the delayed flight from an item to its stock slot.
 * @param startPos Initial item position.
 * @param endPos Destination used in single-player mode. */
void StockBlurEffect::start(sead::Vector3f startPos, sead::Vector3f endPos) {
    mStartPos = startPos;
    mPos = startPos;
    mEndPos = GameDataFunction::isSingleMode(GameDataHolderAccessor(mLayout->getParentLayout()))
                  ? endPos : sMultiplayerEndPos;
    mStep = 18;
}
/** @brief Advances the curved flight and emits the arrival effect. */
void StockBlurEffect::update() {
    if (mStep < 0)
        return;
    if (mStep > 15) {
        --mStep;
        return;
    }
    if (mStep == 15)
        al::emitEffect(mLayout->getParentLayout(), mEffectName.cstr(), nullptr);
    float rate = 1.0f - mStep / 15.0f;
    float height;
    if (rate < 0.3f) {
        float curve = rate / 0.3f - 1.0f;
        height = (1.0f - curve * curve) * 300.0f;
    } else {
        float curve = (rate - 0.3f) / 0.7f;
        height = (1.0f - curve * curve) * 300.0f;
    }
    al::lerpVec(&mPos, mStartPos, mEndPos, al::easeIn(rate));
    mPos.y = height + mPos.y;
    if (mStep-- <= 0) {
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(mLayout->getParentLayout())))
            al::emitEffect(mLayout->getParentLayout(), "StockIn", &mPos);
        stop(false);
    }
}
/** @brief Stops the trail and optionally removes all particles from its owner.
 * @param isForce Whether to delete all owner emitters and particles. */
void StockBlurEffect::stop(bool isForce) {
    mStep = -1;
    if (isForce)
        al::tryDeleteEmitterAndParticleAll(mLayout->getParentLayout());
    else
        al::tryDeleteEffect(mLayout->getParentLayout(), mEffectName.cstr());
}
/** @brief Returns the vertical offset of the trail's starting position. */
s32 StockBlurEffect::getEffectStartOffsetY() { return 100; }
/** @brief Returns the depth of the trail's starting position. */
s32 StockBlurEffect::getEffectStartPosZ() { return 0; }
/** @brief Returns the number of flight frames. */
s32 StockBlurEffect::getEffectFrameNum() { return 15; }
/** @brief Returns the delay before the flight starts. */
s32 StockBlurEffect::getEffectDelayFrames() { return 3; }
