#pragma once
#include "Library/Scene/SceneFunction.hpp"
#include "Scene/PhaseBossScene.hpp"
class ProjectSceneFactory : public alSceneFunction::SceneFactory {
public:
    ProjectSceneFactory();
    PhaseBossScene::MemorySceneHeapCustomAlloc* tryGetCustomAlloc(const char* name) override;
};
