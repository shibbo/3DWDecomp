#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadColor.h>
#include <math/seadVector.h>

namespace al {
class ByamlIter;
}

/**
 * @brief Look parameters of an ocean surface (colors, normal maps, reflections, foam and surf).
 */
class OceanData {
  public:
    OceanData();

    static OceanData lerp(const OceanData& rFrom, const OceanData& rTo, f32 rate);
    void readFromYaml(const char* pName, const al::ByamlIter& rIter);

    s32 mFresnelType;                       // 0x000
    sead::Color4f mAmbient;                 // 0x004
    sead::Vector4f mAlbedoUV;               // 0x014
    f32 mAlbedoUVRot;                       // 0x024
    sead::Color4f mAlbedoColor;             // 0x028
    f32 mAlbedoIntensity;                   // 0x038
    sead::Vector4f mNormalMap1UV;           // 0x03c
    f32 mNormalMap1UVRot;                   // 0x04c
    f32 mNormalMap1Intensity;               // 0x050
    sead::Vector4f mNormalMap2UV;           // 0x054
    f32 mNormalMap2UVRot;                   // 0x064
    f32 mNormalMap2Intensity;               // 0x068
    f32 mRefractionFactor;                  // 0x06c
    sead::Color4f mRefractionColor;         // 0x070
    f32 mRefractionFadeHeight;              // 0x080
    f32 mReflectionDistance;                // 0x084
    f32 mReflectionBias;                    // 0x088
    f32 mReflectionBlend;                   // 0x08c
    f32 mReflectionFactor;                  // 0x090
    sead::Color4f mReflectionTintColor;     // 0x094
    f32 mReflectionDepthCutoff;             // 0x0a4
    f32 mCubeMapReflectionFactor;           // 0x0a8
    f32 mCubeMapReflectionBlend;            // 0x0ac
    f32 mAmbientWaveAmplitude;              // 0x0b0
    sead::Vector2f mAmbientWave1;           // 0x0b4 (speed, wavelength)
    sead::Vector2f mAmbientWave2;           // 0x0bc (speed, wavelength)
    sead::Color4f mFoamColor;               // 0x0c4
    sead::Vector2f mFoamTileSize;           // 0x0d4
    sead::Vector2f mFoamAnimSpeed;          // 0x0dc
    sead::Vector2f mFoamAnimWaveLength;     // 0x0e4
    sead::Vector2f mFoamAnimWaveAmplitude;  // 0x0ec
    f32 mSurfAnimSpeed;                     // 0x0f4
    f32 mSurfNormalMapScale;                // 0x0f8
    sead::Vector2f mSurfAnimWavelength;     // 0x0fc
    f32 mSurfAnimAmplitude;                 // 0x104
    f32 mSurfInOutAnimDistance;             // 0x108
    f32 mSurfWidth;                         // 0x10c
    f32 mCubeMapIncidenceAngleMin;          // 0x110
    f32 mCubeMapIncidenceAngleMax;          // 0x114
};

static_assert(sizeof(OceanData) == 0x118);

f32 lerpFloat(f32 from, f32 to, f32 rate);
