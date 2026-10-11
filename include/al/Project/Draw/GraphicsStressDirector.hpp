#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Project/Draw/GraphicsParamKeeper.hpp"
#include "Project/Draw/StressParam.hpp"

namespace al {
class GraphicsQualityController;
class GraphicsSystemInfo;

/**
 * Picks the render resolution and quality settings from the stress parameters and the GPU load.
 */
class GraphicsStressDirector : public GraphicsParamRequestInterpKeeper<StressParam> {
public:
    struct StressInfo {
        s32 baseWidth;
        s32 baseHeight;
        s32 renderWidth;
        s32 renderHeight;
        s32 levelNum;
        s32 widthScales[5];
        s32 heightScales[5];
        f32 recoverPercents[6];
    };

    static_assert(sizeof(StressInfo) == 0x54);

    GraphicsStressDirector(GraphicsSystemInfo* pInfo);

    s32 getBufferSizeX() const;
    s32 getBufferSizeY() const;
    f32 getFXAAAlphaOut() const;
    void setQualityControlEnable(bool isEnable);
    void setForceDisable(bool isForceDisable);
    void setStressInfoMode(bool isSingleMode, s32 mode);
    s32 getStressInfoMode() const;
    void setSingleMode(bool isSingleMode);
    void setForceStressOff(bool isForceStressOff);
    void movement();
    void calcPseudoAAProjOffset(sead::Vector2f* pOffset, s32 width, s32 height) const;

    bool isClearGBufferViewNrm() const { return getCurrentParam().isClearGBufferViewNrm(); }

    bool isClearGBufferViewDepth() const { return getCurrentParam().isClearGBufferViewDepth(); }

    bool isForceStressOff() const { return mIsForceStressOff; }

    bool isFullResolution() const { return mIsFullResolution; }

    void setFullResolution(bool isFullResolution) { mIsFullResolution = isFullResolution; }

private:
    s32 mPseudoAAFrame = 0;
    sead::Vector2f mPseudoAAOffset = sead::Vector2f::zero;
    GraphicsQualityController* mQualityController;
    bool mIsFullResolution;
    bool mIsIgnoreQualityControl;
    bool mIsSingleMode;
    bool mIsForceDisable;
    bool mIsQualityControlEnableBeforeDisable;
    bool mIsForceStressOff;
    s32 mStressInfoModeBeforeForceOff;
    StressInfo* mStressInfo;
};

static_assert(sizeof(GraphicsStressDirector) == 0xa60);

}  // namespace al
