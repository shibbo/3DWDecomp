#include "MapObj/TestNut.hpp"
#include "MapObj/ItemStatePlayerHold.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(TestNut, Wait);
    NERVE_DECL(TestNut, PlayerHold);
    NERVE_DECL(TestNut, Throw);
    NERVES_MAKE_STRUCT(TestNut, Wait, PlayerHold, Throw)
    ItemStatePopUpFrontParam sThrowParam(sead::Vector3f(0.0f, 28.0f, 5.0f),
                                       1.0f, 0.99f, 30, 0.7f, true, "PopUp", false, nullptr);
}
TestNut::TestNut(const char* pName) : al::LiveActor(pName) {}
TestNut::~TestNut() {}
void TestNut::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Nut", nullptr);
    al::initNerve(this, &NrvTestNut.Wait, 2);
    mHoldState = new ItemStatePlayerHold(this, nullptr, false, false);
    al::initNerveState(this, mHoldState, &NrvTestNut.PlayerHold, "プレイヤーに持たれる");
    mThrowState = new ItemStatePopUpFront(this);
    al::initNerveState(this, mThrowState, &NrvTestNut.Throw, "投げられる");
    al::offCollide(this);
    const char* itemType;
    al::tryGetStringArg(&itemType, rInfo, "ItemType");
    rc::initItemByHostInfo(this, rInfo, 1);
    makeActorAppeared();
}
void TestNut::kill() {
    al::tryOnSwitchDeadOn(this);
    al::LiveActor::kill();
}
bool TestNut::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvTestNut.Wait) || al::isNerve(this, &NrvTestNut.Throw)) {
        if (mHoldState->tryStartCarryUp(pMsg, pOther, true)) {
            al::setNerve(this, &NrvTestNut.PlayerHold);
            return true;
        }
        return false;
    }
    if (al::isNerve(this, &NrvTestNut.PlayerHold)) {
        if (mHoldState->receiveMsg(pMsg, pOther, pSelf)) {
            mThrower = pOther;
            mThrowState->setParam(sThrowParam, pOther);
            al::setNerve(this, &NrvTestNut.Throw);
            return true;
        }
        return false;
    }
    return false;
}
void TestNut::exeWait() {}
void TestNut::exePlayerHold() { al::updateNerveState(this); }
void TestNut::exeThrow() {
    if (al::updateNerveState(this) || al::isCollidedVelocity(this)) {
        sead::Vector3f direction;
        al::calcFrontDir(&direction, this);
        al::appearItem(this, al::getTrans(this), direction, mThrower);
        kill();
    }
}
