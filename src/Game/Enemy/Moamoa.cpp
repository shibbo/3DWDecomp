#include "Enemy/Moamoa.hpp"
#include "Enemy/MoamoaHolder.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(Moamoa, Wait)
NERVE_DECL(Moamoa, MicWeakInit)
class MoamoaNrvShockwave : public al::Nerve {
public:
    /** @brief Runs the shockwave weakening state. @param pKeeper State owner. */
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<Moamoa>()->exeWeak(); }
};
NERVE_DECL(Moamoa, Die)
NERVE_DECL(Moamoa, Weak)
class MoamoaNrvTouchWeak : public al::Nerve {
public:
    /** @brief Runs the assisted weakening state. @param pKeeper State owner. */
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<Moamoa>()->exeWeak(); }
};
class MoamoaNrvTouchWeakWait : public al::Nerve {
public:
    /** @brief Waits after assisted weakening. @param pKeeper State owner. */
    void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<Moamoa>()->exeWeakWait(); }
};
NERVE_DECL(Moamoa, WeakWait)
NERVE_DECL(Moamoa, RevivalSign)
NERVE_DECL(Moamoa, Revival)
NERVES_MAKE_STRUCT(Moamoa, Wait, MicWeakInit, Shockwave, Die, Weak, TouchWeak, TouchWeakWait, WeakWait, RevivalSign, Revival)
struct MoamoaParam {
    sead::Vector2f lookLimit = sead::Vector2f(90.0f, 90.0f);
    sead::Vector3f shadowSize = sead::Vector3f(95.0f, 500.0f, 95.0f);
    sead::Vector3f weakShadowSize = sead::Vector3f(15.0f, 500.0f, 15.0f);
    ActorJointLookControllerParam horizontal{1.5f, sead::Vector2f(-7.0f, 7.0f), false, nullptr, nullptr};
    ActorJointLookControllerParam vertical{1.5f, sead::Vector2f(-5.0f, 12.5f), true, nullptr, nullptr};
} sMoamoaParam;
}

/** @brief Creates a held enemy and its eye controller. @param pName Actor name. @param pHolder Owning grid. */
Moamoa::Moamoa(const char* pName, const MoamoaHolder* pHolder)
    : al::LiveActor(pName), mHolder(pHolder), mLookController(new ActorJointLookController(this, 4)),
      mOffset(0.0f, 0.0f, 0.0f), mMicDelay(0) {}

/** @brief Initializes the model, eyes and shadow. @param rInfo Placement information. */
void Moamoa::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Moamoa", nullptr);
    al::initNerve(this, &NrvMoamoa.Wait, 0);
    al::invalidateHitSensor(this, "Shockwave");
    al::setShadowMaskSize(this, "シャドウマスク", sMoamoaParam.shadowSize);
    al::initJointControllerKeeper(this, 4);
    mLookController->appendJoint("EyeL", -sead::Vector3f::ey, &sMoamoaParam.horizontal);
    mLookController->appendJoint("EyeL", sead::Vector3f::ez, &sMoamoaParam.vertical);
    mLookController->appendJoint("EyeR", -sead::Vector3f::ey, &sMoamoaParam.horizontal);
    mLookController->appendJoint("EyeR", sead::Vector3f::ez, &sMoamoaParam.vertical);
    mLookController->setLimit(sMoamoaParam.lookLimit);
    makeActorAppeared();
    bool hideShadow = false;
    bool found = al::tryGetArg(&hideShadow, rInfo, "IsHideShadow");
    if (hideShadow && found)
        al::invalidateShadow(this);
}

/** @brief Updates the eyes and delays microphone weakening according to player distance. */
void Moamoa::control() {
    mLookController->update();
    if (al::isNerve(this, &NrvMoamoa.MicWeakInit) || al::isNerve(this, &NrvMoamoa.Wait) ||
        al::isNerve(this, &NrvMoamoa.RevivalSign) || al::isNerve(this, &NrvMoamoa.Revival)) {
        if (!al::isNerve(this, &NrvMoamoa.MicWeakInit) && al::isMicInputOn(this)) {
            f32 distance = (al::getTrans(this) - al::findNearestPlayerPos(this)).length();
            if (!(distance > 1000.0f)) {
                mMicDelay = static_cast<s32>(al::easeIn(al::normalize(distance, 0.0f, 1000.0f)) * 30.0f);
                al::setNerve(this, &NrvMoamoa.MicWeakInit);
            }
        }
    }
}

/** @brief Attacks the player and spreads a weakening shockwave. @param pSelf Own sensor. @param pOther Contact sensor. */
void Moamoa::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isNerve(this, &NrvMoamoa.Shockwave) && al::isSensorEye(pSelf) && al::isSensorEnemyBody(pOther))
        rc::sendMsgKillerShockWave(pOther, pSelf);
    if (al::isNerve(this, &NrvMoamoa.Wait) && al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther) &&
        al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
        al::setNerve(this, &NrvMoamoa.Shockwave);
        mLookController->stopLook();
    }
}

/** @brief Handles attacks and eating. @param pMsg Message. @param pOther Sender sensor. @param pSelf Own sensor. @return Whether handled. */
bool Moamoa::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!(al::isNerve(this, &NrvMoamoa.MicWeakInit) || al::isNerve(this, &NrvMoamoa.Wait) ||
          al::isNerve(this, &NrvMoamoa.RevivalSign) || al::isNerve(this, &NrvMoamoa.Revival)))
        return false;
    if (rc::isMsgPackunEatStart(pMsg))
        return true;
    if (rc::isMsgPackunEat(pMsg)) {
        rc::addScore(this, pOther, 100.0f, 0);
        kill();
        return true;
    }
    if (al::isMsgPlayerInvincibleAttack(pMsg) || al::isMsgPlayerGiantAttack(pMsg) || al::isMsgLaserAttack(pMsg)) {
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScore(this, pOther, 100.0f, 0);
        al::setNerve(this, &NrvMoamoa.Die);
        return true;
    }
    if (al::isMsgPlayerSlidingAttack(pMsg))
        return false;
    if (EnemyStateUtil::isMsgBlowDown(pMsg) || al::isMsgBallTrample(pMsg) ||
        rc::isMsgGongShockwave(pMsg) || rc::isMsgKillerShockWave(pMsg)) {
        al::setNerve(this, &NrvMoamoa.Weak);
        if (al::isMsgBallAttack(pMsg))
            return false;
        return !al::isMsgBallTrample(pMsg);
    }
    return false;
}

/** @brief Handles touch assistance. @param pMsg Message. @param pPointer Pointer. @param pTarget Target. @return Whether handled. */
bool Moamoa::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssist(pMsg) && al::isNerve(this, &NrvMoamoa.TouchWeak))
        return true;
    if (al::isNerve(this, &NrvMoamoa.MicWeakInit) || al::isNerve(this, &NrvMoamoa.Wait) ||
        al::isNerve(this, &NrvMoamoa.RevivalSign) || al::isNerve(this, &NrvMoamoa.Revival)) {
        if (al::isMsgTouchAssist(pMsg)) {
            al::setNerve(this, &NrvMoamoa.TouchWeak);
            return true;
        }
    }
    return false;
}

/** @brief Waits while tracking the player when allowed by the holder. */
void Moamoa::exeWait() {
    if (al::isFirstStep(this))
        al::startActionAtRandomFrame(this, "Wait");
    if (mHolder->isLookCtrl())
        mLookController->setLookAtNearestPlayer(800.0f);
    else
        mLookController->stopLook();
}

/** @brief Plays the defeat animation and releases an item. */
void Moamoa::exeDie() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Die");
        al::invalidateClipping(this);
        mLookController->stopLook();
    }
    if (al::isActionEnd(this)) {
        al::validateClipping(this);
        al::appearItem(this);
        kill();
    }
}

/** @brief Waits for the microphone effect to reach this enemy. */
void Moamoa::exeMicWeakInit() {
    if (al::isGreaterEqualStep(this, mMicDelay))
        al::setNerve(this, &NrvMoamoa.TouchWeak);
}

/** @brief Shrinks the enemy and optionally expands its shockwave sensor. */
void Moamoa::exeWeak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Weak");
        mLookController->stopLook();
    }
    if (al::isNerve(this, &NrvMoamoa.Shockwave)) {
        if (al::isFirstStep(this)) {
            al::validateHitSensor(this, "Shockwave");
            al::setSensorRadius(this, "Shockwave", 0.0f);
        }
        if (al::isActionEnd(this))
            al::invalidateHitSensor(this, "Shockwave");
        al::setSensorRadius(this, "Shockwave", al::calcNerveRate(this, 10) * 125.0f);
    }
    if (!al::isHideShadow(this)) {
        sead::Vector3f size(0.0f, 0.0f, 0.0f);
        al::lerpVec(&size, sMoamoaParam.shadowSize, sMoamoaParam.weakShadowSize, al::calcNerveRate(this, 5));
        al::setShadowMaskSize(this, "シャドウマスク", size);
    }
    if (al::isActionEnd(this)) {
        if (al::isNerve(this, &NrvMoamoa.TouchWeak))
            al::setNerve(this, &NrvMoamoa.TouchWeakWait);
        else
            al::setNerve(this, &NrvMoamoa.WeakWait);
    }
}

/** @brief Remains weakened before beginning revival. */
void Moamoa::exeWeakWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "WeakWait");
    al::isNerve(this, &NrvMoamoa.WeakWait);
    if (al::isGreaterEqualStep(this, 120))
        al::setNerve(this, &NrvMoamoa.RevivalSign);
}

/** @brief Signals that the enemy is about to recover. */
void Moamoa::exeRevivalSign() {
    if (al::isFirstStep(this))
        al::startAction(this, "Shake");
    if (al::isGreaterEqualStep(this, 10))
        al::setNerve(this, &NrvMoamoa.Revival);
}

/** @brief Restores the enemy and its shadow to normal size. */
void Moamoa::exeRevival() {
    if (al::isFirstStep(this))
        al::startAction(this, "Revival");
    if (!al::isHideShadow(this)) {
        sead::Vector3f size(0.0f, 0.0f, 0.0f);
        al::lerpVec(&size, sMoamoaParam.weakShadowSize, sMoamoaParam.shadowSize, al::calcNerveRate(this, 20));
        al::setShadowMaskSize(this, "シャドウマスク", size);
    }
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvMoamoa.Wait);
}
