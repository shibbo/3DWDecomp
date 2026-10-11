#include "MapObj/JumpFlipSweetsCream.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(JumpFlipSweetsCream, Wait);
    NERVE_DECL(JumpFlipSweetsCream, TouchAction);
    NERVES_MAKE_NOSTRUCT(JumpFlipSweetsCream, Wait, TouchAction)
}
JumpFlipSweetsCream::JumpFlipSweetsCream(const char* pName) : al::LiveActor(pName) {}
JumpFlipSweetsCream::~JumpFlipSweetsCream() {}
void JumpFlipSweetsCream::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvJumpFlipSweetsCreamWait, 0);
    al::tryGetArg(&mIsAppearCoin, rInfo, "IsAppearCoin");
    makeActorAppeared();
}
void JumpFlipSweetsCream::control() {
    if (mCooldown - 1 >= 0)
        --mCooldown;
}
bool JumpFlipSweetsCream::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor*) {
    if (al::isNerve(this, &NrvJumpFlipSweetsCreamTouchAction))
        return false;
    if (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg) ||
        al::isMsgPlayerBoomerangAttack(pMsg)) {
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::setAppearItemAttackerSensor(this, pOther);
        al::setNerve(this, &NrvJumpFlipSweetsCreamTouchAction);
        return false;
    }
    if (al::isMsgPush(pMsg) || al::isMsgPlayerItemGet(pMsg) || al::isMsgExplosion(pMsg)) {
        if (mCooldown > 0) {
            mCooldown = 30;
            return false;
        }
        al::setAppearItemFactor(this, "直接攻撃", pOther);
        al::setAppearItemAttackerSensor(this, pOther);
        al::setNerve(this, &NrvJumpFlipSweetsCreamTouchAction);
        return true;
    }
    return false;
}
bool JumpFlipSweetsCream::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                               al::ScreenPointer* pPointer, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssist(pMsg) || al::isMsgTouchAssistTrig(pMsg)) {
        if (al::isNerve(this, &NrvJumpFlipSweetsCreamWait)) {
            al::setNerve(this, &NrvJumpFlipSweetsCreamTouchAction);
            al::setAppearItemFactor(this, "直接攻撃", nullptr);
            rc::setAppearItemAttackerSensorByScreenPointer(this, pPointer);
        }
        return true;
    }
    return false;
}
void JumpFlipSweetsCream::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}
void JumpFlipSweetsCream::exeTouchAction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TouchAction");
        al::startHitReaction(this, "接触");
        if (mIsAppearCoin) {
            al::appearItem(this);
            mIsAppearCoin = false;
        }
    }
    if (al::isActionEnd(this)) {
        mCooldown = 30;
        al::setNerve(this, &NrvJumpFlipSweetsCreamWait);
    }
}
