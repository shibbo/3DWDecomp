#include "MapObj/BlockBrickBig.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(BlockBrickBig, Wait);
    NERVE_DECL(BlockBrickBig, Break);
    NERVE_DECL(BlockBrickBig, Reaction);
    NERVE_DECL(BlockBrickBig, CrakReaction);
    NERVES_MAKE_NOSTRUCT(BlockBrickBig, Wait, Break, Reaction, CrakReaction)
    const char* sReactionNames[] = {"Reaction1", "Reaction2", "Reaction3", "Reaction4"};
}
BlockBrickBig::BlockBrickBig(const char* name) : al::LiveActor(name), mComboCounter(new al::ComboCounter) {}
BlockBrickBig::~BlockBrickBig() {}
void BlockBrickBig::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "BlockBrickBig", nullptr);
    al::initNerve(this, &NrvBlockBrickBigWait, 0);
    al::offCollide(this);
    mConnector = al::tryCreateMtxConnector(this, info);
    makeActorAppeared();
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 2.8274333477020264f, 0.08f, 7);
    al::startAction(this, "Wait");
    mFrameRate = al::getSklAnimFrameRate(this, 0);
    al::setForceCollisionScaleOne(this);
}
void BlockBrickBig::initAfterPlacement() { if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false); }
void BlockBrickBig::control() {
    if (mConnector) al::connectPoseQT(this, mConnector);
    if (mReactionCount - 1 >= 0) --mReactionCount;
}
void BlockBrickBig::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    int user = mControlUserId;
    auto* combo = mComboCounter;
    if ((al::isNerve(this, &NrvBlockBrickBigReaction) && al::isLessStep(this, 5)) || al::isNerve(this, &NrvBlockBrickBigBreak))
        rc::trySendMsgBlockToUpperObj(receiver, sender, user, combo);
}
bool BlockBrickBig::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (rc::isMsgAskControlUserId(msg, mControlUserId)) return true;
    if (al::isNerve(this, &NrvBlockBrickBigBreak)) return false;
    if (!rc::isMsgForBlockAll(msg, sender, receiver, 350.0f)) return false;
    if (!(al::isMsgKickKouraAttackCollide(msg) || al::isMsgBallAttackCollide(msg) || al::isMsgExplosion(msg) || al::isMsgExplosionCollide(msg) || al::isMsgPlayerBoomerangAttackCollide(msg) || rc::isMsgBullAttack(msg) || al::isMsgPlayerGiantTouch(msg) || al::isMsgLaserAttack(msg)) && rc::isPlayerMini(sender) && mReactionCount == 0) {
        mReactionCount = rc::getReactionCountByMsg(msg);
        mControlUserId = rc::tryFindRelativeControlUserId(sender);
        al::setNerve(this, &NrvBlockBrickBigReaction);
        if (al::isMsgPlayerHipDropAll(msg)) return false;
        return rc::getMsgReturnValueForBlock(msg);
    }
    if (mReactionCount != 0) return false;
    int durability;
    if (al::isMsgExplosion(msg) || rc::isMsgBullAttack(msg) || al::isMsgPlayerGiantTouch(msg) || al::isMsgLaserAttack(msg) || al::isMsgPlayerGiantHipDrop(msg)) durability = 0;
    else durability = mDurability - 1;
    mDurability = durability;
    mReactionCount = rc::getReactionCountByMsg(msg);
    if (mDurability <= 0) {
        rc::addScore(this, sender, 0.0f, 0);
        mControlUserId = rc::tryFindRelativeControlUserId(sender);
        al::setNerve(this, &NrvBlockBrickBigBreak);
        return true;
    }
    al::startSklAnim(this, "Crack");
    al::setSklAnimFrame(this, 5 - mDurability, 0);
    al::setSklAnimFrameRate(this, 0.0f, 0);
    al::setNerve(this, &NrvBlockBrickBigCrakReaction);
    return al::isMsgPlayerHipDropAll(msg);
}
bool BlockBrickBig::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvBlockBrickBigBreak)) return false;
    if (al::isMsgTouchAssistTrig(msg)) {
        if (--mDurability <= 0) {
            rc::addScore(this, pointer, 0.0f, 0);
            mControlUserId = rc::tryFindRelativeControlUserId(this, pointer);
            al::setNerve(this, &NrvBlockBrickBigBreak);
        } else al::setNerve(this, &NrvBlockBrickBigCrakReaction);
        return true;
    }
    return false;
}
void BlockBrickBig::exeWait() {}
void BlockBrickBig::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "チビリアクション");
        mRumble->start(0);
    }
    mRumble->calc();
    al::setScaleY(this, mRumble->getValueY() + 1.0f);
    if (al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvBlockBrickBigWait);
        al::setScaleY(this, 1.0f);
    }
}
void BlockBrickBig::exeCrakReaction() {
    if (al::isFirstStep(this)) {
        al::setScaleY(this, 1.0f);
        al::startAction(this, sReactionNames[int(4u - mDurability)]);
        al::setSklAnimFrameRate(this, mFrameRate, 0);
    }
    if (al::isActionEnd(this)) {
        al::setSklAnimFrameRate(this, 0.0f, 0);
        al::setNerve(this, &NrvBlockBrickBigWait);
    }
}
void BlockBrickBig::exeBreak() {
    if (al::isFirstStep(this)) {
        al::setScaleY(this, 1.0f);
        al::startAction(this, "Break");
        al::setSklAnimFrameRate(this, mFrameRate, 0);
        al::invalidateCollisionParts(this);
        al::invalidateClipping(this);
    }
    if (al::isActionEnd(this)) {
        al::tryOnSwitchDeadOn(this);
        kill();
    }
}
