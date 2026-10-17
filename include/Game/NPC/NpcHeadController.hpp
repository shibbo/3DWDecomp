#pragma once
#include "NPC/NpcTargetFunction.hpp"
#include "Library/Joint/JointLookAtController.hpp"
#include "container/seadPtrArray.h"
namespace al {
class LiveActor;
}
class NpcTargetFinder;
class NpcHeadControllerParam {
public:
    NpcHeadControllerParam(const char* pJointName, float rate, const sead::Vector2f& rYaw,
                           const sead::Vector2f& rPitch, const sead::Vector3f& rFront,
                           const sead::Vector3f& rUp);
    const char* mJointName;
    float mRate;
    sead::Vector2f mYaw;
    sead::Vector2f mPitch;
    sead::Vector3f mFront;
    sead::Vector3f mUp;
};
class NpcHeadController {
public:
    NpcHeadController(al::LiveActor* pActor, const NpcHeadControllerParam* pFirst,
                      const NpcHeadControllerParam* pSecond, int maxActions,
                      NpcTargetFinder* pFinder, npc::NpcFindTargetType type, int keepFrames,
                      const sead::Vector3f& rOffset);
    /** @param pTarget Target position, or nullptr. */
    void setLookAtTarget(const sead::Vector3f* pTarget) { mTarget = pTarget; }
    /** @brief Resets head tracking on the next update. */
    void requestResetLook() { mSkipFrame = true; }
    void update();
    bool validateLookAt();
    void addAction(const char* pAction, bool first, bool second);
    void trySetActionParam(const char* pAction);

private:
    struct ActionParam {
        /** @brief Stores enabled joints for an action. @param pName Action name. @param first First
         * joint enabled. @param second Second joint enabled. */
        ActionParam(const char* pName, bool first, bool second)
            : mName(pName), mFirst(first), mSecond(second) {}
        const char* mName;
        bool mFirst;
        bool mSecond;
    };
    al::LiveActor* mActor;
    ActionParam* mAction;
    sead::PtrArray<ActionParam> mActions;
    NpcTargetFinder* mFinder;
    npc::NpcFindTargetType mType;
    const sead::Vector3f* mTarget;
    al::JointLookAtController* mLook;
    void* mUnknown40;
    int mKeepTimer;
    sead::Vector3f mTargetPos;
    al::JointLookAtController::JointLookAtInfo* mFirst;
    al::JointLookAtController::JointLookAtInfo* mSecond;
    int mKeepFrames;
    sead::Vector3f mOffset;
    bool mSkipFrame;
};
static_assert(sizeof(NpcHeadController) == 0x80);
