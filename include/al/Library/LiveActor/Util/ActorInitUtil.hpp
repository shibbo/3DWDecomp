#pragma once

#include "Library/Nerve/NerveAction.hpp"
#include <math/seadBoundBox.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace al {
    class LiveActor;
    class ActorInitInfo;
    class Nerve;
    struct PlacementInfo;
    class ByamlIter;
    class LayoutInitInfo;
    class AudioDirector;
    class ActorFactory;
    class HitSensor;
    class Resource;

    void initActorCollision(LiveActor* pActor, const sead::SafeString& rName, HitSensor* pSensor,
                            const sead::Matrix34f* pMtx);
    void initActorCollisionWithResource(LiveActor* pActor, Resource* pResource,
                                        const sead::SafeString& rName, HitSensor* pSensor,
                                        const sead::Matrix34f* pMtx, const char* pSuffix);
    void initActorCollisionWithArchiveName(LiveActor* pActor, const sead::SafeString& rArchiveName,
                                           const sead::SafeString& rName, HitSensor* pSensor,
                                           const sead::Matrix34f* pMtx);
    void initActorCollisionWithFilePtr(LiveActor* pActor, void* pKcl, const void* pAttribute,
                                       HitSensor* pSensor, const sead::Matrix34f* pMtx,
                                       const char* pSpecialPurpose, s32 priority);

    void initActorChangeModelSuffix(LiveActor*, const ActorInitInfo&, const char*);
    void initActorWithArchiveName(LiveActor*, const ActorInitInfo&, const sead::SafeString&, const char*);
    void initActorWithArchiveCategoryName(LiveActor*, const ActorInitInfo&, const sead::SafeString&, const sead::SafeString&, const char*);
    void initActorWithArchiveNameWithPlacementInfo(LiveActor*, const ActorInitInfo&, const sead::SafeString&, const char*);
    void initActorWithArchiveNameNoPlacementInfo(LiveActor*, const ActorInitInfo&, const sead::SafeString&, const char*);

    const PlacementInfo& getPlacementInfo(const ActorInitInfo&);

    void initNerve(LiveActor*, const Nerve*, s32);
    void initNerveAction(LiveActor*, const char*, alNerveFunction::NerveActionCollector*, s32);

    void initLinksActor(LiveActor*, const ActorInitInfo&, const char*, s32);

    void initActorPoseTRSV(LiveActor*);
    void initActorPoseTRMSV(LiveActor*);
    void initActorPoseTFSV(LiveActor*);
    void initActorPoseTFGSV(LiveActor*);
    void initActorPoseTQSV(LiveActor*);
    void initActorSRT(LiveActor*, const ActorInitInfo&);
    void initActorSRT_ParentY(LiveActor*, const ActorInitInfo&);

    void initMapPartsActor(LiveActor*, const ActorInitInfo&, const char*, int);
    const char* tryGetMapPartsSuffix(const ActorInitInfo&, const char*);

    void initActorSuffix(LiveActor*, const ActorInitInfo&, const char*);

    void initCreateActorWithPlacementInfo(LiveActor*, const ActorInitInfo&);
    void initCreateActorWithPlacementInfo(LiveActor*, const ActorInitInfo&, const PlacementInfo&);

    void initCreateActorNoPlacementInfo(LiveActor*, const ActorInitInfo&);

    void initSubActorKeeperNoFile(LiveActor*, const ActorInitInfo&, int);

    void registerSubActorSyncClipping(LiveActor*, LiveActor*, bool);

    void initActor(LiveActor*, const ActorInitInfo&);

    void initExecutorWatchObj(LiveActor*, const ActorInitInfo&);
    void initExecutorMapObjMovement(LiveActor*, const ActorInitInfo&);
    void initExecutorUpdate(LiveActor*, const ActorInitInfo&, const char*);

    void initActorAudioKeeperWithout3D(LiveActor*, const ActorInitInfo&, const char*, const char*);

    void initActorSceneInfo(LiveActor*, const ActorInitInfo&);

    void makeMapPartsModelName(sead::BufferedSafeString* pModelName, sead::BufferedSafeString* pPath,
                               const PlacementInfo& rInfo);
    void makeMapPartsModelName(sead::BufferedSafeString* pModelName, sead::BufferedSafeString* pPath,
                               const ActorInitInfo& rInfo);

    void initExecutorDraw(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pListName);
    void initExecutorPlayer(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorPlayerPreMovement(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorPlayerMovement(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorPlayerModel(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorPlayerDecoration(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorEnemy(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorEnemyMovement(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorEnemyDecoration(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorEnemyDecorationMovement(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorEnemyMapObjMovement(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorMapObj(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorMapObjDecoration(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorShadowVolume(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorShadowVolumeFillStencil(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorCollisionMapObjDecorationMovement(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initExecutorDebugMovement(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initActorModelKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pArchiveName, s32 bufferNum, const char* pAnimArchiveName);
    void initActorModelKeeperWithInitFile(LiveActor* pActor, const ActorInitInfo& rInfo, const ByamlIter& rIter, const char* pArchiveName, s32 bufferNum, const char* pAnimArchiveName, const char* pSuffix);
    void initActorModelKeeperShadowVolume(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pArchiveName);
    void initActorModelKeeperShadowVolumeFillStencil(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pArchiveName);
    void initActorModelForceCubeMap(LiveActor* pActor, const ActorInitInfo& rInfo);
    void initActorEffectKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pName, bool isUnused);
    void initActorAudioKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pSeName, const char* pBgmName);
    void initActorAudioKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pSeName, const char* pBgmName, const sead::Vector3f* pPos, const sead::Matrix34f* pMtx);
    void initActorOceanWaveKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, ByamlIter& rIter);
    void initActorOceanWaveKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pArchiveName, const char* pFileName);
    void initHitReactionKeeper(LiveActor* pActor, const char* pName);
    void initHitReactionKeeper(LiveActor* pActor, const Resource* pResource, const char* pName);
    void initActorShadowKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, const ByamlIter& rIter, const char* pUnused, const sead::SafeString& rUnused1, const sead::SafeString& rUnused2);
    void initActorItemKeeper(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pArchiveName, const char* pFileName);
    void initActorAlphaCtrl(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pArchiveName, const char* pFileName);
    void initActorPrePassLightKeeper(LiveActor* pActor, const Resource* pResource, const ActorInitInfo& rInfo, const ByamlIter& rIter, const char* pUnused);
    void setSubActorOffSyncClipping(LiveActor* pActor);
    void setSubActorOnSyncAppear(LiveActor* pActor);
    void initActorChangeModel(LiveActor* pActor, const ActorInitInfo& rInfo);
    void getLinksActorInfo(ActorInitInfo* pInfo, PlacementInfo* pPlacementInfo, const ActorInitInfo& rInfo, const char* pLinkName, s32 index);
    ActorInitInfo* createLinksPlayerActorInfo(LiveActor* pActor, const ActorInitInfo& rInfo);
    const char* getLinksActorClassName(const ActorInitInfo& rInfo, const char* pLinkName, s32 index);
    const char* getLinksActorDisplayName(const ActorInitInfo& rInfo, const char* pLinkName, s32 index);
    void initCreateActorNoPlacementInfoNoViewId(LiveActor* pActor, const ActorInitInfo& rInfo);
    LiveActor* createPlacementActorFromFactory(const ActorFactory& rFactory, const ActorInitInfo& rInfo, const PlacementInfo* pPlacementInfo);
    LiveActor* createLinksActorFromFactory(const ActorFactory& rFactory, const ActorInitInfo& rInfo, const char* pLinkName, s32 index);
    bool trySyncStageSwitchAppear(LiveActor* pActor);
    bool trySyncStageSwitchKill(LiveActor* pActor);
    bool trySyncStageSwitchAppearAndKill(LiveActor* pActor);
    bool tryListenStageSwitchAppear(LiveActor* pActor);
    bool tryListenStageSwitchKill(LiveActor* pActor);
    void syncSensorScaleY(LiveActor* pActor);
    void syncSensorAndColliderScaleY(LiveActor* pActor);
    void setMaterialCode(LiveActor* pActor, const char* pMaterialCode);
    bool tryAddDisplayOffset(LiveActor* pActor, const ActorInitInfo& rInfo);
    bool tryAddDisplayScale(LiveActor* pActor, const ActorInitInfo& rInfo);
    const LayoutInitInfo& getLayoutInitInfo(const ActorInitInfo& rInfo);
    AudioDirector* getAudioDirector(const ActorInitInfo& rInfo);
    void getActorRecourseDataF32(f32* pValue, LiveActor* pActor, const char* pFileName, const char* pKey);
    void getActorRecourseDataString(const char** pValue, LiveActor* pActor, const char* pFileName, const char* pKey);
    void getActorRecourseDataV3f(sead::Vector3f* pValue, LiveActor* pActor, const char* pFileName, const char* pKey);
    void getActorRecourseDataBox3f(sead::BoundBox3f* pValue, LiveActor* pActor, const char* pFileName, const char* pKey);
    void initMapPartsActorNoPlacementInfo(LiveActor* pActor, const ActorInitInfo& rInfo, const char* pArchiveName);
}  // namespace al
