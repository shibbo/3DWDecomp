#include "MapObj/SingleModeCheckpoint.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
NERVE_DECL(SingleModeCheckpoint, Before);
NERVE_DECL(SingleModeCheckpoint, ShakeEnd);
NERVE_DECL(SingleModeCheckpoint, Get);
NERVE_DECL(SingleModeCheckpoint, ShakeBefore);
NERVE_DECL(SingleModeCheckpoint, Shake);
NERVE_DECL(SingleModeCheckpoint, After);
NERVES_MAKE_STRUCT(SingleModeCheckpoint, Before, ShakeEnd, Get, ShakeBefore, Shake)
NERVES_MAKE_NOSTRUCT(SingleModeCheckpoint, After)
}
SingleModeCheckpoint::SingleModeCheckpoint(const char* name) : al::LiveActor(name), mPlacementId(new al::PlacementId) {}
SingleModeCheckpoint::~SingleModeCheckpoint() {}
void SingleModeCheckpoint::init(const al::ActorInitInfo& info) {
    const char* model = "CheckpointFlag";
    alPlacementFunction::tryGetModelName(&model, info);
    al::initActorWithArchiveName(this, info, model, nullptr);
    al::initNerve(this, &NrvSingleModeCheckpoint.Before, 0);
    mPlayerInfo = al::createLinksPlayerActorInfo(this, info);
    if (al::tryGetPlacementID(mPlacementId, info)) {
        al::tryGetArg(&mCheckpointId, *info.mPlacementInfo, "CheckpointID");
        al::killPrePassLight(this, "体ポイントライト", -1);
        makeActorAppeared();
    } else makeActorDead();
}
void SingleModeCheckpoint::initAfterPlacement() {}
void SingleModeCheckpoint::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorPlayer(receiver) || !al::isSensorName(sender, "Push") || !al::isNerve(this, &NrvSingleModeCheckpoint.ShakeEnd)) return;
    sead::Vector3f dir = sead::Vector3f::ez;
    al::calcDirBetweenSensorsH(&dir, receiver, sender);
    if (al::isParallelDirection(dir, rc::getPlayerFront(receiver), 0.01f)) {
        if (!rc::sendMsgCameraPushToPlayer(al::getSensorHost(receiver), sender, dir.cross(sead::Vector3f::ey) * 5.0f)) return;
    } else if (!al::sendMsgPush(receiver, sender)) return;
    al::setNerve(this, &NrvSingleModeCheckpoint.ShakeEnd);
}
bool SingleModeCheckpoint::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvSingleModeCheckpoint.Before) || al::isNerve(this, &NrvSingleModeCheckpoint.ShakeBefore)) {
        if (al::isMsgBallAttack(msg) ||
            al::isMsgBallItemGet(msg) ||
            al::isMsgBallTrample(msg) ||
            al::isMsgKickKouraAttack(msg) ||
            al::isMsgKillerItemGet(msg) ||
            al::isMsgPlayerKick(msg) ||
            al::isMsgPlayerBodyAttack(msg) ||
            al::isMsgPlayerBoomerangAttack(msg) ||
            rc::isMsgPlayerCheckpointTouch(msg) ||
            al::isMsgPlayerClimbAttack(msg) ||
            al::isMsgPlayerGiantAttack(msg) ||
            al::isMsgPlayerGiantHipDrop(msg) ||
            al::isMsgPlayerObjHipDropAll(msg) ||
            al::isMsgPlayerObjRollingAttack(msg) ||
            al::isMsgPlayerSlidingAttack(msg) ||
            al::isMsgPlayerSpinAttack(msg) ||
            al::isMsgPlayerTailAttack(msg) ||
            rc::isMsgSkateShoesAttack(msg) ||
            rc::isMsgPackunEat(msg) ||
            (al::isSensorPlayer(sender) && rc::isPlayerSquat(al::getSensorHost(sender)) && al::isMsgPlayerItemGet(msg))) {
            if (!al::isSensorPlayer(sender)) {
                int userId = rc::tryFindRelativeControlUserId(sender);
                if (userId == -1) return false;
                if (rc::isMsgPlayerCheckpointTouch(msg) || rc::isPlayerHolding(this, userId, sender)) rc::tryChangeToSuperMario(this, userId);
                rc::setPlayerColorAnimByControlUserId(this, userId, "CheckpointFlag");
                SingleModeDataFunction::setCheckpointPass(GameDataHolderWriter(this), mPlacementHolder->getZoneNo() - 1, mCheckpointId - 1, this);
            } else {
                rc::setPlayerColorAnimBySensor(this, sender, "CheckpointFlag");
                SingleModeDataFunction::setCheckpointPass(GameDataHolderWriter(this), mPlacementHolder->getZoneNo() - 1, mCheckpointId - 1, this);
                rc::tryChangeToSuperMario(sender);
                rc::tryChangeHoldedPlayerToSuperMario(sender);
            }
            rc::sendMsgRequestPlayerGetReaction(sender, receiver, "中間ポイントゲット");
            al::setNerve(this, &NrvSingleModeCheckpoint.Get);
            return true;
        }
        if (rc::isMsgPackunEatStart(msg)) return true;
    }
    if (!al::isNerve(this, &NrvSingleModeCheckpoint.Before) && !al::isNerve(this, &NrvSingleModeCheckpoint.Get) &&
        !al::isNerve(this, &NrvSingleModeCheckpoint.ShakeBefore) && !al::isNerve(this, &NrvSingleModeCheckpoint.Shake)) {
        if (al::isMsgItemGetAll(msg) || EnemyStateUtil::isMsgBlowDown(msg)) {
            if (al::isNerve(this, &NrvSingleModeCheckpoint.ShakeEnd)) al::setNerve(this, &NrvSingleModeCheckpoint.ShakeEnd);
            else al::setNerve(this, &NrvSingleModeCheckpoint.Shake);
        }
    }
    return false;
}
bool SingleModeCheckpoint::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvSingleModeCheckpoint.Get) || al::isNerve(this, &NrvSingleModeCheckpoint.ShakeBefore) ||
        al::isNerve(this, &NrvSingleModeCheckpoint.Shake) || al::isNerve(this, &NrvSingleModeCheckpoint.ShakeEnd)) return false;
    if (al::isMsgTouchAssistAll(msg) && !al::isNerve(this, &NrvSingleModeCheckpoint.ShakeBefore) && !al::isNerve(this, &NrvSingleModeCheckpoint.Shake)) {
        if (al::isNerve(this, &NrvSingleModeCheckpoint.Before)) al::setNerve(this, &NrvSingleModeCheckpoint.ShakeBefore);
        else al::setNerve(this, &NrvSingleModeCheckpoint.Shake);
    }
    return false;
}
void SingleModeCheckpoint::exeBefore() { if (al::isFirstStep(this)) al::startAction(this, "Before"); }
void SingleModeCheckpoint::exeAfter() { if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, "After"); }
void SingleModeCheckpoint::exeGet() {
    if (al::isFirstStep(this)) { al::startAction(this, "Get"); al::startHitReactionGet(this); al::appearPrePassLight(this, "体ポイントライト", -1); }
    if (al::isActionEnd(this)) { al::startAction(this, "After"); al::setNerve(this, &NrvSingleModeCheckpoint.ShakeEnd); }
}
void SingleModeCheckpoint::exeShakeBefore() {
    if (al::isFirstStep(this)) al::startAction(this, "ShakeBefore");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvSingleModeCheckpoint.Before);
}
void SingleModeCheckpoint::exeShake() {
    if (al::isFirstStep(this)) al::startAction(this, "Shake");
    if (al::isActionEnd(this)) { al::startAction(this, "After"); al::setNerve(this, &NrvSingleModeCheckpoint.ShakeEnd); }
}
void SingleModeCheckpoint::exeShakeEnd() { if (al::isGreaterEqualStep(this, 10)) al::setNerve(this, &NrvSingleModeCheckpointAfter); }
void SingleModeCheckpoint::setStateAfter() {
    al::startAction(this, "After");
    al::appearPrePassLight(this, "体ポイントライト", -1);
    al::startMtpAnimAndSetFrameAndStop(this, "CheckpointFlag", 1.0f);
    al::setNerve(this, &NrvSingleModeCheckpointAfter);
}
void SingleModeCheckpoint::setStateBefore() {
    al::killPrePassLight(this, "体ポイントライト", -1);
    rc::setPlayerColorAnimDefault(this, "CheckpointFlag");
    al::setNerve(this, &NrvSingleModeCheckpoint.Before);
}
