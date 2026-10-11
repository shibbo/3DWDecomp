#include "MapObj/TestTatsutaBlock.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(TestTatsutaBlock, Move);
    NERVE_DECL(TestTatsutaBlock, Wait);
    NERVE_DECL(TestTatsutaBlock, Break);
    NERVES_MAKE_NOSTRUCT(TestTatsutaBlock, Break, Move, Wait)
}

TestTatsutaBlock::TestTatsutaBlock(const char* pName) : al::LiveActor(pName) {}
TestTatsutaBlock::~TestTatsutaBlock() {}

void TestTatsutaBlock::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTestTatsutaBlockMove, 0);
    if (al::isExistRail(this))
        al::setSyncRailToNearestPos(this);
    else
        al::setNerve(this, &NrvTestTatsutaBlockWait);
    mBreakModel = new al::BreakModel(this, "壊れモデル", "BlockBrickBreak", nullptr,
                                    nullptr, "Break", true);
    mBreakModel->init(rInfo);
    makeActorAppeared();
}

bool TestTatsutaBlock::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (al::isMsgPlayerUpperPunch(pMsg) || al::isMsgPlayerHipDropAll(pMsg)) {
        al::setNerve(this, &NrvTestTatsutaBlockBreak);
        return true;
    }
    return false;
}

void TestTatsutaBlock::exeStop() {}
void TestTatsutaBlock::exeWait() {}

void TestTatsutaBlock::exeMove() {
    al::moveSyncRail(this, 3.0f);
}

void TestTatsutaBlock::exeBreak() {
    al::setTrans(mBreakModel, al::getTrans(this));
    mBreakModel->appear();
    kill();
}
