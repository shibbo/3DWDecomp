#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
#include <prim/seadEnum.h>
#include <prim/seadSafeString.h>
#include <utility/aglParameter.h>
#include <utility/aglParameterIO.h>
#include <utility/aglParameterObj.h>

#include "Library/Nerve/IUseNerve.hpp"
#include "Project/AreaObj/IUseAreaObj.hpp"

namespace al {
class AreaObj;
class AreaObjDirector;
class GraphicsSystemInfo;
class NerveKeeper;
class PlayerHolder;
class SceneCameraInfo;

SEAD_ENUM(GraphicsAreaParamType, Bloom, DirectionalLight, CubeMapCapturePoint, Exposure,
          DepthShadow, AlphaMaskProjection, GraphicsStress, MirrorRendering, Ssao,
          ColorCorrection, GodRay, FlareFilter, Fog, YFog, LightStreak, HdrCompose, Ssii, SdwMask,
          AtmosScatter, Skybox, Water)

/**
 * Parameter names a graphics area currently provides, together with the interpolation state.
 */
class CurrentGraphicsAreaParam {
public:
    CurrentGraphicsAreaParam();

    const char* mParamName = nullptr;
    const char* mPrevParamName = nullptr;
    f32 mRate = 1.0f;
    s32 _14 = 0;  // lerp step
    bool mIsLerp = false;
    bool mIsNoParam = false;  // set on the first frame of a lerp
    s32 mPriority = -1;
};

static_assert(sizeof(CurrentGraphicsAreaParam) == 0x20);

/**
 * Per graphics area parameter set: one parameter name per GraphicsAreaParamType.
 */
class GraphicsAreaInfo : public agl::utl::IParameterObj {
public:
    typedef agl::utl::Parameter<sead::FixedSafeString<32>> ParamNameParameter;

    GraphicsAreaInfo(const AreaObj* pAreaObj);

    s32 getLerpStep() const { return **mLerpStep; }

    bool isUsePrevLerpStep() const { return **mIsUsePrevLerpStep; }

    bool isForceCameraAreaFindMode() const { return **mIsForceCameraAreaFindMode; }

    const char* getParamName(GraphicsAreaParamType type) const {
        return (*mParamNames[type])->cstr();
    }

    const char* getName() const { return mName.cstr(); }

    const AreaObj* getAreaObj() const { return mAreaObj; }

private:
    agl::utl::Parameter<s32>* mLerpStep = nullptr;
    agl::utl::Parameter<bool>* mIsUsePrevLerpStep = nullptr;
    agl::utl::Parameter<bool>* mIsForceCameraAreaFindMode = nullptr;
    sead::PtrArray<ParamNameParameter> mParamNames;
    sead::FixedSafeString<32> mName;
    const AreaObj* mAreaObj;
};

static_assert(sizeof(GraphicsAreaInfo) == 0x98);

/**
 * Tracks which graphics area is active and interpolates between the parameters of the areas.
 */
class GraphicsAreaDirector : public IUseAreaObj, public IUseNerve, public agl::utl::IParameterIO {
public:
    GraphicsAreaDirector(const GraphicsSystemInfo* pSystemInfo);

    void init(AreaObjDirector* pAreaObjDirector, const SceneCameraInfo* pCameraInfo,
              const PlayerHolder* pPlayerHolder);
    void setStageName(const char* pStageName, s32 scenarioNo);
    void initAfterPlacement();
    s32 getGraphicsAreaNum() const;
    void endInit();
    void update();
    GraphicsAreaInfo* findInfo(const AreaObj* pAreaObj) const;
    void getCurrentGraphicsAreaParam(CurrentGraphicsAreaParam* pParam,
                                     GraphicsAreaParamType type) const;
    GraphicsAreaInfo* getGraphicsAreaInfoByTrans(const sead::Vector3f& rTrans) const;
    GraphicsAreaInfo* getGraphicsAreaInfoByIndex(s32 index) const;
    bool isLerp() const;
    void cancelLerp();

    void exeWait();
    void exeLerpPause();
    void exeLerp();

    AreaObjDirector* getAreaObjDirector() const override { return mAreaObjDirector; }

    NerveKeeper* getNerveKeeper() const override { return mNerveKeeper; }

    bool isLerpPaused() const { return mIsLerpPaused; }
    void setLerpStep(s32 step) { mLerpStep = step; }
    void setLerpRate(f32 rate) { mLerpRate = rate; }
    void setLerpPaused(bool paused) { mIsLerpPaused = paused; }

    /**
     * @brief Locks the current graphics area (used when the player dies).
     */
    void lockArea() { mIsLockArea = true; }

private:
    const GraphicsSystemInfo* mSystemInfo;
    const char* mStageName = nullptr;
    GraphicsAreaInfo** mInfos = nullptr;
    s32 mInfoNum = 0;
    GraphicsAreaInfo* mCurrentInfo = nullptr;
    GraphicsAreaInfo* mPrevInfo = nullptr;
    AreaObjDirector* mAreaObjDirector = nullptr;
    AreaObj* mCurrentArea = nullptr;
    const SceneCameraInfo* mCameraInfo = nullptr;
    const PlayerHolder* mPlayerHolder = nullptr;
    NerveKeeper* mNerveKeeper = nullptr;
    s32 mLerpStep = -1;
    f32 mLerpRate = -1.0f;
    bool mIsLerpPaused = false;
    s32 mLerpFrame;
    s32 mPausedLerpFrame;
    s32 mScenarioNo = 0;
    bool mIsLockArea = false;
};

static_assert(sizeof(GraphicsAreaDirector) == 0x2a8);

}  // namespace al
