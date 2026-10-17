#pragma once

#include <math/seadVector.h>
#include <prim/seadSafeString.h>
namespace al { class LiveActor; class IUseSceneObjHolder; }
class GhostPlayerPlayer;
class GhostPlayerDirector;
class GameDataHolderAccessor;
class GhostPlayerLoaderBase;
class MetaBinaryData;
struct GhostRecorderPoseData;
struct GhostRecorderMarkerData;
namespace al { class NexGhostDataHolder; }

namespace GhostPlayerFunction {
GhostPlayerDirector* getGhostPlayerDirector(const al::IUseSceneObjHolder*);
bool isEnablePlayGhostPlayer(GameDataHolderAccessor, int);
bool isCurrentCourseGhostPlayer(GameDataHolderAccessor);
bool isCourseGhostPlayer(GameDataHolderAccessor, int);
bool isEnablePlayGhostPlayer(GameDataHolderAccessor);
bool isAvailableNetwork(const GhostPlayerPlayer*);
GhostPlayerLoaderBase* createGhostPlayerLoader(GhostPlayerPlayer*, const char*);
bool copyGhostDataToStocker(al::NexGhostDataHolder*, u8*, int, const MetaBinaryData*, const char*);
void getTimeAttackRank(sead::BufferedSafeString*, GameDataHolderAccessor, const char*, int);
void getTimeAttackRivalRank(sead::BufferedSafeString*, GameDataHolderAccessor, const char*, int);
const char* getGhostPlayActionData(const void*, int);
const char* getGhostPlayActionDataRewind(const void*, int);
int getGhostPlayActionDataNum(const void*);
const GhostRecorderPoseData* getGhostPlayData(const void*, int);
const GhostRecorderMarkerData* getMarkerData(const void*, int);
int getMarkerDataNum(const void*);
void calcGhostPlayDataActionNameRewind(const char**, const void*, int);
void registerGhostPlayerPlayer(GhostPlayerPlayer*);
bool isExistGhostPlayerDirector(const al::IUseSceneObjHolder*);
u32 getGhostPlayerRecorderBufferSize();
bool isValidGhostPlayerDataVersion(const void*);
struct WarpObjData {
    char name[0x14];
    u16 frame;
    bool isStart;
};
static_assert(sizeof(WarpObjData) == 24);
const WarpObjData* getWarpObjData(const void*, int);
int getWarpObjDataNum(const void*);
void calcGhostPlayDataActionName(const char**, const void*, int);
void makeGhostActionName(sead::BufferedSafeString*, const char*, const al::LiveActor*);
void calcGhostPlayDataSklAnimFrame(float*, const void*, int, bool);
int getGhostPlayDataNum(const void*);
void calcGhostPlayDataTrans(sead::Vector3f*, const void*, int, float);
void calcGhostPlayDataRotate(sead::Vector3f*, const void*, int);
}
