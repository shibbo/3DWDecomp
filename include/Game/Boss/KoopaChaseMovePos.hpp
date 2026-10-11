#pragma once
#include "Library/StageSwitch/Core/IUseStageSwitch.hpp"
#include <math/seadVector.h>

class KoopaChaseMovePos : public al::IUseStageSwitch {
public:
    const char* getName() const override;
    al::StageSwitchKeeper* getStageSwitchKeeper() const override;
    void initStageSwitchKeeper() override;

    sead::Vector3f mTrans;
    int mValue14;
    KoopaChaseMovePos* mPoint18;
    int mType;
    int mValue24;
    KoopaChaseMovePos* mPoint28;
    KoopaChaseMovePos* mPoint30;
    KoopaChaseMovePos* mPoint38;
    const char* mName;
    al::StageSwitchKeeper* mStageSwitchKeeper;
};
