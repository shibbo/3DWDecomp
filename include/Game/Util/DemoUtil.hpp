#pragma once

#include <basis/seadTypes.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

namespace al {
class LiveActor;
}  // namespace al

class PlayerActor;

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
}  // namespace rc
