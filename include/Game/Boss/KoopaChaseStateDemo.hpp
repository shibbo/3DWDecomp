#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>

namespace al {
class ActorInitInfo;
class LiveActor;
}  // namespace al
class KoopaChaseDemoInfo;

class KoopaChaseStateDemo : public al::NerveStateBase {
public:
    KoopaChaseStateDemo(const char* pName, al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                        KoopaChaseDemoInfo* pDemoInfo);
    void entryDemoActor(al::LiveActor* pActor);
    void setReturnTrans(const sead::Vector3f& rTrans);

    bool isSkipped() const { return mIsSkipped; }

private:
    // Remaining state members are not yet reconstructed.
    unsigned char mStateData11[0xb7];
    bool mIsSkipped;
    unsigned char mStateDataC9[0x17];
};
static_assert(sizeof(KoopaChaseStateDemo) == 0xe0);
