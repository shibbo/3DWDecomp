#include "MapObj/TestBlockTuccondor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
    NERVE_DECL(TestBlockTuccondor, Wait);
    NERVE_DECL(TestBlockTuccondor, Reaction);
    NERVES_MAKE_NOSTRUCT(TestBlockTuccondor, Wait, Reaction)
}

TestBlockTuccondor::TestBlockTuccondor(const char* pName) : al::LiveActor(pName) {}
TestBlockTuccondor::~TestBlockTuccondor() {}

void TestBlockTuccondor::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTestBlockTuccondorWait, 1);
    makeActorAppeared();
}

bool TestBlockTuccondor::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
    al::HitSensor* pSelf) {
    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
        al::isMsgPlayerHipDropAll(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
        al::isMsgPlayerUpperPunch(pMsg)) {
        al::setNerve(this, &NrvTestBlockTuccondorReaction);
        return true;
    }
    if (rc::isMsgTuccondorAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) ||
        al::isMsgPlayerGiantTouch(pMsg)) {
        kill();
        return true;
    }
    return false;
}

void TestBlockTuccondor::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvTestBlockTuccondorWait);
}

void TestBlockTuccondor::exeReaction() {
    if (al::isFirstStep(this))
        al::startAction(this, "Reaction");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvTestBlockTuccondorWait);
}
