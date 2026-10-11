#include "MapObj/KoopaFireBallGenerator.hpp"
#include "MapObj/KoopaFireBall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
    NERVE_DECL(KoopaFireBallGenerator, Hide);
    NERVE_DECL(KoopaFireBallGenerator, Delay);
    NERVE_DECL(KoopaFireBallGenerator, Fall);
    NERVES_MAKE_NOSTRUCT(KoopaFireBallGenerator, Hide, Delay, Fall)
}
KoopaFireBallGenerator::KoopaFireBallGenerator(const char* pName) : al::LiveActor(pName) {}
KoopaFireBallGenerator::~KoopaFireBallGenerator() {}
void KoopaFireBallGenerator::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "KoopaFireBallGenerator", nullptr);
    al::tryGetArg(&mDistance, rInfo, "Distance");
    al::tryGetArg(&mDelayAppearFrame, rInfo, "DelayAppearFrame");
    al::tryGetArg(&mGenerateFrame, rInfo, "GenerateFrame");
    al::tryGetArg(&mIsDisasterCameraOn, rInfo, "IsDisasterCameraOn");
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    bool isGiant = al::isEqualString(getName(), "クッパ火炎弾生成(大)");
    mFireBall = new KoopaFireBall("クッパ火炎弾(ステージ用)", isGiant, nullptr);
    al::initCreateActorNoPlacementInfo(mFireBall, rInfo);
    al::initNerve(this, &NrvKoopaFireBallGeneratorHide, 0);
    al::tryListenStageSwitchAppear(this);
}
void KoopaFireBallGenerator::appear() {
    if (mIsSingleMode && mIsDisasterCameraOn && !al::isDisasterMode(this)) return;
    al::LiveActor::appear();
    if (mDelayAppearFrame > 0) al::setNerve(this, &NrvKoopaFireBallGeneratorDelay);
    else al::setNerve(this, &NrvKoopaFireBallGeneratorFall);
}
void KoopaFireBallGenerator::exeDelay() {
    if (al::isLessStep(this, mDelayAppearFrame)) return;
    al::setNerve(this, &NrvKoopaFireBallGeneratorFall);
}
void KoopaFireBallGenerator::exeFall() {
    if (al::isFirstStep(this)) {
        if (mIsSingleMode && mIsDisasterCameraOn && !al::isDisasterMode(this)) {
            al::setNerve(this, &NrvKoopaFireBallGeneratorHide);
            return;
        }
        sead::Vector3f front = sead::Vector3f::ez;
        al::calcFrontDir(&front, this);
        sead::Vector3f start = al::getTrans(this) - front * mDistance;
        mFireBall->appearAttack(start, al::getTrans(this), 16.0f);
    }
    if (al::isDead(mFireBall)) al::setNerve(this, &NrvKoopaFireBallGeneratorHide);
}
void KoopaFireBallGenerator::exeHide() {
    if (al::isLessStep(this, mGenerateFrame)) return;
    if (mIsSingleMode && mIsDisasterCameraOn && !al::isDisasterMode(this)) return;
    al::setNerve(this, &NrvKoopaFireBallGeneratorFall);
}
