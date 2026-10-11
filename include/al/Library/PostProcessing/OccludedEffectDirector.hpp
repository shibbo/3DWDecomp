#pragma once

#include <basis/seadTypes.h>
#include <common/aglShaderEnum.h>

namespace agl {
class RenderBuffer;
class RenderTargetDepth;
}  // namespace agl

namespace sead {
class Camera;
class Projection;
class Viewport;
}  // namespace sead

namespace al {
class GraphicsSystemInfo;
class OccludedEffectRequestInfo;

/**
 * Keeps the occluded effects (lens flares) and their occlusion queries.
 */
class OccludedEffectDirector {
public:
    OccludedEffectDirector(GraphicsSystemInfo* pInfo, s32 viewNum);
    ~OccludedEffectDirector();

    OccludedEffectRequestInfo* createInfoByPresetName(const char*);
    void clear();
    void movement();
    void calcView(s32 viewIndex, const sead::Camera* pCamera, const sead::Projection* pProjection);
    agl::ShaderMode draw(s32 viewIndex, const agl::RenderBuffer& rRenderBuffer,
                         const sead::Viewport& rViewport, const agl::RenderTargetDepth& rDepthTarget,
                         agl::ShaderMode shaderMode) const;

private:
    u8 _0[0x18];
};

static_assert(sizeof(OccludedEffectDirector) == 0x18);

}  // namespace al
