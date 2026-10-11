#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class CameraInfo;
class CameraTicket;
class LiveActor;
}  // namespace al

/** @brief Name and settings of a gate keeper boss's opening demo. */
class GateKeeperStateDemoParam {
public:
    GateKeeperStateDemoParam(const char* pActionName, s32 unk);

    const char* mActionName;
    u8 mUnknown08[0x20 - 0x08];
};

static_assert(sizeof(GateKeeperStateDemoParam) == 0x20);

/** @brief Opening demo shared by the gate keeper bosses (Punpun, Bunbun, ...). */
class GateKeeperStateDemo : public al::ActorStateBase {
public:
    GateKeeperStateDemo(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                        const GateKeeperStateDemoParam* pParam, al::CameraInfo* pCamera);
    GateKeeperStateDemo(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                        const GateKeeperStateDemoParam* pParam, al::CameraTicket* pCamera);

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
    void setLookAtOffset(const sead::Vector3f& rOffset) { mLookAtOffset.e = rOffset.e; }

    /** @brief Marks the demo camera as an RS camera ticket (single mode). */
    void setIsCameraTicket() { mIsCameraTicket = true; }

    /**
     * @brief Checks whether the demo itself has started.
     * @return Whether the demo has started.
     */
    bool isDemoStarted() const { return mIsDemoStarted; }

private:
    u8 mUnknown20[0xa8 - 0x20];
    sead::Vector3f mLookAtOffset;  // 0xa8
    u8 mUnknownB4[0xcd - 0xb4];
    bool mIsControlPlayer;  // 0xcd
    bool mIsCameraTicket;   // 0xce
    bool mIsDemoStarted;    // 0xcf
    u8 mUnknownD0[0xd8 - 0xd0];
};

static_assert(sizeof(GateKeeperStateDemo) == 0xd8);
