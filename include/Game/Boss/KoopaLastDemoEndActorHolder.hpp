#pragma once
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
namespace al { class LiveActor; class ActorInitInfo; }
class KoopaLastPartsModel;
class KoopaLastDemoEndActorHolder {
public:
    explicit KoopaLastDemoEndActorHolder(al::LiveActor* pHost);
    void init(const al::ActorInitInfo& rInfo, const sead::Matrix34f* pMtx);
    void appear();
    void startEndDemo();
private:
    al::LiveActor* mHost;
    sead::PtrArray<KoopaLastPartsModel> mActors;
    sead::PtrArray<KoopaLastPartsModel> mDemoActors;
};
static_assert(sizeof(KoopaLastDemoEndActorHolder) == 0x28);
