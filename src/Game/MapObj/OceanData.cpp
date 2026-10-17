#include "MapObj/OceanData.hpp"

OceanData::OceanData()
    : mFresnelType(41),
      mAmbient(201.0f / 255.0f, 226.0f / 255.0f, 1.0f, 0.0f),
      mAlbedoUV(1.0f, 1.0f, 0.0f, 0.0f),
      mAlbedoUVRot(0.0f),
      mAlbedoColor(1.0f, 1.0f, 1.0f, 0.0f),
      mAlbedoIntensity(1.0f),
      mNormalMap1UV(0.5f, 0.5f, 0.0f, 0.0f),
      mNormalMap1UVRot(0.0f),
      mNormalMap1Intensity(1.0f),
      mNormalMap2UV(0.5f, 0.5f, 0.0f, 0.0f),
      mNormalMap2UVRot(0.0f),
      mNormalMap2Intensity(1.0f),
      mRefractionFactor(0.1f),
      mRefractionColor(0.196078f, 0.694118f, 0.909804f, 0.0f),
      mRefractionFadeHeight(450.0f),
      mReflectionDistance(100000.0f),
      mReflectionBias(0.115f),
      mReflectionBlend(0.5f),
      mReflectionFactor(1.0f),
      mReflectionTintColor(1.0f, 1.0f, 1.0f, 1.0f),
      mReflectionDepthCutoff(0.01f),
      mCubeMapReflectionFactor(1.5f),
      mCubeMapReflectionBlend(0.5f),
      mAmbientWaveAmplitude(10.0f),
      mAmbientWave1(3.0f, 300.0f),
      mAmbientWave2(2.0f, 400.0f),
      mFoamColor(0.8f, 0.8f, 0.9f, 1.0f),
      mFoamTileSize(1700.0f, 1700.0f),
      mFoamAnimSpeed(0.2f, 0.5f),
      mFoamAnimWaveLength(100.0f, 500.0f),
      mFoamAnimWaveAmplitude(0.01f, 0.05f),
      mSurfAnimSpeed(1.0f),
      mSurfNormalMapScale(50.0f),
      mSurfAnimWavelength(50.0f, 50.0f),
      mSurfAnimAmplitude(20.0f),
      mSurfInOutAnimDistance(15.0f),
      mSurfWidth(70.0f),
      mCubeMapIncidenceAngleMin(0.5f),
      mCubeMapIncidenceAngleMax(0.7f) {}

f32 lerpFloat(f32 from, f32 to, f32 rate) {
    float scaledFrom = (1.0f - rate) * from;
    return rate * to + scaledFrom;
}

OceanData OceanData::lerp(const OceanData& from, const OceanData& to, f32 rate) {
    OceanData result;
    result.mFresnelType = rate >= 0.5f ? to.mFresnelType : from.mFresnelType;
    result.mAmbient.setLerp(from.mAmbient, to.mAmbient, rate);
    result.mAlbedoUV = rate >= 0.5f ? to.mAlbedoUV : from.mAlbedoUV;
    result.mAlbedoUVRot = lerpFloat(from.mAlbedoUVRot, to.mAlbedoUVRot, rate);
    result.mAlbedoColor.setLerp(from.mAlbedoColor, to.mAlbedoColor, rate);
    result.mAlbedoIntensity = lerpFloat(from.mAlbedoIntensity, to.mAlbedoIntensity, rate);
    result.mNormalMap1UV = rate >= 0.5f ? to.mNormalMap1UV : from.mNormalMap1UV;
    result.mNormalMap1UVRot = lerpFloat(from.mNormalMap1UVRot, to.mNormalMap1UVRot, rate);
    result.mNormalMap1Intensity = lerpFloat(from.mNormalMap1Intensity, to.mNormalMap1Intensity, rate);
    result.mNormalMap2UV = rate >= 0.5f ? to.mNormalMap2UV : from.mNormalMap2UV;
    result.mNormalMap2UVRot = lerpFloat(from.mNormalMap2UVRot, to.mNormalMap2UVRot, rate);
    result.mNormalMap2Intensity = lerpFloat(from.mNormalMap2Intensity, to.mNormalMap2Intensity, rate);
    result.mRefractionFactor = lerpFloat(from.mRefractionFactor, to.mRefractionFactor, rate);
    result.mRefractionColor.setLerp(from.mRefractionColor, to.mRefractionColor, rate);
    result.mRefractionFadeHeight = lerpFloat(from.mRefractionFadeHeight, to.mRefractionFadeHeight, rate);
    result.mReflectionDistance = lerpFloat(from.mReflectionDistance, to.mReflectionDistance, rate);
    result.mReflectionBias = lerpFloat(from.mReflectionBias, to.mReflectionBias, rate);
    result.mReflectionBlend = lerpFloat(from.mReflectionBlend, to.mReflectionBlend, rate);
    result.mReflectionFactor = lerpFloat(from.mReflectionFactor, to.mReflectionFactor, rate);
    result.mReflectionTintColor.setLerp(from.mReflectionTintColor, to.mReflectionTintColor, rate);
    result.mReflectionDepthCutoff = lerpFloat(from.mReflectionDepthCutoff, to.mReflectionDepthCutoff, rate);
    result.mCubeMapReflectionFactor = lerpFloat(from.mCubeMapReflectionFactor, to.mCubeMapReflectionFactor, rate);
    result.mCubeMapReflectionBlend = lerpFloat(from.mCubeMapReflectionBlend, to.mCubeMapReflectionBlend, rate);
    result.mAmbientWaveAmplitude = lerpFloat(from.mAmbientWaveAmplitude, to.mAmbientWaveAmplitude, rate);
    result.mAmbientWave1 = rate >= 0.5f ? to.mAmbientWave1 : from.mAmbientWave1;
    result.mAmbientWave2 = rate >= 0.5f ? to.mAmbientWave2 : from.mAmbientWave2;
    result.mFoamColor.setLerp(from.mFoamColor, to.mFoamColor, rate);
    result.mFoamTileSize = rate >= 0.5f ? to.mFoamTileSize : from.mFoamTileSize;
    result.mFoamAnimSpeed = rate >= 0.5f ? to.mFoamAnimSpeed : from.mFoamAnimSpeed;
    result.mFoamAnimWaveLength = rate >= 0.5f ? to.mFoamAnimWaveLength : from.mFoamAnimWaveLength;
    result.mFoamAnimWaveAmplitude = rate >= 0.5f ? to.mFoamAnimWaveAmplitude : from.mFoamAnimWaveAmplitude;
    result.mSurfAnimSpeed = rate >= 0.5f ? to.mSurfAnimSpeed : from.mSurfAnimSpeed;
    result.mSurfNormalMapScale = lerpFloat(from.mSurfNormalMapScale, to.mSurfNormalMapScale, rate);
    result.mSurfAnimWavelength = rate >= 0.5f ? to.mSurfAnimWavelength : from.mSurfAnimWavelength;
    result.mSurfAnimAmplitude = lerpFloat(from.mSurfAnimAmplitude, to.mSurfAnimAmplitude, rate);
    result.mSurfInOutAnimDistance = lerpFloat(from.mSurfInOutAnimDistance, to.mSurfInOutAnimDistance, rate);
    result.mSurfWidth = lerpFloat(from.mSurfWidth, to.mSurfWidth, rate);
    result.mCubeMapIncidenceAngleMin = lerpFloat(from.mCubeMapIncidenceAngleMin, to.mCubeMapIncidenceAngleMin, rate);
    result.mCubeMapIncidenceAngleMax = lerpFloat(from.mCubeMapIncidenceAngleMax, to.mCubeMapIncidenceAngleMax, rate);
    return result;
}

void OceanData::readFromYaml(const char*, const al::ByamlIter&) {}
