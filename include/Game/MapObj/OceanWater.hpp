#pragma once

#include <basis/seadTypes.h>
#include <common/aglGPUMemBlock.h>
#include <common/aglTextureData.h>
#include <gfx/seadColor.h>
#include <math/seadVector.h>
#include <thread/seadMessageQueue.h>

#include "MapObj/Ocean.hpp"
#include "MapObj/OceanData.hpp"
#include "MapObj/OceanWaveGenerator.hpp"
#include "MapObj/WaveGrid.hpp"
#include "Project/OceanWave/OceanWaveDirector.hpp"

namespace agl {
class RenderBuffer;
class ShaderProgram;
class VertexAttribute;
class VertexBuffer;
}  // namespace agl

namespace al {
class ActorInitInfo;
class FullScreenQuadModel;
class HitSensor;
class IUseSceneObjHolder;
class SensorMsg;
class UniformBlock;
}  // namespace al

namespace sead {
class DelegateThread;
class Thread;
}  // namespace sead

struct OceanVertex;

/**
 * @brief The dynamic ocean surface of Bowser's Fury (scene object), drawn from a wave grid that a
 * worker thread regenerates every frame.
 */
class OceanWater : public al::OceanWaveDirector {
  public:
    /**
     * @brief A named set of ocean looks, one for the normal and one for the disaster mode.
     */
    struct Params {
        const char* mName;
        OceanData mModes[2];
    };

    static_assert(sizeof(Params) == 0x238);

    enum RenderMode : s32 {
        RenderMode_Forward = 0,
        RenderMode_Indirect = 1,
        RenderMode_Deferred = 2,
    };

    static OceanWater* getOceanWater(const al::IUseSceneObjHolder* pHolder);

    OceanWater(const char* pName);
    ~OceanWater() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void threadFunc_(sead::Thread* pThread, s64 message);
    void setSeaCenterPosition(long unused, f32* pX, f32* pZ, f32* pDirX, f32* pDirZ);
    void initAfterPlacement() override;
    void initFromYaml(const al::ByamlIter& rIter, const char* pName) override;
    void initModeFromYaml(const char* pName, OceanData& rData, const al::ByamlIter& rIter);
    Params* findParamsByName(const char* pName) const;
    void initColorFromYaml(const char* pName, sead::Color4f& rColor, const al::ByamlIter& rIter);
    void initMaterialFromYaml(const char* pName, sead::Vector2f& rUV, sead::Vector2f& rUVScroll,
                              const al::ByamlIter& rIter);
    void initMaterialFromYaml(const char* pName, sead::Vector4f& rUV, f32& rUVRot,
                              f32& rIntensity, const al::ByamlIter& rIter);
    void initRefractionFromYaml(const char* pName, sead::Color4f& rColor, f32& rFactor,
                                f32& rFadeHeight, const al::ByamlIter& rIter);
    void initReflectionFromYaml(const char* pName, OceanData& rData, const al::ByamlIter& rIter);
    bool createWave(const al::LiveActor* pActor, const al::OceanWaveInfo* pInfo) override;
    f32 getY(const sead::Vector3f& rPos) override;
    void kill() override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void draw() const override;
    void drawNormals() const;
    void drawReflections() const;
    void drawCompose(const agl::RenderBuffer* pRenderBuffer) const;
    void drawDeffered() const;
    void bindUniformBlock(u32 index) const;
    void bindTextures(const agl::ShaderProgram* pProgram) const;
    void execute() override;
    void receiveThreadMessage();
    void update(bool isMovement);
    void movement() override;
    void movementPaused(bool isPaused) override;
    void hide() override;
    void show() override;
    void setHeightMapScale(f32 scale, bool isImmediate);
    void setupStreams(agl::VertexBuffer* pBuffer, agl::VertexAttribute* pAttribute);
    void calculateNormals(agl::GPUMemBlock<OceanVertex>& rVertices);
    void calculateUVs();
    void bindTexturesDeferred(const agl::ShaderProgram* pProgram) const;

    /**
     * @brief Get the name of the scene object.
     * @return The scene object name.
     */
    const char* getSceneObjName() const override { return "OceanWater"; }

    /**
     * @brief Get the render type of the ocean.
     * @return Always 0.
     */
    s32 getRenderType() const override { return 0; }

    /**
     * @brief Set the stage the ocean is in (unused).
     * @param pStageName stage name
     */
    void setStageName(const char* pStageName) override {}

    /**
     * @brief Get the water height at a position.
     * @param rPos position
     * @param isIgnoreWave whether to ignore the waves
     * @return Always 0.
     */
    virtual f32 getWaterHeight(const sead::Vector3f& rPos, bool isIgnoreWave) { return 0.0f; }

  protected:
    sead::Vector3f mFlowDir = {0.0f, 0.0f, 0.0f};                // 0x158
    sead::Vector2f _164;                                         // 0x164
    WaveGrid::Data mGridData;                                    // 0x170
    u32 mBufferIndex = 0;                                        // 0x23bd90
    al::UniformBlock* mUniformBlocks[2];                         // 0x23bd98
    f32 mReflectionRayMaxStep;                                   // 0x23bda8
    f32 mReflectionRayStartScale;                                // 0x23bdac
    f32 mReflectionRayScaleFactor;                               // 0x23bdb0
    bool mIsGaussianBlur;                                        // 0x23bdb4
    f32 mWaveFactorDampDistMin;                                  // 0x23bdb8
    f32 mWaveFactorDampDistMax;                                  // 0x23bdbc
    s32 mCubeMapIndex = -1;                                      // 0x23bdc0
    s32 mDisasterCubeMapIndex = -1;                              // 0x23bdc4
    s32 mDeferredCubeMapIndex = -1;                              // 0x23bdc8
    Params* mParams = nullptr;                                   // 0x23bdd0
    s32 mParamsNum = 0;                                          // 0x23bdd8
    Params* mCurrentParams = nullptr;                            // 0x23bde0
    OceanData mOceanData;                                        // 0x23bde8
    f32 mHeightMapScale = 0.0f;                                  // 0x23bf00
    f32 mTargetHeightMapScale = 1.0f;                            // 0x23bf04
    bool _23bf08;                                                // 0x23bf08
    u32 mNormalModeIndex;                                        // 0x23bf0c
    u32 mDisasterModeIndex;                                      // 0x23bf10
    f32 mDisasterRate = 0.0f;                                    // 0x23bf14
    u32 mTextureSetIndex = 0;                                    // 0x23bf18
    sead::Vector2f mAlbedoScroll = {0.0f, 0.0f};                 // 0x23bf1c
    sead::Vector2f mNormalMap1Scroll = {0.0f, 0.0f};             // 0x23bf24
    sead::Vector2f mNormalMap2Scroll = {0.0f, 0.0f};             // 0x23bf2c
    bool mIsVisible;                                             // 0x23bf34
    bool mIsFirstUpdate = true;                                  // 0x23bf35
    sead::DelegateThread* mThread;                               // 0x23bf38
    u32 mThreadBufferIndex = 0;                                  // 0x23bf40
    sead::MessageQueue mMessageQueue;                            // 0x23bf48
    const agl::VertexAttribute* mCurrentVertexAttributes;        // 0x23bf98
    s32 _23bfa0 = 0;                                             // 0x23bfa0
    Ocean mOcean;                                                // 0x23bfa8
    OceanWaveGenerator mGenerator;                               // 0x288890
    s32 mPolygonModeFront;                                       // 0x2892d8
    s32 mPolygonModeBack;                                        // 0x2892dc
    s32 _2892e0;                                                 // 0x2892e0
    s32 _2892e4;                                                 // 0x2892e4
    s32 mReflectionType;                                         // 0x2892e8
    agl::ShaderProgram* mShader;                                 // 0x2892f0
    agl::ShaderProgram* mNormalsShader;                          // 0x2892f8
    agl::ShaderProgram* mReflectionsShader;                      // 0x289300
    agl::ShaderProgram* mComposeShader;                          // 0x289308
    agl::ShaderProgram* mDeferredShader;                         // 0x289310
    al::FullScreenQuadModel* mQuadModel;                         // 0x289318
    RenderMode mRenderMode;                                      // 0x289320
    agl::TextureData mTextures[2][3];                            // 0x289328
    agl::TextureData mInkEdgeTexture;                            // 0x289a18
    agl::TextureData mSeaFoamTexture;                            // 0x289b40
    f32 mSeaY = 0.0f;                                            // 0x289c68
    s16 mSeaOffset;                                              // 0x289c6c
    u32 mFrame;                                                  // 0x289c70
    al::LiveActor* mPlayer;                                      // 0x289c78
    f32 _289c80 = -1.0f;                                         // 0x289c80
};
