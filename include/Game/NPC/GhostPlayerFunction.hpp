#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

class GameDataHolder;
class GameDataHolderAccessor;
class GhostPlayerDirector;
class GhostPlayerPlayer;

namespace al {
class IUseSceneObjHolder;
class LiveActor;
class NetworkSystem;
} // namespace al

/** @brief A checkpoint (warp object) record stored in recorded ghost play data. */
struct GhostWarpObjData {
    char mName[20];
    u16 mFrame;
    bool mIsRestartPoint;
};

namespace GhostPlayerFunction {
void registerGhostPlayerPlayer(GhostPlayerPlayer* pPlayer);
u32 getGhostPlayerRecorderBufferSize();
bool isExistGhostPlayerDirector(const al::IUseSceneObjHolder* pHolder);
GhostPlayerDirector* getGhostPlayerDirector(const al::IUseSceneObjHolder* pHolder);
bool isEnablePlayGhostPlayer(GameDataHolderAccessor accessor, s32 courseId);
bool isCurrentCourseGhostPlayer(GameDataHolderAccessor accessor);
void makeGhostActionName(sead::BufferedSafeString* pOut, const char* pActionName,
                         const al::LiveActor* pActor);
bool isValidGhostPlayerDataVersion(const void* pData);
const GhostWarpObjData* getWarpObjData(const void* pData, s32 index);
s32 getWarpObjDataNum(const void* pData);
s32 getGhostPlayDataNum(const void* pData);
void calcGhostPlayDataTrans(sead::Vector3f* pTrans, const void* pData, s32 frame, f32 maxDist);
void calcGhostPlayDataRotate(sead::Vector3f* pRotate, const void* pData, s32 frame);
void calcGhostPlayDataSklAnimFrame(f32* pFrame, const void* pData, s32 frame, bool isOneTime);
void calcGhostPlayDataActionName(const char** pName, const void* pData, s32 frame);
} // namespace GhostPlayerFunction

namespace rc {
bool tryRequestDownloadGhostData(al::NetworkSystem* pNetworkSystem,
                                 const GameDataHolder* pGameDataHolder, const char* pStageName,
                                 s32 fileId, bool isWorldWarp);
} // namespace rc

class GhostPlayerLoaderBase;
class MetaBinaryData;
namespace al {
class NexGhostDataHolder;
}
namespace GhostPlayerFunction {
bool isCourseGhostPlayer(GameDataHolderAccessor data, int courseId);
bool isEnablePlayGhostPlayer(GameDataHolderAccessor data);
bool isAvailableNetwork(const GhostPlayerPlayer* pPlayer);
GhostPlayerLoaderBase* createGhostPlayerLoader(GhostPlayerPlayer* pPlayer, const char* pName);
bool copyGhostDataToStocker(al::NexGhostDataHolder* pHolder, unsigned char* pData, int size,
                            const MetaBinaryData* pMeta, const char* pName);
} // namespace GhostPlayerFunction
