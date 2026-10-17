#include "MapObj/WarpObjUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "System/StageTimer.hpp"
#include <math/seadMathCalcCommon.h>
namespace {
const sead::Vector3f cJumpOutVelocity[21] = {
    {5.4f, 19.0f, 5.5f},
    {3.6f, 19.0f, 5.5f},
    {1.8f, 19.0f, 5.5f},
    {0.0f, 19.0f, 5.5f},
    {-1.8f, 19.0f, 5.5f},
    {-3.6f, 19.0f, 5.5f},
    {-5.4f, 19.0f, 5.5f},
    {5.4f, 19.0f, 9.0f},
    {3.6f, 19.0f, 9.0f},
    {1.8f, 19.0f, 9.0f},
    {0.0f, 19.0f, 9.0f},
    {-1.8f, 19.0f, 9.0f},
    {-3.6f, 19.0f, 9.0f},
    {-5.4f, 19.0f, 9.0f},
    {5.4f, 19.0f, 12.5f},
    {3.6f, 19.0f, 12.5f},
    {1.8f, 19.0f, 12.5f},
    {0.0f, 19.0f, 12.5f},
    {-1.8f, 19.0f, 12.5f},
    {-3.6f, 19.0f, 12.5f},
    {-5.4f, 19.0f, 12.5f},
};
const sead::Vector3f cJumpOutTrans[21] = {
    {-225.0f, 200.0f, 150.0f},
    {-150.0f, 200.0f, 150.0f},
    {-75.0f, 200.0f, 150.0f},
    {0.0f, 200.0f, 150.0f},
    {75.0f, 200.0f, 150.0f},
    {150.0f, 200.0f, 150.0f},
    {225.0f, 200.0f, 150.0f},
    {-225.0f, 200.0f, 225.0f},
    {-150.0f, 200.0f, 225.0f},
    {-75.0f, 200.0f, 225.0f},
    {0.0f, 200.0f, 225.0f},
    {75.0f, 200.0f, 225.0f},
    {150.0f, 200.0f, 225.0f},
    {225.0f, 200.0f, 225.0f},
    {-225.0f, 200.0f, 300.0f},
    {-150.0f, 200.0f, 300.0f},
    {-75.0f, 200.0f, 300.0f},
    {0.0f, 200.0f, 300.0f},
    {75.0f, 200.0f, 300.0f},
    {150.0f, 200.0f, 300.0f},
    {225.0f, 200.0f, 300.0f},
};
const int cJumpOutOrder[12][12] = {
    {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 3, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 2, 4, 6, 0, 0, 0, 0, 0, 0, 0, 0},
    {8, 10, 12, 2, 4, 0, 0, 0, 0, 0, 0, 0},
    {8, 10, 12, 1, 3, 5, 0, 0, 0, 0, 0, 0},
    {7, 9, 11, 13, 1, 3, 5, 0, 0, 0, 0, 0},
    {7, 9, 11, 13, 0, 2, 4, 6, 0, 0, 0, 0},
    {15, 17, 19, 8, 10, 12, 1, 3, 5, 0, 0, 0},
    {14, 16, 18, 20, 8, 10, 12, 1, 3, 5, 0, 0},
    {14, 16, 18, 20, 7, 9, 11, 13, 1, 3, 5, 0},
    {14, 16, 18, 20, 7, 9, 11, 13, 0, 2, 4, 6},
};
}
namespace WarpObjUtil {
void getJumpOutLocalVelocity(sead::Vector3f* pVelocity, int index, int count) {
    pVelocity->set(cJumpOutVelocity[cJumpOutOrder[sead::Mathi::min(count, 12) - 1][index % 12]]);
}
void getJumpOutLocalTrans(sead::Vector3f* pTrans, int index, int count) {
    pTrans->set(cJumpOutTrans[cJumpOutOrder[sead::Mathi::min(count, 12) - 1][index % 12]]);
}
void stopStageTimer(const al::LiveActor* pActor) {
    StageTimer* timer = al::tryGetSceneObj<StageTimer>(pActor, 23);
    if (timer && timer->isCountDown()) timer->requestStop(pActor);
}
void restartStageTimer(const al::LiveActor* pActor) {
    StageTimer* timer = al::tryGetSceneObj<StageTimer>(pActor, 23);
    if (timer && timer->isStop()) timer->requestRestart(pActor);
}
}
