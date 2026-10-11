#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include <container/seadPtrArray.h>
class RaftStep;
class RaftKeyKeeper;
class RaftConveyer : public al::LiveActor {
public:
    RaftConveyer(const char*);
    ~RaftConveyer() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    void startClipped() override;
    void endClipped() override;
private:
    al::DeriveActorGroup<RaftStep>* mRafts = nullptr;
    sead::PtrArray<RaftStep> mAvailableRafts;
    RaftStep* mLastRaft = nullptr;
    RaftKeyKeeper* mKeyKeeper = nullptr;
    float mPartsInterval = 200.0f;
    sead::Vector3f mClippingCenter{0.0f, 0.0f, 0.0f};
};
