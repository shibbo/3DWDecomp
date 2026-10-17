#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class ActorInitInfo; }
class KoopaChase;
class KoopaChaseStateFire;

class KoopaChaseStateProvocation : public al::NerveStateBase {
public:
    KoopaChaseStateProvocation(KoopaChase* pHost, const al::ActorInitInfo& rInfo);
    void appear() override;
    bool tryEnd(bool isKeepProvoking);
    void exeWait();
    void exeProvocation();
    void exeFire();

    void setFireEnabled(bool isEnabled) { mIsFireEnabled = isEnabled; }
    void setClearInterpole(bool isClear) { mIsClearInterpole = isClear; }

private:
    KoopaChase* mHost;
    KoopaChaseStateFire* mStateFire = nullptr;
    bool mIsFireEnabled = false;
    int mProvocationCount = 0;
    bool mIsClearInterpole = false;
};
static_assert(sizeof(KoopaChaseStateProvocation) == 0x38);
