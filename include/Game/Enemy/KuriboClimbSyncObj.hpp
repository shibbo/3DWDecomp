#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
class KuriboClimbRail;

class KuriboClimbSyncObj : public al::LiveActor {
public:
    KuriboClimbSyncObj(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
private:
    sead::PtrArray<KuriboClimbRail> mActors;
};
static_assert(sizeof(KuriboClimbSyncObj) == 0x158);
