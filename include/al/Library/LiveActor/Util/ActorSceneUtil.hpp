#pragma once

#include <basis/seadTypes.h>

namespace al {
class ActorInitInfo;
class LiveActor;

void stopScene(const LiveActor*, s32, s32, bool, bool);
bool isStopScene(const LiveActor*);
void requestCaptureScreenCover(const LiveActor*, s32);
void requestCaptureScreenSceneCover(const LiveActor*);
void resetRequestCaptureScreenSceneCover(const LiveActor*);
bool requestStartDemo(const LiveActor*, const char*);
void requestEndDemo(const LiveActor*, const char*);
void addDemoActor(LiveActor*);
void setDisasterMode(LiveActor*, bool);
bool isDisasterMode(LiveActor*);
void stopAllPadRumble(LiveActor*);
bool isSingleMode(const LiveActor*);
bool isSingleMode(const ActorInitInfo& rInfo);
}  // namespace al
