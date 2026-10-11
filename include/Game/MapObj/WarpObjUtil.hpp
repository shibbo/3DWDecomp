#pragma once
#include <math/seadVector.h>

namespace al {
class LiveActor;
}  // namespace al

namespace WarpObjUtil {
void getJumpOutLocalVelocity(sead::Vector3f*, int, int);
void getJumpOutLocalTrans(sead::Vector3f*, int, int);
void stopStageTimer(const al::LiveActor* pActor);
void restartStageTimer(const al::LiveActor* pActor);
}  // namespace WarpObjUtil
