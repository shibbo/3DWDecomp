#pragma once
#include "Library/StageSwitch/Core/IUseStageSwitch.hpp"
#include <math/seadVector.h>
#include <container/seadPtrArray.h>
namespace al { class LiveActor; struct PlacementInfo; class ActorInitInfo; }
class WarpCube;
class KoopaChaseWarpDummy;

class KoopaChaseMovePos : public al::IUseStageSwitch {
public:
    KoopaChaseMovePos(const al::PlacementInfo& rPlacement, const al::ActorInitInfo& rInfo,
                      KoopaChaseMovePos* pPrevious, sead::PtrArray<KoopaChaseMovePos>* pPoints);
    const char* getName() const override;
    al::StageSwitchKeeper* getStageSwitchKeeper() const override;
    void initStageSwitchKeeper() override;

    sead::Vector3f mTrans;
    int mValue14;
    KoopaChaseMovePos* mPoint18;
    int mType;
    int mValue24;
    al::LiveActor* mWarpActor;
    WarpCube* mWarpCube;
    KoopaChaseWarpDummy* mWarpDummy;
    const char* mName;
    al::StageSwitchKeeper* mStageSwitchKeeper;
};

static_assert(sizeof(KoopaChaseMovePos) == 0x50);
