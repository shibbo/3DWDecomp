#pragma once

#include <math/seadVector.h>

#include "NPC/NpcTargetFinder.hpp"

namespace al {
class LiveActor;
}  // namespace al

/**
 * @brief Joint and look-at limits of one joint (head or spine) driven by an NpcHeadController.
 * @note The fields have not been reconstructed yet.
 */
class NpcHeadControllerParam {
public:
    NpcHeadControllerParam(const char* pJointName, f32 lookAtRate,
                           const sead::Vector2f& rLookYawRange,
                           const sead::Vector2f& rLookPitchRange,
                           const sead::Vector3f& rJointFront, const sead::Vector3f& rJointUp);

private:
    alignas(8) u8 _0[0x38];
};

static_assert(sizeof(NpcHeadControllerParam) == 0x38);

/**
 * @brief Turns the head of an NPC towards a look-at target.
 * @note Only what reconstructed code needs is declared so far.
 */
class NpcHeadController {
public:
    NpcHeadController(al::LiveActor* pActor, const NpcHeadControllerParam* pHeadParam,
                      const NpcHeadControllerParam* pSpineParam, s32 actionNum,
                      NpcTargetFinder* pTargetFinder, npc::NpcFindTargetType targetType,
                      s32 minLookAtFrame, const sead::Vector3f& rLookAtOffset);

    void addAction(const char* pActionName, bool isControlHead, bool isControlSpine);
    void update();

    /**
     * @brief Set the position the head looks at.
     * @param pTarget Position to look at, or nullptr to stop looking; must stay valid.
     */
    void setLookAtTarget(const sead::Vector3f* pTarget) { mLookAtTarget = pTarget; }

    /** @brief Make the head go back to its rest pose on the next update. */
    void requestResetLook() { mIsRequestResetLook = true; }

private:
    u8 _0[0x30];
    const sead::Vector3f* mLookAtTarget;  // 0x30
    u8 _38[0x78 - 0x38];
    bool mIsRequestResetLook;  // 0x78
    u8 _79[0x80 - 0x79];
};

static_assert(sizeof(NpcHeadController) == 0x80);
