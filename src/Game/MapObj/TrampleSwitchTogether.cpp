#include "MapObj/TrampleSwitchTogether.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(TrampleSwitchTogether, OffWait);
    NERVE_DECL(TrampleSwitchTogether, On);
    NERVE_DECL(TrampleSwitchTogether, OnWait);
    NERVE_DECL(TrampleSwitchTogether, Off);
    NERVE_DECL(TrampleSwitchTogether, Success);
    NERVES_MAKE_NOSTRUCT(TrampleSwitchTogether, OffWait, On, OnWait, Off, Success)
}

TrampleSwitchTogether::TrampleSwitchTogether(const char* pName) : al::LiveActor(pName) {
}

void TrampleSwitchTogether::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTrampleSwitchTogetherOffWait, 0);
    al::killPrePassLightAll(this, 0);
    makeActorAppeared();
}

void TrampleSwitchTogether::control() {
    if (mPressFrames != 0) {
        --mPressFrames;
    }
}

bool TrampleSwitchTogether::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerFloorTouch(pMsg) || al::isMsgBallTrample(pMsg) ||
        al::isMsgBallAttack(pMsg) || al::isMsgBallAttackCollide(pMsg) ||
        al::isMsgBallTrampleCollide(pMsg)) {
        mPressFrames = 8;
        if (al::isNerve(this, &NrvTrampleSwitchTogetherOffWait)) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvTrampleSwitchTogetherOn);
            return true;
        }
        return false;
    }
    return false;
}

void TrampleSwitchTogether::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OffWait");
        al::validateClipping(this);
    }
}

void TrampleSwitchTogether::exeOn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "On");
        al::appearPrePassLightAll(this, 10);
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTrampleSwitchTogetherOnWait);
    }
}

void TrampleSwitchTogether::exeOnWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OnWait");
    }
    if (mPressFrames == 0) {
        al::setNerve(this, &NrvTrampleSwitchTogetherOff);
    }
}

void TrampleSwitchTogether::exeOff() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Off");
        al::killPrePassLightAll(this, 10);
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTrampleSwitchTogetherOffWait);
    }
}

void TrampleSwitchTogether::exeSuccess() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Success");
        al::killPrePassLightAll(this, 10);
        al::validateClipping(this);
    }
}

bool TrampleSwitchTogether::isOnWait() const {
    return al::isNerve(this, &NrvTrampleSwitchTogetherOnWait);
}

void TrampleSwitchTogether::success() {
    al::setNerve(this, &NrvTrampleSwitchTogetherSuccess);
}

TrampleSwitchTogether::~TrampleSwitchTogether() {
}
