#pragma once

#include <math/seadVector.h>

namespace al {
    class HitSensor;
    class IUseSceneObjHolder;
    class LiveActor;
    class ScreenPointer;
};

namespace rc {
    bool isEnableTouchPointerGrabItem(const al::LiveActor*);
    bool isTouchDrcAssistByPointer(const al::LiveActor*);
    void getTouchPointerUpDir(sead::Vector3f*, const al::LiveActor*);
    bool isEnableTouchPointer(const al::LiveActor*);
    const sead::Vector3f& getTouchPointerPosition(const al::LiveActor*);
    al::LiveActor* findDrcAssistDirectorPlayer(const al::IUseSceneObjHolder*,
                                               const al::ScreenPointer*);
    al::LiveActor* findDrcAssistDirectorPlayer(const al::IUseSceneObjHolder*,
                                               const al::HitSensor*);
    al::LiveActor* findDrcAssistDirectorTouchPointer(const al::IUseSceneObjHolder*,
                                                     const al::ScreenPointer*);
    bool tryCalcTouchPointerSlideDirOnWorldByPointer(sead::Vector3f*, const al::LiveActor*);
    void releaseAllTouchPointerHoldItem(const al::LiveActor* pActor);
    void releaseTouchPointerHoldItem(const al::LiveActor* pActor, const al::LiveActor* pPointer);
};