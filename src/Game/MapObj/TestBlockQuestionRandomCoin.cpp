#include "MapObj/TestBlockQuestionRandomCoin.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(TestBlockQuestionRandomCoin, Wait);
    NERVE_DECL(TestBlockQuestionRandomCoin, Reaction);
    NERVE_DECL(TestBlockQuestionRandomCoin, HipDropReaction);
    NERVE_DECL(TestBlockQuestionRandomCoin, AppearCoin);
    NERVE_DECL(TestBlockQuestionRandomCoin, Empty);
    NERVES_MAKE_NOSTRUCT(TestBlockQuestionRandomCoin, Wait, Reaction, HipDropReaction, AppearCoin, Empty)
}
TestBlockQuestionRandomCoin::TestBlockQuestionRandomCoin(const char* name) : al::LiveActor(name) {}
TestBlockQuestionRandomCoin::~TestBlockQuestionRandomCoin() {}
void TestBlockQuestionRandomCoin::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvTestBlockQuestionRandomCoinWait, 1);
    mEmptyBlock = new BlockEmpty("空ブロック", "BlockEmpty");
    al::initCreateActorWithPlacementInfo(mEmptyBlock, info);
    mEmptyBlock->makeActorDead();
    makeActorAppeared();
}
bool TestBlockQuestionRandomCoin::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if ((al::isNerve(this, &NrvTestBlockQuestionRandomCoinReaction) && al::isLessStep(this, 7)) || al::isNerve(this, &NrvTestBlockQuestionRandomCoinHipDropReaction)) return false;
    if (al::isMsgPlayerUpperPunch(msg) || al::isMsgPlayerRollingAttack(msg) || al::isMsgPlayerTailAttack(msg) || al::isMsgPlayerBoomerangAttack(msg) || al::isMsgKickKouraAttack(msg)) {
        sead::Vector3f direction = sead::Vector3f::ey;
        direction += sead::Vector3f(al::getRandom(-3.5f, 3.5f), 0.0f, al::getRandom(-3.5f, 3.5f));
        if (!al::normalizeOrZero(&direction)) {
            al::appearItemTiming(this, "出現", al::getTrans(this) + sead::Vector3f::ey * 225.0f, direction * 1.5f);
            al::setNerve(this, &NrvTestBlockQuestionRandomCoinReaction);
            return true;
        }
    }
    if (al::isMsgPlayerHipDropAll(msg)) {
        al::setNerve(this, &NrvTestBlockQuestionRandomCoinHipDropReaction);
        return true;
    }
    return false;
}
void TestBlockQuestionRandomCoin::control() {
    if (al::isNerve(this, &NrvTestBlockQuestionRandomCoinAppearCoin) || al::isNerve(this, &NrvTestBlockQuestionRandomCoinReaction)) {
        if (mCoinTimer++ >= 180) al::setNerve(this, &NrvTestBlockQuestionRandomCoinEmpty);
    }
}
void TestBlockQuestionRandomCoin::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
}
void TestBlockQuestionRandomCoin::exeAppearCoin() {}
void TestBlockQuestionRandomCoin::exeEmpty() {
    al::copyPose(mEmptyBlock, this);
    mEmptyBlock->appear();
    kill();
}
void TestBlockQuestionRandomCoin::exeReaction() {
    if (al::isFirstStep(this)) al::startAction(this, "Reaction");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvTestBlockQuestionRandomCoinAppearCoin);
}
void TestBlockQuestionRandomCoin::exeHipDropReaction() {
    if (al::isIntervalStep(this, 7, 0)) {
        al::startAction(this, "Reaction");
        sead::Vector3f direction = sead::Vector3f::ey;
        direction += sead::Vector3f(al::getRandom(-3.5f, 3.5f), 0.0f, al::getRandom(-3.5f, 3.5f));
        if (!al::normalizeOrZero(&direction)) al::appearItemTiming(this, "出現", al::getTrans(this) + sead::Vector3f::ey * 200.0f, direction * 1.5f);
    }
    if (al::isGreaterStep(this, 120)) al::setNerve(this, &NrvTestBlockQuestionRandomCoinEmpty);
}
