#include "Boss/TentackTentacle.hpp"
#include <attributes.h>
#include <math/seadMathCalcCommon.h>
#include "Boss/TentackAttachItem.hpp"
#include "Boss/TentackAttachItemHolder.hpp"
#include "Boss/TentackBase.hpp"
#include "Boss/TentackTentacleStep.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

/**
 * Declares a tentacle nerve whose execute runs an exe function of another name.
 * @param Action Nerve name.
 * @param ExeAction Name of the exe function the nerve runs.
 */
#define TENTACLE_NERVE_DECL_EXE(Action, ExeAction)                                                 \
    class TentackTentacleNrv##Action : public al::Nerve {                                          \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<TentackTentacle>()->exe##ExeAction();                               \
        }                                                                                          \
    };

namespace {
NERVE_DECL(TentackTentacle, AppearDelay);
NERVE_DECL(TentackTentacle, AppearSign);
NERVE_DECL(TentackTentacle, Stiff);
TENTACLE_NERVE_DECL_EXE(BiteFromStiff, Bite);
NERVE_DECL(TentackTentacle, Bite);
NERVE_DECL(TentackTentacle, PushMax);
NERVE_DECL(TentackTentacle, Back);
NERVE_DECL(TentackTentacle, Damage);
TENTACLE_NERVE_DECL_EXE(DamageDead, Damage);
NERVE_DECL(TentackTentacle, PushBack);
NERVE_DECL(TentackTentacle, PushEnd);
NERVE_DECL(TentackTentacle, Swing);
NERVE_DECL(TentackTentacle, StiffEnd);
TENTACLE_NERVE_DECL_EXE(DamageEnd, Back);
NERVE_DECL(TentackTentacle, BiteStart);
NERVE_DECL(TentackTentacle, StiffStart);
NERVE_DECL(TentackTentacle, BackSign);
NERVES_MAKE_NOSTRUCT(TentackTentacle, AppearDelay, AppearSign, Stiff, BiteFromStiff, Bite, PushMax,
                     Back, Damage, DamageDead, PushBack, PushEnd, Swing, StiffEnd, DamageEnd,
                     BiteStart, StiffStart, BackSign)

/**
 * @brief Moves an actor onto a tentacle's position at the given height, turns it randomly and
 * makes it appear.
 * @param pActor Actor to place.
 * @param pTentacle Tentacle whose position is used.
 * @param y Height to place the actor at.
 */
inline void appearRandomRotateY(al::LiveActor* pActor, const TentackTentacle* pTentacle, f32 y) {
    al::setTrans(pActor, al::getTrans(pTentacle));
    al::getTransPtr(pActor)->y = y;
    al::rotateQuatYDirDegree(pActor, al::getRandomDegree());
    pActor->appear();
}

/**
 * @brief Changes the nerve unless the tentacle is appearing, going back or taking damage.
 * @param pTentacle Tentacle to change.
 * @param pNerve Nerve to set.
 * @return Whether the nerve was changed.
 */
NOINLINE bool trySetNerveIfActive(TentackTentacle* pTentacle, const al::Nerve* pNerve) {
    if (al::isNerve(pTentacle, &NrvTentackTentacleAppearDelay) ||
        al::isNerve(pTentacle, &NrvTentackTentacleAppearSign) ||
        al::isNerve(pTentacle, &NrvTentackTentacleBackSign) ||
        al::isNerve(pTentacle, &NrvTentackTentacleBack) || pTentacle->isDamage()) {
        return false;
    }

    al::setNerve(pTentacle, pNerve);
    return true;
}
}  // namespace

/** @brief Creates an empty tentacle setup. */
TentackTentacleInfo::TentackTentacleInfo() = default;

/** @brief Clears the setup flags and item type while keeping the push height. */
void TentackTentacleInfo::reset() {
    mIsAttack = false;
    mIsBite = false;
    mIsStepBreakable = false;
    mItemType = 0;
}

/**
 * @brief Copies this setup into another.
 * @param pDst Setup to overwrite.
 */
void TentackTentacleInfo::copy(TentackTentacleInfo* pDst) {
    pDst->mPushHeight = mPushHeight;
    pDst->mIsAttack = mIsAttack;
    pDst->mIsBite = mIsBite;
    pDst->mIsStepBreakable = mIsStepBreakable;
    pDst->mItemType = mItemType;
}

/**
 * @brief Creates a tentacle and its two hill models.
 * @param pName Actor name.
 * @param pHost Tentack battle that owns the tentacle.
 */
TentackTentacle::TentackTentacle(const char* pName, TentackBase* pHost)
    : al::LiveActor(pName), mHost(pHost) {}

/**
 * @brief Initializes the model, scaffold, crack break model, sensors and collisions.
 * @param rInfo Actor placement and scene initialization information.
 */
void TentackTentacle::init(const al::ActorInitInfo& rInfo) {
    bool isLv2 = al::isObjectName(rInfo, "TentackLv2");
    al::initActorWithArchiveName(
        this, rInfo, sead::SafeString(isLv2 ? "TentackTentacleLv2" : "TentackTentacle"), nullptr);
    al::initNerve(this, &NrvTentackTentacleAppearDelay, 0);
    const sead::Vector3f& trans = al::getTrans(this);
    mCrackPos.set(trans);
    mBasePos.set(trans);
    mBasePos.y += -1400.0f;
    al::calcFrontDir(&mFrontDir, this);

    mStep = new TentackTentacleStep("テンタック足場", this, &mCrackPos);
    al::initCreateActorWithPlacementInfo(mStep, rInfo);
    al::startMtpAnimAndSetFrameAndStop(mStep, "TentackTentacleStep", isLv2 ? 1.0f : 0.0f);
    mHill->initActorWithModelName(rInfo, "TentackTentacleHill", nullptr);
    mHillAttack->initActorWithModelName(rInfo, "TentackTentacleHill", "Attack");

    mCrackBreakMtx = *getBaseMtx();
    mCrackBreak = new al::BreakModel(this, "テンタック子蛇の出現時ひび割れ壊れモデル",
                                     "TentackTentacleCrackBreak", nullptr, &mCrackBreakMtx,
                                     "Break", false);
    al::initCreateActorWithPlacementInfo(mCrackBreak, rInfo);
    al::initPrePassLightMtxConnector(this, "登場スポットライト", &mLightMtx);
    al::killPrePassLight(this, "登場スポットライト", -1);
    al::setEffectFollowPosPtr(this, "Ray", &mEffectPos);

    mSensorNoseBite = al::getHitSensor(this, "BodyNoseBite");
    mSensorNoseWait = al::getHitSensor(this, "BodyNoseWait");
    mSensorFloorPush = al::getHitSensor(this, "FloorPush");
    al::invalidateHitSensor(this, "TrampleRumbleFixed");
    mCollisionDamaged = al::createCollisionObjMtx(this, rInfo, "TentackTentacleDamaged",
                                                  al::getHitSensor(this, "CollisionParts"),
                                                  getBaseMtx(), nullptr);
    mCollisionDamageDead = al::createCollisionObjMtx(this, rInfo, "TentackTentacleDamageDead",
                                                     al::getHitSensor(this, "CollisionParts"),
                                                     getBaseMtx(), nullptr);
    al::setHitSensorPosPtr(this, "CrackBreak", &mCrackPos);
    al::invalidateHitSensor(this, "CrackBreak");
    al::resetPosition(this, mBasePos, false);

    mCollisionDamaged->makeActorDead();
    mCollisionDamageDead->makeActorDead();
    mCrackBreak->makeActorDead();
    makeActorDead();
}

/** @brief Starts the appearance sign with the prepared setup and attaches an item. */
void TentackTentacle::appear() {
    if (mInfo.mIsAttack) {
        return;
    }

    al::LiveActor::appear();
    al::setNerve(this, &NrvTentackTentacleAppearSign);
    mInfo.copy(&mCurInfo);
    f32 x = al::getTrans(this).x;
    f32 z = al::getTrans(this).z;
    mCrackPos.x = x;
    mCrackPos.z = z;
    mCurHill = !mCurInfo.mIsAttack ? mHill : mHillAttack;

    mAttachItem = mHost->getAttachItemHolder()->tryAttachItem(mStep, mCurInfo.mItemType);

    if (mAttachItem == nullptr) {
        mAttachItem = mHost->getAttachItemHolder()->attachItem(mStep, 0);
    }

    mCollisionDamaged->appear();
    mCollisionDamageDead->appear();
    switchCollisionParts(CollisionType_Body);
    mPushHeight = mCurInfo.mPushHeight;
    al::setTransY(this, mBasePos.y);
    al::resetPosition(this, false);
    al::faceToDirection(this, mFrontDir);
    al::hideModelIfShow(this);
    al::invalidateHitSensors(this);
}

/**
 * @brief Enables the collision of the given set and disables the others.
 * @param type Collision set to enable (CollisionType).
 */
void TentackTentacle::switchCollisionParts(s32 type) {
    switch (type) {
    case CollisionType_Body:
        al::validateCollisionParts(this);
        al::invalidateCollisionParts(mCollisionDamaged);
        al::invalidateCollisionParts(mCollisionDamageDead);
        break;
    case CollisionType_Damaged:
        al::validateCollisionParts(mCollisionDamaged);
        al::invalidateCollisionParts(this);
        al::invalidateCollisionParts(mCollisionDamageDead);
        break;
    case CollisionType_DamageDead:
        al::validateCollisionParts(mCollisionDamageDead);
        al::invalidateCollisionParts(this);
        al::invalidateCollisionParts(mCollisionDamaged);
        break;
    }
}

/**
 * @brief Appears and optionally waits before showing the appearance sign.
 * @param delay Steps to wait, or 0 to start right away.
 */
void TentackTentacle::appearDelay(s32 delay) {
    appear();

    if (delay != 0) {
        mAppearDelay = delay;
        al::setNerve(this, &NrvTentackTentacleAppearDelay);
    }
}

/** @brief Hides the tentacle with its hill, crack, collisions, light and attached item. */
void TentackTentacle::kill() {
    al::LiveActor::kill();

    if (al::isAlive(mCurHill)) {
        mCurHill->setDisappear();
    }

    tryBreakCrack();
    mUnknown240 = nullptr;
    mCollisionDamaged->kill();
    mCollisionDamageDead->kill();

    if (!al::isNerve(this, &NrvTentackTentacleAppearDelay)) {
        al::killPrePassLight(this, "登場スポットライト", 10);
        al::tryDeleteEffect(this, "Ray");
    }

    releaseAndTryKillAttachItem();
}

/** @brief Breaks the floor crack the tentacle came out of, if it is still shown. */
void TentackTentacle::tryBreakCrack() {
    if (mCrack == nullptr || al::isDead(mCrack)) {
        return;
    }

    mCrack->kill();
    mCrackBreakMtx.setTranslation(al::getTrans(mCrack));
    al::appearBreakModelRandomRotateY(mCrackBreak);
}

/**
 * @brief Releases the attached item and kills it if possible.
 * @return Whether an item was attached.
 */
bool TentackTentacle::releaseAndTryKillAttachItem() {
    if (mAttachItem == nullptr) {
        return false;
    }

    mAttachItem->releaseAndTryKill();
    mAttachItem = nullptr;
    return true;
}

/** @brief Counts down the scaffold collision timer and plays the trample rumble. */
void TentackTentacle::control() {
    if (mStepCollisionTime > 0) {
        mStepCollisionTime--;
    }

    if (mRumbleStep < 0) {
        return;
    }

    if (mRumbleStep == 0) {
        mRumbleCalc->start(0);
    }

    if (mRumbleStep >= 20) {
        al::setScaleY(this, 1.0f);
        mRumbleStep = -1;
    } else {
        mRumbleCalc->calc();
        al::setScaleY(this, mRumbleCalc->getValueY() + 1.0f);
        mRumbleStep++;
    }
}

/**
 * @brief Bites or pushes the player and breaks magma balls.
 * @param pSelf Attacking sensor.
 * @param pOther Receiving sensor.
 */
void TentackTentacle::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!mStep->isExistStep() && al::isSensorEnemyAttack(pSelf)) {
        rc::sendMsgTentackMagmaBallBreak(pOther, pSelf);

        if (al::isNerve(this, &NrvTentackTentacleStiff) &&
            al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
            al::startAction(this, "Bite");
            al::setNerve(this, &NrvTentackTentacleBiteFromStiff);
            return;
        }
    }

    if (al::isSensorName(pSelf, "CrackBreak")) {
        if (al::isNerve(this, &NrvTentackTentacleAppearSign) &&
            sead::Mathf::abs(al::getSensorPos(pSelf).y - al::getSensorPos(pOther).y) < 20.0f) {
            rc::sendMsgTentackMagmaBallBreak(pOther, pSelf);
        }

        return;
    }

    if (isStiff()) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorPlayer(pOther)) {
        if (pSelf == mSensorNoseBite && al::isNerve(this, &NrvTentackTentacleBiteFromStiff)) {
            if (rc::isPlayerOnGround(pOther)) {
                al::sendMsgPushVeryStrong(pOther, pSelf);
            } else {
                al::sendMsgPushStrong(pOther, pSelf);
            }

            return;
        }

        if (pSelf == mSensorNoseWait && al::isActionPlaying(this, "Wait")) {
            if (!rc::isPlayerClimbOrClimbSpecial(pOther) || rc::isPlayerOnGround(pOther)) {
                al::sendMsgPush(pOther, pSelf);
            }

            return;
        }

        if (pSelf == mSensorFloorPush && (al::isNerve(this, &NrvTentackTentacleBite) ||
                                          al::isNerve(this, &NrvTentackTentacleBiteFromStiff))) {
            f32 top = al::getSensorPos(pSelf).y + al::getSensorRadius(pSelf) + -50.0f;

            if (top < al::getSensorPos(pOther).y) {
                return;
            }

            al::sendMsgPushStrong(pOther, pSelf);
            return;
        }
    }

    if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        if (al::isNerve(this, &NrvTentackTentacleBite) ||
            al::isNerve(this, &NrvTentackTentacleBiteFromStiff) ||
            al::isActionPlaying(this, "Attack")) {
            if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
                if (!al::isActionPlaying(this, "Attack")) {
                    al::startAction(this, "Bite");
                    al::setNerve(this, &NrvTentackTentacleBiteFromStiff);
                }

                return;
            }

            al::sendMsgPush(pOther, pSelf);
        }
    }
}

/**
 * @brief Checks whether the tentacle is stiffened by a climbing player.
 * @return Whether a stiff nerve is active.
 */
bool TentackTentacle::isStiff() const {
    return al::isNerve(this, &NrvTentackTentacleStiffStart) ||
           al::isNerve(this, &NrvTentackTentacleStiff) ||
           al::isNerve(this, &NrvTentackTentacleStiffEnd);
}

/**
 * @brief Stiffens for a climbing player and plays the trample rumble when stepped on.
 * @param pMsg Incoming message.
 * @param pOther Sending sensor.
 * @param pSelf Receiving sensor.
 * @return Whether the message was handled.
 */
bool TentackTentacle::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf) {
    if (!al::isSensorPlayer(pOther)) {
        return false;
    }

    if (tryStartStiff(pMsg, pOther)) {
        return true;
    }

    if (!al::isSensorEnemyBody(pSelf)) {
        return false;
    }

    if ((isStiff() && !al::isNerve(this, &NrvTentackTentacleStiff) && al::isDead(mStep)) ||
        isDamage()) {
        if (EnemyStateUtil::isMsgPressDownForCrossoverSensor(pMsg, pOther, pSelf) ||
            al::isMsgPlayerObjHipDropReflectAll(pMsg) || al::isMsgPlayerBodyAttackReflect(pMsg)) {
            if (al::isSensorName(pSelf, "TrampleRumble") ||
                al::isSensorName(pSelf, "TrampleRumbleFixed") || pSelf == mSensorNoseWait) {
                mRumbleStep = 0;
                al::startSe(this, "PgTrampled");
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Stiffens the tentacle when a player climbs onto it.
 * @param pMsg Incoming message.
 * @param pOther Player sensor.
 * @return Whether the tentacle started stiffening.
 */
bool TentackTentacle::tryStartStiff(const al::SensorMsg* pMsg, const al::HitSensor* pOther) {
    if (!al::isMsgPlayerTouch(pMsg)) {
        return false;
    }

    if (!al::isSensorPlayer(pOther)) {
        return false;
    }

    if (!rc::isPlayerClimbOrClimbSpecial(pOther)) {
        return false;
    }

    const char* actionName = al::getActionName(al::getSensorHost(pOther));

    if (actionName == nullptr) {
        return false;
    }

    if (!al::isEqualSubString(actionName, "ClimbClimbWall")) {
        return false;
    }

    if (isStiff()) {
        mStep->switchCollisionParts(true);
    }

    if (mCurInfo.mIsAttack) {
        return false;
    }

    if (al::isNerve(this, &NrvTentackTentacleStiffStart)) {
        return false;
    }

    if (al::isNerve(this, &NrvTentackTentacleSwing) ||
        al::isNerve(this, &NrvTentackTentacleBiteStart) ||
        al::isNerve(this, &NrvTentackTentacleBite) ||
        al::isNerve(this, &NrvTentackTentacleBiteFromStiff) ||
        al::isNerve(this, &NrvTentackTentaclePushMax) ||
        al::isNerve(this, &NrvTentackTentaclePushBack) ||
        al::isNerve(this, &NrvTentackTentaclePushEnd) || isStiff()) {
        mStiffHeight = al::getSensorPos(pOther).y - al::getTrans(mStep).y;

        if (al::isNerve(this, &NrvTentackTentacleStiff)) {
            al::setNerve(this, &NrvTentackTentacleStiff);
        } else {
            tryCancelPush();
            al::setNerve(this, &NrvTentackTentacleStiffStart);
        }

        return true;
    }

    return false;
}

/**
 * @brief Checks whether the tentacle is taking damage.
 * @return Whether a damage nerve is active.
 */
bool TentackTentacle::isDamage() const {
    return al::isNerve(this, &NrvTentackTentacleDamage) ||
           al::isNerve(this, &NrvTentackTentacleDamageEnd) ||
           al::isNerve(this, &NrvTentackTentacleDamageDead);
}

/**
 * @brief Accepts touch assist messages.
 * @param pMsg Incoming message.
 * @param pPointer Screen pointer; unused.
 * @param pTarget Screen target; unused.
 * @return Whether the message is a touch assist message.
 */
bool TentackTentacle::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssistAll(pMsg);
}

/**
 * @brief Takes damage from the boss, or disappears when still too low to be hit.
 * @param isLast Whether the damage defeats the tentacle.
 * @param damage Damage count; unused.
 */
void TentackTentacle::receiveDamage(bool isLast, s32 damage) {
    tryBreakCrack();

    if (al::isDead(this)) {
        return;
    }

    f32 height = al::getTrans(this).y - mBasePos.y;

    if (al::isNerve(this, &NrvTentackTentacleAppearSign) ||
        (al::isNerve(this, &NrvTentackTentaclePushMax) && height < 185.0f)) {
        if (al::isAlive(mStep)) {
            mStep->kill();
        }

        kill();
        return;
    }

    if (al::isNerve(this, &NrvTentackTentacleBack) && height < 185.0f) {
        kill();
        return;
    }

    al::validateHitSensor(this, "TrampleRumbleFixed");

    if (isLast) {
        switchCollisionParts(CollisionType_DamageDead);
        al::setNerve(this, &NrvTentackTentacleDamageDead);
    } else {
        al::setNerve(this, &NrvTentackTentacleDamage);
        switchCollisionParts(CollisionType_Damaged);
    }
}

/**
 * @brief Gets the head the tentacle belongs to.
 * @return Host's head.
 */
TentackHead* TentackTentacle::getHead() const {
    return mHost->getHead();
}

/** @brief Waits for the appearance delay before showing the sign. */
void TentackTentacle::exeAppearDelay() {
    if (al::isGreaterEqualStep(this, mAppearDelay)) {
        al::setNerve(this, &NrvTentackTentacleAppearSign);
    }
}

/** @brief Shows the spotlight and floor crack, then pushes up. */
void TentackTentacle::exeAppearSign() {
    if (al::isFirstStep(this)) {
        if (mCurInfo.mIsAttack) {
            if (!mHost->tryFindTransNearPlayer(al::getTransPtr(this))) {
                kill();
                return;
            }

            al::setTransY(this, mBasePos.y);
        }

        const sead::Vector3f& trans = al::getTrans(this);
        mLightMtx.makeT(trans.x, mCrackPos.y + 2000.0f, trans.z);
        al::appearPrePassLight(this, "登場スポットライト", 0);
        mEffectPos.set(al::getTrans(this));
        mEffectPos.y = mCrackPos.y;
        al::emitEffect(this, "Ray", nullptr);
        appearCrackAndAddRandomRotateY();
        al::validateHitSensor(this, "CrackBreak");
        al::startSe(this, "PgPushSign");
    }

    if (al::isStep(this, 1)) {
        al::showModelIfHide(mCrack);
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvTentackTentaclePushMax);
    }
}

/** @brief Shows the floor crack the tentacle comes out of. */
void TentackTentacle::appearCrackAndAddRandomRotateY() {
    if (mCurInfo.mIsAttack || mCurInfo.mIsBite) {
        mCrack = al::getSubActor(this, "テンタック子蛇・出現時ひび割れモデル");
        al::setTrans(mCrack, al::getTrans(this));
        al::getTransPtr(mCrack)->y = mCrackPos.y;
        mCrack->appear();
    } else {
        mCrack = al::getSubActor(this, "テンタック子蛇・出現時ひび割れモデル[足場付き]");
        appearRandomRotateY(mCrack, this, mCrackPos.y);
    }

    al::hideModelIfShow(mCrack);
    al::startAction(mCrack, "Appear");
}

/** @brief Pushes up out of the floor carrying the scaffold. */
void TentackTentacle::exePushMax() {
    if (al::isFirstStep(this)) {
        if (mCurInfo.mIsAttack || mCurInfo.mIsBite) {
            al::startAction(this, "Attack");
        } else {
            al::startAction(this, "PushAble");
        }

        al::setVisAnimFrame(this, al::getRandom(al::getVisAnimFrameMax(this)));
        tryBreakCrack();
        al::showModelIfHide(this);
        al::validateHitSensors(this);
        al::invalidateHitSensor(this, "TrampleRumbleFixed");
        al::invalidateHitSensor(this, "CrackBreak");
        appearRandomRotateY(mCurHill, this, mCrackPos.y);

        if (!mCurInfo.mIsAttack) {
            al::resetPosition(mStep, al::getTrans(this), false);

            if (mCurInfo.mIsStepBreakable) {
                mStep->appearBreakable();
            } else if (!mCurInfo.mIsBite) {
                mStep->appear();
                mAttachItem->appear();
            }
        }
    }

    if (al::isStep(this, 35)) {
        mStep->startThrow(20.0f);
    }

    al::getTransPtr(this)->y = al::calcNerveEaseInOutValue(this, 40, mBasePos.y,
                                                            mBasePos.y + getPushHeightMax());

    if (al::isGreaterEqualStep(this, 50)) {
        al::setNerve(this, &NrvTentackTentaclePushBack);
    }
}

/**
 * @brief Gets the highest point of the push-up, relative to the base position.
 * @return Push-up height.
 */
f32 TentackTentacle::getPushHeightMax() const {
    return mPushHeight > 1000.0f ? mPushHeight + 200.0f : 1000.0f;
}

/** @brief Sinks back down to the swing height. */
void TentackTentacle::exePushBack() {
    al::isFirstStep(this);
    al::getTransPtr(this)->y = al::calcNerveEaseInOutValue(
        this, 45, mBasePos.y + getPushHeightMax(), getSwingCenter() + -25.0f);

    if (al::isGreaterEqualStep(this, 45)) {
        if (mCurInfo.mIsAttack) {
            kill();
            return;
        }

        mCurHill->trySetWaitIfNotPlaying();

        if (mCurInfo.mIsBite) {
            al::setNerve(this, &NrvTentackTentacleBite);
        } else {
            al::setNerve(this, &NrvTentackTentaclePushEnd);
        }
    }
}

/** @brief Plays the end of the push-up, then starts swinging. */
void TentackTentacle::exePushEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PushEnd");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTentackTentacleSwing);
    }
}

/** @brief Swings up and down around the swing center. */
void TentackTentacle::exeSwing() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    s32 step = al::getNerveStep(this) % 120;
    f32 rate = al::normalize((f32)step, 0.0f, 60.0f);

    if (step > 60) {
        rate = 1.0f - al::normalize((f32)step, 60.0f, 120.0f);
    }

    f32 t = al::easeInOut(rate);
    al::getTransPtr(this)->y =
        al::lerpValue(t, getSwingCenter() + -25.0f, getSwingCenter() + 25.0f);
}

/**
 * @brief Gets the height the tentacle swings around.
 * @return Swing center height.
 */
f32 TentackTentacle::getSwingCenter() const {
    return mBasePos.y + mPushHeight;
}

/** @brief Starts stiffening and throws the scaffold up. */
void TentackTentacle::exeStiffStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mStep->isExistStep() ? "StiffStart" : "StiffBiteStart");
        mStep->startThrow(30.0f);
        mStep->switchCollisionParts(true);
        mStiffHeight = -1000.0f;
    }

    if (al::isActionEnd(this)) {
        trySetNerveIfActive(this, &NrvTentackTentacleStiff);
    }
}

/** @brief Stays stiff for a while. */
void TentackTentacle::exeStiff() {
    if (al::isFirstStep(this)) {
        al::tryStartActionIfNotPlaying(this, mStep->isExistStep() ? "Stiff" : "StiffBite");
    }

    bool isEnd;

    if (mStep->isExistStep() && mStiffHeight > -200.0f) {
        isEnd = al::isGreaterEqualStep(this, 120);
    } else {
        isEnd = al::isGreaterEqualStep(this, 30);
    }

    if (isEnd) {
        trySetNerveIfActive(this, &NrvTentackTentacleStiffEnd);
    }
}

/** @brief Recovers from stiffening back to the swing height. */
void TentackTentacle::exeStiffEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mStep->isExistStep() ? "StiffEnd" : "StiffBiteEnd");
        mStep->startThrow(27.5f);
    }

    f32 rate = al::getActionFrameRate(this);
    al::setTransY(this, al::lerpValue(rate, al::getTrans(this).y, getSwingCenter()));

    if (al::isActionEnd(this)) {
        mStep->switchCollisionParts(false);
        mStepCollisionTime = 10;

        if (mStep->isExistStep()) {
            trySetNerveIfActive(this, &NrvTentackTentacleSwing);
        } else {
            trySetNerveIfActive(this, &NrvTentackTentacleBite);
        }
    }
}

/** @brief Opens the mouth, then starts biting. */
void TentackTentacle::exeBiteStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BiteStart");
    }

    if (al::isActionEnd(this)) {
        trySetNerveIfActive(this, &NrvTentackTentacleBite);
    }
}

/** @brief Starts the bite animation when entering the bite nerve. */
void TentackTentacle::exeBite() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvTentackTentacleBite)) {
        al::startAction(this, "Bite");
    }
}

/** @brief Shows the sign of going back into the floor. */
void TentackTentacle::exeBackSign() {
    if (al::isFirstStep(this)) {
        al::startAction(this, al::isAlive(mStep) ? "BackSign" : "Attack");
    }

    if (al::isGreaterEqualStep(this, al::getActionFrameMax(this, "BackSign"))) {
        al::setNerve(this, &NrvTentackTentacleBack);
    }
}

/** @brief Drops the scaffold and goes back into the floor. */
void TentackTentacle::exeBack() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvTentackTentacleDamageEnd)) {
            al::startAction(this, "DamageEnd");
        } else {
            al::tryStartActionIfNotPlaying(this, al::isAlive(mStep) ? "Back" : "Attack");
        }

        mBackStartY = al::getTrans(this).y;

        if (mStep->isExistStep()) {
            if (mStep->isThrow()) {
                f32 speed = al::getVelocity(mStep).y;
                mStep->startFall(speed);
            } else {
                mStep->startFall(20.0f);
            }

            releaseAndTryKillAttachItem();
        }

        mCurHill->setReverse();
    }

    al::getTransPtr(this)->y =
        al::lerpValue(al::calcNerveEaseInRate(this, 40), mBackStartY, mBasePos.y);

    if (al::isGreaterEqualStep(this, 40)) {
        kill();
    }
}

/** @brief Plays the damage animation and breaks the scaffold. */
void TentackTentacle::exeDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(this, al::isNerve(this, &NrvTentackTentacleDamage) ? "Damage" : "DamageDead");
        mStep->setBreak();
        releaseAndTryKillAttachItem();
    }

    if (al::isStep(this, 180) && al::isNerve(this, &NrvTentackTentacleDamage)) {
        switchCollisionParts(CollisionType_Body);
        al::invalidateHitSensor(this, "TrampleRumbleFixed");
    }

    if (al::isActionEnd(this)) {
        if (al::isNerve(this, &NrvTentackTentacleDamageDead)) {
            kill();
        } else {
            al::setNerve(this, &NrvTentackTentacleDamageEnd);
        }
    }
}

/**
 * @brief Starts biting, cancelling a push-up in progress.
 * @return Whether the bite started.
 */
bool TentackTentacle::tryStartBite() {
    if (tryCancelPush()) {
        al::setNerve(this, &NrvTentackTentacleBiteStart);
        return true;
    }

    return trySetNerveIfActive(this, &NrvTentackTentacleBiteStart);
}

/**
 * @brief Stops a push-up at the current height.
 * @return Whether a push-up was cancelled.
 */
bool TentackTentacle::tryCancelPush() {
    if (al::isNerve(this, &NrvTentackTentaclePushMax) ||
        al::isNerve(this, &NrvTentackTentaclePushBack)) {
        mPushHeight = al::getTrans(this).y - mBasePos.y;
        mCurHill->trySetWaitIfNotPlaying();
        return true;
    }

    return false;
}

/** @brief Ends the swing and goes back into the floor. */
void TentackTentacle::endSwing() {
    al::setNerve(this, &NrvTentackTentacleBackSign);
}

/** @brief Ends the swing and goes back into the floor regardless of the state. */
void TentackTentacle::endSwingForce() {
    al::setNerve(this, &NrvTentackTentacleBackSign);
}

/**
 * @brief Sets the swing height of the next appearance.
 * @param rate Height rate; the integer part adds whole steps of 500.
 */
void TentackTentacle::setSwingYRate(f32 rate) {
    f32 level = sead::Mathf::floor(rate);
    f32 offset = al::lerpValue(rate - level, 475.0f, 975.0f);
    mInfo.mPushHeight = level * 500.0f + offset;
}

/**
 * @brief Lets the head eat the attached item if it is still alive.
 * @param isForce Whether to eat it forcibly.
 */
void TentackTentacle::eatAttachItemIfAttached(bool isForce) {
    if (mAttachItem != nullptr && al::isAlive(mAttachItem)) {
        mAttachItem->eat(isForce);
    }
}

/**
 * @brief Checks whether the tentacle has appeared.
 * @return Whether it is alive and no longer waiting to appear.
 */
bool TentackTentacle::isAppear() const {
    return al::isAlive(this) && !al::isNerve(this, &NrvTentackTentacleAppearDelay);
}

/**
 * @brief Checks whether the tentacle's position is decided.
 * @return Whether the position no longer depends on the player.
 */
bool TentackTentacle::isDecidedTrans() const {
    return !mCurInfo.mIsAttack || isAppear();
}

/**
 * @brief Checks whether the swing can be ended.
 * @return Whether the tentacle is not stiff.
 */
bool TentackTentacle::isEnableEndSwing() const {
    return !isStiff();
}

/**
 * @brief Checks whether the scaffold can be broken.
 * @return Whether the tentacle is not pushing up.
 */
bool TentackTentacle::isEnableStepBreak() const {
    return !al::isNerve(this, &NrvTentackTentaclePushMax);
}

/**
 * @brief Calculates where the scaffold sits on the tentacle's head.
 * @param pTrans Output scaffold position.
 */
void TentackTentacle::calcStepTrans(sead::Vector3f* pTrans) const {
    sead::Vector3f headPos = {0.0f, 0.0f, 0.0f};
    al::calcJointPos(&headPos, this, "Head");
    f32 offset = isStiff() ? 300.0f : 170.0f;

    if (al::isActionPlaying(this, "Stiff")) {
        offset = 300.0f;
    } else if (mStepCollisionTime > 0) {
        offset = al::lerpValue(1.0f - al::normalize((f32)mStepCollisionTime, 0.0f, 10.0f), 300.0f,
                               170.0f);
    }

    pTrans->set(al::getTrans(this));
    pTrans->y = offset + headPos.y;
}

/**
 * @brief Gets the radius of a tentacle.
 * @return Tentacle radius.
 */
f32 TentackTentacle::getTentacleRadius() {
    return 200.0f;
}

/** @brief Destroys the tentacle. */
TentackTentacle::~TentackTentacle() = default;
