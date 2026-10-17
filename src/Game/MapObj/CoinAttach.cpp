#include "MapObj/CoinAttach.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Util/CoinUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "System/GameDataFunction.hpp"
namespace {
NERVE_DECL(CoinAttach, Attach);
NERVE_DECL(CoinAttach, SpinDrc);
NERVE_DECL(CoinAttach, AttachBubble);
NERVE_DECL(CoinAttach, PopUpFront);
NERVE_DECL(CoinAttach, Land);
NERVES_MAKE_STRUCT(CoinAttach, Attach, SpinDrc, AttachBubble, PopUpFront, Land)
}
CoinAttach::CoinAttach(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
CoinAttach::~CoinAttach() {}
void CoinAttach::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "Coin", "Attach");
    al::initNerve(this, &NrvCoinAttach.Attach, 1);
    initCollider(50.0f, 0.0f, 0);
    mFlashing = new al::FlashingCtrl(this, true, false);
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    al::setQuat(this, sead::Quatf::unit);
    mBaseQuat = al::getQuat(this);
    mAssistRotate = new ItemStateAssistRotate(this, CoinUtil::getCoinAssistRotateParam());
    al::initNerveState(this, mAssistRotate, &NrvCoinAttach.SpinDrc, "[state]DRC回転");
    if (mBubble) al::setNerve(this, &NrvCoinAttach.AttachBubble);
    makeActorAppeared();
    al::offCollide(this);
}
void CoinAttach::appear() {
    al::LiveActor::appear();
    al::setVelocityZero(this);
    mFlashing->end();
    al::showModelIfHide(this);
    if (mBubble) al::setNerve(this, &NrvCoinAttach.AttachBubble);
    else al::setNerve(this, &NrvCoinAttach.Attach);
}
bool CoinAttach::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if ((al::isNerve(this, &NrvCoinAttach.Attach) || al::isNerve(this, &NrvCoinAttach.AttachBubble)) && rc::isMsgItemBubbleBreak(msg)) {
        al::setNerve(this, &NrvCoinAttach.PopUpFront);
        return true;
    }
    if (al::isNerve(this, &NrvCoinAttach.AttachBubble)) {
        if (rc::isMsgItemBubbleBreakAndGetItem(msg) && mBubble->isEnableGetPlayerSensor()) {
            rc::acquirerItemCoin(this, mBubble->getHitPlayerSensor());
            al::startHitReactionGet(this);
            kill();
            return true;
        }
    } else {
        if (rc::isMsgCoinGet(msg)) {
            rc::acquirerItemCoin(this, sender);
            al::startHitReactionGet(this);
            kill();
            return true;
        }
        if (al::isMsgDisasterSpikeAttack(msg)) {
            al::startHitReactionDeath(this);
            kill();
        }
    }
    return false;
}
bool CoinAttach::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvCoinAttach.AttachBubble)) return false;
    if (rc::tryAcquirerCoinIfTouchAssistTrigger(msg, this, pointer)) {
        kill();
        return true;
    }
    if (al::isNerve(this, &NrvCoinAttach.Land) && al::isMsgTouchAssistNoPat(msg)) {
        al::setNerve(this, &NrvCoinAttach.SpinDrc);
        return true;
    }
    return false;
}
void CoinAttach::exeAttach() {
    if (al::isFirstStep(this)) al::offCollide(this);
    rotate();
}
void CoinAttach::rotate() { al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this)); }
void CoinAttach::exeAttachBubble() {
    if (al::isFirstStep(this)) al::offCollide(this);
    if (al::isDead(mBubble)) al::setNerve(this, &NrvCoinAttach.PopUpFront);
}
void CoinAttach::exePopUpFront() {
    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::invalidateClipping(this);
        al::setVelocity(this, sead::Vector3f::ey * 15.0f);
        al::setQuat(this, mBaseQuat);
        mBounceCount = 0;
    }
    bool inWater = rc::isInWaterArea(this);
    float gravity = inWater ? 0.6f : 1.5f;
    float friction = inWater ? 0.9f : 0.99f;
    al::addVelocityToGravity(this, gravity);
    al::scaleVelocity(this, friction);
    if (mCheckWaterEntryOnly) {
        sead::Vector3f previousPos(0.0f, 0.0f, 0.0f);
        sead::Vector3CalcCommon<float>::sub(previousPos, al::getTrans(this), al::getVelocity(this));
        if (inWater && !rc::isInWaterArea(this, previousPos)) al::startHitReaction(this, "水面通過");
    } else rc::startHitReactionIfThroughWater(this);
    rotate();
    if (!EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this)) {
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && (InkUtil::isInInkLimitSphere(this) || rc::isInWaterAreaNoSink(this))) {
            al::startHitReactionDeath(this);
            kill();
        }
    }
    if (al::isOnGround(this, 0, 0.0f)) {
        al::reboundVelocityFromCollision(this, 0.5f, 0.0f, 1.0f);
        if (mBounceCount >= 3) al::setNerve(this, &NrvCoinAttach.Land);
        else ++mBounceCount;
    }
}
void CoinAttach::exeLand() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::offCollide(this);
        mFlashing->start(600);
    }
    rotate();
    mFlashing->movement();
    if (mFlashing->isEnded()) { kill(); return; }
    CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvCoinAttach.SpinDrc);
}
void CoinAttach::exeSpinDrc() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvCoinAttach.Land);
}
