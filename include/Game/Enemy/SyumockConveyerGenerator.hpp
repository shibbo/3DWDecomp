#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class ConveyerKeyKeeper; template<class T> class DeriveActorGroup; }
class SyumockConveyer;

class SyumockConveyerGenerator : public al::LiveActor {
public:
    SyumockConveyerGenerator(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void startClipped() override;
    void endClipped() override;
    void exeMove();
private:
    al::DeriveActorGroup<SyumockConveyer>* mActors;
    al::ConveyerKeyKeeper* mKeys;
    f32 mCoord;
    f32 mMoveSpeed;
    f32 mInterval;
    f32 mMaxCoord;
};
static_assert(sizeof(SyumockConveyerGenerator) == 0x168);
