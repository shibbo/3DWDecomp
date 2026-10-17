#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class CameraInfo;
class CameraTicket;
class LiveActor;
} // namespace al

/** @brief Name and settings of a gate keeper boss's opening demo. */
class GateKeeperStateDemoParam {
  public:
    GateKeeperStateDemoParam(const char *pActionName, s32 unk);

    const char *mActionName;
    const char *mPlayerAction;
    const char *mCameraAction;
    s32 mCameraStartFrames;
    s32 mCameraEndFrames;
};

static_assert(sizeof(GateKeeperStateDemoParam) == 0x20);

/** @brief Opening demo shared by the gate keeper bosses (Punpun, Bunbun, ...). */
class GateKeeperStateDemo : public al::ActorStateBase {
  public:
    GateKeeperStateDemo(al::LiveActor *pHost, const al::ActorInitInfo &rInfo,
                        const GateKeeperStateDemoParam *pParam, al::CameraInfo *pCamera);
    GateKeeperStateDemo(al::LiveActor *pHost, const al::ActorInitInfo &rInfo,
                        const GateKeeperStateDemoParam *pParam, al::CameraTicket *pCamera);

    ~GateKeeperStateDemo() override;
    void appear() override;
    void kill() override;
    bool isDemoFirstStep() const;
    void exeInitOnGround();
    void exeDemoRequest();
    void exeDemo();

    /**
     * @brief Sets whether this demo drives the player (off when another boss owns the demo).
     * @param isControlPlayer Whether the demo controls the player.
     */
    void setIsControlPlayer(bool isControlPlayer) { mIsControlPlayer = isControlPlayer; }

    /**
     * @brief Sets the offset added to the demo camera's look-at position.
     * @param rOffset Look-at offset.
     */
    void setLookAtOffset(const sead::Vector3f &rOffset) { mLookAtOffset = sead::Vector3f(rOffset); }

    /** @brief Marks the demo camera as an RS camera ticket (single mode). */
    void setIsCameraTicket() { mIsCameraTicket = true; }

    /**
     * @brief Checks whether the demo itself has started.
     * @return Whether the demo has started.
     */
    bool isDemoStarted() const { return mIsDemoStarted; }

  private:
    const GateKeeperStateDemoParam *mParam;
    al::CameraInfo *mCameraInfo = nullptr;
    al::CameraTicket *mCameraTicket = nullptr;
    sead::FixedSafeString<32> mStartRecorderId;
    sead::FixedSafeString<32> mEndRecorderId;
    sead::Vector3f mLookAtOffset = sead::Vector3f::zero;
    sead::Vector3f mPlayerPos = sead::Vector3f::zero;
    sead::Vector3f mPlayerFront = sead::Vector3f::zero;
    bool mHasPlayerPos = false;
    bool mIsControlPlayer = true;
    bool mIsCameraTicket = false;
    bool mIsDemoStarted = false;
    bool mCancelPlayerKoura;
};

static_assert(sizeof(GateKeeperStateDemo) == 0xd8);
