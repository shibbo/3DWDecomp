#pragma once
#include <math/seadVector.h>
namespace al { class ActorInitInfo; class LiveActor; }
class KoopaChase;
class KoopaChaseKoopa;
namespace KoopaChaseFunction {
al::LiveActor* tryCreateLinkObj(const al::ActorInitInfo& rInfo, const char* pLinkName, int index);
void startActionWithKoopa(KoopaChase* pActor, const char* pAction);
void startActionWithKoopa(KoopaChase* pActor, const char* pAction, const char* pKoopaAction);
KoopaChaseKoopa* getKoopa(KoopaChase* pActor);
al::LiveActor* getTargetPlayer(KoopaChase* pActor);
al::LiveActor* getLookAtTargetPlayer(KoopaChase* pActor);
int getDamageCount(const KoopaChase* pActor);
void updateKoopaPose(KoopaChase* pActor);
bool tryLookAtTargetPlayer(KoopaChase* pActor);
bool isOverPoint(const KoopaChase* pActor);
bool isPreviousPointJump(const KoopaChase* pActor);
bool isCurrentPointJump(const KoopaChase* pActor);
bool isOverPointJump(const KoopaChase* pActor);
bool isPreviousPointWarp(const KoopaChase* pActor);
bool isCurrentPointWarp(const KoopaChase* pActor);
bool isOverPointWarp(const KoopaChase* pActor);
void calcCurrentPointPos(sead::Vector3f* pOut, const KoopaChase* pActor);
float calcDistanceToCurrentPoint(const KoopaChase* pActor);
void tryResetWarpCube(KoopaChase* pActor);
void tryHideWarpDummy(KoopaChase* pActor);
const char* getAnimNameRun(int damageCount);
const char* getAnimNameRun(const KoopaChase* pActor);
const char* getAnimNameJumpStart(const KoopaChase* pActor);
const char* getAnimNameJumpLoop(const KoopaChase* pActor);
const char* getAnimNameJumpEnd(const KoopaChase* pActor);
const char* getAnimNameWarpJumpStart(const KoopaChase* pActor);
const char* getAnimNameWarpJumpLoop(int damageCount);
const char* getAnimNameWarpJumpLoop(const KoopaChase* pActor);
const char* getAnimNameWarpJumpEnd(int damageCount);
}
