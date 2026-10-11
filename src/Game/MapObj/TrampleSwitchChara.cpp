#include "MapObj/TrampleSwitchChara.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    NERVE_DECL(TrampleSwitchChara, OffWait);
    NERVE_DECL(TrampleSwitchChara, OnWait);
    NERVE_DECL(TrampleSwitchChara, On);
    NERVE_DECL(TrampleSwitchChara, Reaction);
    NERVES_MAKE_NOSTRUCT(TrampleSwitchChara, OffWait, OnWait, On, Reaction)
}

TrampleSwitchChara::TrampleSwitchChara(const char* pName) : al::LiveActor(pName) {
}

void TrampleSwitchChara::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::tryGetArg(&mCharaType, rInfo, "CharaType");
    al::initNerve(this, &NrvTrampleSwitchCharaOffWait, 0);
    mConnector = al::createMtxConnector(this);
    makeActorAppeared();
}

void TrampleSwitchChara::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mConnector, this, false);
}

void TrampleSwitchChara::control() {
    al::connectPoseQT(this, mConnector);
}

void TrampleSwitchChara::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::startAction(this, "OffWait");
        al::setMtpAnimFrameAndStop(this, mCharaType);
    }
}

void TrampleSwitchChara::exeOn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "On");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTrampleSwitchCharaOnWait);
    }
}

void TrampleSwitchChara::exeOnWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::tryOnStageSwitch(this, "SwitchTrampleOn");
        al::startAction(this, "OnWait");
    }
}

void TrampleSwitchChara::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
    }
    if (mReactionFrames <= 0) {
        al::setNerve(this, &NrvTrampleSwitchCharaOffWait);
    }
    --mReactionFrames;
}

bool TrampleSwitchChara::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isNerve(this, &NrvTrampleSwitchCharaOnWait)) {
        return false;
    }
    if (al::isNerve(this, &NrvTrampleSwitchCharaOn)) {
        return false;
    }
    if (!al::isMsgPlayerFloorTouch(pMsg)) {
        return false;
    }
    if (rc::getPlayerCharaType(pSender) == mCharaType) {
        al::invalidateClipping(this);
        rc::sendMsgRequestPlayerGetReaction(pSender, pReceiver, "キャラクタースイッチ踏み");
        rc::addScore(this, pSender, 0.0f, 0);
        al::setNerve(this, &NrvTrampleSwitchCharaOn);
        return true;
    }
    if (al::isNerve(this, &NrvTrampleSwitchCharaOffWait)) {
        mReactionFrames = 2;
        al::setNerve(this, &NrvTrampleSwitchCharaReaction);
        return true;
    }
    if (al::isNerve(this, &NrvTrampleSwitchCharaReaction)) {
        mReactionFrames = 2;
    }
    return false;
}

TrampleSwitchChara::~TrampleSwitchChara() {
}
