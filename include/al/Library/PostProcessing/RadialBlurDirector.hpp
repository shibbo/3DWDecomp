#pragma once

#include <basis/seadTypes.h>
#include <common/aglShaderEnum.h>
#include <effect/aglRadialBlur.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace sead {
class Heap;
}  // namespace sead

namespace al {
class LiveActor;

/**
 * Emits and draws the radial blurs requested by actors.
 */
class RadialBlurDirector {
public:
    RadialBlurDirector();

    ~RadialBlurDirector() { delete mRadialBlur; }

    void initialize(s32 viewNum, s32 blurNum, sead::Heap* pHeap);
    void control();
    void calcView(s32 viewIndex, const sead::Matrix34f& rViewMtx, const sead::Matrix44f& rProjMtx);
    void updateGPU();
    agl::ShaderMode draw(s32 viewIndex, const agl::RenderBuffer& rRenderBuffer,
                         agl::ShaderMode shaderMode) const;

private:
    agl::fx::RadialBlur* mRadialBlur;
    s32 mViewNum;
    s32 mBlurNum;
    void* mBlurInfos;
};

static_assert(sizeof(RadialBlurDirector) == 0x18);

void emitRadialBlur(const LiveActor* pActor, const sead::Vector3f& rPos, f32 radiusBegin,
                    f32 radiusEnd, s32 frame, s32 viewIndex);
void emitRadialBlur(const LiveActor* pActor, const sead::Vector3f* pPos, f32 radiusBegin,
                    f32 radiusEnd, s32 frame, s32 viewIndex);
}  // namespace al
