#pragma once

#include <basis/seadTypes.h>

namespace agl::sdw {
class DepthShadow;
}  // namespace agl::sdw

namespace sead {
class Camera;
class PerspectiveProjection;
}  // namespace sead

namespace al {
class GraphicsSystemInfo;
class LiveActorKit;
class Resource;
class ShaderHolder;
class ShadowMaskKeeper;

class DepthShadowDrawer;
class DepthShadowParam;
template <typename T>
class RequestInterp;

class ShadowDirector {
public:
    ShadowDirector(s32 viewNum, GraphicsSystemInfo* pInfo, LiveActorKit* pKit);
    ~ShadowDirector();

    void initShader(ShaderHolder* pShaderHolder);
    void initStageResource(const Resource* pResource, const char* pStageName);
    void movement();
    void preDrawGraphics();
    void updateViewGpu(s32 viewIndex, const sead::Camera* pCamera,
                       const sead::PerspectiveProjection* pProjection);
    void clear();
    void update();
    void endInit();
    agl::sdw::DepthShadow* getDepthShadow();
    bool isEnableShadowPrePass() const;
    bool isEnableShadowForLightPrePass() const;
    bool isEnableVarianceShadow() const;
    bool isEnableDepthShadow() const;

    ShadowMaskKeeper* getShadowMaskKeeper() const { return mShadowMaskKeeper; }

    /**
     * @brief Requests depth shadow parameters for the current frame.
     * @param priority The request priority.
     * @param step The interpolation frames.
     * @param rParam The requested parameters.
     * @note Needs Project/Base/RequestInterp.hpp and Library/Shadow/DepthShadowParam.hpp.
     */
    template <typename T = DepthShadowParam>
    void requestDepthShadowParam(s32 priority, s32 step, const T& rParam) {
        reinterpret_cast<RequestInterp<T>*>(_18)->requestParam(priority, step, rParam);
    }

    /**
     * @brief Sets the unknown flag at 0x1ef8 (on while the snapshot mode is active).
     * @param isSet The flag value.
     */
    void setUnknown1ef8(bool isSet) { _1ef8 = isSet; }

    void* _0;
    ShadowMaskKeeper* mShadowMaskKeeper;
    DepthShadowDrawer* mDepthShadowDrawer;
    u8 _18[0x190 - 0x18];
    s32 mPcfType;
    u8 _194[0x1b0 - 0x194];
    f32 _1b0;
    u8 _1b4[0x1d0 - 0x1b4];
    bool mIsEnableDistDamp;
    u8 _1d1[0x330 - 0x1d1];
    f32 mDistDampStart;
    u8 _334[0x350 - 0x334];
    f32 mDistDampEnd;
    u8 _354[0x3e0 - 0x354];
    s32 mComposeType;
    u8 _3e4[0x400 - 0x3e4];
    f32 _400;
    u8 _404[0x420 - 0x404];
    f32 _420;
    u8 _424[0x1ef8 - 0x424];
    bool _1ef8;
    u8 _1ef9[0x1f00 - 0x1ef9];
};

static_assert(sizeof(ShadowDirector) == 0x1f00);

}  // namespace al
