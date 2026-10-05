#include "MapObj/SwitchRotateSwitch.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"

namespace {
    NERVE_DECL(SwitchRotateSwitch, OffWait);
    NERVE_DECL(SwitchRotateSwitch, On);
    NERVE_DECL(SwitchRotateSwitch, OnWait);
    NERVE_DECL(SwitchRotateSwitch, Off);
    NERVES_MAKE_NOSTRUCT(SwitchRotateSwitch, OffWait, On, OnWait, Off)
}

SwitchRotateSwitch::SwitchRotateSwitch(const char* pName) : al::LiveActor(pName) {
}

void SwitchRotateSwitch::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TrampleSwitchRotate", nullptr);
    al::initNerve(this, &NrvSwitchRotateSwitchOffWait, 0);
    mMtxConnector = al::createMtxConnector(this);
    al::invalidateClipping(this);
    makeActorAppeared();
}

void SwitchRotateSwitch::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mMtxConnector, this, false);
}

void SwitchRotateSwitch::control() {
    if (mOffDelay - 1 >= 0) {
        --mOffDelay;
    }
    al::connectPoseQT(this, mMtxConnector);
}

bool SwitchRotateSwitch::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if ((al::isMsgPlayerTouch(pMsg) || al::isMsgPlayerFloorTouch(pMsg)) &&
        al::isNerve(this, &NrvSwitchRotateSwitchOffWait)) {
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        sead::Vector3f playerUp;
        al::calcUpDir(&playerUp, al::getSensorHost(pOther));
        if (al::calcAngleDegree(up, playerUp) < 5.0f) {
            al::invalidateCollisionParts(this);
            al::setNerve(this, &NrvSwitchRotateSwitchOn);
            return true;
        }
    }
    return false;
}

bool SwitchRotateSwitch::isOnSwitch() const {
    return al::isNerve(this, &NrvSwitchRotateSwitchOn) || al::isNerve(this, &NrvSwitchRotateSwitchOnWait);
}

void SwitchRotateSwitch::requestOffSwitch(int frames) {
    if (isOnSwitch()) {
        mOffDelay = frames;
    } else {
        mOffDelay = -1;
    }
}

void SwitchRotateSwitch::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OffWait");
    }
}

void SwitchRotateSwitch::exeOn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "On");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSwitchRotateSwitchOnWait);
    }
}

void SwitchRotateSwitch::exeOnWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OnWait");
    }
    if (mOffDelay == 0) {
        al::setNerve(this, &NrvSwitchRotateSwitchOff);
    }
}

void SwitchRotateSwitch::exeOff() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Off");
    }
    if (al::isActionEnd(this)) {
        al::validateCollisionParts(this);
        al::setNerve(this, &NrvSwitchRotateSwitchOffWait);
    }
}

SwitchRotateSwitch::~SwitchRotateSwitch() {
}
