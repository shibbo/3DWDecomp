#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>
namespace al { class ActorInitInfo; class CameraInfo; class WipeSimple; }
class KoopaLastBlockPow;
class KoopaLastDefendeBlockPow;
/** @brief Ending cutscene triggered by the final POW block. */
class KoopaLastBlockPowStateDemoEnd : public al::ActorStateBase {
public:
    KoopaLastBlockPowStateDemoEnd(KoopaLastBlockPow* pHost, KoopaLastDefendeBlockPow* pDefender, const al::ActorInitInfo& rInfo);
    ~KoopaLastBlockPowStateDemoEnd() override;
    void appear() override;
    void exeStart();
    void exeDamageWait();
    void exeDemo();
    void exeFadeOut();
    void exeFadeIn();
    void exeEnd();
private:
    KoopaLastBlockPow* mBlock;
    KoopaLastDefendeBlockPow* mDefender;
    sead::FixedSafeString<32> mStartRecorderId;
    sead::FixedSafeString<32> mEndRecorderId;
    al::CameraInfo* mCameraInfo = nullptr;
    sead::Vector3f mTargetPos = {0.0f,0.0f,0.0f};
    sead::Quatf mTargetQuat = sead::Quatf::unit;
    al::WipeSimple* mWipe = nullptr;
};
static_assert(sizeof(KoopaLastBlockPowStateDemoEnd) == 0xd0);
