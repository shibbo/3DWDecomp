#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class ActorInitInfo;
class AudioDirector;
class CameraInfo;
class LiveActor;
class WipeSimple;
}  // namespace al

class DemoSkipLayout;

/** @brief Camera and timing parameters of a boss's opening demo. */
class BossDemoStartInfo {
public:
    BossDemoStartInfo(al::CameraInfo* pCameraInfo, al::LiveActor* pActor, const char* pActionName,
                      s32 cancelFrame, const sead::Matrix34f* pBaseMtx);

    al::CameraInfo* mCameraInfo;          // 0x00
    al::LiveActor* mActor;                // 0x08
    const char* mActionName;              // 0x10
    s32 _18;                              // 0x18 camera end interpolation frames (-1 = default)
    s32 mCancelFrame;                     // 0x1C step from which the demo may be cancelled
    const sead::Matrix34f* mBaseMtx;      // 0x20 nullptr uses the actor's base matrix
};
static_assert(sizeof(BossDemoStartInfo) == 0x28);

/** @brief Nerve state that plays a boss's opening demo and returns the players afterwards. */
class BossStateDemoStart : public al::ActorStateBase {
public:
    BossStateDemoStart(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                       BossDemoStartInfo* pInfo);

    void appear() override;
    void setReturnTrans(const sead::Vector3f& rTrans);
    void setReturnTransAndQuat(const sead::Vector3f& rTrans, const sead::Quatf& rQuat);
    void exeDemoStart();
    void exeDemo();
    void endDemo(bool isSkip);
    void exeFade();
    void exeCancel();

    /** @brief Checks whether the demo ended by a skip or cancel. @return True if skipped. */
    bool isSkipped() const { return _44; }

private:
    BossDemoStartInfo* mInfo;                         // 0x20
    sead::Vector3f mReturnTrans = {0.0f, 0.0f, 0.0f}; // 0x28
    sead::Quatf mReturnQuat = sead::Quatf::unit;      // 0x34

public:
    bool _44 = false;  // 0x44 demo was skipped or cancelled

private:
    al::AudioDirector* mAudioDirector = nullptr;      // 0x48
    sead::FixedSafeString<32> mStartRecorderId;       // 0x50
    sead::FixedSafeString<32> mEndRecorderId;         // 0x88
    DemoSkipLayout* mSkipLayout = nullptr;            // 0xC0
    al::WipeSimple* mWipe;                            // 0xC8
};
static_assert(sizeof(BossStateDemoStart) == 0xd0);
