#include "MapObj/OneSideStepGenerator.hpp"
#include "MapObj/OneSideStep.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(OneSideStepGenerator, Generate);
    NERVES_MAKE_NOSTRUCT(OneSideStepGenerator, Generate)
}
OneSideStepGenerator::OneSideStepGenerator(const char* name) : al::LiveActor(name) {}
OneSideStepGenerator::~OneSideStepGenerator() {}
void OneSideStepGenerator::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initExecutorMapObjMovement(this, info);
    al::initActorPoseTQSV(this);
    int count = 4;
    al::tryGetArg(&count, info, "PartsCount");
    al::tryGetArg(&mGenerateInterval, info, "GenerateInterval");
    al::tryGetArg(&mDelayTime, info, "DelayTime");
    al::initSubActorKeeperNoFile(this, info, count);
    mSteps = new al::DeriveActorGroup<OneSideStep>("半アタリ床リスト", count);
    if (!al::calcLinkChildNum(info, "Generate")) {
        makeActorDead();
        return;
    }
    for (int i = 0; i < count; ++i) {
        OneSideStep* step = new OneSideStep("半アタリ床");
        al::initLinksActor(step, info, "Generate", 0);
        step->setGenerated();
        step->makeActorDead();
        mSteps->registerActor(step);
    }
    OneSideStep* first = mSteps->getDeriveActor(0);
    float radius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, first->getKeyPoseKeeper(), 500.0f);
    al::initActorClipping(this, info);
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::initGroupClipping(this, info, 64);
    al::initNerve(this, &NrvOneSideStepGeneratorGenerate, 0);
    makeActorAppeared();
}
void OneSideStepGenerator::exeDelay() {
    if (al::isGreaterEqualStep(this, mDelayTime - 1))
        al::setNerve(this, &NrvOneSideStepGeneratorGenerate);
}
void OneSideStepGenerator::exeGenerate() {
    if (!al::isIntervalStep(this, mGenerateInterval, 0)) return;
    OneSideStep* step = mSteps->tryFindDeadDeriveActor();
    if (step) step->appearAndSetStart();
}
