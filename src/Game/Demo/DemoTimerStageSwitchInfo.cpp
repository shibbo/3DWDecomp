#include "Demo/DemoTimerStageSwitchInfo.hpp"

#include "Demo/DemoPlayerController.hpp"
#include "Demo/DemoPlayerControllerHolder.hpp"
#include "Demo/DemoPlayerModelDirector.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "Demo/DemoScenePlayerModel.hpp"
#include "Demo/DemoTimerStageSwitchController.hpp"
#include "Demo/ProjectDemoDirector.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/File/FileUtil.hpp"
#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Stage/StageResourceList.hpp"
#include "Library/ActorUtil.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerModel.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/InputUtil.hpp"

namespace {
const char* const cSwitchNames[] = {"Switch1On", "Switch2On", "Switch3On", "Switch4On",
                                    "Switch5On", "Switch6On", "Switch7On", "Switch8On"};

/**
 * @brief Gets the project demo director of an actor's scene.
 * @param pActor Actor in the scene.
 * @return The scene's demo director.
 */
ProjectDemoDirector* getDemoDirector(const al::LiveActor* pActor) {
    return static_cast<ProjectDemoDirector*>(pActor->getSceneInfo()->demoDirector);
}

/**
 * @brief Gets the project demo director of a scene.
 * @param pScene The scene.
 * @return The scene's demo director.
 */
ProjectDemoDirector* getDemoDirector(const al::Scene* pScene) {
    return static_cast<ProjectDemoDirector*>(pScene->getDemoDirector());
}

/**
 * @brief Gets the demo player controller holder of an actor's scene.
 * @param pActor Actor in the scene.
 * @return The demo player controller holder.
 */
DemoPlayerControllerHolder* getControllerHolder(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->getPlayerControllerHolder();
}

/**
 * @brief Gets the demo player controller holder of a scene.
 * @param pScene The scene.
 * @return The demo player controller holder.
 */
DemoPlayerControllerHolder* getControllerHolder(const al::Scene* pScene) {
    return getDemoDirector(pScene)->getPlayerControllerHolder();
}

/**
 * @brief Gets the demo player model director of an actor's scene.
 * @param pActor Actor in the scene.
 * @return The demo player model director.
 */
DemoPlayerModelDirector* getModelDirector(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->getPlayerModelDirector();
}
}  // namespace

/**
 * @brief Reads one scheduled switch transition.
 * @param rInfo Linked transition placement.
 */
DemoTimerStageSwitchInfo::DemoTimerStageSwitchInfo(const al::PlacementInfo& rInfo) {
    int switchId = 0;
    al::tryGetArg(&switchId, rInfo, "SwitchId");
    mSwitchName = cSwitchNames[switchId];
    al::tryGetArg(&mStep, rInfo, "Step");
    al::tryGetArg(&mIsOn, rInfo, "IsOn");
}

/** @brief Destroys the controller. */
DemoTimerStageSwitchController::~DemoTimerStageSwitchController() = default;

namespace rc {

/**
 * @brief Starts a demo scene.
 * @param pHolder The demo scene.
 * @return Always true.
 */
bool tryStartDemo(DemoSceneActorHolder* pHolder) {
    pHolder->appear();
    return true;
}

/**
 * @brief Ends a demo scene.
 * @param pHolder The demo scene.
 * @return Always true.
 */
bool tryEndDemo(DemoSceneActorHolder* pHolder) {
    pHolder->kill();
    return true;
}

/**
 * @brief Sets how many frames the camera interpolates when the demo ends.
 * @param pHolder The demo scene.
 * @param frames Interpolation frames.
 */
void setEndCameraInterpolateFrame(DemoSceneActorHolder* pHolder, int frames) {
    pHolder->setEndCameraInterpolateFrame(frames);
}

/**
 * @brief Creates a demo scene from its stage archive if the archive exists.
 * @param pName Demo stage name.
 * @param rInfo Actor initialization data.
 * @param pSelector Player retargetting selector for the demo players.
 * @param pBaseMtx Base matrix of the demo, or nullptr.
 * @param isUseFigure Whether the demo uses the players' figures.
 * @param maxPlayers Maximum number of demo players.
 * @return The demo scene, or nullptr when the archive or its object list is missing.
 */
DemoSceneActorHolder* tryCreateDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
                                               PlayerRetargettingSelector* pSelector,
                                               const sead::Matrix34f* pBaseMtx, bool isUseFigure,
                                               int maxPlayers) {
    al::StringTmp<128> archivePath;
    al::makeStageDataArchivePath(&archivePath, pName, 1, "Map",
                                 al::isOneStageDataArchiveExists(pName));
    al::StringTmp<128> fileName("%sMap", pName);

    if (al::isExistArchive(archivePath)) {
        auto* holder = new DemoSceneActorHolder(maxPlayers);
        al::Resource* resource = al::findOrCreateResource(archivePath, nullptr);
        al::PlacementInfo placement;

        if (al::tryGetPlacementInfo(&placement, resource, fileName.cstr(), "DemoObjList")) {
            holder->initPlacementDemoActor(placement, rInfo, pSelector, pBaseMtx, isUseFigure);
            return holder;
        }
    }

    return nullptr;
}

/**
 * @brief Creates a demo scene from its stage archive.
 * @param pName Demo stage name.
 * @param rInfo Actor initialization data.
 * @param pSelector Player retargetting selector for the demo players.
 * @param pBaseMtx Base matrix of the demo, or nullptr.
 * @param isUseFigure Whether the demo uses the players' figures.
 * @param maxPlayers Maximum number of demo players.
 * @return The demo scene, or nullptr when it could not be created.
 */
DemoSceneActorHolder* createDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
                                            PlayerRetargettingSelector* pSelector,
                                            const sead::Matrix34f* pBaseMtx, bool isUseFigure,
                                            int maxPlayers) {
    return tryCreateDemoSceneHolder(pName, rInfo, pSelector, pBaseMtx, isUseFigure, maxPlayers);
}

/**
 * @brief Creates a demo scene that only consists of layouts.
 * @param pName Demo name.
 * @param rInfo Actor initialization data.
 * @param pSelector Unused.
 * @param pBaseMtx Unused.
 * @param isUseFigure Unused.
 * @param maxPlayers Maximum number of demo players.
 * @return The demo scene.
 */
DemoSceneActorHolder* createDemoSceneHolderLayoutOnly(const char* pName,
                                                      const al::ActorInitInfo& rInfo,
                                                      PlayerRetargettingSelector* pSelector,
                                                      const sead::Matrix34f* pBaseMtx,
                                                      bool isUseFigure, int maxPlayers) {
    al::StringTmp<128> archivePath;
    auto* holder = new DemoSceneActorHolder(maxPlayers);
    holder->initPlacementDemoLayout(pName, rInfo);
    return holder;
}

/**
 * @brief Sets the player information of a demo scene.
 * @param pHolder The demo scene.
 * @param pInfo Player information.
 */
void setDemoScenePlayerInfo(DemoSceneActorHolder* pHolder, IDemoScenePlayerInfo* pInfo) {
    pHolder->setDemoScenePlayerInfo(pInfo);
}

/**
 * @brief Checks whether an actor's scene has a demo director.
 * @param pActor Actor in the scene.
 * @return True if the scene has a demo director.
 */
bool isDemoDirectorAvailable(const al::LiveActor* pActor) {
    return getDemoDirector(pActor) != nullptr;
}

/**
 * @brief Checks whether a scene has a demo director.
 * @param pScene The scene.
 * @return True if the scene has a demo director.
 */
bool isDemoDirectorAvailable(const al::Scene* pScene) {
    return getDemoDirector(pScene) != nullptr;
}

/**
 * @brief Lets items keep updating during demos.
 * @param pActor Actor in the scene.
 */
void setUpdateItemsInDemo(const al::LiveActor* pActor) {
    ProjectDemoDirector* director = getDemoDirector(pActor);

    if (director != nullptr) {
        director->setUnknownE1(true);
    }
}

/**
 * @brief Lets paused watchers keep updating during demos.
 * @param pActor Actor in the scene.
 */
void setUpdatePausedWatchersInDemo(const al::LiveActor* pActor) {
    ProjectDemoDirector* director = getDemoDirector(pActor);

    if (director != nullptr) {
        director->setUnknownE2(true);
    }
}

/**
 * @brief Tries to cancel a boss demo (never possible).
 * @param pActor Demo actor.
 * @return Always false.
 */
bool tryCancelBossDemo(const al::LiveActor* pActor) {
    return false;
}

/**
 * @brief Checks whether any active user pressed the UI decide button to cancel a stage demo.
 * @param pActor Demo actor.
 * @return True if the demo should be cancelled.
 */
bool tryCancelStageDemo(const al::LiveActor* pActor) {
    if (!GameDataHolderAccessor(pActor).getHolder()->getPlayingFile()->isEnableCancelBossDemo()) {
        return false;
    }

    for (int i = 0; i < getControlUserNumMax(); i++) {
        if (isActiveControlUser(pActor, i) &&
            isPadTriggerUiDecideByPort(getControlUserPortNumber(pActor, i))) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether any active user pressed A to cancel a stage demo.
 * @param pActor Demo actor.
 * @return True if the demo should be cancelled.
 */
bool tryCancelStageDemoStrict(const al::LiveActor* pActor) {
    if (!GameDataHolderAccessor(pActor).getHolder()->getPlayingFile()->isEnableCancelBossDemo()) {
        return false;
    }

    for (int i = 0; i < getControlUserNumMax(); i++) {
        if (isActiveControlUser(pActor, i) &&
            al::isPadTriggerA(getControlUserPortNumber(pActor, i))) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Freezes or resumes the scene update, players included.
 * @param pActor Actor in the scene.
 * @param isFreeze Whether to freeze.
 */
void setUpdateFreeze(const al::LiveActor* pActor, bool isFreeze) {
    auto* holder = al::getSceneObj<GameDataHolder>(pActor, SceneObjID_GameDataHolder);
    holder->setFreezeMode(isFreeze);
    holder->setUnknown62(true);
}

/**
 * @brief Freezes or resumes the scene update, leaving the players running while frozen.
 * @param pActor Actor in the scene.
 * @param isFreeze Whether to freeze.
 */
void setUpdateFreezeButPlayer(const al::LiveActor* pActor, bool isFreeze) {
    auto* holder = al::getSceneObj<GameDataHolder>(pActor, SceneObjID_GameDataHolder);
    holder->setFreezeMode(isFreeze);
    holder->setUnknown62(!isFreeze);
}

/**
 * @brief Remembers that the boss demo was already shown.
 * @param pActor Actor in the scene.
 */
void setAlreadyShowBossDemo(const al::LiveActor* pActor) {
    GameDataHolderAccessor(pActor).getHolder()->getPlayingFile()->setAlreadyShowBossDemo();
}

/**
 * @brief Requests a camera demo and registers the actor as a demo actor.
 * @param pActor Demo actor.
 * @param pName Demo camera name.
 * @return True if the demo started.
 */
bool requestStartDemoCamera(al::LiveActor* pActor, const char* pName) {
    if (getDemoDirector(pActor)->requestStartDemoCamera(pActor, pName)) {
        getDemoDirector(pActor)->addDemoActor(pActor);
        return true;
    }

    return false;
}

/**
 * @brief Ends a camera demo.
 * @param pActor Demo actor.
 */
void requestEndDemoCamera(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoCamera(pActor);
}

/**
 * @brief Requests a moving camera demo.
 * @param pActor Demo actor.
 * @param pName Demo camera name.
 * @param isAddDemoActor Whether to register the actor as a demo actor.
 * @return True if the demo started.
 */
bool requestStartDemoMovingCamera(al::LiveActor* pActor, const char* pName, bool isAddDemoActor) {
    if (getDemoDirector(pActor)->requestStartDemoMovingCamera(pActor, pName)) {
        if (isAddDemoActor) {
            getDemoDirector(pActor)->addDemoActor(pActor);
        }

        return true;
    }

    return false;
}

/**
 * @brief Ends a moving camera demo.
 * @param pActor Demo actor.
 */
void requestEndDemoMovingCamera(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoMovingCamera(pActor);
}

/**
 * @brief Requests an intro demo.
 * @param pActor Demo actor.
 * @param pName Demo name.
 * @param isAddDemoActor Whether to register the actor as a demo actor.
 * @return True if the demo started.
 */
bool requestStartDemoIntro(al::LiveActor* pActor, const char* pName, bool isAddDemoActor) {
    if (getDemoDirector(pActor)->requestStartDemoIntro(pActor, pName)) {
        if (isAddDemoActor) {
            getDemoDirector(pActor)->addDemoActor(pActor);
        }

        return true;
    }

    return false;
}

/**
 * @brief Ends an intro demo.
 * @param pActor Demo actor.
 */
void requestEndDemoIntro(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoIntro(pActor);
}

/**
 * @brief Requests a player demo.
 * @param pActor Demo actor.
 * @return True if the demo started.
 */
bool requestStartDemoPlayer(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->requestStartDemoPlayer(pActor);
}

/**
 * @brief Ends a player demo.
 * @param pActor Demo actor.
 */
void requestEndDemoPlayer(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoPlayer(pActor);
}

/**
 * @brief Requests a player demo without a demo actor.
 * @param pScene The scene.
 * @return True if the demo started.
 */
bool requestStartDemoPlayer(const al::Scene* pScene) {
    return getDemoDirector(pScene)->requestStartDemoPlayer(nullptr);
}

/**
 * @brief Ends a player demo without a demo actor.
 * @param pScene The scene.
 */
void requestEndDemoPlayer(const al::Scene* pScene) {
    getDemoDirector(pScene)->requestEndDemoPlayer(nullptr);
}

/**
 * @brief Requests a binding demo.
 * @param pActor Demo actor.
 * @return True if the demo started.
 */
bool requestStartDemoBinding(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->requestStartDemoBinding(pActor);
}

/**
 * @brief Ends a binding demo.
 * @param pActor Demo actor.
 */
void requestEndDemoBinding(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoBinding(pActor);
}

/**
 * @brief Requests a cutscene demo.
 * @param pActor Demo actor.
 * @return True if the demo started.
 */
bool requestStartDemoCutscene(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->requestStartDemoCutscene(pActor);
}

/**
 * @brief Ends a cutscene demo.
 * @param pActor Demo actor.
 */
void requestEndDemoCutscene(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoCutscene(pActor);
}

/**
 * @brief Requests an in-game cutscene demo.
 * @param pActor Demo actor.
 * @return True if the demo started.
 */
bool requestStartDemoInGameCutscene(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->requestStartDemoInGameCutscene(pActor);
}

/**
 * @brief Ends an in-game cutscene demo.
 * @param pActor Demo actor.
 */
void requestEndDemoInGameCutscene(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoInGameCutscene(pActor);
}

/**
 * @brief Requests a player cutscene demo.
 * @param pActor Demo actor.
 * @return True if the demo started.
 */
bool requestStartDemoPlayerCutscene(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->requestStartDemoPlayerCutscene(pActor);
}

/**
 * @brief Ends a player cutscene demo.
 * @param pActor Demo actor.
 */
void requestEndDemoPlayerCutscene(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->requestEndDemoPlayerCutscene(pActor);
}

/**
 * @brief Checks whether any demo is active.
 * @param pActor Actor in the scene.
 * @return True if a demo is active.
 */
bool isActiveDemo(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemo();
}

/**
 * @brief Checks whether a demo started by this actor is active.
 * @param pActor Demo actor.
 * @return True if the actor's demo is active.
 */
bool isActiveSpecificDemo(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemo(pActor);
}

/**
 * @brief Checks whether a camera demo is active.
 * @param pActor Actor in the scene.
 * @return True if a camera demo is active.
 */
bool isActiveDemoCamera(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoCamera();
}

/**
 * @brief Checks whether a moving camera demo is active.
 * @param pActor Actor in the scene.
 * @return True if a moving camera demo is active.
 */
bool isActiveDemoMovingCamera(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoMovingCamera();
}

/**
 * @brief Checks whether an intro demo is active.
 * @param pActor Actor in the scene.
 * @return True if an intro demo is active.
 */
bool isActiveDemoIntro(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoIntro();
}

/**
 * @brief Checks whether a player demo is active.
 * @param pActor Actor in the scene.
 * @return True if a player demo is active.
 */
bool isActiveDemoPlayer(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoPlayer();
}

/**
 * @brief Checks whether a binding demo is active.
 * @param pActor Actor in the scene.
 * @return True if a binding demo is active.
 */
bool isActiveDemoBinding(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoBinding();
}

/**
 * @brief Checks whether a player cutscene demo is active.
 * @param pActor Actor in the scene.
 * @return True if a player cutscene demo is active.
 */
bool isActiveDemoPlayerCutscene(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoPlayerCutscene();
}

/**
 * @brief Checks whether a cutscene demo is active.
 * @param pActor Actor in the scene.
 * @return True if a cutscene demo is active.
 */
bool isActiveDemoCutscene(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoCutscene();
}

/**
 * @brief Checks whether an in-game cutscene demo is active.
 * @param pActor Actor in the scene.
 * @return True if an in-game cutscene demo is active.
 */
bool isActiveDemoInGameCutscene(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isActiveDemoInGameCutscene();
}

/**
 * @brief Checks whether any demo, including other running demos, is active.
 * @param pActor Actor in the scene.
 * @return True if any demo is active.
 */
bool isAnyActiveDemo(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isAnyActiveDemo();
}

/**
 * @brief Checks whether any demo other than a camera demo is active.
 * @param pActor Actor in the scene.
 * @return True if a non-camera demo is active.
 */
bool isAnyActiveButDemoCameraDemo(const al::LiveActor* pActor) {
    if (isAnyActiveDemo(pActor) && !isActiveDemoCamera(pActor)) {
        return true;
    }

    return false;
}

/**
 * @brief Marks that a demo outside the demo director is running.
 * @param pActor Demo actor.
 * @param isRunning Whether the other demo is running.
 */
void setOtherActiveDemo(al::LiveActor* pActor, bool isRunning) {
    getDemoDirector(pActor)->setIsOtherDemoRunning(pActor, isRunning);
}

/**
 * @brief Registers an actor that keeps updating during demos.
 * @param pActor Demo actor.
 */
void addDemoActor(al::LiveActor* pActor) {
    getDemoDirector(pActor)->addDemoActor(pActor);
}

/**
 * @brief Unregisters an actor that kept updating during demos.
 * @param pActor Demo actor.
 */
void removeDemoActor(al::LiveActor* pActor) {
    getDemoDirector(pActor)->removeDemoActor(pActor);
}

/**
 * @brief Registers a player and its models as demo actors.
 * @param pPlayer The player.
 */
void addDemoPlayer(PlayerActor* pPlayer) {
    pPlayer->addToDemo(getDemoDirector(pPlayer));
}

/**
 * @brief Unregisters a player, its current model and the model's sub actors as demo actors.
 * @param pPlayer The player.
 */
void removeDemoPlayer(PlayerActor* pPlayer) {
    ProjectDemoDirector* director = getDemoDirector(pPlayer);
    director->removeDemoActor(pPlayer);

    PlayerModelHolder* modelHolder = pPlayer->getModelHolder();

    if (modelHolder == nullptr) {
        return;
    }

    PlayerModel* model = modelHolder->getCurrentModel();
    director->removeDemoActor(model);

    al::SubActorKeeper* subActorKeeper = model->getSubActorKeeper();

    if (subActorKeeper == nullptr) {
        return;
    }

    for (int i = 0; i < subActorKeeper->getSubActorNum(); i++) {
        director->removeDemoActor(subActorKeeper->getSubActorInfo(i)->mSubActor);
    }
}

/**
 * @brief Requests that the next demo switch happens immediately.
 * @param pActor Actor in the scene.
 */
void setImmediateSwitchFlag(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->setImmediateDemoSwitch();
}

/**
 * @brief Clears the immediate demo switch request.
 * @param pActor Actor in the scene.
 */
void resetImmediateSwitchFlag(const al::LiveActor* pActor) {
    getDemoDirector(pActor)->resetImmediateDemoSwitch();
}

/**
 * @brief Checks the immediate demo switch request.
 * @param pActor Actor in the scene.
 * @return True if the next demo switch happens immediately.
 */
bool getImmediateSwitchFlag(const al::LiveActor* pActor) {
    return getDemoDirector(pActor)->isImmediateDemoSwitch();
}

/**
 * @brief Gets the number of demo players.
 * @param pActor Actor in the scene.
 * @return Number of demo players.
 */
int getDemoPlayerNum(const al::LiveActor* pActor) {
    return getControllerHolder(pActor)->getDemoPlayerNum();
}

/**
 * @brief Gets a demo player.
 * @param pActor Actor in the scene.
 * @param index Demo player index.
 * @return The demo player.
 */
DemoPlayerController* getDemoPlayer(const al::LiveActor* pActor, int index) {
    return getControllerHolder(pActor)->getDemoPlayer(index);
}

/**
 * @brief Gets the demo player of a character.
 * @param pActor Actor in the scene.
 * @param character Character id.
 * @return The demo player.
 */
DemoPlayerController* getDemoPlayerByCharacter(const al::LiveActor* pActor, int character) {
    return getControllerHolder(pActor)->getDemoPlayerByCharacter(character);
}

/**
 * @brief Gets the demo player of a player actor.
 * @param pActor The player actor.
 * @return The demo player.
 */
DemoPlayerController* getDemoPlayerByActor(const al::LiveActor* pActor) {
    return getControllerHolder(pActor)->getDemoPlayerByActor(pActor);
}

/**
 * @brief Gets the number of demo players.
 * @param pScene The scene.
 * @return Number of demo players.
 */
int getDemoPlayerNum(const al::Scene* pScene) {
    return getControllerHolder(pScene)->getDemoPlayerNum();
}

/**
 * @brief Gets a demo player.
 * @param pScene The scene.
 * @param index Demo player index.
 * @return The demo player.
 */
DemoPlayerController* getDemoPlayer(const al::Scene* pScene, int index) {
    return getControllerHolder(pScene)->getDemoPlayer(index);
}

/**
 * @brief Gets the demo player of a character.
 * @param pScene The scene.
 * @param character Character id.
 * @return The demo player.
 */
DemoPlayerController* getDemoPlayerByCharacter(const al::Scene* pScene, int character) {
    return getControllerHolder(pScene)->getDemoPlayerByCharacter(character);
}

/**
 * @brief Gets the demo player of a player actor.
 * @param pScene The scene.
 * @param pActor The player actor.
 * @return The demo player.
 */
DemoPlayerController* getDemoPlayerByActor(const al::Scene* pScene, const al::LiveActor* pActor) {
    return getControllerHolder(pScene)->getDemoPlayerByActor(pActor);
}

/**
 * @brief Hides every demo player.
 * @param pActor Actor in the scene.
 */
void hideDemoPlayerAll(const al::LiveActor* pActor) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pActor);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->hide();
    }
}

/**
 * @brief Shows every demo player.
 * @param pActor Actor in the scene.
 */
void showDemoPlayerAll(const al::LiveActor* pActor) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pActor);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->show();
    }
}

/**
 * @brief Hides every demo player.
 * @param pScene The scene.
 */
void hideDemoPlayerAll(const al::Scene* pScene) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pScene);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->hide();
    }
}

/**
 * @brief Shows every demo player.
 * @param pScene The scene.
 */
void showDemoPlayerAll(const al::Scene* pScene) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pScene);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->show();
    }
}

/**
 * @brief Stops the skeletal animation and deletes the effects of every demo player.
 * @param pActor Actor in the scene.
 */
void stopSklAnimAndDeleteEffectDemoPlayerAll(const al::LiveActor* pActor) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pActor);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->stopSklAnimAndDeleteEffect();
    }
}

/**
 * @brief Stops the skeletal animation and deletes the effects of every demo player.
 * @param pScene The scene.
 */
void stopSklAnimAndDeleteEffectDemoPlayerAll(const al::Scene* pScene) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pScene);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->stopSklAnimAndDeleteEffect();
    }
}

/**
 * @brief Starts an action on every demo player.
 * @param pActor Actor in the scene.
 * @param pActionName Action name.
 */
void startActionDemoPlayerAll(const al::LiveActor* pActor, const char* pActionName) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pActor);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->startAction(pActionName);
    }
}

/**
 * @brief Sets the action frame of every demo player.
 * @param pActor Actor in the scene.
 * @param frame Action frame.
 */
void setActionDemoFrameAll(const al::LiveActor* pActor, int frame) {
    DemoPlayerControllerHolder* holder = getControllerHolder(pActor);
    int num = holder->getDemoPlayerNum();

    for (int i = 0; i < num; i++) {
        holder->getDemoPlayer(i)->setActionFrame(frame);
    }
}

/**
 * @brief Requests demo models of every player.
 * @param pActor Actor in the scene.
 * @param pSuffix Model suffix.
 */
void requestCreateAllPlayer(const al::LiveActor* pActor, const char* pSuffix) {
    getModelDirector(pActor)->requestCreateAllPlayer(pSuffix);
}

/**
 * @brief Requests demo models of every figure of a character.
 * @param pActor Actor in the scene.
 * @param character Character id.
 * @param pSuffix Model suffix.
 */
void requestCreateAllFigure(const al::LiveActor* pActor, int character, const char* pSuffix) {
    getModelDirector(pActor)->requestCreateAllFigure(character, pSuffix, false);
}

/**
 * @brief Requests the super figure demo model of a character.
 * @param pActor Actor in the scene.
 * @param character Character id.
 * @param pSuffix Model suffix.
 */
void requestCreateFigureSuper(const al::LiveActor* pActor, int character, const char* pSuffix) {
    getModelDirector(pActor)->requestCreateFigureSuper(character, pSuffix);
}

/**
 * @brief Requests the climb figure demo model of a character.
 * @param pActor Actor in the scene.
 * @param character Character id.
 * @param pSuffix Model suffix.
 */
void requestCreateFigureClimb(const al::LiveActor* pActor, int character, const char* pSuffix) {
    getModelDirector(pActor)->requestCreateFigureClimb(character, pSuffix);
}

/**
 * @brief Requests the giant climb figure demo model of a character.
 * @param pActor Actor in the scene.
 * @param character Character id.
 * @param pSuffix Model suffix.
 */
void requestCreateFigureClimbGiga(const al::LiveActor* pActor, int character,
                                  const char* pSuffix) {
    getModelDirector(pActor)->requestCreateFigureClimbGiga(character, pSuffix);
}

/**
 * @brief Gets the demo model of a character and binds it to a demo actor.
 * @param pActor Demo actor using the model.
 * @param character Character id.
 * @return The initialized demo model.
 */
DemoScenePlayerModel* getDemoPlayerModel(const al::LiveActor* pActor, int character) {
    DemoScenePlayerModel* model = getModelDirector(pActor)->getDemoPlayerModel(character);
    model->setDemoActor(pActor);
    model->initialize();
    return model;
}

/**
 * @brief Unbinds a demo model from its demo actor and kills it.
 * @param pModel The demo model.
 */
void releaseDemoPlayerModel(DemoScenePlayerModel* pModel) {
    pModel->releaseDemoActor();
    pModel->kill();
}

/**
 * @brief Sets the audio type of the demos.
 * @param pActor Actor in the scene.
 * @param type Audio demo type.
 */
void setDemoAudioType(const al::LiveActor* pActor, alSeFunction::DemoType type) {
    getDemoDirector(pActor)->setAudioDemoType(type);
}

/**
 * @brief Changes the audio type of the active demo.
 * @param pActor Actor in the scene.
 * @param type Audio demo type.
 */
void changeActiveDemoAudioType(const al::LiveActor* pActor, alSeFunction::DemoType type) {
    getDemoDirector(pActor)->changeActiveAudioDemoType(type);
}

/**
 * @brief Gets the audio type of the active demo.
 * @param pActor Actor in the scene.
 * @return Audio demo type.
 */
alSeFunction::DemoType getActiveDemoAudioType(const al::LiveActor* pActor) {
    return static_cast<alSeFunction::DemoType>(getDemoDirector(pActor)->getAudioDemoType());
}

/**
 * @brief Sets whether effects fully update during demos.
 * @param pActor Actor in the scene.
 * @param isFull Whether effects fully update.
 */
void setDemoFullEffectUpdate(const al::LiveActor* pActor, bool isFull) {
    getDemoDirector(pActor)->setUnknownD4(isFull);
}

/**
 * @brief Sets whether sensors fully update during demos.
 * @param pActor Actor in the scene.
 * @param isFull Whether sensors fully update.
 */
void setDemoFullSensorUpdate(const al::LiveActor* pActor, bool isFull) {
    getDemoDirector(pActor)->setUnknownD5(isFull);
}

/**
 * @brief Sets whether the player alive watcher updates during demos.
 * @param pActor Actor in the scene.
 * @param isUpdate Whether the watcher updates.
 */
void setUpdatePlayerAliveWatcher(const al::LiveActor* pActor, bool isUpdate) {
    getDemoDirector(pActor)->setUnknownE3(isUpdate);
}

/**
 * @brief Places every demo player side by side around a position.
 * @param pActor Actor in the scene.
 * @param rTrans Center position.
 * @param rQuat Orientation of the players.
 * @param offset Distance between neighbouring players.
 */
void replaceDemoPlayerAll(const al::LiveActor* pActor, const sead::Vector3f& rTrans,
                          const sead::Quatf& rQuat, f32 offset) {
    sead::Vector3f side = {0.0f, 0.0f, 0.0f};
    al::calcQuatSide(&side, rQuat);
    side = -side;

    int num = getDemoPlayerNum(pActor);

    if (num < 1) {
        return;
    }

    if (num == 1) {
        DemoPlayerController* player = getDemoPlayer(pActor, 0);
        sead::Vector3f trans = rTrans;
        trans += side * 0.0f;
        player->setQuat(rQuat);
        player->setTransAndResetDynamics(trans);
        return;
    }

    f32 center = (num - 1) * 0.5f;

    for (int i = 0; i < num; i++) {
        DemoPlayerController* player = getDemoPlayer(pActor, i);
        sead::Vector3f trans = rTrans;
        trans += side * ((i - center) * offset);
        player->setQuat(rQuat);
        player->setTransAndResetDynamics(trans);
    }
}

/**
 * @brief Hides the fairy princesses of the worlds whose castle leave demo is not enabled yet.
 * @param pHolder The demo scene.
 * @param accessor Game data.
 */
void tryHideDemoDisableFairyPrincess(DemoSceneActorHolder* pHolder,
                                     GameDataHolderAccessor accessor) {
    const int worldIds[] = {1, 4};

    for (int worldId : worldIds) {
        if (GameDataFlagFunction::isEnableDemoLeaveCastleFairyPrincess(accessor, worldId)) {
            continue;
        }

        al::StringTmp<64> name("FairyPrincess%02d", worldId);
        al::LiveActor* fairyPrincess = pHolder->tryFindDemoSceneActor(name.cstr());

        if (fairyPrincess != nullptr) {
            al::hideModelIfShow(fairyPrincess);
            al::stopAction(fairyPrincess);
        }
    }
}

/**
 * @brief Hides the fairy princesses of the worlds whose castle leave demo is not enabled yet.
 * @param pHolder The demo scene.
 * @param pGameDataHolder Game data.
 */
void tryHideDemoDisableFairyPrincess(DemoSceneActorHolder* pHolder,
                                     const GameDataHolder* pGameDataHolder) {
    tryHideDemoDisableFairyPrincess(
        pHolder, GameDataHolderAccessor(const_cast<GameDataHolder*>(pGameDataHolder)));
}

/**
 * @brief Hides the fairy princesses of the worlds whose castle leave demo is not enabled yet.
 * @param pHolder The demo scene.
 * @param pActor Actor in the scene.
 */
void tryHideDemoDisableFairyPrincess(DemoSceneActorHolder* pHolder, const al::LiveActor* pActor) {
    tryHideDemoDisableFairyPrincess(pHolder, GameDataHolderAccessor(pActor));
}

}  // namespace rc
