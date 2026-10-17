#include "MapObj/BlockPow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "MapObj/ExplosionComboCounterHolder.hpp"
#include "MapObj/SnowCover.hpp"
#include "Project/Collision/CollisionPartsKeeperUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include "heap/seadMemBlock.h"

namespace {
    NERVE_DECL(BlockPow, Wait);
    NERVE_DECL(BlockPow, Reaction);
    NERVE_DECL(BlockPow, WaitEnd);
    NERVE_DECL(BlockPow, Empty);
    NERVES_MAKE_NOSTRUCT(BlockPow, Wait, Reaction, WaitEnd, Empty)
};  // namespace

const sead::Color4f cLightColor = sead::Color4f(2.5f, 2.5f, 1.0f, 1.0f);
const f32 cSensorRadius[2] = {900.0f, 600.0f};

BlockPow::BlockPow(const char* pName) : al::LiveActor(pName) {
}

void BlockPow::init(const al::ActorInitInfo& rInfo) {
    al::initActorSuffix(this, rInfo, rc::getBlockSuffixName(rInfo, false));
    al::initNerve(this, &NrvBlockPowWait, 0);
    mSnowCover = SnowCoverFunction::tryCreateSnowCover(this, rInfo, "BlockSnowCover", true, nullptr);
    al::tryGetArg(&mIsAppearEmpty, rInfo, "IsAppearBlockEmpty");

    if (mIsAppearEmpty) {
        mEmptyBlock = new BlockEmpty("空ブロック", "BlockEmpty");
        al::initCreateActorWithPlacementInfo(mEmptyBlock, rInfo);
        al::copyPose(mEmptyBlock, this);
        mEmptyBlock->makeActorDead();
    }

    _171 = rc::isInWaterArea(this);
    al::tryGetArg(&mIsWideRangeLight, rInfo, "IsWideRangeLight");
    rc::createExplosionComboCounter(this);
    bool isExpClip = false;
    al::tryGetArg(&isExpClip, rInfo, "IsExpandClippingShadowLength");

    if (isExpClip) {
        al::tryExpandClippingByShadowLength(this, &_164);

        if (mIsAppearEmpty) {
            al::tryExpandClippingByShadowLength(mEmptyBlock, &_164);
        }
    }

    bool useSwitch = al::listenStageSwitchOnOffAppear(this, al::Functor(this, &BlockPow::appearBySwitch), al::Functor(this, &BlockPow::killBySwitch));

    if (useSwitch) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

void BlockPow::appearBySwitch() {
    GameDataHolderAccessor accessor(this);

    if (GameDataFunction::isSingleMode(accessor)) {
        respawn();
    }

    LiveActor::appear();
}

void BlockPow::killBySwitch() {
    GameDataHolderAccessor accessor(this);

    if (GameDataFunction::isSingleMode(accessor)) {
        if (mEmptyBlock != nullptr && al::isAlive(mEmptyBlock)) {
            mEmptyBlock->makeActorDead();
        }
    }

    LiveActor::kill();
}

void BlockPow::respawn() {
    if (al::isDead(this)) {
        makeActorAppeared();
    }

    al::setNerve(this, &NrvBlockPowWait);
    al::showModelIfHide(this);
    al::onCollide(this);
    al::validateCollisionParts(this);
    al::validateHitSensors(this);
    al::validateClipping(this);
    al::setSensorRadius(this, "Explosion", 0.0f);

    if (mSnowCover != nullptr) {
        mSnowCover->respawn();
    }

    if (mEmptyBlock != nullptr) {
        if (al::isAlive(mEmptyBlock)) {
            mEmptyBlock->makeActorDead();
        }
    }
}

void BlockPow::attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (!al::isNerve(this, &NrvBlockPowWait)) {
        if (!al::isSensorPlayer(pReceiver)) {
            if (al::isSensorName(pSender, "Explosion")) {
                al::sendMsgExplosion(pReceiver, pSender, mComboCounter);
            }
        }
    }
}

bool BlockPow::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (rc::isMsgAskControlUserId(msg, mControlUserId))
        return true;
    if (!al::isNerve(this, &NrvBlockPowWait))
        return false;

    bool isAttack = rc::isMsgForBlockAll(msg, sender, receiver, 100.0f);
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && al::isMsgEnemyAttackFire(msg)) {
        const char* name = al::getSensorHost(sender)->getName();
        isAttack = al::isEqualString(name, "KoopaFireBall") || al::isEqualString(name, "KoopaFireBallGiant") || isAttack;
    }
    if (!isAttack)
        return false;

    mControlUserId = rc::tryFindRelativeControlUserId(sender);
    if (al::isMsgExplosion(msg) || al::isMsgExplosionCollide(msg))
        mComboCounter = rc::tryGetMsgComboCount(msg);
    if (!mComboCounter)
        mComboCounter = rc::getExplosionComboCounterAndNextIndex(this);
    if (mSnowCover)
        mSnowCover->tryBreak();

    if (al::isMsgPlayerUpperPunch(msg) || al::isMsgPlayerHipDropAll(msg) ||
        al::isMsgPlayerClimbAttack(msg) || al::isMsgPlayerTailAttack(msg) ||
        al::isMsgPlayerSpinAttack(msg) || al::isMsgPlayerStatueDrop(msg))
        _174 = 1;
    rc::addScoreByFactor(this, sender, "壊れ", 0.0f, 0);
    startExplosion(al::isMsgPlayerHipDropAll(msg));
    return rc::getMsgReturnValueForBlock(msg);
}

void BlockPow::startExplosion(bool a1) {
    al::startAction(this, a1 ? "ReactionHipDrop" : "Reaction");
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvBlockPowReaction);
    al::onStageSwitch(this, "SwitchKockOn");
}

bool BlockPow::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    al::IUseStageSwitch* sw = static_cast<al::LiveActor*>(this);

    if ((al::isValidStageSwitch(sw, "EnableKnockSwitch") && !al::isOnStageSwitch(sw, "EnableKnockSwitch")) || !isNerve(this, &NrvBlockPowWait) ||
        !al::isMsgTouchAssistTrig(pMsg)) {
        return false;
    }

    mControlUserId = rc::tryFindRelativeControlUserId(this, pPointer);
    mComboCounter = rc::getExplosionComboCounterAndNextIndex(this);
    al::startAction(this, "Reaction");
    al::invalidateHitSensors(this);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvBlockPowReaction);
    al::onStageSwitch(this, "SwitchKockOn");
    rc::addScoreByFactor(this, pPointer, "壊れ", 0.0f, 0);

    return true;
}

void BlockPow::onConnectRailBlock() {
    _170 = 1;
    al::setShadowFixed(this, false);

    if (mIsAppearEmpty) {
        mEmptyBlock->onConnectRailBlock();
    }
}

bool BlockPow::updateSensor() {
    f32 radius = cSensorRadius[_171 == 0];
    f32 sensorRadius = al::getSensorRadius(this, "Explosion");

    if (radius <= sensorRadius) {
        return false;
    }

    f32 newRadius = 10.0f;

    if (!_171) {
        newRadius = 15.0f;
    }

    al::setSensorRadius(this, "Explosion", sensorRadius + newRadius);
    return true;
}

void BlockPow::requestLightExplosion() {
    al::requestPrePassLightColor(this, mIsWideRangeLight ? "爆発[広範囲]" : "爆発", cLightColor);
}

void BlockPow::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

void BlockPow::exeReaction() {
    if (al::isFirstStep(this)) {
        al::setSensorRadius(this, "Explosion", 200.0f);
        al::validateHitSensor(this, "Explosion");
        al::startHitReaction(this, _171 ? "爆発[水中]" : "爆発");
    }

    requestLightExplosion();
    updateSensor();

    if (al::isActionEnd(this)) {
        if (mIsAppearEmpty) {
            al::copyPose(mEmptyBlock, this);
            mEmptyBlock->appear();
        }

        al::setNerve(this, &NrvBlockPowWaitEnd);
    }
}

void BlockPow::exeWaitEnd() {
    if (al::isFirstStep(this)) {
        al::hideModel(this);
        al::offCollide(this);
    }
    if (al::isStep(this, 0)) {
        if (_174)
            al::startHitReaction(this, "爆発振動");
        else
            al::startHitReaction(this, "爆発振動[タッチ]");
    }
    if (_170 && mIsAppearEmpty)
        al::copyPose(mEmptyBlock, this);
    requestLightExplosion();
    if (!updateSensor()) {
        if (_170 && mIsAppearEmpty)
            al::setNerve(this, &NrvBlockPowEmpty);
        else
            kill();
    }
}

void BlockPow::exeEmpty() {
    if (al::isFirstStep(this)) {
        al::invalidateHitSensors(this);
        al::invalidateCollisionParts(this);
    }

    al::copyPose(mEmptyBlock, this);
}

BlockPow::~BlockPow() {
}