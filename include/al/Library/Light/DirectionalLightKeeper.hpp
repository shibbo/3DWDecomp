#pragma once

#include <container/seadPtrArray.h>
#include <gfx/seadColor.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
#include "utility/aglParameter.h"
#include "utility/aglParameterIO.h"
#include "utility/aglParameterObj.h"

#include "Library/Light/DirectionParam.hpp"
#include "Project/Base/RequestInterp.hpp"

namespace al {
class GraphicsParamFilePath;
class GraphicsSystemInfo;
class LiveActor;
class Resource;

class DirLightParam {
public:
    DirLightParam() {
        init({-1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, 1.0f}, {1.0f, 0.75f, 0.5f, 1.0f},
             {5.5f, 5.3f, 5.0f, 1.0f}, 80.0f);
    }

    void init(const sead::Vector3f& rDir, const sead::Vector3f& rSpcDir,
              const sead::Color4f& rColor, const sead::Color4f& rSpcColor, f32 spcPower);
    const sead::Vector3f& getDirectionFrom() const;
    const sead::Vector3f& getSpcDirectionFrom() const;
    bool operator==(const DirLightParam& rOther) const;
    DirLightParam& operator=(const DirLightParam& rOther);
    void interp(const DirLightParam& rA, const DirLightParam& rB, f32 rate);

    DirectionParam* getDirection() const { return mDirection; }

    DirectionParam* getSpcDirection() const { return mSpcDirection; }

    sead::Color4f& getColor() { return *mColor; }

    const sead::Color4f& getColor() const { return *mColor; }

    sead::Color4f& getSpcColor() { return *mSpcColor; }

    const sead::Color4f& getSpcColor() const { return *mSpcColor; }

    f32& getSpcPower() { return *mSpcPower; }

    f32 getSpcPower() const { return *mSpcPower; }

    agl::utl::ParameterObj* getParamObj() { return &mParamObj; }

    void syncToDirection() {
        mDirection->syncToDirection();
        mSpcDirection->syncToDirection();
    }

private:
    DirectionParam* mDirection;
    DirectionParam* mSpcDirection;
    agl::utl::Parameter<sead::Color4f> mColor;
    agl::utl::Parameter<sead::Color4f> mSpcColor;
    agl::utl::Parameter<f32> mSpcPower;
    agl::utl::ParameterObj mParamObj;
};

static_assert(sizeof(DirLightParam) == 0xb0);

class NamedDirLightParam : public DirLightParam {
public:
    NamedDirLightParam();

    const char* getName() const { return mName->cstr(); }

    sead::FixedSafeString<64>& getNameString() { return *mName; }

private:
    agl::utl::Parameter<sead::FixedSafeString<64>> mName;
};

static_assert(sizeof(NamedDirLightParam) == 0x120);

class DirectionalLightKeeper {
public:
    DirectionalLightKeeper(GraphicsSystemInfo* pGraphicsSystemInfo);

    void endInit();
    const sead::Vector3f& getLightDirFrom() const;
    const sead::Vector3f& getSpecularLightDirFrom() const;
    void clearRequest();
    void execute();
    NamedDirLightParam* findDirLightParamByName(const char* pName) const;
    void requestDirectionalLight(s32 priority, s32 step, const DirLightParam& rParam);
    const DirLightParam* tryGetNamedOrCurrentDirLight(const char* pName) const;
    const DirLightParam* tryGetNamedOrDefaultDirLight(const char* pName) const;
    NamedDirLightParam* getDirLightByIndex(s32 index);
    void initStageResource(const Resource* pResource, const char* pStageName);

    const sead::Color4f& getCurrentColor() const { return mInterp.getCurrentParam().getColor(); }

    const DirLightParam& getCurrentParam() const { return mInterp.getCurrentParam(); }

    DirLightParam& getCurrentParam() { return mInterp.getCurrentParam(); }

private:
    using NamedParamArray = sead::FixedPtrArray<NamedDirLightParam, 64>;

    bool mIsLoaded = false;
    RequestInterp<DirLightParam> mInterp;
    GraphicsSystemInfo* mGraphicsSystemInfo;
    DirLightParam mDefaultParam;
    agl::utl::Parameter<bool> mIsEnableDefaultParam;
    NamedParamArray mNamedParams;
    agl::utl::IParameterIO mParamIO;
    GraphicsParamFilePath* mParamFilePath;
};

static_assert(sizeof(DirectionalLightKeeper) == 0x7f0);

}  // namespace al

namespace DirLightFunction {
al::DirectionalLightKeeper* getDirectionalLightKeeper(const al::LiveActor* pActor);
}  // namespace DirLightFunction
