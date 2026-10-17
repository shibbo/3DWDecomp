#pragma once

#include <math/seadVector.h>

class DrcTouchPointer;

namespace al {
class HitSensor;
class IUseSceneObjHolder;
class LiveActor;
class ScreenPointer;
} // namespace al

namespace rc {
void setDrcAssistEnable(const al::IUseSceneObjHolder *holder, bool enable);
void validateDrcAssist(const al::IUseSceneObjHolder *holder);
void invalidateDrcAssist(const al::IUseSceneObjHolder *holder, bool force);
void setValidDrcAssist(const al::IUseSceneObjHolder *holder, bool valid);
void setValidDrcAssist(const al::IUseSceneObjHolder *holder, bool valid, int port);
void invalidateDrcAssistImmediate(const al::IUseSceneObjHolder *holder);
void checkPreStartDrcAssistPose(const al::IUseSceneObjHolder *holder);
void updateDrcAssistDirector(const al::IUseSceneObjHolder *holder);
bool isTouchDrcAssist(const al::LiveActor *actor);
bool isTouchDrcAssist(const al::LiveActor *actor, const sead::Vector3f &position, float radius);
bool isTouchDrcAssistByPointer(const al::LiveActor *actor);
bool isTouchTriggerDrcAssist(const al::LiveActor *actor, const sead::Vector3f &position,
                             float radius);
bool isEnableTouchPointer(const al::LiveActor *actor);
bool isEnableTouchPointer(const al::IUseSceneObjHolder *holder);
bool isEnableTouchPointerGrabItem(const al::LiveActor *actor);
void releaseTouchPointerHoldItem(const al::LiveActor *actor, const al::LiveActor *pointer);
void releaseAllTouchPointerHoldItem(const al::LiveActor *actor);
al::LiveActor *findDrcAssistDirectorPlayer(const al::IUseSceneObjHolder *holder,
                                           const al::ScreenPointer *pointer);
al::LiveActor *findDrcAssistDirectorPlayer(const al::IUseSceneObjHolder *holder,
                                           const al::HitSensor *pointer);
al::LiveActor *findDrcAssistDirectorTouchPointer(const al::IUseSceneObjHolder *holder,
                                                 const al::ScreenPointer *pointer);
al::LiveActor *getTouchPointer(const al::LiveActor *actor);
al::LiveActor *getFirstTouchingTouchPointer(const al::LiveActor *actor);
DrcTouchPointer *getFirstTouchingDrcTouchPointer(const al::LiveActor *actor);
const sead::Vector3f &getTouchPointerPosition(const al::LiveActor *actor);
const sead::Vector3f &getTouchedPointerPosition(const al::LiveActor *actor);
const sead::Vector3f &getTouchPointerPosition(const al::IUseSceneObjHolder *actor);
const sead::Vector3f &getTouchPointerNormal(const al::LiveActor *actor);
void calcTouchPointerUpDir(sead::Vector3f *out, const al::LiveActor *actor);
void getTouchPointerUpDir(sead::Vector3f *out, const al::LiveActor *actor);
bool tryCalcTouchPointerSlideDirOnScreenByPointer(sead::Vector2f *out, const al::LiveActor *actor);
bool tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f *out, const al::LiveActor *actor, int port);
bool tryCalcTouchPointerSlideDirOnWorldByPointer(sead::Vector3f *out, const al::LiveActor *actor);
float tryCalcTouchPointerSlideSpeedOnScreenByPointer(const al::LiveActor *actor);
} // namespace rc
