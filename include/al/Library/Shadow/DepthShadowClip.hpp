#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>
#include "utility/aglParameter.h"
#include "utility/aglParameterObj.h"

namespace agl::sdw {
class DepthShadow;
}  // namespace agl::sdw

namespace al {
class AreaObj;
class GraphicsSystemInfo;
class PlaneParam;

/**
 * @brief Clips the depth shadow volume with user planes and / or a graphics area box.
 */
class DepthShadowClip {
public:
    typedef agl::utl::Parameter<sead::FixedSafeString<64>> StringParameter;

    /**
     * @brief One optional user clip plane.
     */
    struct ClipPlane {
        PlaneParam* mPlane = nullptr;
        agl::utl::Parameter<bool> mIsUse;
    };

    static constexpr s32 cClipPlaneNum = 6;

    DepthShadowClip(const GraphicsSystemInfo* pInfo);

    void applyClipPlane(agl::sdw::DepthShadow* pDepthShadow) const;
    const AreaObj* getClipAreaObj() const;

private:
    agl::utl::ParameterObj mParamObj;
    StringParameter mName;
    agl::utl::Parameter<bool> mIsUsingCurrentGraphicsAreaClip;
    StringParameter mClipAreaName;
    ClipPlane mClipPlanes[cClipPlaneNum];
    const GraphicsSystemInfo* mSystemInfo;
};

static_assert(sizeof(DepthShadowClip) == 0x228);

}  // namespace al
