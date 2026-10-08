#include "MapObj/PlayerCrown.hpp"

#include <math/seadQuat.h>

#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/IUsePlayerAnimator.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Player/Normal/PlayerGiantDirector.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Player.hpp"
#include "Player/PlayerDef.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/RigidBodyUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    NERVE_DECL(PlayerCrown, Attach)
    NERVE_DECL(PlayerCrown, Fall)
    NERVE_DECL(PlayerCrown, Land)
    NERVE_DECL(PlayerCrown, SinkSand)
    NERVE_DECL(PlayerCrown, Wait)
    NERVES_MAKE_NOSTRUCT(PlayerCrown, Attach, Fall, Land, Wait, SinkSand)

    // Offsets of the crown from the head joint, per character and per figure (EPlayerFigure).
    const sead::Vector3f sMarioRotate[] = {
        {-0.6386f, 0.0f, 0.0f},
        {-0.3596f, 0.0f, 0.0f},
        {-0.6386f, 0.0f, 0.0f},
        {-0.5704f, 0.0f, 0.0f},
        {-0.4278f, 0.0f, 0.0f},
        {-0.3844f, 0.0f, 0.0f},
        {-0.4278f, 0.0f, 0.0f},
        {-0.5704f, 0.0f, 0.0f},
        {-0.5704f, 0.0f, 0.0f},
        {-0.5704f, 0.0f, 0.0f},
    };

    const sead::Vector3f sMarioTrans[] = {
        {0.0f, 41.0f, -2.0f},
        {0.0f, 30.0f, -1.0f},
        {0.0f, 41.0f, -2.0f},
        {0.0f, 38.0f, -11.5f},
        {0.0f, 40.2f, -10.0f},
        {0.0f, 40.0f, -10.0f},
        {0.0f, 40.2f, -10.0f},
        {0.0f, 38.0f, -11.5f},
        {0.0f, 38.0f, -11.5f},
        {0.0f, 38.0f, -11.5f},
    };

    const sead::Vector3f sLuigiRotate[] = {
        {-0.682f, 0.0f, 0.0f},
        {-0.3038f, 0.0f, 0.0f},
        {-0.682f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.5332f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
    };

    const sead::Vector3f sLuigiTrans[] = {
        {0.0f, 51.0f, -6.0f},
        {0.0f, 34.0f, -3.0f},
        {0.0f, 51.0f, -6.0f},
        {0.0f, 50.0f, -12.3f},
        {0.0f, 46.5f, -10.9f},
        {0.0f, 48.0f, -11.0f},
        {0.0f, 46.5f, -10.9f},
        {0.0f, 50.0f, -12.3f},
        {0.0f, 50.0f, -12.3f},
        {0.0f, 50.0f, -12.3f},
    };

    const sead::Vector3f sPeachRotate[] = {
        {-0.0682f, 0.0f, 0.0f},
        {-0.0682f, 0.0f, 0.0f},
        {-0.0682f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
        {-0.0744f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
    };

    const sead::Vector3f sPeachTrans[] = {
        {0.0f, 35.5f, 0.0f},
        {0.0f, 33.5f, 0.0f},
        {0.0f, 35.5f, 0.0f},
        {0.0f, 34.0f, -15.0f},
        {0.0f, 31.0f, -15.0f},
        {0.0f, 35.0f, -5.5f},
        {0.0f, 31.0f, -15.0f},
        {0.0f, 34.0f, -15.0f},
        {0.0f, 34.0f, -15.0f},
        {0.0f, 34.0f, -15.0f},
    };

    const sead::Vector3f sKinopioRotate[] = {
        {-0.3348f, 0.0f, 0.0f},
        {-0.35f, 0.0f, 0.0f},
        {-0.3348f, 0.0f, 0.0f},
        {-0.25f, 0.0f, 0.0f},
        {-0.24f, 0.0f, 0.0f},
        {-0.3f, 0.0f, 0.0f},
        {-0.24f, 0.0f, 0.0f},
        {-0.25f, 0.0f, 0.0f},
        {-0.25f, 0.0f, 0.0f},
        {-0.25f, 0.0f, 0.0f},
    };

    const sead::Vector3f sKinopioTrans[] = {
        {0.0f, 75.0f, -15.0f},
        {0.0f, 52.0f, -14.0f},
        {0.0f, 75.0f, -15.0f},
        {0.0f, 60.0f, -11.0f},
        {0.0f, 75.0f, -18.0f},
        {0.0f, 67.0f, -17.0f},
        {0.0f, 75.0f, -18.0f},
        {0.0f, 60.0f, -11.0f},
        {0.0f, 60.0f, -11.0f},
        {0.0f, 60.0f, -11.0f},
    };

    const sead::Vector3f sRosettaRotate[] = {
        {-0.0682f, 0.0f, 0.0f},
        {-0.0682f, 0.0f, 0.0f},
        {-0.0682f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
        {-0.0744f, 0.0f, 0.0f},
        {-0.65f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
        {-0.68f, 0.0f, 0.0f},
    };

    const sead::Vector3f sRosettaTrans[] = {
        {0.0f, 32.0f, 0.0f},
        {0.0f, 32.0f, 0.0f},
        {0.0f, 32.0f, 0.0f},
        {0.0f, 34.0f, -15.0f},
        {0.0f, 31.0f, -15.0f},
        {0.0f, 35.0f, -5.5f},
        {0.0f, 31.0f, -15.0f},
        {0.0f, 34.0f, -15.0f},
        {0.0f, 34.0f, -15.0f},
        {0.0f, 34.0f, -15.0f},
    };

    const sead::Vector3f sKinopioBrigadeRotate[] = {
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
        {-0.4154f, 0.0f, 0.0f},
    };

    const sead::Vector3f sKinopioBrigadeTrans[] = {
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
    };

    const sead::Vector3f sGiantTrans[] = {
        {0.0f, 45.0f, -4.0f},
        {0.0f, 54.5f, -8.0f},
        {0.0f, 41.0f, -1.5f},
        {0.0f, 79.8f, -16.0f},
        {0.0f, 41.0f, -0.8f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
        {0.0f, 68.0f, -18.0f},
    };

    const sead::Vector3f sManekinekoRotate[] = {
        {-0.6f, 0.0f, 0.0f},
        {-0.6f, 0.0f, 0.0f},
        {-0.6f, 0.0f, 0.0f},
        {-0.38f, 0.0f, 0.0f},
        {-0.6138f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
    };

    const sead::Vector3f sManekinekoTrans[] = {
        {0.0f, 107.0f, -10.0f},
        {0.0f, 116.0f, -3.5f},
        {0.0f, 100.0f, -9.0f},
        {0.0f, 109.0f, -12.0f},
        {0.0f, 102.0f, -8.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
    };

    // Indexed by EPlayerChara; the Kinopio Brigade members share one table.
    const sead::Vector3f* const sTransTable[] = {
        sMarioTrans,          sLuigiTrans,          sPeachTrans,
        sKinopioTrans,        sRosettaTrans,        sKinopioBrigadeTrans,
        sKinopioBrigadeTrans, sKinopioBrigadeTrans, sKinopioBrigadeTrans,
    };

    const sead::Vector3f* const sRotateTable[] = {
        sMarioRotate,          sLuigiRotate,          sPeachRotate,
        sKinopioRotate,        sRosettaRotate,        sKinopioBrigadeRotate,
        sKinopioBrigadeRotate, sKinopioBrigadeRotate, sKinopioBrigadeRotate,
    };

    // Blow off parameters, indexed by the goal release type (0 when released by damage).
    const f32 sBlowFrontRate[] = {0.25f, 1.0f, 1.0f};
    const f32 sBlowSideRate[] = {0.0f, 0.0f, 0.3f};
    const f32 sBlowSpeed[] = {13.0f, 13.0f, 13.0f};

    // Frames the crown lies around before disappearing.
    const u32 sKillFrame = 1000;
    const u32 sGoalKillFrame = 40;
}  // namespace

/**
 * @brief Constructs the crown; it starts dead until a player wears it.
 * @param rInfo Actor init info.
 * @param pSuffix Suffix of the archive's init files.
 */
PlayerCrown::PlayerCrown(const al::ActorInitInfo& rInfo, const char* pSuffix)
    : al::LiveActor("王冠") {
    RigidBodyUtil::calcInertiaTensorOfQuadraticPrism(&mInertiaTensor, 1.0f, 80.0f, 70.0f, 80.0f);
    al::initActorWithArchiveNameNoPlacementInfo(this, rInfo, "PlayerCrown", pSuffix);
    al::invalidateClipping(this);
    al::initNerve(this, &NrvPlayerCrownAttach, 0);
    makeActorDead();
    mFlashingCtrl = new al::FlashingCtrl(this, true, false);
}

/**
 * @brief Lets the crown be blown off its host, or picked up by a player.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the crown.
 * @return True if the message was handled.
 */
bool PlayerCrown::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                             al::HitSensor* pSelf) {
    if (al::isMsgPlayerDisregard(pMsg)) {
        return true;
    }

    if (al::isNerve(this, &NrvPlayerCrownAttach)) {
        if (al::isMsgPlayerReleaseEquipment(pMsg)) {
            shiftFall(0);
            return true;
        }

        if (al::isMsgPlayerReleaseEquipmentGoal(pMsg)) {
            shiftFall(al::getPlayerReleaseEquipmentGoalType(pMsg));
            return true;
        }

        return false;
    }

    if (!isNerveGettable()) {
        return false;
    }

    if (rc::isMsgStartGoalDemoPole(pMsg) || rc::isMsgStartGoalDemoHouse(pMsg)) {
        kill();
        return true;
    }

    if (al::isSensorPlayer(pOther) && al::isMsgPlayerItemGet(pMsg)) {
        if (!rc::tryPlayerEquipCrown(pOther, al::getHitSensor(this, "Normal"), 0)) {
            return false;
        }

        mHost = static_cast<PlayerActor*>(al::getSensorHost(pOther));
        al::setNerve(this, &NrvPlayerCrownAttach);
        al::startSe(this, "Attach", nullptr);
        return true;
    }

    return false;
}

/**
 * @brief Blows the crown off its host.
 * @param goalType Goal release type, or 0 when released by damage.
 */
void PlayerCrown::shiftFall(u32 goalType) {
    mGoalType = goalType;
    al::setNerve(this, &NrvPlayerCrownFall);
}

/**
 * @brief Checks if a player can pick up the crown.
 * @return True once the crown has lain on its own long enough (never after a goal).
 */
bool PlayerCrown::isNerveGettable() const {
    if (!al::isNerve(this, &NrvPlayerCrownFall) && !al::isNerve(this, &NrvPlayerCrownLand) &&
        !al::isNerve(this, &NrvPlayerCrownWait) && !al::isNerve(this, &NrvPlayerCrownSinkSand)) {
        return false;
    }

    return mDropTimer >= 30 && mGoalType == 0;
}

/**
 * @brief Sets the player wearing the crown.
 * @param pHost New host player.
 */
void PlayerCrown::changeHost(PlayerActor* pHost) {
    mHost = pHost;
}

/** @brief Appears on the host (the first alive player by default), or falls if it can't. */
void PlayerCrown::appear() {
    al::LiveActor::appear();
    mDropTimer = -1;
    mFlashingCtrl->end();

    if (mHost == nullptr) {
        auto* pPlayer = static_cast<PlayerActor*>(al::tryFindAlivePlayerActorFirst(this));

        if (pPlayer != nullptr) {
            mHost = pPlayer;
        }
    }

    if (mHost != nullptr &&
        rc::tryPlayerEquipCrown(getPlayerSensor(), al::getHitSensor(this, "Normal"), 0)) {
        al::setNerve(this, &NrvPlayerCrownAttach);
    } else {
        al::setNerve(this, &NrvPlayerCrownFall);
    }
}

/**
 * @brief Gets the body sensor of the host.
 * @return The host's body sensor.
 */
al::HitSensor* PlayerCrown::getPlayerSensor() const {
    return al::getHitSensor(mHost, "Body");
}

/** @brief Disappears with its hit reaction. */
void PlayerCrown::kill() {
    al::startHitReaction(this, "消滅");
    al::LiveActor::kill();
}

/** @brief Follows the host's head and syncs the crown's animation with the host's. */
void PlayerCrown::exeAttach() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        mActionName = "Wait";
        mDropTimer = -1;
        mFlashingCtrl->end();
        al::offCollide(this);
        al::validateHitSensor(this, "Normal");
        al::invalidateHitSensor(this, "WaitItem");
        al::getVelocityPtr(this)->set(0.0f, 0.0f, 0.0f);

        if (al::isExistShadow(this)) {
            al::hideShadow(this);
        }
    }

    if (checkModelHideCondition()) {
        hide();
    } else {
        show();
    }

    if (mHost->getModelHolder()->isSilhouetteHidden()) {
        al::hideSilhouetteModelIfShow(this);
    } else {
        al::showSilhouetteModelIfHide(this);
    }

    const char* pAnimName = mHost->getPlayer()->getAnimator()->getAnimName();

    if (!al::isEqualString(mActionName, sead::SafeString(pAnimName))) {
        al::StringTmp<128> syncName("Sync%s", pAnimName);
        al::startAction(this, al::isExistAction(this, syncName.cstr()) ? syncName.cstr() : "Wait");
        mActionName = pAnimName;
    }

    if (rc::isPlayerDamageTrigOn(mHost) || rc::isPlayerDead(mHost)) {
        rc::removePlayerEquipCrown(al::getHitSensor(mHost, "Body"));
    } else {
        calcFollowMtx();
    }
}

/**
 * @brief Checks if the crown has to be hidden with its host.
 * @return True while the host is hidden, mashed or entering a pipe sideways.
 */
bool PlayerCrown::checkModelHideCondition() const {
    if (rc::isPlayerManekinekoStatueOn(mHost)) {
        return false;
    }

    if (rc::isPlayerHideModel(mHost)) {
        return true;
    }

    if (rc::isPlayerMash(al::getHitSensor(mHost, "Body"))) {
        return true;
    }

    return isModelHideAnim();
}

/** @brief Hides the model and its effects. */
void PlayerCrown::hide() {
    al::hideModelIfShow(this);
    al::offCalcAndDrawEffect(this);
}

/** @brief Shows the model and its effects. */
void PlayerCrown::show() {
    al::showModelIfHide(this);
    al::onCalcAndDrawEffect(this);
}

/** @brief Computes the crown's matrix from the host's head joint (or its statue pose). */
void PlayerCrown::calcFollowMtx() {
    const s32 chara = mHost->getChara();

    if (rc::isPlayerManekinekoStatueOn(mHost)) {
        sead::Matrix34f hostMtx;
        al::makeMtxRT(&hostMtx, mHost);
        sead::Matrix34f localMtx;
        localMtx.makeRT(sManekinekoRotate[chara], sManekinekoTrans[chara]);
        mFollowMtx.setMul(hostMtx, localMtx);
        return;
    }

    s32 figure = mHost->getModelHolder()->getCurrentIndex();
    PlayerGiantDirector* pGiantDirector = mHost->getPlayer()->getGiantDirector();
    f32 giantRate = static_cast<f32>(pGiantDirector->getTimer()) /
                    static_cast<f32>(pGiantDirector->getChangeFrame());
    giantRate = giantRate > 1.0f ? 1.0f : giantRate;
    sead::Vector3f trans = sTransTable[chara][figure] * (1.0f - giantRate) +
                           sGiantTrans[chara] * giantRate;

    const sead::Matrix34f* pHeadMtx = rc::getPlayerModelJointMtxPtr(mHost, "Head");
    sead::Matrix34f jointRotMtx;
    jointRotMtx.makeR({0.0f, sead::Mathf::deg2rad(-90.0f), sead::Mathf::deg2rad(180.0f)});
    mFollowMtx.setMul(*pHeadMtx, jointRotMtx);

    sead::Matrix34f localMtx;
    localMtx.makeRT(sRotateTable[chara][figure], trans);
    mFollowMtx.setMul(mFollowMtx, localMtx);
}

/** @brief Gets blown off the host, then falls until it lands. */
void PlayerCrown::exeFall() {
    if (al::isFirstStep(this)) {
        show();
        al::showSilhouetteModelIfHide(this);

        if (al::isExistShadow(this)) {
            al::showShadow(this);
        }

        al::onCollide(this);
        startAction("Blow");
        al::validateHitSensor(this, "Normal");
        al::invalidateHitSensor(this, "WaitItem");

        if (mDropTimer < 0) {
            mDropTimer = 0;
            mFlashingCtrl->start(sKillFrame);
            sead::Vector3f front = rc::getPlayerFront(mHost);
            sead::Vector3f side;
            rc::calcPlayerSide(&side, mHost);
            sead::Vector3f headPos;
            rc::calcPlayerHeadColliderPos(&headPos, mHost);
            sead::Vector3f colliderPos;
            al::calcColliderPos(this, &colliderPos);
            sead::Vector3f dir = colliderPos - headPos;

            if (!al::normalizeOrZero(&dir)) {
                colliderPos += dir * al::getColliderRadius(this);
                sead::Vector3f hitPos;
                sead::Vector3f hitNormal;

                if (alCollisionUtil::getHitPosAndNormalOnArrow(this, &hitPos, &hitNormal, headPos,
                                                               colliderPos - headPos, nullptr,
                                                               nullptr)) {
                    sead::Vector3f* pTrans = al::getTransPtr(this);
                    pTrans->set(hitPos + hitNormal * al::getColliderRadius(this));
                }
            }

            f32 frontRate = sBlowFrontRate[mGoalType];
            f32 sideRate = sBlowSideRate[mGoalType];
            sead::Vector3f blowDir = -(front * frontRate) + side * sideRate + sead::Vector3f::ey;
            al::normalize(&blowDir);
            const sead::Vector3f& rPlayerVelocity = rc::getPlayerVelocity(mHost);
            sead::Vector3f velocity;

            if (mGoalType != 0) {
                al::startSe(this, "PgReleaseGoal", nullptr);
                velocity.set(0.0f, 0.0f, 0.0f);
            } else {
                velocity = rPlayerVelocity;
                velocity.y *= 0.0f;
                al::startSe(this, "PgRelease", nullptr);
            }

            sead::Vector3f* pVelocity = al::getVelocityPtr(this);
            *pVelocity = velocity + blowDir * sBlowSpeed[mGoalType];
            calcQuatFromFollowMtx();
        }
    }

    if (al::isCollidedGround(this)) {
        al::setNerve(this, &NrvPlayerCrownLand);
        return;
    }

    if (rc::isInAreaObj(this, rc::AreaObjType::SinkSandArea)) {
        al::setNerve(this, &NrvPlayerCrownSinkSand);
        return;
    }

    sead::Vector3f* pGravityVelocity = al::getVelocityPtr(this);
    *pGravityVelocity += {0.0f, -getFallGravity(), 0.0f};

    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    f32 fallSpeedMax = getFallSpeedMax();
    sead::Vector3f horizontal;
    al::verticalizeVec(&horizontal, sead::Vector3f::ey, *pVelocity);

    if (horizontal.length() > 20.0f) {
        *pVelocity -= horizontal;
        al::normalizeOrZero(&horizontal);
        horizontal *= 20.0f;
        *pVelocity = horizontal + *pVelocity;
    }

    sead::Vector3f vertical;
    al::parallelizeVec(&vertical, sead::Vector3f::ey, *pVelocity);

    if (vertical.dot(sead::Vector3f::ey) < 0.0f && vertical.length() > fallSpeedMax) {
        *pVelocity -= vertical;
        al::normalizeOrZero(&vertical);
        vertical *= fallSpeedMax;
        *pVelocity = vertical + *pVelocity;
    }
}

/**
 * @brief Starts an action, slowed down in water.
 * @param pActionName Action name.
 */
void PlayerCrown::startAction(const char* pActionName) {
    al::startAction(this, pActionName);

    if (mIsInWater) {
        al::setActionFrameRate(this, 0.3f);
    } else {
        al::setActionFrameRate(this, 1.0f);
    }
}

/** @brief Turns the crown so that its up axis matches the one it followed. */
void PlayerCrown::calcQuatFromFollowMtx() {
    sead::Vector3f up;
    mFollowMtx.getBase(up, 1);

    if (up.dot(sead::Vector3f::ey) < -0.999f) {
        al::setQuat(this, sead::Quatf::unit);
        return;
    }

    sead::Quatf rotate;
    rotate.makeVectorRotation(up, sead::Vector3f::ey);
    sead::Quatf* pQuat = al::getQuatPtr(this);
    pQuat->setMul(rotate, al::getQuat(this));

    if (al::getQuatPtr(this)->normalize() == 0.0f) {
        al::setQuat(this, sead::Quatf::unit);
    }
}

/**
 * @brief Gets the gravity while falling.
 * @return Gravity, weaker in water.
 */
f32 PlayerCrown::getFallGravity() const {
    return mIsInWater ? 0.5f : 0.8f;
}

/**
 * @brief Gets the maximum fall speed.
 * @return Maximum fall speed, lower in water.
 */
f32 PlayerCrown::getFallSpeedMax() const {
    return mIsInWater ? 8.0f : 30.0f;
}

/** @brief Lands on the ground; disappears on a game over. */
void PlayerCrown::exeLand() {
    if (al::isFirstStep(this)) {
        startAction("Land");
        al::getVelocityPtr(this)->set(0.0f, -getFallGravity(), 0.0f);
        al::validateHitSensor(this, "Normal");
        al::invalidateHitSensor(this, "WaitItem");

        if (al::isExistSceneObj(this, SceneObjID_PlayerAliveWatcher) &&
            al::getSceneObj<PlayerAliveWatcher>(this, SceneObjID_PlayerAliveWatcher)
                ->isGameOver()) {
            kill();
        }
    }

    if (!al::isCollidedGround(this)) {
        al::setNerve(this, &NrvPlayerCrownFall);
        return;
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvPlayerCrownWait);
    }
}

/** @brief Waits on the ground for a player to pick it up. */
void PlayerCrown::exeWait() {
    if (al::isFirstStep(this)) {
        startAction("WaitItem");
        al::validateHitSensor(this, "WaitItem");
        al::invalidateHitSensor(this, "Normal");
    }

    if (!al::isCollidedGround(this)) {
        al::setNerve(this, &NrvPlayerCrownFall);
    }
}

/** @brief Sinks into quicksand while inside a sink sand area. */
void PlayerCrown::exeSinkSand() {
    if (al::isFirstStep(this)) {
        al::setActionFrameRate(this, 0.0f);
        al::setVelocity(this, {0.0f, -1.0f, 0.0f});
    }

    if (!rc::isInAreaObj(this, rc::AreaObjType::SinkSandArea)) {
        al::setNerve(this, &NrvPlayerCrownFall);
    }
}

/**
 * @brief Checks if the crown is worn by its host.
 * @return True while attached.
 */
bool PlayerCrown::isAttach() const {
    return al::isNerve(this, &NrvPlayerCrownAttach);
}

/** @brief Updates water state, follows the host, or counts down until disappearing. */
void PlayerCrown::control() {
    if (mIsInWater) {
        if (!rc::isInWaterArea(this)) {
            al::setActionFrameRate(this, 1.0f);
            mIsInWater = false;
        }
    } else if (rc::isInWaterArea(this)) {
        al::setActionFrameRate(this, 0.3f);
        mIsInWater = true;
    }

    al::updateEffectMaterialWater(this, mIsInWater);
    al::updateSeMaterialWater(this, mIsInWater);

    if (!al::isNerve(this, &NrvPlayerCrownAttach)) {
        rc::startHitReactionIfThroughWater(this);

        if (rc::isInDeathArea(this) || rc::isCollidedDamageFire(this) ||
            rc::isCollidedPoison(this)) {
            kill();
            return;
        }
    }

    if (al::isNerve(this, &NrvPlayerCrownAttach)) {
        sead::Matrix34f poseMtx = mFollowMtx;
        al::normalize(&poseMtx);
        al::updatePoseMtx(this, &poseMtx);
        return;
    }

    if (mDropTimer >= 0) {
        mDropTimer++;

        if (mGoalType != 0) {
            if (mDropTimer >= sGoalKillFrame) {
                rc::addScore(this, al::getHitSensor(mHost, "Body"), 0.0f, 0);
                kill();
                return;
            }
        } else if (mDropTimer >= sKillFrame) {
            kill();
            return;
        }
    }

    mFlashingCtrl->movement();
}

/**
 * @brief Checks if the host plays an animation that hides the crown.
 * @return True while the host enters a pipe sideways.
 */
bool PlayerCrown::isModelHideAnim() const {
    const char* pAnimName = mHost->getPlayer()->getAnimator()->getAnimName();
    return al::isEqualString(pAnimName, "DokanSideIn") ||
           al::isEqualString(pAnimName, "PeachDokanSideIn") ||
           al::isEqualString(pAnimName, "RosettaDokanSideIn");
}

PlayerCrown::~PlayerCrown() = default;

/**
 * @brief Does nothing; the crown only receives messages.
 * @param pSelf Sensor of the crown.
 * @param pOther Sensor of the other actor.
 */
void PlayerCrown::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {}
