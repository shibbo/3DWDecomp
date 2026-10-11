#include "MapObj/Fury/GigaBell.hpp"

#include <cmath>
#include <math/seadMathCalcCommon.h>
#include <nn/oe.h>

#include "Demo/DemoAnimatic.hpp"
#include "Demo/DemoCutscene.hpp"
#include "Enemy/SuperBowser.hpp"
#include "Enemy/SuperBowserLaserState.hpp"
#include "Layout/CounterLockGigaBell.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Library/Camera/CameraPoseInfo.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/HitReactionKeeper.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/HitSensor/HitSensorKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Play/Camera/CameraPoserFixActor.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/Fury/DisasterSpikeDirector.hpp"
#include "MapObj/Fury/GigaBellBindPuppeteer.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "MapObj/Fury/GigaBellPedestal.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "MapObj/ItemStatePopUpAbove.hpp"
#include "MapObj/TimerManager.hpp"
#include "Camera/DummyCameraTarget.hpp"
#include "Player/IUsePlayerPuppet.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(GigaBell, Wait)
NERVE_DECL(GigaBell, PopUpAbove)
NERVE_DECL(GigaBell, Unlocked)
NERVE_DECL(GigaBell, Locked)
NERVE_DECL(GigaBell, ReappearInBattle)
NERVE_DECL(GigaBell, UnlockCutscene)
NERVE_DECL(GigaBell, LockedSufficientGoalItems)
NERVE_DECL(GigaBell, TransitionWakeUp)
NERVE_DECL(GigaBell, TransitionSleep)
NERVE_DECL(GigaBell, CollectingBossBattle)
NERVE_DECL(GigaBell, PlessieChase)
NERVE_DECL(GigaBell, CollectDemo)
NERVE_DECL(GigaBell, ExplainCutscene)
NERVE_DECL(GigaBell, PreExplainCutscene)
NERVE_DECL(GigaBell, Collecting)
NERVE_DECL(GigaBell, DebugCollectWait)
NERVE_DECL(GigaBell, WaitReviveInBattle)
NERVE_DECL(GigaBell, BattleDefeatDemo)
NERVE_DECL(GigaBell, WaitInBattle)
NERVE_DECL(GigaBell, PreUnlockCutscene)
NERVE_DECL(GigaBell, PreResetCutscene)
NERVE_DECL(GigaBell, ResetCutscene)
NERVE_DECL(GigaBell, ResetCutsceneEnd)
NERVE_DECL(GigaBell, LockedDisasterMode)
NERVE_DECL(GigaBell, LockedPlayerGiga)
NERVE_DECL(GigaBell, PlessieChaseHit)
NERVE_DECL(GigaBell, CollectingCameraPending)

/// Unlock cutscene started by the bell itself (startUnlockCutscene()); it shares exeUnlockCutscene().
class GigaBellNrvUnlockCutsceneSelf : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GigaBell>()->exeUnlockCutscene();
    }
};

/// Preparation of the unlock cutscene requested through trySetNerve(); shares exePreUnlockCutscene().
class GigaBellNrvPreUnlockCutsceneRequested : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<GigaBell>()->exePreUnlockCutscene();
    }
};

// Most nerves live in .data; a few are constant objects placed next to their vtables.
GigaBellNrvWait NrvGigaBellWait;
GigaBellNrvPopUpAbove NrvGigaBellPopUpAbove;
GigaBellNrvUnlocked NrvGigaBellUnlocked;
GigaBellNrvLocked NrvGigaBellLocked;
const GigaBellNrvReappearInBattle NrvGigaBellReappearInBattle{};
GigaBellNrvUnlockCutscene NrvGigaBellUnlockCutscene;
GigaBellNrvUnlockCutsceneSelf NrvGigaBellUnlockCutsceneSelf;
GigaBellNrvLockedSufficientGoalItems NrvGigaBellLockedSufficientGoalItems;
GigaBellNrvTransitionWakeUp NrvGigaBellTransitionWakeUp;
GigaBellNrvTransitionSleep NrvGigaBellTransitionSleep;
GigaBellNrvCollectingBossBattle NrvGigaBellCollectingBossBattle;
GigaBellNrvPlessieChase NrvGigaBellPlessieChase;
GigaBellNrvCollectDemo NrvGigaBellCollectDemo;
const GigaBellNrvExplainCutscene NrvGigaBellExplainCutscene{};
const GigaBellNrvPreExplainCutscene NrvGigaBellPreExplainCutscene{};
const GigaBellNrvCollecting NrvGigaBellCollecting{};
const GigaBellNrvDebugCollectWait NrvGigaBellDebugCollectWait{};
const GigaBellNrvWaitReviveInBattle NrvGigaBellWaitReviveInBattle{};
GigaBellNrvBattleDefeatDemo NrvGigaBellBattleDefeatDemo;
GigaBellNrvWaitInBattle NrvGigaBellWaitInBattle;
GigaBellNrvPreUnlockCutscene NrvGigaBellPreUnlockCutscene;
GigaBellNrvPreResetCutscene NrvGigaBellPreResetCutscene;
const GigaBellNrvResetCutscene NrvGigaBellResetCutscene{};
const GigaBellNrvResetCutsceneEnd NrvGigaBellResetCutsceneEnd{};
GigaBellNrvPreUnlockCutsceneRequested NrvGigaBellPreUnlockCutsceneRequested;
GigaBellNrvLockedDisasterMode NrvGigaBellLockedDisasterMode;
GigaBellNrvLockedPlayerGiga NrvGigaBellLockedPlayerGiga;
const GigaBellNrvPlessieChaseHit NrvGigaBellPlessieChaseHit{};
const GigaBellNrvCollectingCameraPending NrvGigaBellCollectingCameraPending{};

/// Potted Piranha Plant held by the player: only its pot is needed here.
class PackunFlowerWithPot : public al::LiveActor {
public:
    /** @brief The pot actor. @return The pot, or nullptr. */
    al::LiveActor* getPot() const { return mPot; }

private:
    al::LiveActor* mPot;
};

/**
 * @brief Make an actor's collision follow a matrix and update it right away.
 * @param pActor The collision actor.
 * @param pMtx The matrix to follow.
 */
inline void syncCollisionToMtx(al::LiveActor* pActor, const sead::Matrix34f* pMtx) {
    pActor->getCollisionParts()->setSyncCollisionMtx(pMtx);
    pActor->getCollisionParts()->syncMtx();
    pActor->getCollisionParts()->updateMtx();
}

/**
 * @brief Kill the host of a sensor along with its effects.
 * @param pSensor The sensor.
 */
inline void killSensorHost(al::HitSensor* pSensor) {
    al::getSensorHost(pSensor)->kill();
    al::tryKillEmitterAndParticleAll(al::getSensorHost(pSensor));
}

/**
 * @brief Convert a placement layer id to the phase of the bell's explanation cutscenes.
 * @param layerId Placement layer id.
 * @return The phase, or -1 when the layer has none.
 */
inline s32 convertLayerToPhase(s32 layerId) {
    if (layerId == 13 || layerId == 14) {
        return 1;
    }

    if (layerId == 16 || layerId == 17) {
        return 3;
    }

    if (layerId == 19 || layerId == 20) {
        return 5;
    }

    if (layerId >= 22 && layerId <= 24) {
        return 8;
    }

    return -1;
}

/**
 * @brief Access the fixed-actor poser of a camera ticket.
 * @param pTicket The camera ticket.
 * @return The poser.
 */
inline al::CameraPoserFixActor* getFixActorPoser(al::CameraTicket* pTicket) {
    return pTicket->getPoser<al::CameraPoserFixActor>();
}
}  // namespace

/**
 * @brief Construct a Giga Bell.
 * @param pName Name of the actor.
 */
GigaBell::GigaBell(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the bell, its cutscenes, camera, collisions and lock counter.
 * @param rInfo Actor init info.
 */
void GigaBell::init(const al::ActorInitInfo& rInfo) {
    al::initNerve(this, &NrvGigaBellWait, 2);
    al::initActorWithArchiveName(this, rInfo, "GigaBell", nullptr);
    al::invalidateHitSensor(this, "Cutscene");
    mStatePopUpAbove = new ItemStatePopUpAbove(this);
    mStatePopUpAbove->setSpeedScale(6.0f);
    al::initNerveState(this, mStatePopUpAbove, &NrvGigaBellPopUpAbove, "PopUpAbove");

    if (al::calcLinkChildNum(rInfo, "ExplainActivateArea") != 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "ExplainActivateArea",
                                0);
        al::AreaInitInfo areaInitInfo(placementInfo, rInfo.getStageSwitchDirector());
        mExplainArea = new al::AreaObj("ExplainActivateArea");
        mExplainArea->init(areaInitInfo);
    }

    al::tryGetArg(&mIslandId, rInfo, "AttachedIslandID");

    if (mIslandId >= 0) {
        IslandKeeper* keeper = IslandKeeper::tryGetIslandKeeper(this);

        if (keeper != nullptr) {
            keeper->addActorLinkToIsland(this, mIslandId);
        }
    }

    bool isDemoEndScene = false;

    if (al::calcLinkChildNum(rInfo, "SwitchCutscene") >= 1) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo, "SwitchCutscene", 0);
        al::ActorInitInfo childInfo;
        childInfo.initNoViewId(&placementInfo, rInfo);
        mGetDemo = new DemoAnimatic("GigaGetDemo", static_cast<alSeFunction::DemoType>(3));
        mGetDemo->init(childInfo);
        al::tryGetArg(&isDemoEndScene, rInfo, "IsDemoEndScene");

        if (isDemoEndScene) {
            mGetDemo->setEndSceneFlag();
        }
    }

    if (al::calcLinkChildNum(rInfo, "ConversationCutscenePos") >= 1) {
        mConversationMtx = new sead::Matrix34f;
        al::getLinksMatrix(mConversationMtx, rInfo, "ConversationCutscenePos");
    }

    al::calcLinkChildNum(rInfo, "CameraArea");

    if (al::calcLinkChildNum(rInfo, "CameraArea") != 0) {
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo.getPlacementInfo(), "CameraArea", 0);
        al::AreaInitInfo areaInitInfo(placementInfo, rInfo.getStageSwitchDirector());
        mCameraArea = new al::AreaObj("CameraArea");
        mCameraArea->init(areaInitInfo);
        mCameraArea->invalidate();
        al::AreaObjGroup* group = al::tryFindAreaObjGroup(this, "CameraArea");

        if (group != nullptr) {
            group->resisterAreaObj(mCameraArea);
        }
    }

    mGetCutscene = new DemoCutscene("DemoGigaBellGet", static_cast<alSeFunction::DemoType>(1));
    mGetCutscene->setDemoName("DemoGigaBellGet");
    mGetCutscene->init(rInfo);
    mGetCutscene->setEndSceneFlag();
    mCameraTarget = new DummyCameraTarget("DummyCameraTarget");
    mCameraTarget->init(rInfo);
    al::setTrans(mCameraTarget, al::getTrans(this));
    mCameraTarget->makeActorDead();
    mHasRestartPos = al::tryGetLinksTrans(&mRestartPos, rInfo, "PlayerRestartPos");

    if (mHasRestartPos) {
        sead::Quatf quat = sead::Quatf::unit;
        al::tryGetLinksQuat(&quat, rInfo, "PlayerRestartPos");
        al::calcQuatFront(&mRestartFront, quat);
    }

    mHasPlessieChasePos = al::tryGetLinksTrans(&mPlessieChasePos, rInfo, "PlayerPlessieChasePos");

    if (mHasPlessieChasePos) {
        sead::Quatf quat = sead::Quatf::unit;
        al::tryGetLinksQuat(&quat, rInfo, "PlayerPlessieChasePos");
        al::calcQuatFront(&mPlessieChaseFront, quat);
    }

    al::setClippingInfo(this, 3000.0f, nullptr);
    mCounterLock = new CounterLockGigaBell("CounterLockGigaBell", al::getLayoutInitInfo(rInfo),
                                           al::getTransPtr(this), getSceneInfo()->demoDirector);
    makeActorAppeared();
    al::offCollide(this);
    mBaseQuat = al::getQuat(this);
    mBasePos = al::getTrans(this);
    mBaseScale = al::getScaleX(this);
    mIsSufficientGoalItems = false;

    if (mManager != nullptr) {
        mCameraTicket = al::initObjectCamera_RS(this, rInfo, nullptr);
        mCameraTicket->setPriority(al::CameraTicket::Priority_Demo2);
        mIsSufficientGoalItems =
            SingleModeDataFunction::getGoalItemsCollected(GameDataHolderWriter(this)) >=
            mManager->getGoalItemsRequired();
    }

    al::startAction(this, "WaitFreeze");

    if (mIsSufficientGoalItems) {
        al::tryStartMclAnimIfNotPlaying(this, "OnWeak");
    } else {
        al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
        setInk(true);
    }

    mLayerId = al::tryGetLayerID(rInfo);
    mLayerId = convertLayerToPhase(mLayerId);
    mBindPuppeteer = new GigaBellBindPuppeteer("GigaBellBindPuppeteer");
    mBindPuppeteer->setActionName("Wait");

    for (s32 i = 0; i < getHitSensorKeeper()->getSensorNum(); i++) {
        if (al::isSensorBindableAll(al::getHitSensor(this, i))) {
            break;
        }
    }

    mWipe = new al::WipeSimple("GigaBellWipe", "WipeFadeWhite", *rInfo.getLayoutInitInfo(),
                               nullptr);
    mDemoSkipLayout = new DemoSkipLayout(*rInfo.getLayoutInitInfo(), true);
    mDemoSkipLayout->kill();
    al::initSubActorKeeperNoFile(this, rInfo, 3);
    mCollisionNormal = al::createCollisionObj(this, rInfo, "Normal",
                                              al::getHitSensor(this, "Collision"), nullptr, nullptr);
    mCollisionDamage = al::createCollisionObj(this, rInfo, "Damage",
                                              al::getHitSensor(this, "Collision"), nullptr, nullptr);
    mCollisionHalf = al::createCollisionObj(this, rInfo, "Half",
                                            al::getHitSensor(this, "Collision"), nullptr, nullptr);
    mCollisionNormal->makeActorAppeared();
    mCollisionDamage->makeActorAppeared();
    mCollisionHalf->makeActorAppeared();
    al::registerSubActorSyncClipping(this, mCollisionNormal, false);
    al::registerSubActorSyncClipping(this, mCollisionDamage, false);
    al::registerSubActorSyncClipping(this, mCollisionHalf, false);
    al::setSubActorOnSyncAppear(this);
    al::validateCollisionParts(mCollisionNormal);
    al::invalidateCollisionParts(this);
    al::invalidateCollisionParts(mCollisionDamage);
    al::invalidateCollisionParts(mCollisionHalf);
    updateCollisionMtx();
    syncCollisionToMtx(mCollisionNormal, &mCollisionMtx);
    syncCollisionToMtx(mCollisionDamage, &mCollisionMtx);
    syncCollisionToMtx(mCollisionHalf, &mCollisionMtx);
}

/**
 * @brief Set the position the bell rests at.
 * @param basePos The position.
 */
void GigaBell::setBasePosition(sead::Vector3f basePos) {
    mBasePos = basePos;
}

/**
 * @brief Show or hide the ink covering the bell, keeping the current color animation.
 * @param isInk Whether the bell is covered in ink.
 */
void GigaBell::setInk(bool isInk) {
    bool isPlaying = al::isMtpAnimPlaying(this, "GigaBell");
    f32 frame = al::getMtpAnimFrame(this);
    al::tryStartMtpAnimIfNotPlaying(this, "GigaBellInk");
    al::setMtpAnimFrameAndStop(this, isInk);

    if (isPlaying) {
        al::startMtpAnim(this, "GigaBell");
        al::setMtpAnimFrame(this, frame);
    }
}

/**
 * @brief Rebuild the matrix the collision actors follow from the bell's pose and scale.
 */
void GigaBell::updateCollisionMtx() {
    mCollisionMtx.makeQT(al::getQuat(this), sead::Vector3f(0.0f, 0.0f, 0.0f));
    mCollisionMtx.setTranslation(al::getTrans(this));
    sead::Vector3f scale = al::getScaleX(this) * sead::Vector3f::ones;
    mCollisionMtx.scaleBases(scale.x, scale.y, scale.z);
}

/**
 * @brief Appear the bell in its wait state.
 */
void GigaBell::appear() {
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    al::setNerve(this, &NrvGigaBellWait);
    mCollisionNormal->appear();
    mCollisionDamage->appear();
    mCollisionHalf->appear();
    al::validateCollisionParts(mCollisionNormal);
    al::invalidateCollisionParts(this);
    al::invalidateCollisionParts(mCollisionDamage);
    al::invalidateCollisionParts(mCollisionHalf);
}

/**
 * @brief Put an unlocked bell back to sleep.
 */
void GigaBell::startReset() {
    if (al::isNerve(this, &NrvGigaBellUnlocked)) {
        al::tryKillEmitterAndParticleAll(this);
        al::tryStartAction(this, "Sleep");
    }
}

/**
 * @brief Remember that a demo started while the bell is visible.
 * @param demoId Id of the demo.
 */
void GigaBell::startDemoActor(s32 demoId) {
    if (al::isAlive(this) && (!al::isClipped(this) || !al::isInvalidClipping(this))) {
        mIsInDemo = true;
    }
}

/**
 * @brief Restore the locked effect once a demo ends.
 * @param demoId Id of the demo.
 */
void GigaBell::endDemoActor(s32 demoId) {
    if (al::isAlive(this) && (!al::isClipped(this) || !al::isInvalidClipping(this))) {
        mIsInDemo = false;

        if (al::isNerve(this, &NrvGigaBellLocked)) {
            al::emitEffect(this, "WaitFreeze", nullptr);
        }
    }
}

/**
 * @brief Appear the bell popping up in front of its spawner.
 */
void GigaBell::appearPopUpFront() {
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    al::setNerve(this, &NrvGigaBellPopUpAbove);
    al::setSensorRadius(this, "Body", 900.0f);
    al::showModelIfHide(this);
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
}

/**
 * @brief Appear the bell popping up above its spawner.
 */
void GigaBell::appearPopUpAbove() {
    al::LiveActor::appear();
    al::startSe(this, "PgAppear", nullptr);
    al::setNerve(this, &NrvGigaBellPopUpAbove);
    al::setSensorRadius(this, "Body", 900.0f);
    al::showModelIfHide(this);
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
}

/**
 * @brief Appear the bell already unlocked, without a sound.
 */
void GigaBell::appearPopUpAboveSilent() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvGigaBellUnlocked);
    al::setSensorRadius(this, "Body", 900.0f);
    al::showModelIfHide(this);
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
}

/**
 * @brief Make the bell reappear during the Fury Bowser battle.
 */
void GigaBell::reappearInBattle() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvGigaBellReappearInBattle);
    al::setSensorRadius(this, "Body", 900.0f);
    al::showModelIfHide(this);
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
}

/**
 * @brief Kill the bell, its effects and its collision actors.
 */
void GigaBell::kill() {
    getEffectKeeper()->tryKillEmitterAndParticleAll();
    mCollisionNormal->kill();
    mCollisionDamage->kill();
    mCollisionHalf->kill();
    al::LiveActor::kill();
}

/**
 * @brief Push away actors touching the bell and reflect shells off it.
 * @param pSelf Sensor of the bell.
 * @param pOther Sensor touched.
 */
void GigaBell::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorName(pSelf, "Push")) {
        bool isPushAll = al::isNerve(this, &NrvGigaBellUnlockCutscene) ||
                         al::isNerve(this, &NrvGigaBellUnlockCutsceneSelf);
        DisasterModeController* controller = DisasterModeController::tryGetController(this);

        if (controller != nullptr) {
            isPushAll |= al::isNerve(this, &NrvGigaBellLockedSufficientGoalItems) &&
                         controller->isDisasterMode();
        }

        if (mManager != nullptr) {
            isPushAll &= !mManager->shouldPlayUnlockCutscene();
        }

        if (al::isNerve(this, &NrvGigaBellTransitionWakeUp) || isPushAll ||
            al::isNerve(this, &NrvGigaBellTransitionSleep) ||
            (!al::isNerve(this, &NrvGigaBellUnlocked) && al::isSensorPlayer(pOther) &&
             (al::getTrans(this) - al::getSensorPos(pOther)).squaredLength() < 202500.0f)) {
            if (al::isSensorPlayer(pOther) || al::isSensorRide(pOther) ||
                al::isSensorNpc(pOther) || al::isSensorEnemyBody(pOther) ||
                al::isSensorKoopaJr(pOther)) {
                al::sendMsgGigaBellPush(pOther, pSelf);
            }
        } else if (al::isNerve(this, &NrvGigaBellUnlocked) && al::isSensorKoopaJr(pOther)) {
            al::sendMsgPushStrong(pOther, pSelf);
        }
    } else if (al::isSensorName(pSelf, "Cutscene")) {
        al::sendMsgCutsceneStart(pOther, pSelf);
    }

    if (((isUnlocked() || al::isNerve(this, &NrvGigaBellLockedSufficientGoalItems)) &&
         al::isSensorName(pSelf, "Unlocked1")) ||
        (isUnlocked() && al::isSensorName(pSelf, "Unlocked2"))) {
        if (al::isSensorKickKoura(pOther)) {
            al::sendMsgKickKouraReflect(pOther, pSelf);
            return;
        }

        if (!al::isSensorPlayer(pOther) && !al::isSensorRide(pOther)) {
            al::sendMsgPush(pOther, pSelf);
        }
    }
}

/**
 * @brief Check whether the bell can be rung.
 * @return True without a manager, or while unlocked or in the Plessie chase.
 */
bool GigaBell::isUnlocked() {
    if (mManager == nullptr || al::isNerve(this, &NrvGigaBellUnlocked)) {
        return true;
    }

    return al::isNerve(this, &NrvGigaBellPlessieChase);
}

/**
 * @brief Handle a sensor message: cutscene binds, ball hits and the player ringing the bell.
 * @param pMsg Received message.
 * @param pOther Sensor that sent the message.
 * @param pSelf Sensor of the bell.
 * @return Whether the message was handled.
 */
bool GigaBell::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                          al::HitSensor* pSelf) {
    if (al::isSensorName(pSelf, "BindCutscene")) {
        if (al::isMsgBindStart(pMsg)) {
            return true;
        }

        if (al::isMsgBindInit(pMsg)) {
            if (mPuppet == nullptr) {
                mPuppet = rc::startPuppet(pSelf, pOther);
            }

            return true;
        }
    }

    if (isUnlocked() &&
        (al::isSensorName(pSelf, "Unlocked1") || al::isSensorName(pSelf, "Unlocked2")) &&
        al::isMsgBallAttack(pMsg)) {
        if (mBallHitCooldown <= 0) {
            al::startHitReactionHitEffect(this, "BallHit", pOther, pSelf);
            mBallHitCooldown = 30;
        }

        return true;
    }

    if (!al::isSensorPlayer(pOther) && !al::isSensorRide(pOther)) {
        return false;
    }

    if (!al::isSensorName(pOther, "Body")) {
        return false;
    }

    if (mManager == nullptr) {
        if (al::isMsgBindStart(pMsg) && !al::isNerve(this, &NrvGigaBellCollectingBossBattle)) {
            collectGigaBellBossFight(pOther, pSelf);
        }

        return false;
    }

    DisasterModeController* controller = DisasterModeController::tryGetController(this);

    if (!isUnlocked()) {
        return false;
    }

    if (rc::isAnyActiveDemo(this) || controller == nullptr) {
        return false;
    }

    if (!controller->isDisasterMode()) {
        return false;
    }

    if (controller->getSuperBowser()->isLastPhase3Bowser()) {
        if (al::isNerve(this, &NrvGigaBellPlessieChase) && al::isMsgItemGetAll(pMsg)) {
            plessieChaseHit(pOther, pSelf);
        }

        return false;
    }

    if (controller->getSuperBowser()->isHidden()) {
        return false;
    }

    if (al::isMsgBindStart(pMsg)) {
        if (mGetCutscene == nullptr) {
            return true;
        }

        mCollectSensor = pOther;
        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvGigaBellCollectDemo);
        rc::acquirerItemGigaBell(this, pOther);
        return false;
    }

    if (al::isMsgBindInit(pMsg)) {
        collectGigaBell(pOther, pSelf);
        return true;
    }

    return false;
}

/**
 * @brief Check whether the bell can be rung by an attack.
 * @return Always false.
 */
bool GigaBell::canRing() {
    return false;
}

/**
 * @brief Check whether a message is an attack of the player.
 * @param pMsg The message.
 * @return Whether the message is a player attack.
 */
bool GigaBell::isMsgPlayerAttack(const al::SensorMsg* pMsg) {
    return al::isMsgPlayerKick(pMsg) || al::isMsgPlayerObjRollingAttack(pMsg) ||
           al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangBreak(pMsg) ||
           al::isMsgPlayerBoomerangReflect(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
           al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
           al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerClimbSlidingAttack(pMsg) ||
           al::isMsgPlayerBodyAttack(pMsg);
}

/**
 * @brief Let the player collect the bell during the Fury Bowser battle.
 * @param pOther Sensor of the player.
 * @param pSelf Sensor of the bell.
 */
void GigaBell::collectGigaBellBossFight(al::HitSensor* pOther, al::HitSensor* pSelf) {
    mPlayer = static_cast<PlayerActor*>(al::getSensorHost(pOther));

    if (mPlayer != nullptr) {
        mPlayer->cancelBind();
    }

    rc::tryChangeToGigaClimbMario(this, al::getSensorHost(pOther), false);
    al::setNerve(this, &NrvGigaBellCollectingBossBattle);
}

/**
 * @brief Handle Plessie hitting the bell during the final chase.
 * @param pOther Sensor of the attacker.
 * @param pSelf Sensor of the bell.
 */
void GigaBell::plessieChaseHit(al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!mManager->canHitBellPlessieChase()) {
        return;
    }

    if (mIsHitPlessieChase) {
        mManager->tryHitNextBellPlessieChase(pOther, pSelf);
        return;
    }

    mIsHitPlessieChase = true;

    if (pSelf != nullptr) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        al::sendMsgPushStrong(al::getHitSensor(raidon, "Body"), pSelf);
    }

    doPlessieChaseHitEffect();
    mManager->setLastHitBell(this);
    mManager->hitBellPlessieChase();
    al::setNerve(this, &NrvGigaBellPlessieChaseHit);
}

/**
 * @brief Let the player collect the bell: bind the player and start the collect sequence.
 * @param pOther Sensor of the player.
 * @param pSelf Sensor of the bell.
 * @return Always true.
 */
bool GigaBell::collectGigaBell(al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isReallyPlayerActor(pOther)) {
        mPlayer = static_cast<PlayerActor*>(al::getSensorHost(pOther));

        if (mPlayer != nullptr) {
            mPlayer->cancelBind();
        }
    }

    if (mManager != nullptr) {
        mBindPuppeteer->startBind(pOther, pSelf);
        al::sendMsgHoldCancel(pOther, pSelf);
        rc::forceEndSubActionPuppet(mBindPuppeteer->getPlayerPuppet());
    }

    if (mManager == nullptr) {
        rc::tryChangeToGigaClimbMario(this, al::getSensorHost(pOther), false);
    } else if (mManager->getCollectDemo() == nullptr) {
        al::hideModelIfShow(this);
        rc::acquirerItemGigaBell(this, pOther);
    }

    al::startHitReactionGet(this);
    al::invalidateHitSensors(this);
    al::offCollide(this);
    al::invalidateCollisionParts(this);
    al::invalidateCollisionParts(mCollisionNormal);
    al::invalidateCollisionParts(mCollisionDamage);
    al::invalidateCollisionParts(mCollisionHalf);
    al::requestCaptureScreenCover(this, 2);
    al::setNerve(this, &NrvGigaBellCollectingCameraPending);
    al::tryStopSe(this, "Unlocked");

    if (mManager != nullptr) {
        mManager->gigaBellCollected(this);
    }

    if (mHasRestartPos) {
        SingleModeDataFunction::setGigaBellPlayerRespawnPoint(GameDataHolderWriter(this),
                                                              mRestartPos, mRestartFront);
    }

    return true;
}

/**
 * @brief Make the bell dead and hide its lock counter.
 */
void GigaBell::makeActorDead() {
    al::LiveActor::makeActorDead();

    if (mCounterLock != nullptr) {
        mCounterLock->endShow();
    }
}

/**
 * @brief Update the collision, the bind and the lock counter shown to the player.
 */
void GigaBell::control() {
    rc::updateMaterialCodeWater(this);
    updateCollisionMtx();

    if (mBindPuppeteer != nullptr) {
        mBindPuppeteer->update();
    }

    if (mManager != nullptr) {
        isUnlocked();
        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        bool isLastPhase = false;

        if (controller != nullptr && controller->getSuperBowser() != nullptr) {
            isLastPhase = controller->getSuperBowser()->isLastPhase3Bowser();
        }

        bool isVisible = false;

        if (!al::isHideModel(this) && !al::isNerve(this, &NrvGigaBellUnlockCutscene) &&
            !(isLastPhase | al::isNerve(this, &NrvGigaBellUnlockCutsceneSelf))) {
            const sead::Vector3f& cameraPos =
                getSceneCameraInfo()->getViewAt(0)->getLookAtCam().getPos();
            sead::Vector3f from = al::getTrans(this) + sead::Vector3f(0.0f, 300.0f, 0.0f);
            sead::Vector3f dir = cameraPos - from;

            if (dir.squaredLength() < 100000000.0f) {
                al::CollisionPartsFilterActor filter(this);
                isVisible = alCollisionUtil::getStrikeArrowCollisionParts(
                                this, nullptr, from, dir, &filter, nullptr) == nullptr;
            }
        }

        bool isShow =
            isVisible &&
            SingleModeDataFunction::getGoalItemsCollected(GameDataHolderWriter(this)) <
                mManager->getGoalItemsRequired();

        if (!mCounterLock->isAlive()) {
            if (isShow) {
                mCounterLock->startShow(mManager->getGoalItemsRequired());
            }
        } else if (!isShow) {
            mCounterLock->endShow();
        }
    }

    if (mBallHitCooldown > 0) {
        mBallHitCooldown--;
    }

    if (mUnlockDemoSwitchTimer > 0) {
        mUnlockDemoSwitchTimer--;

        if (mUnlockDemoSwitchTimer == 0) {
            al::tryOffStageSwitchInstant(this, "UnlockDemoOn");
        }
    }
}

/**
 * @brief Hide the lock counter when the bell gets clipped.
 */
void GigaBell::startClipped() {
    if (mCounterLock != nullptr) {
        mCounterLock->endShow();
    }

    al::LiveActor::startClipped();
}

/**
 * @brief Stop the unlocked sound when the bell comes back from clipping while locked.
 */
void GigaBell::endClipped() {
    al::LiveActor::endClipped();

    if (!al::isNerve(this, &NrvGigaBellUnlocked)) {
        al::tryStopSe(this, "Unlocked");
    }
}

/**
 * @brief Stop the sound of the unlocked bell.
 */
void GigaBell::clearUnlockSe() {
    al::tryStopSe(this, "Unlocked");
}

/**
 * @brief Wait.
 */
void GigaBell::exeWait() {}

/**
 * @brief Prepare the explanation cutscene of the locked bell.
 */
void GigaBell::exePreExplainCutscene() {
    if (al::isFirstStep(this)) {
        if (rc::findNearestActivePlayerActor(this) != nullptr) {
            rc::setSilentLand(al::getPlayerActor(this, 0));
        }

        al::requestCaptureScreenCover(this, 2);
        al::setNerve(this, &NrvGigaBellExplainCutscene);
    }
}

/**
 * @brief Play the explanation cutscene of the locked bell.
 */
void GigaBell::exeExplainCutscene() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitchInstant(this, "ExplainDemoOn");
        trySetPreDemoKoura();
        rc::addDemoActor(this);

        if (mHasRestartPos) {
            mExplainCutscene->setEndAtCutscenePos(mRestartPos, mRestartFront);
        } else {
            mExplainCutscene->setKeepPlayerPos();
        }

        mExplainCutscene->setHidePlayer();

        if (mConversationMtx != nullptr) {
            mExplainCutscene->overrideBaseMtx(mConversationMtx);
            mExplainCutscene->setUseBaseMtx();
        }

        mExplainCutscene->startDemo();
        mExplainCutscene->setSkipEndWipe();
        TimerManager::tryGetTimerManager(this)->forceCancelCurrent(false, false);
    }

    if (mDemoKoura != nullptr) {
        if (al::isFirstStep(this)) {
            al::requestCaptureScreenCover(this, 2);
        }

        if (al::isStep(this, 1)) {
            auto* player = static_cast<PlayerActor*>(al::findNearestPlayerActor(this));

            if (player != nullptr && player->getPlayerPuppet() != nullptr) {
                rc::hidePuppet(player->getPlayerPuppet());
            }
        }

        if (mExplainCutscene->isWipeCloseEnd()) {
            rc::addDemoActor(mDemoKoura);
            mDemoKoura = nullptr;
        }
    }

    if (mExplainCutscene->isFadedOut() && mCameraArea != nullptr &&
        !(mCameraArea->mIsValid && !mCameraArea->mIsDisabled && mCameraArea->_66)) {
        mCameraArea->validate();
    }

    if (mExplainCutscene->isEndDemo()) {
        al::tryOffStageSwitchInstant(this, "ExplainDemoOn");
        mExplainCutscene = nullptr;
        SingleModeDataFunction::setHasSeenCutscene(GameDataHolderWriter(this), mCutsceneId);
        SaveDataAccessFunction::startSaveDataWriteSync(GameDataHolderWriter(this).getHolder(),
                                                       true);
        al::requestCaptureScreenCover(this, 4);
        al::setNerve(this, &NrvGigaBellWait);
    }
}

/**
 * @brief Remember the shell the nearest player is riding to add it to the demo.
 */
void GigaBell::trySetPreDemoKoura() {
    auto* player = static_cast<PlayerActor*>(al::tryFindNearestPlayerActor(this));

    if (player != nullptr && player->isInKoura()) {
        mDemoKoura = al::getSensorHost(player->getBindSensor());
        return;
    }

    mDemoKoura = nullptr;
}

/**
 * @brief Unlocked: the bell can be rung.
 */
void GigaBell::exeUnlocked() {
    if (al::isFirstStep(this)) {
        if (mIsCameraReturn) {
            rc::setOtherActiveDemo(this, false);
            mIsCameraReturn = false;
        }

        if (al::getCurPlayingBgmPlayName(this) != nullptr) {
            al::changeBgmVolume(this, 1.0f, 30);
        }

        if (!al::isActionPlaying(this, "WaitStartUp")) {
            al::startAction(this, "WaitStartUp");
        }

        setInk(false);
        mIsStartUnlockSe = true;
        al::emitEffect(this, "Before", nullptr);
        al::tryEmitEffect(this, "Before01", nullptr);
        al::invalidateCollisionParts(mCollisionNormal);
        al::invalidateCollisionParts(mCollisionDamage);
        al::invalidateCollisionParts(mCollisionHalf);
        al::invalidateClipping(this);
    }

    if (!rc::isAnyActiveDemo(this) && mIsStartUnlockSe) {
        al::startSe(this, "Unlocked", nullptr);
        mIsStartUnlockSe = false;
    }
}

/**
 * @brief Locked although enough goal items are collected: waits to be unlocked.
 */
void GigaBell::exeLockedSufficientGoalItems() {
    if (al::isFirstStep(this)) {
        if (mIsCameraReturn) {
            rc::setOtherActiveDemo(this, false);
            mIsCameraReturn = false;
        }

        al::validateClipping(this);
        al::setQuat(this, mBaseQuat);

        if (SingleModeDataFunction::isGigaBellUnlocked(GameDataHolderWriter(this))) {
            al::startAction(this, "WaitUnlock");
            al::tryStartMclAnimIfNotPlaying(this, "OnWeak");
            setInk(false);
            setDamaging(false);
        } else if (mIsSufficientGoalItems) {
            startWaitUnravel();
        } else {
            al::startAction(this, "Unravel");
        }

        al::tryStopSe(this, "Unlocked");
    }

    if (al::isActionPlaying(this, "Unravel") && al::isActionEnd(this)) {
        startWaitUnravel();
    }
}

/**
 * @brief Switch the collision between the damaging and the normal one.
 * @param isDamaging Whether the bell hurts the player.
 */
void GigaBell::setDamaging(bool isDamaging) {
    if (isDamaging) {
        al::invalidateCollisionParts(mCollisionNormal);
        al::validateCollisionParts(mCollisionDamage);
        al::invalidateCollisionParts(mCollisionHalf);
    } else {
        al::validateCollisionParts(mCollisionNormal);
        al::invalidateCollisionParts(mCollisionDamage);
        al::invalidateCollisionParts(mCollisionHalf);
    }
}

/**
 * @brief Start waiting half unravelled.
 */
void GigaBell::startWaitUnravel() {
    al::startAction(this, "WaitUnravel");
    setDamagingHalf();
}

/**
 * @brief Locked while Fury Bowser rages.
 */
void GigaBell::exeLockedDisasterMode() {
    if (al::isFirstStep(this)) {
        al::setQuat(this, mBaseQuat);
        al::startAction(this, "DisasterMode");
        setInk(true);
        setDamaging(true);
    }
}

/**
 * @brief Locked: shows the explanation cutscene when the player comes close.
 */
void GigaBell::exeLocked() {
    if (al::isFirstStep(this)) {
        al::setQuat(this, mBaseQuat);
        al::startAction(this, "WaitFreeze");
        al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
        setInk(true);
        setDamaging(true);
    }

    mExplainCutscene = mManager->getNextExplainationCutscene(mLayerId, &mCutsceneId);

    if (mIsInDemo && al::isActionPlaying(this, "WaitFreeze")) {
        al::tryDeleteEffectAndParticle(this, "WaitFreeze");
    }

    if (mExplainCutscene == nullptr) {
        return;
    }

    al::LiveActor* player;

    if (mExplainArea != nullptr) {
        player = rc::findNearestActivePlayerActor(this);

        if (!mExplainArea->isInVolume(al::getTrans(player))) {
            return;
        }
    } else {
        player = rc::tryFindNearestActivePlayerActorInSphere(this, 2050.0f);
    }

    if (player == nullptr) {
        return;
    }

    DisasterModeController* controller = DisasterModeController::tryGetController(this);

    if (controller != nullptr) {
        if (controller->isDisasterMode() || controller->isDisasterStarting()) {
            return;
        }
    }

    if (rc::isPlayerOnRaidon(player)) {
        return;
    }

    SingleModeSceneLayout* layout = SingleModeSceneLayout::tryGetSingleModeSceneLayout(this);

    if (layout != nullptr && layout->isTimerActive()) {
        return;
    }

    al::setNerve(this, &NrvGigaBellPreExplainCutscene);
}

/**
 * @brief Locked while the player is Giga Cat Mario, until Fury Bowser leaves.
 */
void GigaBell::exeLockedPlayerGiga() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "DisasterMode");
    }

    if (!DisasterModeController::tryGetController(this)->isDisasterMode()) {
        trySetNerve("Wait", &NrvGigaBellWait);
    }
}

/**
 * @brief Change the state of the bell unless a transition or the Plessie chase prevents it.
 * @param pName Name of the requested state.
 * @param pNerve Nerve of the requested state.
 */
void GigaBell::trySetNerve(const char* pName, const al::Nerve* pNerve) {
    if (al::isNerve(this, &NrvGigaBellPlessieChase) ||
        al::isNerve(this, &NrvGigaBellTransitionSleep)) {
        return;
    }

    if (al::isNerve(this, &NrvGigaBellTransitionWakeUp)) {
        if (pName != "TransitionSleep") {
            return;
        }

        al::stopAllSeFromUser(this, 5);
        al::tryKillEmitterAndParticleAll(this);
    } else if (pName == "Unlocked" && mManager != nullptr) {
        if (!mIsUnlockedOnce) {
            if (!al::isNerve(this, &NrvGigaBellPreUnlockCutsceneRequested)) {
                al::requestCaptureScreenCover(this, 4);
                al::setNerve(this, &NrvGigaBellPreUnlockCutsceneRequested);
                al::invalidateClipping(this);

                if (al::isClipped(this)) {
                    endClipped();
                }
            }

            return;
        }

        if (!al::isNerve(this, pNerve)) {
            al::setNerve(this, pNerve);
        }

        return;
    }

    if (al::isNerve(this, &NrvGigaBellUnlocked)) {
        mIsUnlockedOnce = false;
        al::tryStopSe(this, "Unlocked");
        getEffectKeeper()->tryKillEmitterAndParticleAll();
    }

    if (!al::isNerve(this, pNerve)) {
        al::setNerve(this, pNerve);
    }
}

/**
 * @brief Ring: swing the bell, rumble the pad and show it with the camera.
 */
void GigaBell::exeRinging() {
    if (al::isFirstStep(this)) {
    }

    if (al::isLessEqualStep(this, 100)) {
        f32 rate = al::getNerveStep(this) / 100.0f;
        f32 rateRest = 1.0f - rate;
        f32 ease = al::easeOut(rateRest);
        sead::Quatf swing = sead::Quatf::unit;
        f32 wave = sinf(rate * 6.0f * 360.0f * 0.017453292f);
        al::makeQuatXDegree(&swing, ease * 10.0f * wave);
        al::setQuat(this, mBaseQuat * swing);

        if (al::isStep(this, 100)) {
            alPadRumbleFunction::stopPadRumbleDirectValue(this);
        } else {
            f32 volume = rateRest * rateRest * rateRest * rateRest;
            alPadRumbleFunction::startPadRumbleDirectValue(
                this, 0.0f, 100.0f, 0.0f, volume * (wave > 0.0f ? wave : -wave), 1.0f, 1.0f);
        }
    }

    if (al::isStep(this, 20)) {
        activateCamera(mCameraTarget, 7000.0f, -5.0f, calculateAngleHToShell(), 1500.0f, 45.0f,
                       true);
    }

    al::isStep(this, 50);

    if (al::isGreaterEqualStep(this, 200)) {
        deactivateCamera();
        al::setQuat(this, mBaseQuat);
        al::setNerve(this, &NrvGigaBellWait);
    }
}

/**
 * @brief Start the camera following an actor around the bell.
 * @param pTarget Actor the camera looks at.
 * @param distance Distance of the camera.
 * @param angleV Vertical angle of the camera.
 * @param angleH Horizontal angle of the camera.
 * @param offsetY Height offset of the look-at point.
 * @param interpoleFrame Interpolation frames, -1 for the default.
 * @param isDemo Whether the camera belongs to a demo.
 * @return Whether the camera was started.
 */
bool GigaBell::activateCamera(al::LiveActor* pTarget, f32 distance, f32 angleV, f32 angleH,
                              f32 offsetY, f32 interpoleFrame, bool isDemo) {
    if (isDemo && rc::isAnyActiveDemo(this)) {
        return false;
    }

    if (mIsCameraActive) {
        return false;
    }

    mIsCameraActive = true;
    mIsCameraReturn = false;
    al::CameraPoserFixActor* poser = getFixActorPoser(mCameraTicket);

    if (al::isEqualString(poser->getName(), "FixedActor")) {
        poser->mIsReturn = false;
        poser->mIsReturnEnd = false;
        poser->mTargetActor = pTarget;
        poser->mOffset.x = 0.0f;
        poser->mOffset.z = 0.0f;
        poser->mOffset.y = offsetY;
        poser->mDistance = distance;
        poser->mAngleV = angleV;
        poser->mAngleH = angleH;
    }

    al::startCamera_RS(this, mCameraTicket, interpoleFrame == -1.0f ? 45.0f : interpoleFrame);

    if (isDemo) {
        rc::setOtherActiveDemo(this, true);
    }

    return true;
}

/**
 * @brief Calculate the horizontal camera angle facing Fury Bowser's shell.
 * @return The angle in degrees.
 */
f32 GigaBell::calculateAngleHToShell() {
    f32 angle = 0.0f;
    DisasterModeController* controller = DisasterModeController::tryGetController(this);

    if (controller != nullptr) {
        al::LiveActor* shell = controller->getShell();

        if (shell != nullptr) {
            const sead::Vector3f& shellTrans = al::getTrans(shell);
            const sead::Vector3f& targetTrans = al::getTrans(mCameraTarget);
            f32 dx = shellTrans.x - targetTrans.x;
            f32 dz = shellTrans.z - targetTrans.z;
            f32 absX = dx > 0.0f ? dx : -dx;
            f32 absZ = dz > 0.0f ? dz : -dz;

            if (absZ > absX) {
                angle = dz > 0.0f ? 180.0f : 0.0f;
            } else {
                angle = dx > 0.0f ? -90.0f : 90.0f;
            }
        }
    }

    return angle;
}

/**
 * @brief End the camera started with activateCamera().
 */
void GigaBell::deactivateCamera() {
    if (!mIsCameraActive) {
        return;
    }

    if (mManager != nullptr && mManager->isCameraReturn()) {
        al::CameraPoserFixActor* poser = getFixActorPoser(mCameraTicket);

        if (al::isEqualString(poser->getName(), "FixedActor") &&
            getSceneInfo()->cameraDirector->isStoredCamera()) {
            getSceneInfo()->cameraDirector->restoreCamera();
            poser->setReturn(0, getSceneInfo()->cameraDirector->getStoredCameraPos(),
                             getSceneInfo()->cameraDirector->getStoredLookAtPos());
            mIsCameraReturn = true;
        }

        mManager->clearCameraReturn();
    }

    bool isCameraReturn = mIsCameraReturn;
    mIsCameraActive = false;

    if (isCameraReturn) {
        al::CameraPoseInfo poseInfo;
        poseInfo.pos = getSceneInfo()->cameraDirector->getStoredCameraPos();
        poseInfo.at = getSceneInfo()->cameraDirector->getStoredLookAtPos();
        poseInfo.up = sead::Vector3f::ey;
        al::endCameraWithNextCameraPose(this, mCameraTicket, &poseInfo, 0);
        getSceneInfo()->cameraDirector->clearStoredCamera();
        return;
    }

    if (mCameraTicket != nullptr) {
        al::endCamera_RS(this, mCameraTicket, 0, false);
    }

    rc::setOtherActiveDemo(this, false);
}

/**
 * @brief Wait for the player cutscene before collecting the bell.
 */
void GigaBell::exeCollectingCameraPending() {
    if (mManager != nullptr) {
        if (al::isFirstStep(this)) {
            TimerManager::tryGetTimerManager(this)->forceCancelCurrent(true, false);
        }

        DemoObjBase* demo = mManager->getCollectDemo();

        if (demo != nullptr) {
            demo->setPlacementBaseMtx(getBaseMtx());
            demo->startDemo();
        }

        kill();
        return;
    }

    if (!rc::requestStartDemoPlayerCutscene(this)) {
        return;
    }

    rc::addDemoActor(this);
    rc::addDemoActor(DisasterModeController::tryGetController(this));
    al::setNerve(this, &NrvGigaBellCollecting);

    if (mBowserJointMtx != nullptr) {
        activateCamera(this, 5500.0f, -20.0f, calculateAngleHToShell(), 1800.0f, 60.0f, false);
        mBowserJointMtx = nullptr;
        return;
    }

    if (mPlayer != nullptr) {
        mPlayer->getProperty()->mTrans = al::getTrans(this);
    }

    sead::LookAtCamera camera = getSceneCameraInfo()->getViewAt(0)->getLookAtCam();
    f32 dx = camera.getPos().x - al::getTrans(mCameraTarget).x;
    f32 dz = camera.getPos().z - al::getTrans(mCameraTarget).z;
    sead::Vector2f dir(dx, dz);
    activateCamera(mCameraTarget, 5500.0f, -20.0f, al::calcAngleDegree(dir, sead::Vector2f::ey),
                   1800.0f, 60.0f, false);

    if (mPlayer != nullptr) {
        sead::Vector3f front(dx, 0.0f, dz);
        front.normalize();
        mPlayer->getProperty()->setFrontVec(front);
    }
}

/**
 * @brief Collect: grow the bound player into Giga Cat Mario.
 */
void GigaBell::exeCollecting() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Collecting");
        mPlayerScale = 1.0f;
    }

    if (mManager == nullptr) {
        kill();
        return;
    }

    if (mPlayer != nullptr && !mPlayer->getPlayerGigaDirector()->isStaying()) {
        f32 rate = al::squareOut(mPlayer->getPlayerGigaDirector()->getScaleRate());
        f32 scale = al::lerpValue(rate, 1.0f, mPlayer->getPlayerGigaDirector()->getScaleMax());
        f32 scaleDiff = scale - mPlayerScale;
        IUsePlayerPuppet* puppet = mBindPuppeteer->getPlayerPuppet();
        sead::Vector3f trans = puppet->getTrans() + sead::Vector3f::ey * (scaleDiff * 0.0f);
        puppet->setTrans(trans);
        mPlayerScale = scale;
    }

    if (al::getNerveStep(this) == 150) {
        if (mIsDebugCollect) {
            al::setNerve(this, &NrvGigaBellDebugCollectWait);
        } else {
            SingleModeDataFunction::setShouldFadeToWhite(GameDataHolderWriter(this), true);
            SingleModeDataFunction::setPhaseEnd(GameDataHolderWriter(this), true);
        }
    }
}

/**
 * @brief Collected during the Fury Bowser battle.
 */
void GigaBell::exeCollectingBossBattle() {
    if (al::isFirstStep(this)) {
        al::stopAction(this);
        al::hideModelIfShow(this);
        al::tryKillEmitterAndParticleAll(this);
        al::startHitReactionGet(this);
        al::invalidateHitSensors(this);
        al::tryStopSe(this, "Unlocked");
        al::setEffectParticleScale(this, "Get", 10.0f);
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvGigaBellWaitReviveInBattle);
    }
}

/**
 * @brief Reappear during the Fury Bowser battle.
 */
void GigaBell::exeReappearInBattle() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
        setInk(false);
        al::invalidateCollisionParts(mCollisionNormal);
        al::invalidateCollisionParts(mCollisionDamage);
        al::invalidateCollisionParts(mCollisionHalf);
        al::invalidateClipping(this);
    }

    if (rc::isActiveDemo(this)) {
        al::setNerve(this, &NrvGigaBellBattleDefeatDemo);
    } else if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvGigaBellWaitInBattle);
    }
}

/**
 * @brief Wait to be rung during the Fury Bowser battle.
 */
void GigaBell::exeWaitInBattle() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitStartUp");
        al::emitEffect(this, "Before", nullptr);
        al::tryEmitEffect(this, "Before01", nullptr);
    }

    if (rc::isActiveDemo(this)) {
        al::setNerve(this, &NrvGigaBellBattleDefeatDemo);
    }
}

/**
 * @brief Wait to reappear after being rung during the Fury Bowser battle.
 */
void GigaBell::exeWaitReviveInBattle() {
    if (!al::isGreaterEqualStep(this, 3600) || rc::isActiveDemo(this)) {
        return;
    }

    if (mIsRevivableInBattle) {
        reappearInBattle();
        al::showModelIfHide(this);
        al::tryKillEmitterAndParticleAll(this);
        al::validateHitSensors(this);
    } else {
        kill();
    }
}

/**
 * @brief Freeze during the demo of Fury Bowser's defeat.
 */
void GigaBell::exeBattleDefeatDemo() {
    if (al::isFirstStep(this)) {
        al::stopAllSeFromUser(this, 5);
        al::setActionFrameRate(this, 0.0f);
    }

    if (al::isStep(this, 60)) {
        al::startAction(this, "WaitStartUp");
        al::emitEffect(this, "Before", nullptr);
        al::tryEmitEffect(this, "Before01", nullptr);
    }
}

/**
 * @brief Pop up above the spawner, then become unlocked.
 */
void GigaBell::exePopUpAbove() {
    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvGigaBellUnlocked);
    }
}

/**
 * @brief Bind the player and put it in place before the unlock cutscene.
 */
void GigaBell::exePreUnlockCutscene() {
    if (al::isFirstStep(this)) {
        mPuppet = nullptr;
        PlayerActor* player = static_cast<PlayerActor*>(rc::tryFindNearestActivePlayerActorInSphere(
            this, al::getSensorRadius(this, "Cutscene")));

        if (player != nullptr) {
            trySetPreDemoKoura();

            if (rc::isPlayerOnSkateShoes(player)) {
                if (player->getBindSensor() != nullptr) {
                    rc::addDemoActor(al::getSensorHost(player->getBindSensor()));
                }
            } else {
                rc::addDemoPlayer(player);
            }

            rc::requestBindAllPlayerAcceptReviveBubble(this,
                                                       al::getHitSensor(this, "BindCutscene"));
        }

        al::validateHitSensor(this, "Cutscene");
    }

    if (mPuppet == nullptr && !al::isGreaterStep(this, 1)) {
        return;
    }

    if (mPuppet != nullptr) {
        addPlayerItemsToDemo();
        rc::setPuppetTrans(mPuppet, mRestartPos);
        rc::setPuppetFrontVec(mPuppet, mRestartFront);
        rc::startPuppetAction(mPuppet, "Wait");
        rc::setPuppetActionRate(mPuppet, 0.0f);
        rc::killAllPlayerEffect(al::getSensorHost(rc::getPuppetSensor(mPuppet)));
        PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

        if (koopaJr != nullptr) {
            koopaJr->resetTransformPostCutscene(mRestartPos, mRestartFront);
        }
    }

    al::invalidateHitSensor(this, "Cutscene");

    if (al::isNerve(this, &NrvGigaBellPreUnlockCutscene)) {
        al::setNerve(this, &NrvGigaBellUnlockCutsceneSelf);
    } else {
        al::setNerve(this, &NrvGigaBellUnlockCutscene);
    }
}

/**
 * @brief Add the items held by the bound player and its shell to the running demo.
 */
void GigaBell::addPlayerItemsToDemo() {
    if (mPuppet != nullptr) {
        al::HitSensor* sensor = mPuppet->getMsgTargetSensor();

        if (sensor != nullptr && rc::isReallyPlayerActor(sensor)) {
            auto* player = static_cast<PlayerActor*>(al::getSensorHost(sensor));

            if (player->getHoldingSensor() != nullptr) {
                auto* item = static_cast<PackunFlowerWithPot*>(
                    al::getSensorHost(player->getHoldingSensor()));
                rc::addDemoActor(item);

                if (al::isEqualString(item->getName(), "パックンフラワー（鉢植えあり）") &&
                    item->getPot() != nullptr) {
                    rc::addDemoActor(item->getPot());
                }
            }
        }
    }

    if (mDemoKoura != nullptr) {
        rc::addDemoActor(mDemoKoura);
        mDemoKoura = nullptr;
    }
}

/**
 * @brief Play the unlock cutscene, then become unlocked.
 */
void GigaBell::exeUnlockCutscene() {
    const char* actionName =
        SingleModeDataFunction::isGigaBellUnlocked(GameDataHolderWriter(this)) ? "ReleaseRepeat" :
                                                                                  "Release";

    if (al::isFirstStep(this)) {
        al::tryOnStageSwitchInstant(this, "UnlockDemoOn");
        mDemoSkipLayout->startHidden();
        al::changeBgmVolume(this, 0.5f, 30);

        if (mManager->shouldPlayUnlockCutscene() && mIsBellInCutscene) {
            al::setTrans(mCameraTarget, al::getTrans(this));
            sead::Vector3f rotate = sead::Vector3f::zero;
            al::calcQuatRotateDegree(&rotate, al::getQuat(this));
            al::setRotate(mCameraTarget, rotate);
            activateCamera(mCameraTarget, 6000.0f, 0.0f, 15.0f, 600.0f, 0.0f, false);
        }

        mManager->updateUnlockedGoalItems();
        al::startAction(this, actionName);

        if (SingleModeDataFunction::isGigaBellUnlocked(GameDataHolderWriter(this))) {
            al::tryStartVisAnimIfExist(this, "ReleaseStone");
        }

        al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
        setInk(false);
        setDamaging(false);
        mIsDemoSkipped = false;
    }

    al::WipeSimple* wipe = mManager->getWipeFadeBlack();
    s32 wipeFrame = al::getActionFrameMax(wipe, "Appear", nullptr);
    f32 step = al::getNerveStep(this);
    bool isSkipNow = false;

    if (al::getActionFrameMax(this, actionName) - wipeFrame > step && !mIsDemoSkipped &&
        mIsBellInCutscene && mIsCameraActive &&
        mDemoSkipLayout->isSkip(sead::BitFlag<u16>(1 << al::getMainControllerPort()))) {
        mIsDemoSkipped = true;
        isSkipNow = true;
    }

    if (mIsCameraActive) {
        al::CameraPoserFixActor* poser = getFixActorPoser(mCameraTicket);

        if (al::isEqualString(poser->getName(), "FixedActor")) {
            f32 rate = step / 240.0f;

            if (rate < 0.0f) {
                rate = 0.0f;
            } else if (rate > 1.0f) {
                rate = 1.0f;
            }

            f32 ease = al::easeInOut(rate);
            poser->mDistance = al::lerpValue(ease, 6000.0f, 4000.0f);
            f32 offsetY = al::lerpValue(ease, 600.0f, 320.0f);
            poser->mOffset.x = 0.0f;
            poser->mOffset.z = 0.0f;
            poser->mOffset.y = offsetY;
        }
    }

    if (mIsDemoSkipped) {
        if (isSkipNow) {
            mDemoSkipLayout->end();
            mManager->getWipeFadeBlack()->startClose(-1);
        }

        if (mManager->getWipeFadeBlack()->isCloseEnd()) {
            nn::oe::ExitApplicationAndGoBackToInStoreDemoMenu();
        }

        return;
    }

    if (mIsBellInCutscene ||
        !SingleModeDataFunction::isGigaBellUnlocked(GameDataHolderWriter(this))) {
        DisasterModeController* controller = DisasterModeController::tryGetController(this);

        if (controller == nullptr || controller->isInDisasterOrInstant()) {
            if (!al::isActionPlaying(this, actionName) || !al::isActionEnd(this)) {
                return;
            }
        }
    }

    if (mIsCameraActive) {
        deactivateCamera();
    }

    mManager->endCutscene(al::isNerve(this, &NrvGigaBellUnlockCutsceneSelf));
    al::requestCaptureScreenCover(this, 50);

    if (mIsBellInCutscene) {
        SingleModeDataFunction::setGigaBellUnlocked(GameDataHolderWriter(this), true);
    }

    mIsUnlockedOnce = true;
    mUnlockDemoSwitchTimer = 2;
    al::setNerve(this, &NrvGigaBellUnlocked);

    if (mPuppet != nullptr) {
        rc::removeDemoPlayer(
            static_cast<PlayerActor*>(al::getSensorHost(rc::getPuppetSensor(mPuppet))));
        rc::endBindOnGroundAndPuppetNull(&mPuppet);
    }
}

/**
 * @brief Start the cutscene returning the bell to its locked state.
 * @param isReset Whether the bell is reset with a cutscene.
 */
void GigaBell::startReturnCutscene(bool isReset) {
    if (isReset) {
        al::invalidateClipping(this);
        al::setNerve(this, &NrvGigaBellPreResetCutscene);
        return;
    }

    al::setNerve(this, &NrvGigaBellTransitionSleep);
}

/**
 * @brief Bind the player and put it in place before the reset cutscene.
 */
void GigaBell::exePreResetCutscene() {
    if (al::isFirstStep(this)) {
        DisasterModeController* controller = DisasterModeController::tryGetController(this);

        if (controller != nullptr) {
            controller->getSpikeDirector()->forceKillAllSpikes();
        }

        mPuppet = nullptr;
        PlayerActor* player = static_cast<PlayerActor*>(rc::tryFindNearestActivePlayerActorInSphere(
            this, al::getSensorRadius(this, "Cutscene")));

        if (player != nullptr) {
            trySetPreDemoKoura();
            rc::addDemoPlayer(player);
            rc::requestBindAllPlayerAcceptReviveBubble(this,
                                                       al::getHitSensor(this, "BindCutscene"));
        }

        al::validateHitSensor(this, "Cutscene");
    }

    if (al::isStep(this, 1)) {
        if (mPuppet != nullptr) {
            addPlayerItemsToDemo();
            rc::setPuppetTrans(mPuppet, mRestartPos);
            rc::setPuppetFrontVec(mPuppet, mRestartFront);
            rc::startPuppetAction(mPuppet, "Wait");
            rc::killAllPlayerEffect(al::getSensorHost(rc::getPuppetSensor(mPuppet)));
            PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);

            if (koopaJr != nullptr) {
                koopaJr->resetTransformPostCutscene(mRestartPos, mRestartFront);
            }
        }

        al::invalidateHitSensor(this, "Cutscene");
        al::stopAllSeFromUser(this, 5);
        al::setNerve(this, &NrvGigaBellResetCutscene);
    }
}

/**
 * @brief Play the cutscene putting the bell back to sleep.
 */
void GigaBell::exeResetCutscene() {
    s32 endStep = al::getActionFrameMax(this, "Sleep") + 15.0f + 45.0f;

    if (al::isFirstStep(this)) {
        rc::changeActiveDemoAudioType(this, static_cast<alSeFunction::DemoType>(3));
        mDemoSkipLayout->startHidden();
        al::changeBgmVolume(this, 0.5f, 30);
        DisasterModeController::tryGetController(this);
        al::setTrans(mCameraTarget, al::getTrans(this));
        sead::Vector3f rotate = sead::Vector3f::zero;
        al::calcQuatRotateDegree(&rotate, al::getQuat(this));
        al::setRotate(mCameraTarget, rotate);
        activateCamera(mCameraTarget, 6000.0f, 0.0f, 15.0f, 600.0f, 0.0f, false);
        setInk(false);
        al::tryKillEmitterAndParticleAll(this);
        al::cancelCameraShake(this);
        al::tryStartAction(this, "Sleep");
        al::setActionFrameRate(this, 0.0f);
        mIsDemoSkipped = false;
        al::CameraPoserFixActor* poser = getFixActorPoser(mCameraTicket);

        if (al::isEqualString(poser->getName(), "FixedActor")) {
            poser->mOffset.set(0.0f, 320.0f, 0.0f);
            poser->mDistance = 4000.0f;
        }
    }

    s32 wipeFrame = al::getActionFrameMax(mManager->getWipeFadeBlack(), "Appear", nullptr);

    if (al::getNerveStep(this) < endStep - wipeFrame && !mIsDemoSkipped &&
        mDemoSkipLayout->isSkip(sead::BitFlag<u16>(1 << al::getMainControllerPort()))) {
        mIsDemoSkipped = true;
    }

    if (al::isStep(this, 15)) {
        al::setActionFrameRate(this, 1.0f);
    }

    if (mIsDemoSkipped || al::isGreaterEqualStep(this, endStep)) {
        mManager->getWipeFadeBlack()->startClose(-1);
        al::setNerve(this, &NrvGigaBellResetCutsceneEnd);
    }
}

/**
 * @brief End the reset cutscene once the screen is faded out.
 */
void GigaBell::exeResetCutsceneEnd() {
    if (al::isFirstStep(this)) {
        mManager->getWipeFadeBlack()->startClose(-1);
    }

    if (mManager->getWipeFadeBlack()->isCloseEnd()) {
        mManager->getWipeFadeBlack()->startOpen(-1);
        deactivateCamera();
        mManager->endReturnCutscene();
        al::requestCaptureScreenCover(this, 4);
        al::setNerve(this, &NrvGigaBellLockedSufficientGoalItems);
        mIsUnlockedOnce = false;

        if (mPuppet != nullptr) {
            rc::removeDemoPlayer(
                static_cast<PlayerActor*>(al::getSensorHost(rc::getPuppetSensor(mPuppet))));
            rc::endBindOnGroundAndPuppetNull(&mPuppet);
        }

        al::validateClipping(this);

        if (al::getCurPlayingBgmPlayName(this) != nullptr) {
            al::changeBgmVolume(this, 1.0f, 30);
        }
    }
}

/**
 * @brief Follow Fury Bowser during the final Plessie chase.
 */
void GigaBell::exePlessieChase() {}

/**
 * @brief Hit by Plessie during the final chase.
 */
void GigaBell::exePlessieChaseHit() {
    if (al::isFirstStep(this)) {
        al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
        setInk(false);
        al::startVisAnim(this, "PlessieChaseHitSecond");
        s32 hitCount = mManager->getPlessieChaseHitCount();

        if (hitCount == 2 || hitCount == 1) {
            al::startSe(this, "PlessieChaseHitRing", nullptr);
        }

        al::setNerve(this, &NrvGigaBellPlessieChase);
    }
}

/**
 * @brief Fall asleep, then become locked.
 */
void GigaBell::exeTransitionSleep() {
    if (al::isFirstStep(this)) {
        al::tryStartAction(this, "Sleep");
        return;
    }

    al::setNerveAtActionEnd(this, &NrvGigaBellLockedSufficientGoalItems);
}

/**
 * @brief Wake up, then become unlocked.
 */
void GigaBell::exeTransitionWakeUp() {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);

    if (controller != nullptr && controller->getSuperBowser() != nullptr &&
        al::isDead(controller->getSuperBowser())) {
        return;
    }

    if (al::isFirstStep(this)) {
        al::invalidateCollisionParts(this);
        al::invalidateCollisionParts(mCollisionNormal);
        al::invalidateCollisionParts(mCollisionDamage);
        al::invalidateCollisionParts(mCollisionHalf);
        al::tryStartAction(this, "WakeUp");

        if (mIsBellInCutscene) {
            al::startSe(this, "UnlockNoCutscene", nullptr);
        }

        return;
    }

    al::setNerveAtActionEnd(this, &NrvGigaBellUnlocked);
}

/**
 * @brief Debug: wait after being collected, then become unlocked again.
 */
void GigaBell::exeDebugCollectWait() {
    if (al::isFirstStep(this)) {
        mBindPuppeteer->endBind(nullptr);
        deactivateCamera();
        rc::requestEndDemoPlayerCutscene(this);

        if (mPlayer != nullptr) {
            mPlayer->getPlayerGigaDirector()->forceEnd();
            mPlayer->getPlayerGigaDirector()->resetScale();
        }

        al::startAction(this, "WaitFreeze");
    }

    if (al::getNerveStep(this) > 300) {
        al::showModelIfHide(this);
        al::validateHitSensors(this);
        al::setNerve(this, &NrvGigaBellUnlocked);
    }
}

/**
 * @brief Play the demo of the player collecting the bell.
 */
void GigaBell::exeCollectDemo() {
    if (al::isFirstStep(this)) {
        if (rc::isReallyPlayerActor(mCollectSensor)) {
            auto* player = static_cast<PlayerActor*>(al::getSensorHost(mCollectSensor));
            al::HitSensor* bindSensor = player->getBindSensor();

            if (bindSensor != nullptr) {
                if (player->isRaidonExist()) {
                    killSensorHost(bindSensor);
                }

                if (al::isSensorHostName(bindSensor, "スケート靴")) {
                    killSensorHost(bindSensor);
                }

                if (al::isSensorKickKoura(bindSensor)) {
                    killSensorHost(bindSensor);
                }
            }

            al::HitSensor* holdingSensor = player->getHoldingSensor();

            if (holdingSensor != nullptr) {
                killSensorHost(holdingSensor);
            }
        }

        al::tryOnStageSwitchInstant(this, "CollectDemoOn");
        sead::Vector3f front = sead::Vector3f::ez;
        al::calcFrontDir(&front, mPedestal);
        sead::Vector2f frontH(front.x, front.z);
        f32 angle = al::calcAngleDegree(frontH, sead::Vector2f::ey);
        sead::Quatf quat;
        quat.setAxisAngle(sead::Vector3f::ey, angle * 3.1415927f / 180.0f);
        sead::Matrix34f demoMtx;
        demoMtx.makeQT(quat, sead::Vector3f(0.0f, 0.0f, 0.0f));
        demoMtx.setTranslation(al::getTrans(mPedestal) + sead::Vector3f::ey * 600.0f);
        mGetCutscene->overrideBaseMtx(&demoMtx);
        mGetCutscene->setForceIgnoreCharId();
        mGetCutscene->startDemo();

        if (mHasRestartPos &&
            SingleModeDataFunction::getUnlockedPhase(GameDataHolderWriter(this)) != 1) {
            SingleModeDataFunction::setGigaBellPlayerRespawnPoint(GameDataHolderWriter(this),
                                                                  mRestartPos, mRestartFront);
        }

        al::hideModelIfShow(this);
        al::startHitReaction(this, "GetDemo");
        al::invalidateHitSensors(this);
        al::offCollide(this);
        al::invalidateCollisionParts(this);
        al::invalidateCollisionParts(mCollisionNormal);
        al::invalidateCollisionParts(mCollisionDamage);
        al::invalidateCollisionParts(mCollisionHalf);
        al::tryStopSe(this, "Unlocked");
        getEffectKeeper()->deleteEffectAll();
        rc::addDemoActor(DisasterModeController::tryGetController(this));
        rc::addDemoActor(this);
        al::stopAllBgm(this, 5);

        if (mGetCutscene->isCancelWipeActive()) {
            al::stopAllSeFromUser(this, 30);
            SingleModeDataFunction::setDemoWasCancelled(GameDataHolderWriter(this), true);

            if (mCollectSensor != nullptr) {
                rc::removeAllEquipFromPlayerSilent(mCollectSensor);
                mCollectSensor = nullptr;
            }
        }
    }

    if (al::isStep(this, 4)) {
        DisasterModeController* controller = DisasterModeController::tryGetController(this);

        if (controller != nullptr) {
            controller->getSuperBowser()->getLaserState()->forceKill();

            if (controller->getSuperBowser() != nullptr) {
                controller->getSuperBowser()->forceKillFireballs();
            }
        }
    }

    if (al::isStep(this, 30) && !al::isActionPlaying(this, "Collecting")) {
        al::startAction(this, "Collecting");
    }

    if (!mGetCutscene->isCancelWipeActive() && al::isStep(this, 167)) {
        mWipe->startClose(45);
        SingleModeDataFunction::setShouldFadeToWhite(GameDataHolderWriter(this), true);

        if (mCollectSensor != nullptr) {
            rc::removeAllEquipFromPlayerSilent(mCollectSensor);
            mCollectSensor = nullptr;
        }
    }
}

/**
 * @brief Start the unlock cutscene of a bell placed in a cutscene.
 */
void GigaBell::startUnlockCutscene() {
    if (mIsBellInCutscene) {
        trySetNerve("Unlocked", &NrvGigaBellPreUnlockCutsceneRequested);
        al::setNerve(this, &NrvGigaBellPreUnlockCutscene);
    }
}

/**
 * @brief Change the lock state of the bell if its current state allows it.
 * @param state The requested state.
 */
void GigaBell::trySetLockState(LockState state) {
    if (!al::isNerve(this, &NrvGigaBellWait) && !al::isNerve(this, &NrvGigaBellLocked) &&
        !al::isNerve(this, &NrvGigaBellLockedDisasterMode) &&
        !al::isNerve(this, &NrvGigaBellLockedSufficientGoalItems) &&
        !al::isNerve(this, &NrvGigaBellUnlocked) &&
        !al::isNerve(this, &NrvGigaBellTransitionWakeUp)) {
        return;
    }

    switch (state) {
    case LockState::Locked:
        trySetNerve("Locked", &NrvGigaBellLocked);
        break;
    case LockState::LockedDisasterMode:
        trySetNerve("LockedDisasterMode", &NrvGigaBellLockedDisasterMode);
        break;
    case LockState::LockedSufficientGoalItems:
        if (al::isNerve(this, &NrvGigaBellUnlocked) ||
            al::isNerve(this, &NrvGigaBellTransitionWakeUp)) {
            trySetNerve("TransitionSleep", &NrvGigaBellTransitionSleep);
        } else {
            trySetNerve("LockedSufficientGoalItems", &NrvGigaBellLockedSufficientGoalItems);
        }
        break;
    case LockState::Unlocked:
        al::invalidateClipping(this);

        if (!mManager->shouldPlayUnlockCutscene() &&
            al::isNerve(this, &NrvGigaBellLockedSufficientGoalItems)) {
            trySetNerve("TransitionWakeUp", &NrvGigaBellTransitionWakeUp);
        } else {
            trySetNerve("Unlocked", &NrvGigaBellUnlocked);
        }
        break;
    }
}

/**
 * @brief Lock the bell while the player is Giga Cat Mario.
 */
void GigaBell::startLockedPlayerGiga() {
    if (al::isNerve(this, &NrvGigaBellUnlocked)) {
        getEffectKeeper()->tryKillEmitterAndParticleAll();
    }

    al::setNerve(this, &NrvGigaBellLockedPlayerGiga);
}

/**
 * @brief Unlock the bell right away.
 */
void GigaBell::forceUnlock() {
    al::setNerve(this, &NrvGigaBellUnlocked);
}

/**
 * @brief Set the horizontal angle of the bell's camera.
 * @param angleH The angle.
 */
void GigaBell::setCameraAngleH(f32 angleH) {
    al::CameraPoserFixActor* poser = getFixActorPoser(mCameraTicket);

    if (al::isEqualString(poser->getName(), "FixedActor")) {
        poser->mAngleH = angleH;
    }
}

/**
 * @brief Set the distance of the bell's camera.
 * @param distance The distance.
 */
void GigaBell::setCameraDistance(f32 distance) {
    al::CameraPoserFixActor* poser = getFixActorPoser(mCameraTicket);

    if (al::isEqualString(poser->getName(), "FixedActor")) {
        poser->mDistance = distance;
    }
}

/**
 * @brief Get the unlocked phase of the game.
 * @return The phase.
 */
s32 GigaBell::getPhase() {
    return SingleModeDataFunction::getUnlockedPhase(GameDataHolderWriter(this));
}

/**
 * @brief Set whether the bell is placed in a cutscene.
 * @param isInCutscene Whether the bell is in a cutscene.
 */
void GigaBell::setIsBellInCutscene(bool isInCutscene) {
    mIsBellInCutscene = isInCutscene;
}

/**
 * @brief Get the position the player respawns at after ringing the bell.
 * @return The position, or zero when the bell has none.
 */
sead::Vector3f GigaBell::getRespawnPlayerPos() {
    return mHasRestartPos ? mRestartPos : sead::Vector3f::zero;
}

/**
 * @brief Get the position of the player for the Plessie chase.
 * @return The position.
 */
sead::Vector3f GigaBell::getPlessieChasePos() {
    if (mHasPlessieChasePos) {
        return mPlessieChasePos;
    }

    return getRespawnPlayerPos();
}

/**
 * @brief Get the front of the player for the Plessie chase.
 * @return The front direction.
 */
sead::Vector3f GigaBell::getPlessieChaseFront() {
    return mPlessieChaseFront;
}

/**
 * @brief Check whether Plessie already hit the bell in the chase.
 * @return Whether the bell was hit.
 */
bool GigaBell::hasBeenHitPlessieChase() {
    return mIsHitPlessieChase;
}

/**
 * @brief Attach the bell to Fury Bowser for the final Plessie chase.
 * @param pBowser Fury Bowser.
 */
void GigaBell::startPlessieChase(SuperBowser* pBowser) {
    if (al::isClipped(this)) {
        endClipped();
    }

    al::invalidateClipping(this);
    mBowserJointMtx = al::getJointMtxPtr(pBowser, "AllRoot");
    mPlessieChaseOffset = mManager->getPlessieChaseBellBaseOffset();
    al::startAction(this, "PlessieChaseInitialState");
    trySetNerve("PlessieChase", &NrvGigaBellPlessieChase);
    al::invalidateCollisionParts(mCollisionNormal);
    al::invalidateCollisionParts(mCollisionDamage);
    al::invalidateCollisionParts(mCollisionHalf);
}

/**
 * @brief Move the bell along with Fury Bowser during the Plessie chase.
 * @param index Index of the bell in the chase.
 */
void GigaBell::updatePlessieChase(s32 index) {
    if (mBowserJointMtx == nullptr) {
        return;
    }

    SuperBowser* bowser = DisasterModeController::tryGetController(this)->getSuperBowser();
    sead::Vector3f offset =
        mManager->getPlessieChaseBellBaseOffset() + mManager->calcMoveOffset(index);

    if (mManager->canHitBellPlessieChase()) {
        mPlessieChaseOffset = offset;
        mPlessieChasePrevOffset = offset;
    } else {
        al::lerpVec(&mPlessieChaseOffset, mPlessieChasePrevOffset, offset, bowser->getJumpRate());
    }

    sead::Vector3f base = mBowserJointMtx->getTranslation();
    sead::Vector3f rotated = mPlessieChaseOffset;
    al::rotateVectorQuat(&rotated, al::getQuat(this));
    al::setTrans(this, base + rotated);
}

/**
 * @brief Play the reaction of Plessie hitting the bell.
 */
void GigaBell::doPlessieChaseHitEffect() {
    sead::Vector3f front = sead::Vector3f::ez;
    al::calcFrontDir(&front, this);
    const sead::Vector3f& trans = al::getTrans(this);
    sead::Vector3f pos = front * al::getSensorRadius(this, "Body") + trans;
    getHitReactionKeeper()->start("命中", &pos, nullptr, nullptr);
    al::emitEffect(this, "GigaBellClean", nullptr);
}

/**
 * @brief Reset the bell for a new Plessie chase.
 */
void GigaBell::plessieChaseReset() {
    mIsHitPlessieChase = false;
    al::tryStartAction(this, "PlessieChaseInitialState");
    al::tryStartMclAnimIfNotPlaying(this, "BaseColor");
    setInk(false);
    getEffectKeeper()->tryKillEmitterAndParticleAll();
    al::setNerve(this, &NrvGigaBellPlessieChase);
}

/**
 * @brief Check whether the unlock cutscene is running.
 * @return Whether the bell is in the unlock cutscene or its preparation.
 */
bool GigaBell::isInUnlockCutscene() {
    return al::isNerve(this, &NrvGigaBellUnlockCutscene) ||
           al::isNerve(this, &NrvGigaBellUnlockCutsceneSelf) ||
           al::isNerve(this, &NrvGigaBellPreUnlockCutsceneRequested) ||
           al::isNerve(this, &NrvGigaBellPreUnlockCutscene);
}

/**
 * @brief Use the half damaging collision.
 */
void GigaBell::setDamagingHalf() {
    al::invalidateCollisionParts(mCollisionNormal);
    al::invalidateCollisionParts(mCollisionDamage);
    al::validateCollisionParts(mCollisionHalf);
}
