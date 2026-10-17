#pragma once

#include <basis/seadTypes.h>
#include <common/aglShaderEnum.h>

#include "Library/Nerve/NerveExecutor.hpp"

namespace agl {
class RenderBuffer;
class TextureData;
}  // namespace agl

namespace agl::utl {
class ParameterObj;
}

namespace sead {
class Projection;
}  // namespace sead

namespace al {

class DepthOfFieldParam {
public:
    DepthOfFieldParam();

    agl::utl::ParameterObj* getParamObj() const { return mParamObj; }

private:
    agl::utl::ParameterObj* mParamObj;
    u8 _8[0x50];
};

static_assert(sizeof(DepthOfFieldParam) == 0x58);

class AreaObjDirector;
class GraphicsSystemInfo;
class PlayerHolder;
class SceneCameraInfo;

class DepthOfFieldDrawer : public NerveExecutor {
public:
    DepthOfFieldDrawer(const GraphicsSystemInfo* pInfo, s32 viewNum);
    ~DepthOfFieldDrawer() override;

    void init(AreaObjDirector* pAreaObjDirector, SceneCameraInfo* pSceneCameraInfo,
              const PlayerHolder* pPlayerHolder);
    void initAfterPlacement();
    void update(bool isPaused);
    void cancelLerp();
    void disable();
    agl::ShaderMode draw(s32 viewIndex, const agl::RenderBuffer& rRenderBuffer,
                         const agl::TextureData& rDepthTexture, const sead::Projection& rProjection,
                         f32 near, f32 far, bool isLinearDepth, agl::ShaderMode shaderMode) const;

    void setStageName(const char* pStageName) { mStageName = pStageName; }

private:
    u8 _10[0x28 - 0x10];
    const char* mStageName;
    u8 _30[0x1f0 - 0x30];
};

static_assert(sizeof(DepthOfFieldDrawer) == 0x1f0);

}  // namespace al
