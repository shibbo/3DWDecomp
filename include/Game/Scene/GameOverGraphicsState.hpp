#pragma once

#include <basis/seadTypes.h>
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Fog/FogDirector.hpp"
#include "Library/Fog/FogParam.hpp"
#include "Library/Fog/YFogParam.hpp"
#include "Library/Light/DirectionalLightKeeper.hpp"
#include "Library/Light/LightIntensityDirector.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/Shader/ForwardRendering/CubeMapKeeper.hpp"
#include "Library/Shadow/DepthShadowParam.hpp"
#include "Library/Shadow/ShadowDirector.hpp"
#include "Project/Base/RequestInterp.hpp"

/**
 * @brief Graphics settings (bloom, cube map, light, fog and depth shadow) forced while the
 *        game over demo and menu are shown.
 */
class GameOverGraphicsState {
public:
    /**
     * Constructs the game over graphics settings.
     * @param pScene The scene whose graphics are overridden.
     */
    GameOverGraphicsState(al::Scene* pScene)
        : mScene(pScene), mBloomParam(false),
          mDirLightParam({-0.219846f, -0.766044f, -0.604023f}, {-0.57735f, -0.57735f, 0.57735f},
                         {1.8f, 1.3f, 1.1f, 1.0f}, {5.5f, 5.3f, 5.0f, 1.0f}, 80.0f) {
        mFogParam.init();
        mYFogParam.init();
        mDepthShadowParam.init();
        *mDepthShadowParam._130 = 1;
        *mDepthShadowParam._50 = true;
        *mDepthShadowParam._30 = true;
        *mDepthShadowParam._270 = 500.0f;
        *mDepthShadowParam._90 = false;
        *mDepthShadowParam._290 = 1000.0f;
    }

    /**
     * Requests the game over graphics settings for the current frame.
     */
    void setGraphicsState() {
        al::GraphicsSystemInfo* pInfo = mScene->getLiveActorKit()->getGraphicsSystemInfo();
        pInfo->getLightIntensityDirector()->setForceBloomParam(&mBloomParam);
        al::ShaderCubeMapKeeper* pCubeMapKeeper = pInfo->getShaderCubeMapKeeper();
        pCubeMapKeeper->setForceCubeMapInfo(pCubeMapKeeper->findCubeMapInfoByName("GameOver"));
        pInfo->getDirectionalLightKeeper()->requestDirectionalLight(10000, 0, mDirLightParam);
        pInfo->getFogDirector()->requestFog(10000, 0, mFogParam);
        pInfo->getFogDirector()->requestYFog(10000, 0, mYFogParam);
        pInfo->getShadowDirector()->requestDepthShadowParam(10000, 0, mDepthShadowParam);
    }

private:
    al::Scene* mScene;
    al::BloomNamedParam mBloomParam;
    al::DirLightParam mDirLightParam;
    al::FogParam mFogParam;
    al::YFogParam mYFogParam;
    al::DepthShadowParam mDepthShadowParam;
    f32 _860 = 2.0f;
};

static_assert(sizeof(GameOverGraphicsState) == 0x868);
