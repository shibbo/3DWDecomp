#include "Scene/ProjectSceneFactoryEntries.hpp"
#include "Scene/BootScene.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "Scene/DemoEndingScene.hpp"
#include "Scene/DemoSingleModeEndingScene.hpp"
#include "Scene/DemoOpeningScene.hpp"
#include "Scene/DemoScene.hpp"
#include "Scene/DemoSceneSingleMode.hpp"
#include "Scene/SingleModeScene.hpp"
#include "Scene/IslandScene.hpp"
#include "Scene/Phase0Scene.hpp"
#include "Scene/PhaseScene.hpp"
#include "Scene/PhasePlessieChaseScene.hpp"
#include "Scene/PhaseBossScene.hpp"
#include "Scene/StageScene.hpp"
#include "Scene/TitleScene.hpp"

namespace alSceneFunction {
template <typename T> al::Scene* createSceneFunc() { return new T; }
template al::Scene* createSceneFunc<BootScene>();
template <> al::Scene* createSceneFunc<CourseSelectScene>() { return new CourseSelectScene(false); }
template al::Scene* createSceneFunc<DemoEndingScene>();
template al::Scene* createSceneFunc<DemoSingleModeEndingScene>();
template al::Scene* createSceneFunc<DemoOpeningScene>();
template <> al::Scene* createSceneFunc<DemoScene>() { return new DemoScene(false, false); }
template al::Scene* createSceneFunc<DemoSceneSingleMode>();
template <> al::Scene* createSceneFunc<SingleModeScene>() {
    return new SingleModeScene("SingleModeScene");
}
template al::Scene* createSceneFunc<IslandScene>();
template al::Scene* createSceneFunc<Phase0Scene>();
template al::Scene* createSceneFunc<PhaseScene>();
template al::Scene* createSceneFunc<PhasePlessieChaseScene>();
template al::Scene* createSceneFunc<PhaseBossScene>();
template <> al::Scene* createSceneFunc<StageScene>() { return new StageScene(nullptr); }
template <> al::Scene* createSceneFunc<TitleScene>() { return new TitleScene(nullptr, 0); }
} // namespace alSceneFunction
