#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al { class JointAimInfo; }

class KoopaLastStateTop : public al::ActorStateBase {
public:
    explicit KoopaLastStateTop(al::LiveActor* pActor);
    void appear() override;
    void exeClimbEnd();
    void exeWait();
    void exeShout();

private:
    int mWaitFrames = 0;
    al::JointAimInfo* mJointAimInfo = nullptr;
};

static_assert(sizeof(KoopaLastStateTop) == 0x30);
