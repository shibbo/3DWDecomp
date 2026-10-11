#pragma once

#include <container/seadSafeArray.h>
#include <container/seadTList.h>
#include <hostio/seadHostIONode.h>
#include <math/seadVector.h>

#include "utility/aglMultiFilterUnit.h"
#include "utility/aglParameter.h"
#include "utility/aglParameterIO.h"
#include "utility/aglParameterList.h"
#include "utility/aglParameterObj.h"

namespace sead {
class Heap;
class LogicalFrameBuffer;
class Viewport;
namespace hostio {
class Context;
class PropertyEvent;
}  // namespace hostio
}  // namespace sead

namespace agl {
class DrawContext;
class TextureData;
}  // namespace agl

namespace agl::utl {

class DebugTexturePage;

class MultiFilter : public sead::hostio::Node, public IParameterIO {
public:
    static constexpr s32 cUnitNum = 4;

    MultiFilter();
    ~MultiFilter() override;

    void initialize(sead::Heap* pHeap, sead::Heap* pDebugHeap);
    void setUseTextureAlpha(bool useAlpha);
    const MultiFilterResultInfo& calcResultInfo(s32 width, s32 height) const;
    void draw(DrawContext* pDrawContext, const TextureData& rTexture) const;
    void draw(DrawContext* pDrawContext, const TextureData& rTexture,
              const sead::Vector2f& rTrimScale, const sead::Vector2f& rTrimCenter) const;
    void drawDebug(DrawContext* pDrawContext, const sead::LogicalFrameBuffer& rFrameBuffer,
                   const sead::Viewport& rViewport) const;
    void freeResultTexture() const;
    void inactivateAll();

    void genMessage(sead::hostio::Context* pContext);
    void listenPropertyEvent(const sead::hostio::PropertyEvent* pEvent);

    const TextureData* getResultTexture() const { return mDrawContext.mResultTexture; }
    void setUnknownBaa(bool value) { _baa = value; }

protected:
    bool preWrite_() const override;
    void postRead_() override;

private:
    friend class MultiFilterUnit;

    void drawFilter_(DrawContext* pDrawContext, const MultiFilterUnit& rUnit, s32 index) const;
    sead::TListNode<MultiFilterUnit*>* addFilter_(MultiFilterUnit::FilterType type);

    bool isDrawable_() const { return mIsEnable && (mActiveUnits.front() || mTrimming.isEnable()); }

    sead::TList<MultiFilterUnit*> mActiveUnits;
    sead::SafeArray<sead::TList<MultiFilterUnit*>, MultiFilterUnit::cFilterType_Num> mFreeUnits;
    ParameterList mTypeParamLists[MultiFilterUnit::cFilterType_Num];
    MultiFilterUnit* mUnits[MultiFilterUnit::cFilterType_Num][cUnitNum];
    mutable MultiFilterDrawContext mDrawContext;
    mutable MultiFilterResultInfo mResultInfo;
    u32 mSelectId = 0;
    bool mIsEnable = true;
    bool mIsDrawDebug = false;
    bool _baa = false;
    DebugTexturePage* mDebugTexturePage = nullptr;
    ParameterObj mParamObj;
    Parameter<bool> mResultSamplerLinear{true, "result_sampler_linear",
                                         "結果のテクスチャフィルタをLinearにする", &mParamObj};
    mutable Trimming mTrimming{-1};
};
static_assert(sizeof(MultiFilter) == 0xdb0);

}  // namespace agl::utl
