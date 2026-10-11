#include "MapObj/CheckpointFlag.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/GameDataFunction.hpp"
#include "MapObj/ActorMicRumbler.hpp"
#include "Library/Movement/AnimScaleController.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Util/ScoreUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
struct CheckpointScaleParam : al::AnimScaleParam {
    CheckpointScaleParam() { _24 = 1.0f; _2c = 0.025f; }
};
const CheckpointScaleParam sCheckpointScaleParam;
inline void setCheckpointPassForPlayer(al::LiveActor* actor, const al::HitSensor* sensor) {
    if (rc::isPlayerCharaMario(sensor)) GameDataFunction::setCheckpointPassMario(GameDataHolderWriter(GameDataHolderAccessor(actor)));
    else if (rc::isPlayerCharaLuigi(sensor)) GameDataFunction::setCheckpointPassLuigi(GameDataHolderWriter(GameDataHolderAccessor(actor)));
    else if (rc::isPlayerCharaPeach(sensor)) GameDataFunction::setCheckpointPassPeach(GameDataHolderWriter(GameDataHolderAccessor(actor)));
    else if (rc::isPlayerCharaKinopio(sensor)) GameDataFunction::setCheckpointPassKinopio(GameDataHolderWriter(GameDataHolderAccessor(actor)));
    else if (rc::isPlayerCharaRosetta(sensor)) GameDataFunction::setCheckpointPassRosetta(GameDataHolderWriter(GameDataHolderAccessor(actor)));
}
inline void setCheckpointPassForUser(al::LiveActor* actor, int userId) {
    switch (rc::getControlUserCharacterType(GameDataHolderAccessor(actor), userId)) {
    case 0: GameDataFunction::setCheckpointPassMario(GameDataHolderWriter(GameDataHolderAccessor(actor))); break;
    case 1: GameDataFunction::setCheckpointPassLuigi(GameDataHolderWriter(GameDataHolderAccessor(actor))); break;
    case 2: GameDataFunction::setCheckpointPassPeach(GameDataHolderWriter(GameDataHolderAccessor(actor))); break;
    case 3: GameDataFunction::setCheckpointPassKinopio(GameDataHolderWriter(GameDataHolderAccessor(actor))); break;
    case 4: GameDataFunction::setCheckpointPassRosetta(GameDataHolderWriter(GameDataHolderAccessor(actor))); break;
    }
}

NERVE_DECL(CheckpointFlag, Before);
NERVE_DECL(CheckpointFlag, ShakeEnd);
NERVE_DECL(CheckpointFlag, Get);
NERVE_DECL(CheckpointFlag, ShakeBefore);
NERVE_DECL(CheckpointFlag, Shake);
NERVE_DECL(CheckpointFlag, After);
NERVES_MAKE_STRUCT(CheckpointFlag, Before, Get, ShakeEnd, ShakeBefore, Shake)
NERVES_MAKE_NOSTRUCT(CheckpointFlag, After)
}
CheckpointFlag::CheckpointFlag(const char* name) : al::LiveActor(name), mPlacementId(new al::PlacementId) {}
CheckpointFlag::~CheckpointFlag() {}
void CheckpointFlag::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvCheckpointFlag.Before, 0);
    mPlayerInfo = al::createLinksPlayerActorInfo(this, info);
    if (al::tryGetPlacementID(mPlacementId, info)) {
        mRumbler = new ActorMicRumbler(this, &sCheckpointScaleParam);
        al::killPrePassLight(this, "体ポイントライト", -1);
        makeActorAppeared();
    } else makeActorDead();
}
void CheckpointFlag::control() { if (!al::isNerve(this, &NrvCheckpointFlag.Get)) mRumbler->update(); }
void CheckpointFlag::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isSensorPlayer(receiver) || !al::isSensorName(sender, "Push") || !al::isNerve(this, &NrvCheckpointFlag.ShakeEnd)) return;
    sead::Vector3f dir = sead::Vector3f::ez;
    al::calcDirBetweenSensorsH(&dir, receiver, sender);
    if (al::isParallelDirection(dir, rc::getPlayerFront(receiver), 0.01f)) {
        if (!rc::sendMsgCameraPushToPlayer(al::getSensorHost(receiver), sender, dir.cross(sead::Vector3f::ey) * 5.0f)) return;
    } else if (!al::sendMsgPush(receiver, sender)) return;
    al::setNerve(this, &NrvCheckpointFlag.ShakeEnd);
}
bool CheckpointFlag::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvCheckpointFlag.Before) || al::isNerve(this, &NrvCheckpointFlag.ShakeBefore)) {
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
                setCheckpointPassForUser(this, userId);
            } else {
                rc::setPlayerColorAnimBySensor(this, sender, "CheckpointFlag");
                setCheckpointPassForPlayer(this, sender);
                rc::tryChangeToSuperMario(sender);
                rc::tryChangeHoldedPlayerToSuperMario(sender);
            }
            rc::sendMsgRequestPlayerGetReaction(sender, receiver, "中間ポイントゲット");
            rc::addScore(this, sender, 150.0f, 0);
            al::setNerve(this, &NrvCheckpointFlag.Get);
            return true;
        }
        if (rc::isMsgPackunEatStart(msg)) return true;
    }
    if (!al::isNerve(this, &NrvCheckpointFlag.Before) && !al::isNerve(this, &NrvCheckpointFlag.Get) &&
        !al::isNerve(this, &NrvCheckpointFlag.ShakeBefore) && !al::isNerve(this, &NrvCheckpointFlag.Shake)) {
        if (al::isMsgItemGetAll(msg) || EnemyStateUtil::isMsgBlowDown(msg)) {
            if (al::isNerve(this, &NrvCheckpointFlag.ShakeEnd)) al::setNerve(this, &NrvCheckpointFlag.ShakeEnd);
            else al::setNerve(this, &NrvCheckpointFlag.Shake);
        }
    }
    return false;
}
bool CheckpointFlag::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvCheckpointFlag.Get) || al::isNerve(this, &NrvCheckpointFlag.ShakeBefore) ||
        al::isNerve(this, &NrvCheckpointFlag.Shake) || al::isNerve(this, &NrvCheckpointFlag.ShakeEnd)) return false;
    if (al::isMsgTouchAssistAll(msg) && !al::isNerve(this, &NrvCheckpointFlag.ShakeBefore) && !al::isNerve(this, &NrvCheckpointFlag.Shake)) {
        if (al::isNerve(this, &NrvCheckpointFlag.Before)) al::setNerve(this, &NrvCheckpointFlag.ShakeBefore);
        else al::setNerve(this, &NrvCheckpointFlag.Shake);
    }
    return false;
}
void CheckpointFlag::exeBefore() { if (al::isFirstStep(this)) al::startAction(this, "Before"); }
void CheckpointFlag::exeAfter() { if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, "After"); }
void CheckpointFlag::exeGet() {
    if (al::isFirstStep(this)) { al::startAction(this, "Get"); al::startHitReactionGet(this); al::appearPrePassLight(this, "体ポイントライト", -1); mRumbler->stopAndReset(); rc::setPlacementIdObjGhostPlayerRecorder(this, mPlacementId, false); }
    if (al::isActionEnd(this)) { al::startAction(this, "After"); al::setNerve(this, &NrvCheckpointFlag.ShakeEnd); }
}
void CheckpointFlag::exeShakeBefore() {
    if (al::isFirstStep(this)) al::startAction(this, "ShakeBefore");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvCheckpointFlag.Before);
}
void CheckpointFlag::exeShake() {
    if (al::isFirstStep(this)) al::startAction(this, "Shake");
    if (al::isActionEnd(this)) { al::startAction(this, "After"); al::setNerve(this, &NrvCheckpointFlag.ShakeEnd); }
}
void CheckpointFlag::exeShakeEnd() { if (al::isGreaterEqualStep(this, 10)) al::setNerve(this, &NrvCheckpointFlagAfter); }
void CheckpointFlag::setStateAfter() {
    al::startAction(this, "After");
    al::appearPrePassLight(this, "体ポイントライト", -1);
    if (GameDataFunction::isCheckpointPassMario(GameDataHolderAccessor(this))) al::startMtpAnimAndSetFrameAndStop(this, "CheckpointFlag", 1.0f);
    else if (GameDataFunction::isCheckpointPassLuigi(GameDataHolderAccessor(this))) al::startMtpAnimAndSetFrameAndStop(this, "CheckpointFlag", 2.0f);
    else if (GameDataFunction::isCheckpointPassPeach(GameDataHolderAccessor(this))) al::startMtpAnimAndSetFrameAndStop(this, "CheckpointFlag", 3.0f);
    else if (GameDataFunction::isCheckpointPassKinopio(GameDataHolderAccessor(this))) al::startMtpAnimAndSetFrameAndStop(this, "CheckpointFlag", 4.0f);
    else if (GameDataFunction::isCheckpointPassRosetta(GameDataHolderAccessor(this))) al::startMtpAnimAndSetFrameAndStop(this, "CheckpointFlag", 5.0f);
    rc::setFlagShakeAfterGhostPlayerRecorder(this);
    al::setNerve(this, &NrvCheckpointFlagAfter);
}
