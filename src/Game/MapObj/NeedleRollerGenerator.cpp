#include "MapObj/NeedleRollerGenerator.hpp"
#include "MapObj/NeedleRollerFall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
namespace {
    NERVE_DECL(NeedleRollerGenerator, StandBy);
    NERVE_DECL(NeedleRollerGenerator, Generate);
    NERVES_MAKE_NOSTRUCT(NeedleRollerGenerator, StandBy, Generate)
}
NeedleRollerGenerator::NeedleRollerGenerator(const char* name) : al::LiveActor(name) {}
NeedleRollerGenerator::~NeedleRollerGenerator() {}
void NeedleRollerGenerator::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "NeedleRollerGenerator", nullptr);
    al::initNerve(this, &NrvNeedleRollerGeneratorStandBy, 0);
    al::tryGetArg(&mGenerateInterval, info, "GenerateInterval");
    al::tryGetArg(&mGenerateDelay, info, "GenerateDelay");
    auto* group = new al::DeriveActorGroup<NeedleRollerFall>("落下トゲローラーリスト", 8);
    mRollers = group;
    for (int i = 0; i < group->mMaxActors; ++i) {
        auto* roller = new NeedleRollerFall("落下トゲローラー");
        al::initCreateActorWithPlacementInfo(roller, info);
        group->registerActor(roller);
    }
    if (!al::listenStageSwitchOnOffStart(this, al::Functor(this, &NeedleRollerGenerator::start), al::Functor(this, &NeedleRollerGenerator::stop))) start();
    al::listenStageSwitchOnKill(this, al::Functor(this, &NeedleRollerGenerator::killAll));
    al::invalidateClipping(this);
    makeActorAppeared();
}
void NeedleRollerGenerator::start() {
    if (al::isNerve(this, &NrvNeedleRollerGeneratorStandBy)) al::setNerve(this, &NrvNeedleRollerGeneratorGenerate);
}
void NeedleRollerGenerator::stop() {
    if (al::isNerve(this, &NrvNeedleRollerGeneratorGenerate)) al::setNerve(this, &NrvNeedleRollerGeneratorStandBy);
}
void NeedleRollerGenerator::killAll() {
    al::setNerve(this, &NrvNeedleRollerGeneratorStandBy);
    mRollers->killAll();
    kill();
}
void NeedleRollerGenerator::exeStandBy() {}
void NeedleRollerGenerator::exeGenerate() {
    if (!al::isIntervalStep(this, mGenerateInterval, mGenerateDelay)) return;
    auto* roller = mRollers->tryFindDeadDeriveActor();
    if (roller) roller->start();
}
