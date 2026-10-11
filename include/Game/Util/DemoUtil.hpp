#pragma once

#include <basis/seadTypes.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include <math/seadMatrix.h>

namespace al {
class ActorInitInfo;
class LiveActor;
class Scene;
}  // namespace al

class DemoPlayerController;
class DemoSceneActorHolder;
class DemoScenePlayerModel;
class GameDataHolder;
class GameDataHolderAccessor;
class IDemoScenePlayerInfo;
class PlayerActor;
class PlayerRetargettingSelector;

namespace alSeFunction {
enum DemoType : s32;
}  // namespace alSeFunction

namespace rc {
void setImmediateSwitchFlag(const al::LiveActor*);
void setUpdateFreeze(const al::LiveActor*, bool);
void setOtherActiveDemo(al::LiveActor*, bool);
bool requestStartDemoIntro(al::LiveActor*, const char*, bool);
bool requestStartDemoCamera(al::LiveActor*, const char*);
bool requestStartDemoMovingCamera(al::LiveActor* pActor, const char* pName, bool isKeepPlayer);
void requestEndDemoMovingCamera(const al::LiveActor* pActor);
bool isActiveSpecificDemo(const al::LiveActor*);
bool isActiveDemoCamera(const al::LiveActor*);
bool isAnyActiveButDemoCameraDemo(const al::LiveActor* pActor);
void requestEndDemoIntro(const al::LiveActor*);
void requestEndDemoCamera(const al::LiveActor*);
bool requestStartDemoCutscene(const al::LiveActor* pActor);
void requestEndDemoCutscene(const al::LiveActor* pActor);
bool requestStartDemoInGameCutscene(const al::LiveActor* pActor);
void requestEndDemoInGameCutscene(const al::LiveActor* pActor);
void resetImmediateSwitchFlag(const al::LiveActor* pActor);
void setUpdateItemsInDemo(const al::LiveActor* pActor);
bool requestStartDemoPlayerCutscene(const al::LiveActor* pActor);
void requestEndDemoPlayerCutscene(const al::LiveActor* pActor);
bool requestStartDemoPlayer(const al::LiveActor* pActor);
void requestEndDemoPlayer(const al::LiveActor* pActor);
void hideDemoPlayerAll(const al::LiveActor* pActor);
void showDemoPlayerAll(const al::LiveActor* pActor);
void stopSklAnimAndDeleteEffectDemoPlayerAll(const al::LiveActor* pActor);
void startActionDemoPlayerAll(const al::LiveActor* pActor, const char* pActionName);
void replaceDemoPlayerAll(const al::LiveActor* pActor, const sead::Vector3f& rTrans,
                          const sead::Quatf& rQuat, f32 offset);
bool isAnyActiveDemo(const al::LiveActor* pActor);
bool isActiveDemo(const al::LiveActor* pActor);
bool isActiveDemoMovingCamera(const al::LiveActor* pActor);
bool isActiveDemoCutscene(const al::LiveActor* pActor);
bool tryCancelStageDemoStrict(const al::LiveActor* pActor);
bool isActiveDemoInGameCutscene(const al::LiveActor* pActor);
void addDemoActor(al::LiveActor* pActor);
void setUpdatePausedWatchersInDemo(const al::LiveActor* pActor);
void removeDemoActor(al::LiveActor* pActor);
void setDemoAudioType(const al::LiveActor* pActor, alSeFunction::DemoType type);
void setDemoFullEffectUpdate(const al::LiveActor* pActor, bool isFull);
void setDemoFullSensorUpdate(const al::LiveActor* pActor, bool isFull);
void addDemoPlayer(PlayerActor* pPlayer);
void removeDemoPlayer(PlayerActor* pPlayer);
void changeActiveDemoAudioType(const al::LiveActor* pActor, alSeFunction::DemoType type);
bool tryStartDemo(DemoSceneActorHolder* pHolder);
bool tryEndDemo(DemoSceneActorHolder* pHolder);
void setEndCameraInterpolateFrame(DemoSceneActorHolder* pHolder, int frames);
DemoSceneActorHolder* tryCreateDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
                                               PlayerRetargettingSelector* pSelector,
                                               const sead::Matrix34f* pBaseMtx, bool isUseFigure,
                                               int maxPlayers);
DemoSceneActorHolder* createDemoSceneHolder(const char* pName, const al::ActorInitInfo& rInfo,
                                            PlayerRetargettingSelector* pSelector,
                                            const sead::Matrix34f* pBaseMtx, bool isUseFigure,
                                            int maxPlayers);
DemoSceneActorHolder* createDemoSceneHolderLayoutOnly(const char* pName,
                                                      const al::ActorInitInfo& rInfo,
                                                      PlayerRetargettingSelector* pSelector,
                                                      const sead::Matrix34f* pBaseMtx,
                                                      bool isUseFigure, int maxPlayers);
void setDemoScenePlayerInfo(DemoSceneActorHolder* pHolder, IDemoScenePlayerInfo* pInfo);
bool isDemoDirectorAvailable(const al::LiveActor* pActor);
bool isDemoDirectorAvailable(const al::Scene* pScene);
bool tryCancelBossDemo(const al::LiveActor* pActor);
bool tryCancelStageDemo(const al::LiveActor* pActor);
void setUpdateFreezeButPlayer(const al::LiveActor* pActor, bool isFreeze);
void setAlreadyShowBossDemo(const al::LiveActor* pActor);
bool requestStartDemoBinding(const al::LiveActor* pActor);
void requestEndDemoBinding(const al::LiveActor* pActor);
bool requestStartDemoPlayer(const al::Scene* pScene);
void requestEndDemoPlayer(const al::Scene* pScene);
bool isActiveDemoIntro(const al::LiveActor* pActor);
bool isActiveDemoPlayer(const al::LiveActor* pActor);
bool isActiveDemoBinding(const al::LiveActor* pActor);
bool isActiveDemoPlayerCutscene(const al::LiveActor* pActor);
bool getImmediateSwitchFlag(const al::LiveActor* pActor);
int getDemoPlayerNum(const al::LiveActor* pActor);
DemoPlayerController* getDemoPlayer(const al::LiveActor* pActor, int index);
DemoPlayerController* getDemoPlayerByCharacter(const al::LiveActor* pActor, int character);
DemoPlayerController* getDemoPlayerByActor(const al::LiveActor* pActor);
int getDemoPlayerNum(const al::Scene* pScene);
DemoPlayerController* getDemoPlayer(const al::Scene* pScene, int index);
DemoPlayerController* getDemoPlayerByCharacter(const al::Scene* pScene, int character);
DemoPlayerController* getDemoPlayerByActor(const al::Scene* pScene, const al::LiveActor* pActor);
void hideDemoPlayerAll(const al::Scene* pScene);
void showDemoPlayerAll(const al::Scene* pScene);
void stopSklAnimAndDeleteEffectDemoPlayerAll(const al::Scene* pScene);
void setActionDemoFrameAll(const al::LiveActor* pActor, int frame);
void requestCreateAllPlayer(const al::LiveActor* pActor, const char* pSuffix);
void requestCreateAllFigure(const al::LiveActor* pActor, int character, const char* pSuffix);
void requestCreateFigureSuper(const al::LiveActor* pActor, int character, const char* pSuffix);
void requestCreateFigureClimb(const al::LiveActor* pActor, int character, const char* pSuffix);
void requestCreateFigureClimbGiga(const al::LiveActor* pActor, int character,
                                  const char* pSuffix);
DemoScenePlayerModel* getDemoPlayerModel(const al::LiveActor* pActor, int character);
void releaseDemoPlayerModel(DemoScenePlayerModel* pModel);
alSeFunction::DemoType getActiveDemoAudioType(const al::LiveActor* pActor);
void setUpdatePlayerAliveWatcher(const al::LiveActor* pActor, bool isUpdate);
void tryHideDemoDisableFairyPrincess(DemoSceneActorHolder* pHolder, GameDataHolderAccessor accessor);
void tryHideDemoDisableFairyPrincess(DemoSceneActorHolder* pHolder,
                                     const GameDataHolder* pGameDataHolder);
void tryHideDemoDisableFairyPrincess(DemoSceneActorHolder* pHolder, const al::LiveActor* pActor);
}  // namespace rc
