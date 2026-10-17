#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class OneSideStep;
class OneSideStepGenerator : public al::LiveActor {
public:
    explicit OneSideStepGenerator(const char*);
    ~OneSideStepGenerator() override;
    void init(const al::ActorInitInfo&) override;
    void exeDelay();
    void exeGenerate();
private:
    al::DeriveActorGroup<OneSideStep>* mSteps = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    int mDelayTime = 0;
    int mGenerateInterval = 60;
};
static_assert(sizeof(OneSideStepGenerator) == 0x168);
