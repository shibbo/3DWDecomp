#include "MapObj/KeyMoveLoopLiftGenerator.hpp"
#include "MapObj/KeyMoveLoopLift.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(KeyMoveLoopLiftGenerator, StandBy);
    NERVE_DECL(KeyMoveLoopLiftGenerator, Generate);
    NERVES_MAKE_NOSTRUCT(KeyMoveLoopLiftGenerator, StandBy, Generate)
}
KeyMoveLoopLiftGenerator::KeyMoveLoopLiftGenerator(const char* pName) : al::LiveActor(pName) {}
KeyMoveLoopLiftGenerator::~KeyMoveLoopLiftGenerator() {}
void KeyMoveLoopLiftGenerator::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 64);
    int count = 4;
    al::tryGetArg(&count, rInfo, "PartsCount");
    al::tryGetArg(&mGenerateInterval, rInfo, "GenerateInterval");
    al::initSubActorKeeperNoFile(this, rInfo, count);
    mLifts = new al::DeriveActorGroup<KeyMoveLoopLift>("キー移動ループリフト", count);
    if (!al::calcLinkChildNum(rInfo, "Generate")) {
        makeActorDead();
        return;
    }
    for (int i = 0; i < count; ++i) {
        const char* name = al::getLinksActorDisplayName(rInfo, "Generate", 0);
        KeyMoveLoopLift* lift = new KeyMoveLoopLift(name);
        al::initLinksActor(lift, rInfo, "Generate", 0);
        mLifts->registerActor(lift);
    }
    mStandByLift = mLifts->getDeriveActor(0);
    mStandByLift->startStandBy();
    al::setKeyMoveClippingInfo(this, &mClippingCenter, mStandByLift->getKeyPoseKeeper());
    mStandByArea = al::createLinkAreaGroup(this, rInfo, "LeaveCheckArea", "置いてきぼりチェックエリアグループ", "置いてきぼりチェックエリア");
    al::initNerve(this, &NrvKeyMoveLoopLiftGeneratorStandBy, 0);
    makeActorAppeared();
}
void KeyMoveLoopLiftGenerator::exeStandBy() {
    if (mStandByLift->isStandByEnd()) {
        mStandByLift = nullptr;
        al::setNerve(this, &NrvKeyMoveLoopLiftGeneratorGenerate);
    }
}
void KeyMoveLoopLiftGenerator::exeGenerate() {
    if (!al::isIntervalStep(this, mGenerateInterval, -1)) return;
    KeyMoveLoopLift* lift = mLifts->tryFindDeadDeriveActor();
    if (!lift) return;
    if (isInStandByArea()) {
        mStandByLift = lift;
        lift->startAppearAndStandBy();
        al::setNerve(this, &NrvKeyMoveLoopLiftGeneratorStandBy);
    } else lift->startAppear();
}
bool KeyMoveLoopLiftGenerator::isInStandByArea() const {
    return mStandByArea && al::isInAreaObjPlayerAll(this, mStandByArea);
}
