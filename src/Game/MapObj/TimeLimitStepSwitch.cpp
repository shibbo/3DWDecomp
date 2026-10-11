#include "MapObj/TimeLimitStepSwitch.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Movement/FlashingTimer.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
    NERVE_DECL(TimeLimitStepSwitch, OffWait);
    NERVE_DECL(TimeLimitStepSwitch, OnWait);
    NERVE_DECL(TimeLimitStepSwitch, On);
    NERVE_DECL(TimeLimitStepSwitch, Off);
    NERVES_MAKE_NOSTRUCT(TimeLimitStepSwitch, OffWait, OnWait, On, Off)
}

TimeLimitStepSwitch::TimeLimitStepSwitch(const char* pName) : al::LiveActor(pName) {
}

void TimeLimitStepSwitch::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TrampleSwitch", nullptr);
    al::tryGetArg(&mAppearTime, rInfo, "AppearTime");
    int count = al::calcLinkChildNum(rInfo, "TimeLimitStepList");
    if (count <= 0) {
        makeActorDead();
    } else {
        ProjectActorFactory factory;
        mSteps = new al::LiveActorGroup("足場リスト", count);
        for (int i = 0; i < count; ++i) {
            al::LiveActor* step = al::createLinksActorFromFactory(factory, rInfo, "TimeLimitStepList", i);
            step->makeActorDead();
            mSteps->registerActor(step);
        }
        mTimer = new al::FlashingTimer(mAppearTime, 180, 20, 10);
        al::initNerve(this, &NrvTimeLimitStepSwitchOffWait, 0);
        mMtxConnector = al::createMtxConnector(this);
        makeActorAppeared();
    }
}

void TimeLimitStepSwitch::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mMtxConnector, this, false);
}

void TimeLimitStepSwitch::control() {
    al::connectPoseQT(this, mMtxConnector);
}

void TimeLimitStepSwitch::showModelStep() {
    int count = mSteps->mNumActors;
    for (int i = 0; i < count; ++i) {
        al::sendMsgShowModel(al::getHitSensor(mSteps->getActor(i), 0), al::getHitSensor(this, nullptr));
    }
}

void TimeLimitStepSwitch::hideModelStep() {
    int count = mSteps->mNumActors;
    for (int i = 0; i < count; ++i) {
        al::sendMsgHideModel(al::getHitSensor(mSteps->getActor(i), 0), al::getHitSensor(this, nullptr));
    }
}

void TimeLimitStepSwitch::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::startAction(this, "OffWait");
    }
}

void TimeLimitStepSwitch::exeOn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "On");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTimeLimitStepSwitchOnWait);
    }
}

void TimeLimitStepSwitch::exeOnWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OnWait");
        mTimer->start();
        showModelStep();
        mSteps->appearAll();
    }
    mTimer->update();
    if (mTimer->isVisible() && !mTimer->wasVisible()) {
        if (mTimer->isHurryStart()) {
            hideModelStep();
        }
        if (getAudioKeeper()) {
            al::startSeByName(this, "CountH", nullptr);
        }
    }
    if (!mTimer->isVisible() && mTimer->wasVisible()) {
        if (mTimer->isHurryStart()) {
            showModelStep();
        }
        if (getAudioKeeper()) {
            al::startSeByName(this, "CountL", nullptr);
        }
    }
    if (mTimer->getTime() <= 0) {
        mSteps->killAll();
        al::setNerve(this, &NrvTimeLimitStepSwitchOff);
    }
}

void TimeLimitStepSwitch::exeOff() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Off");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTimeLimitStepSwitchOffWait);
    }
}

bool TimeLimitStepSwitch::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerFloorTouch(pMsg) || al::isMsgBallTrample(pMsg) ||
        al::isMsgBallAttack(pMsg) || al::isMsgBallAttackCollide(pMsg) ||
        al::isMsgBallTrampleCollide(pMsg) || rc::isMsgKillerTouch(pMsg)) {
        if (al::isNerve(this, &NrvTimeLimitStepSwitchOffWait)) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvTimeLimitStepSwitchOn);
            return true;
        }
        return false;
    }
    return false;
}

TimeLimitStepSwitch::~TimeLimitStepSwitch() {
}
