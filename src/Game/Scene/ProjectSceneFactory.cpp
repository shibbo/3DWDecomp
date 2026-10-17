#include "Scene/ProjectSceneFactory.hpp"
#include "Scene/ProjectSceneFactoryEntries.hpp"
#include "Scene/PhaseBossScene.hpp"
#include "Project/Base/StringUtil.hpp"
class BootScene;
class CourseSelectScene;
class DemoEndingScene;
class DemoSingleModeEndingScene;
class DemoOpeningScene;
class DemoScene;
class DemoSceneSingleMode;
class IslandScene;
class Phase0Scene;
class PhaseScene;
class PhasePlessieChaseScene;
class StageScene;
class TitleScene;
namespace {
const al::NameToCreator<alSceneFunction::SceneFunction> sSceneEntries[] = {
    {"BootScene", alSceneFunction::createSceneFunc<BootScene>},
    {"CourseSelectScene", alSceneFunction::createSceneFunc<CourseSelectScene>},
    {"DemoEndingScene", alSceneFunction::createSceneFunc<DemoEndingScene>},
    {"DemoSingleModeEndingScene", alSceneFunction::createSceneFunc<DemoSingleModeEndingScene>},
    {"DemoOpeningScene", alSceneFunction::createSceneFunc<DemoOpeningScene>},
    {"DemoScene", alSceneFunction::createSceneFunc<DemoScene>},
    {"DemoSceneSingleMode", alSceneFunction::createSceneFunc<DemoSceneSingleMode>},
    {"SingleModeScene", alSceneFunction::createSceneFunc<SingleModeScene>},
    {"IslandScene", alSceneFunction::createSceneFunc<IslandScene>},
    {"Phase0Scene", alSceneFunction::createSceneFunc<Phase0Scene>},
    {"PhaseScene", alSceneFunction::createSceneFunc<PhaseScene>},
    {"Phase1Scene", alSceneFunction::createSceneFunc<PhaseScene>},
    {"Phase2Scene", alSceneFunction::createSceneFunc<PhaseScene>},
    {"Phase3Scene", alSceneFunction::createSceneFunc<PhaseScene>},
    {"PhasePlessieChaseScene", alSceneFunction::createSceneFunc<PhasePlessieChaseScene>},
    {"PhaseBossScene", alSceneFunction::createSceneFunc<PhaseBossScene>},
    {"StageScene", alSceneFunction::createSceneFunc<StageScene>},
    {"TitleScene", alSceneFunction::createSceneFunc<TitleScene>},
};
}
ProjectSceneFactory::ProjectSceneFactory() : alSceneFunction::SceneFactory("シーン生成") {
    initFactory(sSceneEntries);
}
PhaseBossScene::MemorySceneHeapCustomAlloc*
ProjectSceneFactory::tryGetCustomAlloc(const char* name) {
    if (al::isEqualString(name, "PhaseBossScene") || al::isEqualString(name, "PhaseBossV2Scene"))
        return &PhaseBossScene::sCustomAlloc;
    return nullptr;
}
