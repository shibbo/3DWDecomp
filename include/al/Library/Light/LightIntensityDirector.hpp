#pragma once

#include <prim/seadSafeString.h>
#include "utility/aglParameter.h"
#include "utility/aglParameterObj.h"

#include "Library/Light/GraphicsNamedParamBase.hpp"
#include "Project/AreaObj/IUseAreaObj.hpp"

namespace agl::pfx {
class Bloom;
class BloomParameter;
}  // namespace agl::pfx

namespace agl::utl {
class IParameterIO;
}

namespace al {
class ExposureParam : public GraphicsNamedParamBase {
public:
    ExposureParam(bool isDefault);

    s32 getParamType() const override;

    f32 getExposure() const { return **mExposure; }

private:
    agl::utl::Parameter<f32>* mExposure = nullptr;
};

static_assert(sizeof(ExposureParam) == 0x40);

class BloomNamedParam : public GraphicsNamedParamBase {
public:
    BloomNamedParam(bool isDefault);

    s32 getParamType() const override;

    f32 getReduceScale() const { return *mReduceScale; }
    const agl::pfx::BloomParameter* getBloomParameter() const { return mBloomParameter; }
    bool isApplied() const { return _68; }
    void setApplied(bool isApplied) { _68 = isApplied; }
    void setReduceScale(f32 scale) { *mReduceScale = scale; }

private:
    agl::pfx::BloomParameter* mBloomParameter = nullptr;
    agl::utl::Parameter<f32> mReduceScale;
    agl::utl::Parameter<bool>* mEnable = nullptr;
    bool _68 = true;
};

static_assert(sizeof(BloomNamedParam) == 0x70);

class AreaObjDirector;
class CurrentGraphicsAreaParam;
class GraphicsAreaDirector;
class PlayerHolder;

class LightIntensityDirector : public IUseAreaObj {
public:
    LightIntensityDirector(AreaObjDirector* pAreaObjDirector, const PlayerHolder* pPlayerHolder);

    void initGraphicsAreaParam(GraphicsAreaDirector* pGraphicsAreaDirector, const char* pStageName);
    void endInit();
    void execute();
    void updateExposure();
    void updateBloom();
    f32 getExposure() const;
    f32 getExposureExp() const;
    const BloomNamedParam* getCurrentParam() const;
    void applyBloomParameter(agl::pfx::Bloom* pBloom, s32 context) const;
    f32 getCurrentReduceScale() const;
    ExposureParam* findExposureParam(const char* pName) const;
    BloomNamedParam* findBloomParam(const char* pName) const;

    AreaObjDirector* getAreaObjDirector() const override { return mAreaObjDirector; }

    bool isLoadedBloomParam() const { return mIsLoadedBloomParam; }

    void setForceBloomParam(const BloomNamedParam* pParam) { mForceBloomParam = pParam; }

private:
    GraphicsAreaDirector* mGraphicsAreaDirector = nullptr;
    s32 mParamNum = 0;
    agl::utl::IParameterIO* mExposureParamIO = nullptr;
    ExposureParam** mExposureParams = nullptr;
    ExposureParam* mDefaultExposureParam = nullptr;
    ExposureParam* mCurrentExposureParam = nullptr;
    ExposureParam* mPrevExposureParam = nullptr;
    bool mIsLoadedExposureParam = false;
    CurrentGraphicsAreaParam* mExposureAreaParam;
    f32 mForceExposure = -1.0f;
    agl::utl::IParameterIO* mBloomParamIO = nullptr;
    BloomNamedParam** mBloomParams = nullptr;
    BloomNamedParam* mLerpBloomParam;
    BloomNamedParam* mPrevLerpBloomParam;
    BloomNamedParam* mCurrentBloomParam = nullptr;
    BloomNamedParam* mPrevBloomParam = nullptr;
    bool mIsLoadedBloomParam = false;
    CurrentGraphicsAreaParam* mBloomAreaParam;
    const BloomNamedParam* mForceBloomParam = nullptr;
    bool mIsEndInit = false;
    AreaObjDirector* mAreaObjDirector;
};

static_assert(sizeof(LightIntensityDirector) == 0xb0);
}  // namespace al
