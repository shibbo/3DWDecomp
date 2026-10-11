#include "MapObj/TrampleSwitch.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    NERVE_DECL(TrampleSwitch, OffWait);
    NERVE_DECL(TrampleSwitch, On);
    NERVE_DECL(TrampleSwitch, OnWait);
    NERVE_DECL(TrampleSwitch, Off);
    NERVES_MAKE_NOSTRUCT(TrampleSwitch, OffWait, On, OnWait, Off)
}

TrampleSwitch::TrampleSwitch(const char* pName) : al::LiveActor(pName) {
}

void TrampleSwitch::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTrampleSwitchOffWait, 0);
    mConnector = al::createMtxConnector(this);
    al::listenStageSwitchOff(this, "SwitchTrampleOn", al::Functor(this, &TrampleSwitch::offSwitch));
    al::listenStageSwitchOff(this, "SwitchReset", al::Functor(this, &TrampleSwitch::resetSwitch));
    if (SingleModeDataFunction::isIslandScenarioIDComplete(this, rInfo)) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

void TrampleSwitch::offSwitch() {
    if (al::isNerve(this, &NrvTrampleSwitchOn) || al::isNerve(this, &NrvTrampleSwitchOnWait)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvTrampleSwitchOff);
    }
}

void TrampleSwitch::resetSwitch() {
    al::setNerve(this, &NrvTrampleSwitchOffWait);
    al::offStageSwitch(this, "SwitchTrampleOn");
}

void TrampleSwitch::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mConnector, this, false);
}

void TrampleSwitch::control() {
    al::connectPoseQT(this, mConnector);
}

void TrampleSwitch::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::startAction(this, "OffWait");
    }
}

void TrampleSwitch::exeOn() {
    bool isSingleMode = mActorSceneInfo->isSingleMode;
    if (isSingleMode) {
        if (al::isFirstStep(this)) {
            al::startAction(this, "On");
        }
        if (al::isStep(this, 2)) {
            al::tryOnStageSwitch(this, "SwitchTrampleOn");
        }
        if (al::isStep(this, 2)) {
            al::startHitReaction(this, "ヒットストップ");
        }
        if (al::isActionPlaying(this, "On") && al::isActionEnd(this) && al::isGreaterStep(this, 2)) {
            al::setNerve(this, &NrvTrampleSwitchOnWait);
        }
    } else {
        if (al::isFirstStep(this)) {
            al::startAction(this, "On");
        }
        if (al::isActionEnd(this)) {
            al::setNerve(this, &NrvTrampleSwitchOnWait);
        }
    }
}

void TrampleSwitch::exeOnWait() {
    bool isSingleMode = mActorSceneInfo->isSingleMode;
    if (isSingleMode) {
        if (al::isFirstStep(this)) {
            al::validateClipping(this);
            if (mIsReusable) {
                mIsHeldOn = true;
            }
            al::startAction(this, "OnWait");
        }
        if (!mIsHeldOn && mIsReusable) {
            al::setNerve(this, &NrvTrampleSwitchOff);
        }
    } else if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::tryOnStageSwitch(this, "SwitchTrampleOn");
        al::startAction(this, "OnWait");
    }
}

void TrampleSwitch::exeOff() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Off");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTrampleSwitchOffWait);
    }
}

bool TrampleSwitch::isTrigSwitchOn() {
    return al::isNerve(this, &NrvTrampleSwitchOn) && al::isStep(this, 2);
}

bool TrampleSwitch::isEarlyTrigOn() {
    return al::isNerve(this, &NrvTrampleSwitchOn);
}

bool TrampleSwitch::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    bool isSingleAttack = GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
                         (rc::isMsgRaidonAttack(pMsg) || al::isMsgKouraThrow(pMsg) || al::isMsgKeyThrow(pMsg));
    if (al::isMsgPlayerFloorTouch(pMsg) || al::isMsgBallTrample(pMsg) || al::isMsgBallAttack(pMsg) ||
        al::isMsgBallAttackCollide(pMsg) || al::isMsgBallTrampleCollide(pMsg) ||
        rc::isMsgKillerTouch(pMsg) || isSingleAttack) {
        do {
            if (!al::isNerve(this, &NrvTrampleSwitchOffWait)) {
                break;
            }
            if (al::isMsgPlayerFloorTouch(pMsg)) {
                if (!rc::isPlayerGiant(pSender)) {
                    float height = al::calcHeight(this, al::getActorTrans(pSender));
                    if (height < 50.0f) {
                        return false;
                    }
                }
            }
            al::invalidateClipping(this);
            rc::addScore(this, pSender, 0.0f, 0);
            al::setNerve(this, &NrvTrampleSwitchOn);
            return true;
        } while (false);
    } else if (al::isSensorKoopaJr(pSender) && al::isMsgPlayerHipDropAll(pMsg) &&
        al::isNerve(this, &NrvTrampleSwitchOffWait)) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvTrampleSwitchOn);
        return true;
    }
    return false;
}
