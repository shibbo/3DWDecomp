#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
#include "utility/aglParameter.h"
#include "utility/aglParameterObj.h"

namespace al {
class DirectionParam;

/**
 * @brief Parameters of the depth shadow, interpolated by the shadow director.
 * @note The members named _30/_50/_90/_130/_270/_290 keep their historical names because
 *       other units poke them directly (see the comments for their parameter names).
 */
class DepthShadowParam {
public:
    typedef agl::utl::Parameter<sead::FixedSafeString<64>> StringParameter;

    DepthShadowParam();

    void init();
    bool operator==(const DepthShadowParam& rOther) const;
    DepthShadowParam& operator=(const DepthShadowParam& rOther);
    void interp(const DepthShadowParam& rA, const DepthShadowParam& rB, f32 rate);

    agl::utl::ParameterObj mParamObj;
    agl::utl::Parameter<bool> _30;  // IsCalcClipVolume
    agl::utl::Parameter<bool> _50;  // IsDrawPlayerShadow
    agl::utl::Parameter<bool> mIsDrawObjectShadow;
    agl::utl::Parameter<bool> _90;  // IsDrawCasterShadow
    agl::utl::Parameter<bool> mIsDrawCasterShadowOnlyMap;
    agl::utl::Parameter<bool> mIsDrawEffectShadow;
    agl::utl::Parameter<bool> mIsShadowMap16UNorm;
    agl::utl::Parameter<bool> mIsShadowMaskEnable;
    agl::utl::Parameter<s32> _130;  // DepthShadowType
    agl::utl::Parameter<s32> mPcfType;
    agl::utl::Parameter<f32> mPcfOffsetValue;
    agl::utl::Parameter<bool> mIsEnableShadowDamp;
    agl::utl::Parameter<s32> mShadowMapSize;
    agl::utl::Parameter<s32> mPrePassShadowMapSize;
    agl::utl::Parameter<f32> mAddViewDepth;
    agl::utl::Parameter<f32> mAddVariance;
    agl::utl::Parameter<f32> mPolygonOffset;
    agl::utl::Parameter<f32> mPolygonSlopeScale;
    agl::utl::Parameter<f32> _270;  // Near
    agl::utl::Parameter<f32> _290;  // Far
    agl::utl::Parameter<f32> mIntensity;
    agl::utl::Parameter<s32> mMatrixCalcType;
    agl::utl::Parameter<f32> mShadowDampStart;
    agl::utl::Parameter<f32> mShadowDampEnd;
    StringParameter mClipPlaneName;
    agl::utl::Parameter<s32> mShadowComposeType;
    agl::utl::Parameter<f32> mDepthShadowFactorMin;
    agl::utl::Parameter<f32> mRepairIrradianceScale;
    agl::utl::Parameter<bool> mIsExpandSizeByDesign;
    DirectionParam* mDirForLpp;
    agl::utl::Parameter<bool> mIsUsingShadowCamera;
    agl::utl::Parameter<sead::Vector3f> mCameraPos;
    agl::utl::Parameter<sead::Vector3f> mCameraAt;
    agl::utl::Parameter<sead::Vector3f> mCameraUp;
    agl::utl::Parameter<f32> mCameraNear;
    agl::utl::Parameter<f32> mCameraFar;
    agl::utl::Parameter<f32> mCameraFovy;
    agl::utl::Parameter<f32> mCameraAspect;
};

static_assert(sizeof(DepthShadowParam) == 0x540);

/**
 * @brief Depth shadow parameters with an editable name.
 */
class NamedDepthShadowParam : public DepthShadowParam {
public:
    NamedDepthShadowParam();

    StringParameter mName;
};

static_assert(sizeof(NamedDepthShadowParam) == 0x5b0);

}  // namespace al
