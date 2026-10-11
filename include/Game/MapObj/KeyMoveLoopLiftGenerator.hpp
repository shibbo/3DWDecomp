#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
namespace al { class AreaObjGroup; }
class KeyMoveLoopLift;
class KeyMoveLoopLiftGenerator : public al::LiveActor {
public:
    explicit KeyMoveLoopLiftGenerator(const char*);
    ~KeyMoveLoopLiftGenerator() override;
    void init(const al::ActorInitInfo&) override;
    void exeStandBy();
    void exeGenerate();
    bool isInStandByArea() const;
private:
    al::DeriveActorGroup<KeyMoveLoopLift>* mLifts = nullptr;
    KeyMoveLoopLift* mStandByLift = nullptr;
    al::AreaObjGroup* mStandByArea = nullptr;
    sead::Vector3f mClippingCenter = sead::Vector3f::zero;
    int mGenerateInterval = 60;
};
