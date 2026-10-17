#include "MapObj/PanelNotePaint.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
const char* cColorNames[] = {nullptr,     "Red",  "Yellow", "Green",
                             "LightBlue", "Blue", "Orange", "Pink"};
NERVE_DECL(PanelNotePaint, OffWait)
NERVE_DECL(PanelNotePaint, OnWaitBefore)
NERVE_DECL(PanelNotePaint, InvincibleTouched)
NERVE_DECL(PanelNotePaint, HipDropped)
NERVE_DECL(PanelNotePaint, OnWait)
NERVE_DECL(PanelNotePaint, ReactionReturn)
NERVE_DECL(PanelNotePaint, OnWaitAfterHipDropped)
NERVE_DECL(PanelNotePaint, Reaction)
NERVES_MAKE_STRUCT(PanelNotePaint, OffWait, OnWaitBefore, InvincibleTouched, HipDropped, OnWait,
                   ReactionReturn, OnWaitAfterHipDropped, Reaction)
} // namespace
PanelNotePaint::PanelNotePaint(const char* name) : al::LiveActor(name) {}
PanelNotePaint::~PanelNotePaint() = default;
void PanelNotePaint::init(const al::ActorInitInfo& info) {
    if (!al::tryGetArg(&mInitiallyOn, info, "IsPush"))
        mInitiallyOn = false;
    mOn = mInitiallyOn;
    al::initNerve(this, &NrvPanelNotePaint.OffWait, 0);
    al::initActorWithArchiveName(this, info, "PanelNotePaint", nullptr);
    al::invalidateHitSensor(this, "Explosion");
    makeActorAppeared();
    al::tryGetArg(&mColor, info, "Color");
    mConnector = al::tryCreateMtxConnector(this, info);
}
void PanelNotePaint::initAfterPlacement() {
    if (mConnector)
        al::attachMtxConnectorToCollision(mConnector, this, false);
}
void PanelNotePaint::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    if (mInitiallyOn)
        al::setNerve(this, &NrvPanelNotePaint.OnWaitBefore);
    else
        al::setNerve(this, &NrvPanelNotePaint.OffWait);
    al::validateClipping(this);
}
void PanelNotePaint::control() {
    if (mConnector)
        al::connectPoseQT(this, mConnector);
    if (mHitReactionStarted) {
        if (mHitReactionTimer >= 41) {
            mHitReactionStarted = false;
            mHitReactionTimer = 0;
        }
        ++mHitReactionTimer;
    }
}
void PanelNotePaint::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorEye(sender))
        rc::sendMsgPanelNoteHipDrop(receiver, sender);
}
bool PanelNotePaint::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (al::isMsgPlayerCooperationHipDrop(msg) || al::isMsgPlayerGiantHipDrop(msg) ||
        rc::isMsgPanelNoteHipDrop(msg) || al::isMsgKickKouraAttackCollide(msg) ||
        al::isMsgExplosion(msg) || al::isMsgEnemyFloorTouch(msg) || al::isMsgEnemyAttackFire(msg) ||
        al::isMsgFireBallFloorTouch(msg) || al::isMsgKickKouraAttackCollide(msg) ||
        al::isMsgBallTrampleCollide(msg)) {
        if (rc::isMsgPanelNoteHipDrop(msg) || al::isMsgPlayerGiantHipDrop(msg) ||
            al::isMsgPlayerCooperationHipDrop(msg) || al::isMsgExplosion(msg)) {
            if (!al::isMsgExplosion(msg) &&
                !al::isNearZero(al::getSensorPos(sender).y - al::getTrans(this).y, 60.0f))
                return false;
            if (tryContinuePushing())
                return !al::isMsgExplosion(msg);
            if (mHitReactionStarted)
                return false;
            mHitReactionStarted = true;
            if (!al::isNerve(this, &NrvPanelNotePaint.InvincibleTouched)) {
                al::startHitReactionStart(this);
                if (al::isMsgPlayerGiantHipDrop(msg) || al::isMsgExplosion(msg))
                    al::startSeWithParam(this, "PgOnShort", float(mPitch), nullptr);
            }
        }
        if (isEnablePush())
            pushFirstTime(sender, true, true);
        else
            tryContinuePushing();
        return !al::isMsgExplosion(msg);
    }
    if ((al::isMsgPlayerTouch(msg) && rc::isPlayerWallSnap(sender)) ||
        al::isMsgPlayerFloorTouch(msg)) {
        if (al::getActorVelocity(sender).y > 0.0f)
            return false;
        if (isEnablePush())
            pushFirstTime(sender, true, true);
        else
            tryContinuePushing();
    }
    if (al::isMsgPlayerHipDropAll(msg)) {
        mScoreSensor = sender;
        pushFirstTime(sender, false, false);
        al::setNerve(this, &NrvPanelNotePaint.HipDropped);
        if (!mHitReactionStarted) {
            mHitReactionStarted = true;
            al::startHitReactionStart(this);
        }
        al::setSensorRadius(this, "Explosion", 200.0f);
        mExplosionRadius = 450.0f;
        al::validateHitSensor(this, "Explosion");
        return false;
    }
    return rc::isMsgAskControlUserId(msg, mScoreSensor);
}
bool PanelNotePaint::tryContinuePushing() {
    bool on = al::isNerve(this, &NrvPanelNotePaint.OnWait);
    bool hip = al::isNerve(this, &NrvPanelNotePaint.OnWaitAfterHipDropped);
    if (on)
        al::setNerve(this, &NrvPanelNotePaint.OnWait);
    else if (hip)
        al::setNerve(this, &NrvPanelNotePaint.OnWaitAfterHipDropped);
    else
        return false;
    return true;
}
bool PanelNotePaint::isEnablePush() const {
    return al::isNerve(this, &NrvPanelNotePaint.OffWait) ||
           al::isNerve(this, &NrvPanelNotePaint.ReactionReturn);
}
void PanelNotePaint::pushFirstTime(al::HitSensor* sensor, bool changeNerve, bool reaction) {
    if (al::isNerve(this, &NrvPanelNotePaint.OffWait)) {
        if (reaction)
            al::startHitReactionStart(this);
        mScoreSensor = sensor;
        rc::addScore(this, sensor, 0.0f, 0);
    }
    if (changeNerve)
        al::setNerve(this, &NrvPanelNotePaint.Reaction);
}
bool PanelNotePaint::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer,
                                           al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistNoPat(msg)) {
        if (isEnablePush())
            pushFirstTimeScreenPoint(pointer, true, true);
        else
            tryContinuePushing();
        return true;
    }
    return false;
}
void PanelNotePaint::pushFirstTimeScreenPoint(al::ScreenPointer* pointer, bool changeNerve,
                                              bool reaction) {
    if (al::isNerve(this, &NrvPanelNotePaint.OffWait)) {
        if (reaction)
            al::startHitReactionStart(this);
        mScoreSensor = DrcFunction::tryFindDrcPlayerSensor(this, pointer);
        rc::addScore(this, pointer, 0.0f, 0);
    }
    if (changeNerve)
        al::setNerve(this, &NrvPanelNotePaint.Reaction);
}
bool PanelNotePaint::isOn() const { return mOn; }
al::HitSensor* PanelNotePaint::getScoreSensor() { return mScoreSensor; }
void PanelNotePaint::setCompleteEffect() { al::startHitReaction(this, "完成"); }
void PanelNotePaint::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "OffWait");
        al::validateClipping(this);
        al::startMclAnim(this, "Wait");
    }
}
void PanelNotePaint::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
        if (mOn)
            al::startSeWithParam(this, "PgReaction", float(mPitch), nullptr);
        al::tryStartMclAnimIfNotPlaying(this, cColorNames[mColor]);
    }
    if (al::isActionEnd(this)) {
        mOn = true;
        al::setNerve(this, &NrvPanelNotePaint.OnWait);
    }
}
void PanelNotePaint::exeOnWaitBefore() { setNerveToOnWait(); }
void PanelNotePaint::setNerveToOnWait() {
    settingForOnWait();
    al::setNerve(this, &NrvPanelNotePaint.OnWait);
}
void PanelNotePaint::exeOnWait() {
    if (al::isGreaterEqualStep(this, 2))
        al::setNerve(this, &NrvPanelNotePaint.ReactionReturn);
}
void PanelNotePaint::exeOnWaitAfterHipDropped() {
    if (al::isGreaterEqualStep(this, 2))
        al::setNerve(this, &NrvPanelNotePaint.ReactionReturn);
}
void PanelNotePaint::exeReactionReturn() {
    if (al::isFirstStep(this))
        al::startAction(this, "ReactionReturn");
}
void PanelNotePaint::exeHipDropped() {
    if (al::isFirstStep(this)) {
        if (mOn)
            al::startSe(this, "PgLittleHit", nullptr);
        else
            al::startSeWithParam(this, "PgLittleHitBefore", float(mPitch % 12), nullptr);
        settingForInvincibleTouched();
    }
    if (al::isActionEnd(this))
        mOn = true;
    float radius = al::getSensorRadius(this, "Explosion") + 15.0f;
    if (!(radius > mExplosionRadius))
        al::setSensorRadius(this, "Explosion", radius);
    if (al::isGreaterEqualStep(this, 30)) {
        setNerveToOnWaitAfterHipDropped();
        al::invalidateHitSensor(this, "Explosion");
    }
}
void PanelNotePaint::setNerveToOnWaitAfterHipDropped() {
    settingForOnWait();
    al::setNerve(this, &NrvPanelNotePaint.OnWaitAfterHipDropped);
}
void PanelNotePaint::exeInvincibleTouched() {
    float radius = al::getSensorRadius(this, "Explosion") + 15.0f;
    if (!(radius > mExplosionRadius))
        al::setSensorRadius(this, "Explosion", radius);
    if (radius > mExplosionRadius && al::isGreaterEqualStep(this, 2)) {
        setNerveToOnWait();
        al::invalidateHitSensor(this, "Explosion");
    }
}
void PanelNotePaint::settingForOnWait() {
    al::startAction(this, "OnWait");
    if (al::tryStartMclAnimIfNotPlaying(this, cColorNames[mColor]) && mInitiallyOn)
        al::setMclAnimFrame(this, al::getMclAnimFrameMax(this, cColorNames[mColor]));
    al::validateClipping(this);
}
void PanelNotePaint::settingForInvincibleTouched() {
    al::startAction(this, "Reaction");
    al::tryStartMclAnimIfNotPlaying(this, cColorNames[mColor]);
    al::validateClipping(this);
}
void PanelNotePaint::setNerveToInvincibleTouched() {
    settingForInvincibleTouched();
    al::setNerve(this, &NrvPanelNotePaint.InvincibleTouched);
}
