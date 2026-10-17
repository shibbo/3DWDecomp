#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Execute/IUseExecutor.hpp"

namespace al {
class ActorInitInfo;
class AreaObjDirector;
class CameraDirector_RS;
class ClippingFarAreaObserver;
class ClippingJudge;
class ExecuteDirector;
class LiveActor;
class PlayerHolder;
class SceneCameraInfo;

class ClippingDirectorBase : public IUseExecutor {
public:
    ClippingDirectorBase(ExecuteDirector* pExecuteDirector, const AreaObjDirector* pAreaObjDirector,
                         const PlayerHolder* pPlayerHolder, SceneCameraInfo* pSceneCameraInfo,
                         CameraDirector_RS* pCameraDirector);

    void execute() override;
    virtual ~ClippingDirectorBase() {}
    virtual void registerActor(LiveActor* pActor, const ActorInitInfo& rInfo) = 0;

    virtual void recreateClipping(LiveActor* pActor, const ActorInitInfo& rInfo) {}

    virtual void registerActorToHost(LiveActor* pActor, const LiveActor* pHost) = 0;
    virtual void addToGroupClipping(LiveActor* pActor, const ActorInitInfo& rInfo, s32 num) = 0;
    virtual void addToClipping(LiveActor* pActor) = 0;
    virtual void removeFromClipping(LiveActor* pActor) = 0;

    virtual void moveToClippingGroup(LiveActor* pActor, LiveActor* pGroupActor) {}

    virtual void setCollisionClippingDisabled(bool isDisabled) {}

    virtual void setActorFarClipLevel(LiveActor* pActor, s32 level) = 0;
    virtual f32 getActorClippingRadius(const LiveActor* pActor) = 0;

    virtual void setNoCollisionClip(LiveActor* pActor, bool isNoClip) {}

    virtual void invalidateActorClipping(LiveActor* pActor) = 0;
    virtual void validateActorClipping(LiveActor* pActor) = 0;
    virtual void setActorClippingInfo(LiveActor* pActor, f32 radius,
                                      const sead::Vector3f* pOffset) = 0;

    virtual void setActorClippingOffset(LiveActor* pActor, const sead::Vector3f& rOffset) {}

    virtual const sead::Vector3f& getActorClippingCenterPos(const LiveActor* pActor) = 0;
    virtual void setActorNearClipDistance(LiveActor* pActor, f32 distance) = 0;
    virtual void setActorNearFarClipDistance(LiveActor* pActor, f32 near, f32 far) = 0;

    virtual void setShadowClippingDistance(LiveActor* pActor, f32 distance) {}

    virtual void setDrawClippingRadius(LiveActor* pActor, f32 radius) {}

    virtual void endInit();

    virtual void resetClippingDistanceStates() {}

    virtual void setExpandedClippingMode(bool isExpanded) {}

    virtual void* findActorInfo(const LiveActor* pActor) const = 0;

    virtual void disableForceClipAreas() {}

    virtual void executeRequestAsyncUpdate() {}

    virtual void waitPendingClippingRequest() {}

    virtual void setLODDisabled(LiveActor* pActor, bool isDisabled) {}

    void setClippingJudgeUsClippingPosAsPlayerPos(bool isUse);

    /**
     * @brief Sets the screen cover frame counter the clipping is suspended by.
     * @param pFrames The frame counter of the scene's screen cover.
     */
    void setScreenCoverFrames(const s32* pFrames) { _18 = pFrames; }

    static bool sLODDisabled;
    static bool sCollisionForcedOn;

    ClippingJudge* mClippingJudge = nullptr;
    ClippingFarAreaObserver* mFarAreaObserver = nullptr;
    const s32* _18 = nullptr;
};
}  // namespace al
