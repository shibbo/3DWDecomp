#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class NeedleRollerFall;
class NeedleRollerGenerator : public al::LiveActor {
public:
    explicit NeedleRollerGenerator(const char*);
    ~NeedleRollerGenerator() override;
    void init(const al::ActorInitInfo&) override;
    void start();
    void stop();
    void killAll();
    void exeStandBy();
    void exeGenerate();
private:
    al::DeriveActorGroup<NeedleRollerFall>* mRollers = nullptr;
    int mGenerateInterval = 180;
    int mGenerateDelay = 0;
};
