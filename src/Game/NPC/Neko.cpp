#include "NPC/Neko.hpp"

#include <cstdlib>
#include <prim/seadSafeString.h>

#include "MapObj/DisasterModeController.hpp"
#include "NPC/IUseNekoModeActor.hpp"
#include "NPC/NekoDisaster.hpp"
#include "NPC/NekoNormal.hpp"
#include "NPC/NekoParent.hpp"
#include "NPC/NpcStateFunction.hpp"
#include "NPC/NpcTargetFinder.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/InkUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/Collision/Collider.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
NERVE_DECL(Neko, Wait)
NERVE_DECL(Neko, Hide)
NERVE_DECL(Neko, Respawn)

NERVES_MAKE_NOSTRUCT(Neko, Hide, Respawn)

NekoNrvWait NrvNekoWait;

const NpcTargetFinderParam sTargetFinderParam(550.0f, 60.0f, 40.0f, 90, 600.0f, 150.0f, 100.0f,
                                              800.0f, false, 10);

/**
 * @brief Get the regular cat mode of a cat.
 * @param pNeko The cat.
 * @return The regular cat mode, or nullptr if the cat is not a regular cat.
 */
NekoNormal* tryGetNekoNormal(const Neko* pNeko) {
    if (pNeko->getNormalModeActor()->getNekoType() <= 4) {
        return static_cast<NekoNormal*>(pNeko->getNormalModeActor());
    }

    return nullptr;
}

/**
 * @brief Check whether an actor stands in a puddle.
 * @param pActor Actor to check.
 * @return Whether the actor is on ground with the puddle material.
 */
bool isOnPuddle(const al::LiveActor* pActor) {
    return al::isOnGround(pActor, 0, 0.0f) &&
           al::isMaterialCode("Puddle", al::getActorCollider(pActor)->mFloor.mTriangle);
}
}  // namespace

/**
 * @brief Construct a cat.
 * @param pName Name of the actor.
 * @param pRideNeko Cat riding this one, if any.
 */
Neko::Neko(const char* pName, Neko* pRideNeko) : al::LiveActor(pName), mRideNeko(pRideNeko) {}

/**
 * @brief Initialize the cat from its placement, using the placed model name to pick its kind.
 * @param rInfo Placement information of the cat.
 */
void Neko::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = nullptr;
    alPlacementFunction::getModelName(&modelName, rInfo);
    initAsModelName(rInfo, modelName);
    startAppearNormal();
}

/**
 * @brief Create and initialize the disaster mode actor.
 * @param rInfo Placement information of the cat.
 * @param colorType Coat color of the disaster mode actor.
 */
inline void Neko::initDisasterModeActor(const al::ActorInitInfo& rInfo, s32 colorType) {
    auto* disaster = new NekoDisaster(this);
    mDisasterModeActor = disaster;
    disaster->init(rInfo, static_cast<neko::ColorType>(colorType), mTargetFinder);
    al::trySetShadowLength(disaster, rInfo, nullptr);
}

/**
 * @brief Initialize the regular mode actor.
 * @param rInfo Placement information of the cat.
 * @param pModeActor The regular mode actor (NekoNormal or NekoParent).
 * @param colorType Coat color of the regular mode actor.
 */
inline void Neko::initNormalModeActor(const al::ActorInitInfo& rInfo,
                                      IUseNekoModeActor* pModeActor, s32 colorType) {
    mNormalModeActor = pModeActor;
    pModeActor->init(rInfo, static_cast<neko::ColorType>(colorType), mTargetFinder);
    al::trySetShadowLength(pModeActor, rInfo, nullptr);
}

/**
 * @brief Initialize the cat as the given model.
 * @param rInfo Placement information of the cat.
 * @param pModelName Model name, which selects the kind and coat color of the cat.
 */
void Neko::initAsModelName(const al::ActorInitInfo& rInfo, const char* pModelName) {
    al::initActorSceneInfo(this, rInfo);
    al::tryGetTrans(&mPlacementTrans, rInfo);

    s32 zoneNo = mPlacementHolder->getZoneNo();
    al::StringTmp<32> id("%s", mPlacementHolder->getId());
    mUID = atoi(id.getPart(3).cstr()) | (zoneNo << 16);

    al::initActorPoseTFSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorUpdate(this, rInfo, "コリジョン地形[Movement]");
    al::initNerve(this, &NrvNekoWait, 0);

    mTargetFinder = new NpcTargetFinder(nullptr, &sTargetFinderParam);
    mTargetFinder->setParam(&sTargetFinderParam);

    if (al::isEqualString(pModelName, "Neko") || al::isEqualString(pModelName, "NekoA")) {
        initDisasterModeActor(rInfo, 6);
        initNormalModeActor(rInfo, new NekoNormal(this), 0);
    } else if (al::isEqualString(pModelName, "NekoB")) {
        initDisasterModeActor(rInfo, 6);
        initNormalModeActor(rInfo, new NekoNormal(this), 1);
    } else if (al::isEqualString(pModelName, "NekoC")) {
        initDisasterModeActor(rInfo, 6);
        initNormalModeActor(rInfo, new NekoNormal(this), 2);
    } else if (al::isEqualString(pModelName, "NekoD")) {
        initDisasterModeActor(rInfo, 6);
        initNormalModeActor(rInfo, new NekoNormal(this), 3);
    } else if (al::isEqualString(pModelName, "NekoE")) {
        initDisasterModeActor(rInfo, 6);
        initNormalModeActor(rInfo, new NekoNormal(this), 4);
    } else if (al::isEqualString(pModelName, "NekoParent") ||
               al::isEqualString(pModelName, "NekoParentA") ||
               al::isEqualString(pModelName, "NekoParentB") ||
               al::isEqualString(pModelName, "NekoParentC") ||
               al::isEqualString(pModelName, "NekoParentD")) {
        initDisasterModeActor(rInfo, 7);
        initNormalModeActor(rInfo, new NekoParent(this), 5);
    } else {
        kill();
        return;
    }

    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        controller->registerStateListener(this);
    }

    al::setScaleAll(mDisasterModeActor, al::getScaleY(mNormalModeActor));
    neko::scaleHitSensors(mDisasterModeActor, al::getScaleY(mNormalModeActor));
}

/**
 * @brief Make the cat appear in its regular mode.
 */
void Neko::startAppearNormal() {
    if (mModeActor == mNormalModeActor) {
        return;
    }

    mFlags |= Flag_AttachedNormal;
    appear();
    mNormalModeActor->appear();
    mDisasterModeActor->startKill(false);
    mModeActor = mNormalModeActor;
    mTargetFinder->changeHost(mModeActor, "React", mModeActor);
    mModeActor->startAppearLinks();
}

/**
 * @brief Start the clipping of the cat unless it is already clipped, hidden or about to hide.
 */
void Neko::tryStartClipped() {
    if (al::isClipped(this) || isHidden() || mIsRequestHide) {
        return;
    }

    startClipped();
}

/**
 * @brief Check whether the cat is hidden (vanished or waiting to respawn).
 * @return Whether the cat is hidden.
 */
bool Neko::isHidden() const {
    return al::isNerve(this, &NrvNekoHide) || al::isNerve(this, &NrvNekoRespawn);
}

/**
 * @brief End the clipping of the cat if it is clipped.
 */
void Neko::tryEndClipped() {
    if (al::isClipped(this)) {
        endClipped();
    }
}

/**
 * @brief Called when the movement of the cat is paused or resumed.
 * @param isPaused Whether the movement is paused.
 */
void Neko::movementPaused(bool isPaused) {
    al::LiveActor::movementPaused(isPaused);
    if (mModeActor == nullptr) {
        return;
    }

    if (al::isDead(mNormalModeActor) && al::isDead(mDisasterModeActor)) {
        return;
    }

    if (!al::isNerve(this, &NrvNekoWait)) {
        return;
    }

    u32 flags = mFlags;
    mFlags &= ~(Flag_AttachedNormal | Flag_AttachedDisaster);
    if ((flags & Flag_Disaster) == 0 && mModeActor != nullptr &&
        mModeActor != mNormalModeActor && mModeActor == mDisasterModeActor) {
        attachNormal({NekoAttachReason::Type_Hide});
        return;
    }

    if (al::isClipped(mModeActor)) {
        return;
    }

    if (mModeActor != nullptr && mModeActor == mNormalModeActor) {
        if ((mFlags & Flag_DisasterAnticipation) != 0) {
            mFlags &= ~Flag_DisasterAnticipation;
        }

        if ((mFlags & Flag_Disaster) != 0) {
            attachDisaster({NekoAttachReason::Type_StartDisaster});
        }
    }
}

/**
 * @brief Check the current mode of the cat.
 * @param mode Mode to check.
 * @return Whether the cat is active in the given mode.
 */
bool Neko::isMode(Mode mode) const {
    if (mModeActor == nullptr) {
        return false;
    }

    return getMode() == mode;
}

/**
 * @brief Switch the cat to its regular mode.
 * @param rReason Why the regular mode gets attached.
 */
void Neko::attachNormal(const NekoAttachReason& rReason) {
    if (mModeActor == mNormalModeActor) {
        return;
    }

    mFlags |= Flag_AttachedNormal;
    mTargetFinder->setParam(&sTargetFinderParam);
    mTargetFinder->clearPriorityMap();
    mNormalModeActor->appear();
    if (mModeActor != nullptr) {
        al::copyPose(mNormalModeActor, mModeActor);
        al::resetPosition(mNormalModeActor, al::getTrans(mModeActor), false);
        mModeActor->startKill(false);
    } else {
        al::setVelocityZero(this);
        al::resetPosition(mNormalModeActor, al::getTrans(this), false);
    }

    mNormalModeActor->startAttach(rReason);
    mModeActor = mNormalModeActor;
    mTargetFinder->changeHost(mModeActor, "React", mModeActor);
}

/**
 * @brief Switch the cat to its disaster mode.
 * @param rReason Why the disaster mode gets attached.
 */
void Neko::attachDisaster(const NekoAttachReason& rReason) {
    if (mModeActor == mDisasterModeActor) {
        return;
    }

    mFlags |= Flag_AttachedDisaster;
    mTargetFinder->setParam(&sTargetFinderParam);
    mTargetFinder->clearPriorityMap();
    mDisasterModeActor->appear();
    if (mModeActor != nullptr) {
        al::copyPose(mDisasterModeActor, mModeActor);
        al::resetPosition(mDisasterModeActor, al::getTrans(mModeActor), false);
        mModeActor->startKill(false);
    } else {
        al::setVelocityZero(this);
        al::resetPosition(mDisasterModeActor, al::getTrans(this), false);
    }

    mDisasterModeActor->startAttach(rReason);
    mModeActor = mDisasterModeActor;
    mTargetFinder->changeHost(mModeActor, "React", mModeActor);
}

/**
 * @brief Nerve: the cat is active; switch modes and hide it when it gets stuck or lost.
 */
void Neko::exeWait() {
    if (mModeActor == nullptr) {
        return;
    }

    if (al::isDead(mNormalModeActor) && al::isDead(mDisasterModeActor)) {
        return;
    }

    u32 flags = mFlags;
    mFlags &= ~(Flag_AttachedNormal | Flag_AttachedDisaster);
    if ((flags & Flag_Disaster) == 0 && mModeActor != nullptr &&
        mModeActor != mNormalModeActor && mModeActor == mDisasterModeActor) {
        attachNormal({NekoAttachReason::Type_Hide});
        return;
    }

    if (mIsRequestHide && !isHidden() && al::isClipped(mModeActor) &&
        !al::isNearPlayer(mModeActor, 10000.0f)) {
        al::setNerve(this, &NrvNekoHide);
        return;
    }

    if (al::isClipped(mModeActor)) {
        return;
    }

    if (mModeActor != nullptr && mModeActor == mNormalModeActor) {
        if ((mFlags & Flag_DisasterAnticipation) != 0) {
            mFlags &= ~Flag_DisasterAnticipation;
        }

        if ((mFlags & Flag_Disaster) != 0) {
            attachDisaster({NekoAttachReason::Type_StartDisaster});
            return;
        }
    }

    if (!al::isNoCollide(mModeActor)) {
        if (rc::isInDeathArea(mModeActor) || rc::isCollidedDamageFire(mModeActor) ||
            rc::isCollidedPoison(mModeActor) || rc::isCollidedInkSlow(mModeActor) ||
            InkUtil::isInInkLimitSphere(mModeActor, al::getTrans(mModeActor), 10.0f) ||
            isOnPuddle(mModeActor) || rc::isCollidedNeedle(mModeActor)) {
            tryStartHide();
            return;
        }

        if (!mModeActor->isHold() && !mModeActor->isRide() &&
            (NpcStateFunction::isInNPCAvoidArea(this, al::getTrans(mModeActor)) ||
             rc::isInWaterAreaNoSink(this, al::getTrans(mModeActor)))) {
            if ((mHideCheckTime > 30 || !al::isOnGround(mModeActor, 3, 0.0f)) && tryStartHide()) {
                return;
            }

            mHideCheckTime++;
            return;
        }
    }

    mHideCheckTime = 0;
}

/**
 * @brief Make the cat vanish in a puff of smoke, to respawn at its placement later.
 * @return Whether the cat started hiding.
 */
bool Neko::tryStartHide() {
    if (mModeActor == nullptr || isHidden()) {
        return false;
    }

    sead::Vector3f trans = al::getTrans(mModeActor);
    al::AreaObj* waterArea = rc::tryFindAreaObj(mModeActor, rc::AreaObjType::WaterArea, trans);
    if (waterArea != nullptr) {
        al::AreaShape* shape = waterArea->getAreaShape();
        sead::Vector3f top = sead::Vector3f::ey * 400.0f + trans;
        shape->checkArrowCollision(&trans, nullptr, top, trans);
    }

    al::tryEmitEffect(mModeActor, "Poof", &trans);
    al::startSe(mModeActor, "PgVanish");
    if (al::isClipped(this)) {
        endClipped();
    }

    al::setNerve(this, &NrvNekoHide);
    return true;
}

/**
 * @brief Nerve: the cat is hidden; respawn it after a while unless a disaster is going on.
 */
void Neko::exeHide() {
    if (al::isFirstStep(this)) {
        if (mModeActor != nullptr) {
            mModeActor->onStartHide();
            mModeActor->startKill(false);
            mModeActor = nullptr;
        }

        mIsRequestHide = false;
    }

    if (al::isGreaterStep(this, 60) && (mFlags & Flag_Disaster) == 0) {
        al::setNerve(this, &NrvNekoRespawn);
    }
}

/**
 * @brief Nerve: the cat reappears at its host position in the mode matching the disaster state.
 */
void Neko::exeRespawn() {
    if (al::isGreaterStep(this, 1)) {
        if ((mFlags & Flag_Disaster) != 0) {
            attachDisaster({NekoAttachReason::Type_AppearAtHost});
        } else {
            attachNormal({NekoAttachReason::Type_AppearAtHost});
        }

        al::setNerve(this, &NrvNekoWait);
    }
}

/**
 * @brief Check whether the cat is active and visible.
 * @return Whether the cat is active and not clipped.
 */
bool Neko::isActiveInView() const {
    return mModeActor != nullptr && al::isNerve(this, &NrvNekoWait) && !al::isDead(mModeActor) &&
           !al::isClipped(mModeActor);
}

/**
 * @brief Get the current mode of the cat.
 * @return The current mode.
 */
Neko::Mode Neko::getMode() const {
    if (mModeActor == mNormalModeActor) {
        return Mode_Normal;
    }

    return mModeActor == mDisasterModeActor ? Mode_Disaster : Mode_None;
}

/**
 * @brief Find the player nearest to the cat.
 * @param range Search range.
 * @return The nearest player within range, or nullptr.
 */
al::LiveActor* Neko::tryGetNearestPlayerInRange(f32 range) {
    s32 playerId = al::findNearestPlayerId(mModeActor, range);
    if (playerId < 0) {
        return nullptr;
    }

    return al::tryGetPlayerActor(this, playerId);
}

/**
 * @brief Kill the cat and both of its mode actors.
 */
void Neko::startKill() {
    mFlags &= ~(Flag_AttachedNormal | Flag_AttachedDisaster);
    mNormalModeActor->startKill(false);
    mDisasterModeActor->startKill(false);
    kill();
    mModeActor = nullptr;
}

/**
 * @brief Make the cat walk to a target.
 * @param pTarget Target to walk to.
 * @param isForce Whether to force the seek.
 */
void Neko::startSeekTarget(const neko::Target* pTarget, bool isForce) {
    mModeActor->startSeekTarget(pTarget, isForce);
}

/**
 * @brief Move the cat and its active mode actor.
 * @param rTrans New position.
 */
void Neko::setActivePosition(const sead::Vector3f& rTrans) {
    al::resetPosition(this, rTrans, false);
    if (mModeActor != nullptr) {
        al::resetPosition(mModeActor, rTrans, false);
    }
}

/**
 * @brief Turn the cat and its active mode actor.
 * @param rDir Direction to face.
 */
void Neko::setActiveFace(const sead::Vector3f& rDir) {
    al::faceToDirection(this, rDir);
    if (mModeActor != nullptr) {
        al::faceToDirection(mModeActor, rDir);
    }
}

/**
 * @brief React to a change of the disaster state (Fury Bowser showing up or leaving).
 * @param state New disaster state.
 */
void Neko::onDisasterModeStateChange(DisasterModeController::State state) {
    switch (static_cast<s32>(state)) {
    case 1:
        mFlags &= ~(Flag_Disaster | Flag_DisasterAnticipation);
        if (al::isClipped(this)) {
            endClipped();
        }

        return;
    case 5:
        mFlags &= ~Flag_DisasterAnticipation;
        if (mModeActor != nullptr) {
            mModeActor->startDisasterDemo();
        }

        return;
    case 6:
    case 9:
        mFlags = (mFlags & ~Flag_Disaster) | Flag_DisasterAnticipation;
        if (mModeActor != nullptr && mModeActor == mNormalModeActor) {
            mModeActor->startDisasterAnticipation(static_cast<s32>(state) == 9);
        }

        return;
    case 10:
        mFlags = (mFlags & ~Flag_DisasterAnticipation) | Flag_Disaster;
        if (tryGetNekoNormal(this) != nullptr && tryGetNekoNormal(this)->isRide()) {
            if (al::isClipped(this)) {
                endClipped();
            }

            if (!al::isNerve(this, &NrvNekoHide)) {
                al::setNerve(this, &NrvNekoHide);
            }

            return;
        }

        if (isActiveInView()) {
            sead::Vector3f trans = al::getTrans(mModeActor);
            al::tryEmitEffect(mDisasterModeActor, "DisasterStart", &trans);
            al::tryStartSe(mDisasterModeActor, "PgDisasterStart");
        }

        return;
    default:
        return;
    }
}
