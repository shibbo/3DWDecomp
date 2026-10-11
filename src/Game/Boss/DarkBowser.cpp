#include "Boss/DarkBowser.hpp"

#include <math/seadMathCalcCommon.h>

#include "Boss/DarkBowserBattle.hpp"
#include "Boss/DarkBowserRingBeam.hpp"
#include "Boss/DarkBowserStateDemo.hpp"
#include "Boss/InkBomb.hpp"
#include "Demo/DemoCutscene.hpp"
#include "Enemy/SuperBowserFireFlame.hpp"
#include "Enemy/SuperBowserHealthBar.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Effect/EffectSystem.hpp"
#include "Library/Effect/EffectSystemInfo.hpp"
#include "Library/Effect/PtclSystem.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMapUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Obj/EffectObjFollowCamera.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/PostProcessing/RadialBlurDirector.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/Fury/GameSkyProjection.hpp"
#include "MapObj/Fury/GigaBellItem.hpp"
#include "MapObj/Fury/InkPuddle.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(DarkBowser, DemoAppear);
NERVE_DECL(DarkBowser, Battle);
NERVE_DECL(DarkBowser, DemoEnd);
NERVES_MAKE_NOSTRUCT(DarkBowser, DemoEnd)

// Nerves that also host nerve states are mutable globals (they get merged into one block).
DarkBowserNrvDemoAppear NrvDarkBowserDemoAppear;
DarkBowserNrvBattle NrvDarkBowserBattle;

typedef al::FunctorV0M<DarkBowser*, void (DarkBowser::*)()> DarkBowserFunctor;

/** @brief Sound effects that keep playing while the defeat cutscene starts. */
const char* sSeExceptList[] = {"SeBmSuperBowserHitBegin"};

/** @brief Ring beam parameters shared by all of Fury Bowser's ring beams. */
DarkBowserRingBeamParam sRingBeamParam(26.0f, 180, "Emit");
}  // namespace

static f32 getHitCountRate(const al::LiveActor* pActor);

/**
 * @brief Fills an actor group with newly created actors.
 * @param pGroup Group to fill up to its capacity.
 * @param rInfo Actor initialization information.
 * @param pName Name of every created actor.
 */
template <typename T>
static void createGroupActors(al::DeriveActorGroup<T>* pGroup, const al::ActorInitInfo& rInfo,
                              const char* pName) {
    for (s32 i = 0; i < pGroup->getMaxActorCount(); i++) {
        auto* actor = new T(pName);
        al::initCreateActorNoPlacementInfo(actor, rInfo);
        pGroup->registerActor(actor);
    }
}

/**
 * @brief Creates Fury Bowser with every spawn point and joint chain reset.
 * @param pName Actor name.
 */
DarkBowser::DarkBowser(const char* pName) : al::LiveActor(pName) {
    mHitPoint = 100;
    _3A4 = 100;
}

/**
 * @brief Sets up the phase, health, nerve states, child actors, cutscenes and collision filters.
 * @param rInfo Actor placement and scene initialization information.
 */
void DarkBowser::init(const al::ActorInitInfo& rInfo) {
    s32 phase = calcPhase(rInfo);
    al::initActorSuffix(this, rInfo, "Boss");
    al::invalidateShadow(this, "DropShell");

    if (phase == 4) {
        mIsV2 = SingleModeDataFunction::isDarkBowserV2Available(this);
    }

    if (mIsV2) {
        al::startMtpAnimAndSetFrameAndStop(this, "DarkBowserV2", 1.0f);
    }

    initBattleHealth(rInfo, phase);
    al::initNerve(this, &NrvDarkBowserDemoAppear, 3);
    mStateDemo = new DarkBowserStateDemo(this, rInfo);
    mBattle = new DarkBowserBattle(this, rInfo);
    al::initNerveState(this, mStateDemo, &NrvDarkBowserDemoAppear, "Demo");
    al::initNerveState(this, mBattle, &NrvDarkBowserBattle, "Battle");
    mIsDemoCancelled = SingleModeDataFunction::isDemoWasCancelled(this);
    al::setNerve(this, &NrvDarkBowserDemoAppear);
    makeActorAppeared();
    al::invalidateClipping(this);
    DarkBowserUtil::initDarkBowserJointControllers(this, rInfo, nullptr, mJointChainA,
                                                   mJointChainB, nullptr);
    jointRelease();
    resetHitSensors();
    createCutscenes(rInfo);
    calcPlayerSpawn(rInfo);
    createChildActors(rInfo);
    initGraphicsAndEffects(rInfo);
    initDemo(phase);
    mAudioDirector = rInfo.getAudioDirector();
    al::validateMaterialCode(this);
    al::setEffectFollowPosPtr(this, "HitGround", &mWaterSurfacePos);
    mEffectSystem = rInfo.getEffectSystemInfo()->getEffectSystem();

    if (phase == 3 && SingleModeDataFunction::isFirstPhase3BossDefeated(this)) {
        mPhaseName.format("Phase%d", 4);
    } else {
        mPhaseName.format("Phase%d", phase);
    }

    auto* moveLimitFilter = new al::CollisionPartsFilterSpecialPurpose("MoveLimit");
    auto* laserFilter = new al::CollisionPartsFilterSpecialPurpose("BowserLaser");
    mColliderFilter = new al::CollisionPartsFilterMergePair(moveLimitFilter, laserFilter);
    al::setColliderFilterCollisionParts(this, mColliderFilter);

    if (mIsSuperHardSky) {
        al::tryOnStageSwitch(this, "SwitchSuperHardSkyOn");
    }
}

/**
 * @brief Derives the battle phase from the unlocked story phase.
 * @param rInfo Actor initialization information (for the game data).
 * @return The phase to fight; the final boss phase counts as phase 4.
 */
s32 DarkBowser::calcPhase(const al::ActorInitInfo& rInfo) {
    s32 unlockedPhase =
        SingleModeDataFunction::getUnlockedPhase(rInfo.getActorSceneInfo().sceneObjHolder);
    mPhase = sead::Mathi::clamp(rc::phaseNumToInt(unlockedPhase), 1, 4);
    mIsFinalBattle = false;
    return unlockedPhase == rc::SingleModePhases::PHASE4_BOSS ? 4 : mPhase;
}

/**
 * @brief Creates the health bar and restores the hit points saved for the phase.
 * @param rInfo Actor initialization information.
 * @param phase Battle phase.
 */
void DarkBowser::initBattleHealth(const al::ActorInitInfo& rInfo, s32 phase) {
    mHealthBar = new SuperBowserHealthBar(
        this, *rInfo.getLayoutInitInfo(), getHitCountRate,
        mIsV2 ? SuperBowserHealthBar::HealthBarType_V2 :
                static_cast<SuperBowserHealthBar::HealthBarType>(phase != 1),
        al::getJointMtxPtr(this, "Face"), al::getJointMtxPtr(this, "ShellSpikes"), -1.0f, -1.0f);

    switch (phase) {
    case 1:
        mHitPoint = SingleModeDataFunction::getPhase1DarkBowserHitPoint(this);
        break;
    case 2:
        mHitPoint = SingleModeDataFunction::getPhase2DarkBowserHitPoint(this);
        break;
    case 3:
        mHitPoint = SingleModeDataFunction::getPhase3DarkBowserHitPoint(this);
        break;
    case 4:
        mHitPoint = SingleModeDataFunction::getPhase4DarkBowserHitPoint(this);
        break;
    default:
        break;
    }

    if (mHitPoint <= 0) {
        mHitPoint = phase == 1 ? 100 : 200;
    }

    if (mIsV2) {
        mHitPoint = 335;
    }

    mHealthStage = phase != 1 && (phase == 4 || mHitPoint <= 100);
}

/** @brief Releases both controlled joint chains. */
void DarkBowser::jointRelease() {
    DarkBowserUtil::jointChainRelease(mJointChainA);
    DarkBowserUtil::jointChainRelease(mJointChainB);
}

/** @brief Validates the body sensors and disables every attack sensor. */
void DarkBowser::resetHitSensors() {
    al::validateHitSensors(this);
    al::invalidateHitSensor(this, "ShellDive");
    al::invalidateHitSensor(this, "SoftBelly");
    al::invalidateHitSensor(this, "SideBelly");
    al::invalidateHitSensor(this, "ShellSpin");
    al::invalidateHitSensor(this, "Scratch");
    al::invalidateHitSensor(this, "Laser");
    al::invalidateHitSensor(this, "MouthLaser");
}

/**
 * @brief Creates the defeat (or retreat) cutscene and the final transformation cutscene.
 * @param rInfo Actor initialization information.
 */
void DarkBowser::createCutscenes(const al::ActorInitInfo& rInfo) {
    if (mIsFinalBattle) {
        mDemoCutscene =
            initCutscene(rInfo, "DefeatCutscenePhase4", "DemoFinalKoopaDefeat", false);
        mDemoCutscene->setHideActor(this);
        mDemoCutscene->setEndSceneFlag();
        return;
    }

    calcAndSetBaseMtx();
    mBaseMtx = *getBaseMtx();
    auto* cutsceneName = new sead::FixedSafeString<64>();
    bool isRetreat = isRetreatPhase();
    cutsceneName->format(isRetreat ? "RetreatCutscenePhase%i" : "DefeatCutscenePhase%i",
                         sead::Mathi::min(mPhase, 3));
    s32 finalCutsceneNum = al::calcLinkChildNum(rInfo, "FinalBowserTransformCutscene");

    if (al::calcLinkChildNum(rInfo, cutsceneName->cstr()) < 1) {
        return;
    }

    mDemoCutscene = initCutscene(rInfo, cutsceneName->cstr(), cutsceneName->cstr(), false);

    if (mDemoCutscene == nullptr) {
        return;
    }

    mDemoCutscene->setHideActor(this);
    mDemoCutscene->setHideKoopaJr(true);

    if (mPhase >= 3 && finalCutsceneNum >= 1 && mHealthStage != 0) {
        mDemoCutscene->setFollowedByDemo(true);
        mFinalCutscene = initCutscene(rInfo, "FinalBowserTransformCutscene",
                                      "FinalBowserTransform", true);
        mFinalCutscene->setEndSceneFlag();
        mFinalCutscene->setHideActor(this);
        mFinalCutscene->registerCancelDemoHook(
            DarkBowserFunctor(this, &DarkBowser::cancelFinalBowserTransform));
        mFinalCutscene->registerFrameHook(DarkBowserFunctor(this, &DarkBowser::setSkyToNight),
                                          0);
    } else {
        mDemoCutscene->setEndSceneFlag();
    }

    if (isRetreat) {
        mDemoCutscene->setCancelAudioFlag(true);
    }

    if (mFinalCutscene != nullptr) {
        mFinalCutscene->setCancelAudioFlag(true);
    }
}

/**
 * @brief Reads the player spawn points (and their Giga Bell placements) valid for this phase.
 * @param rInfo Actor initialization information.
 */
void DarkBowser::calcPlayerSpawn(const al::ActorInitInfo& rInfo) {
    mSpawnInfoNum = al::calcLinkChildNum(rInfo, "PlayerSpawnPos");

    if (mSpawnInfoNum < 1) {
        return;
    }

    al::PlacementInfo placementInfo;
    al::ActorInitInfo spawnInitInfo;
    s32 validNum = 0;

    for (s32 i = 0; i < mSpawnInfoNum; i++) {
        al::getLinksInfoByIndex(&placementInfo, rInfo, "PlayerSpawnPos", i);
        s32 layerId = al::tryGetLayerID(placementInfo);

        if (layerId != 0) {
            if (mPhase == 3 || mPhase == 4) {
                if (layerId >= 23) {
                    continue;
                }
            } else if (mPhase == 1) {
                if (layerId > 15) {
                    continue;
                }
            } else if (mPhase == 2) {
                if (layerId >= 19) {
                    continue;
                }
            } else {
                continue;
            }
        }

        SpawnInfo& spawnInfo = mSpawnInfos[validNum];
        al::getChildLinkT(&spawnInfo.mTrans, rInfo, "PlayerSpawnPos", i);
        const sead::Vector3f& trans = al::getTrans(this);
        spawnInfo.mFront.set(trans.x - spawnInfo.mTrans.x, 0.0f, trans.z - spawnInfo.mTrans.z);
        al::normalizeOrDirZ(&spawnInfo.mFront);
        spawnInitInfo.initNoViewId(&placementInfo, rInfo);
        al::tryGetArg(&spawnInfo.mHorizontalCameraAngle, spawnInitInfo, "HorizontalCameraAngle");
        al::tryGetArg(&spawnInfo.mVerticalCameraAngle, spawnInitInfo, "VerticalCameraAngle");

        if (al::calcLinkChildNum(spawnInitInfo, "GigaBellPos") != 0) {
            al::getChildLinkTQ(&spawnInfo.mGigaBellTrans, &spawnInfo.mGigaBellQuat, spawnInitInfo,
                               "GigaBellPos", 0);
        } else {
            spawnInfo.mGigaBellTrans = spawnInfo.mTrans;
        }

        validNum++;
    }

    mSpawnInfoNum = validNum;
}

/**
 * @brief Creates the ink puddles, fireballs, ink bombs, ring beams and Giga Bells.
 * @param rInfo Actor initialization information.
 */
void DarkBowser::createChildActors(const al::ActorInitInfo& rInfo) {
    mInkPuddleGroup = new al::DeriveActorGroup<InkPuddle>("InkPuddle", cInkPuddleNum);
    mFireballGroup = new al::DeriveActorGroup<SuperBowserFireFlame>("DarkBowserFireball", 48);
    mInkBombGroup = new al::DeriveActorGroup<InkBomb>("InkBomb", cInkBombNum);

    createGroupActors(mInkPuddleGroup, rInfo, "InkPuddle");
    createGroupActors(mFireballGroup, rInfo, "DarkBowserFireball");
    createGroupActors(mInkBombGroup, rInfo, "InkBomb");

    for (s32 i = 0; i < cInkBombNum; i++) {
        mInkBombGroup->getDeriveActor(i)->setHost(this);
    }

    sead::Vector3f ringBeamScale = sead::Vector3f::ones * 10.0f;

    for (s32 i = 0; i < cRingBeamNum; i++) {
        mRingBeams[i] = new DarkBowserRingBeam("DarkBowserRingBeam", this);
        al::initCreateActorNoPlacementInfo(mRingBeams[i], rInfo);
        mRingBeams[i]->setRingBeamParam(&sRingBeamParam);
        mRingBeams[i]->setBeamMode(1);
        al::setScale(mRingBeams[i], ringBeamScale);
    }

    if (mSpawnInfoNum < 1) {
        return;
    }

    mGigaBellGroup = new al::DeriveActorGroup<GigaBellItem>("GigaBellItem", mSpawnInfoNum);

    createGroupActors(mGigaBellGroup, rInfo, "GigaBellItem");

    for (s32 i = 0; i < mGigaBellGroup->getActorCount(); i++) {
        al::setScaleAll(mGigaBellGroup->getActor(i), 1.1f);
        al::invalidateClipping(mGigaBellGroup->getActor(i));
    }
}

/**
 * @brief Creates the skies, the rain effect, and the disaster mode layout.
 * @param rInfo Actor initialization information.
 */
void DarkBowser::initGraphicsAndEffects(const al::ActorInitInfo& rInfo) {
    if (al::calcLinkChildNum(rInfo, "SkyLake") != 0 &&
        al::calcLinkChildNum(rInfo, "SkyDisaster") != 0 &&
        al::calcLinkChildNum(rInfo, "SkyDisasterSuperHard") != 0) {
        if ((mPhase >= 3 && (mPhase == 4 || mHealthStage != 0)) || mIsV2) {
            mIsSuperHardSky = true;
            mSkyDisaster = new GameSkyProjection("SkyDisasterSuperHard");
            al::initLinksActor(mSkyDisaster, rInfo, "SkyDisasterSuperHard", 0);
        } else {
            mSkyDisaster = new GameSkyProjection("SkyDisaster");
            al::initLinksActor(mSkyDisaster, rInfo, "SkyDisaster", 0);
        }

        mSkyLake = new GameSkyProjection("SkyLake");
        al::initLinksActor(mSkyLake, rInfo, "SkyLake", 0);
        mSkyLake->kill();
        mSkyDisaster->appear();
    }

    if (al::calcLinkChildNum(rInfo, "RainEffectObj") >= 1) {
        mRainEffect = new al::EffectObjFollowCamera("EffectObjDarkBowserRain");
        al::initLinksActor(mRainEffect, rInfo, "RainEffectObj", 0);
        mRainEffect->startAppear();
    }

    al::setEffectFollowPosPtr(this, "ShellLandWarning", &mShellLandWarningPos);
    mDisasterModeLayout = al::createSimpleLayout("DisasterMode", "DisasterMode",
                                                 al::getLayoutInitInfo(rInfo), nullptr);
}

/**
 * @brief Registers the sky and fade hooks on the cutscenes.
 * @param phase Battle phase.
 */
void DarkBowser::initDemo(s32 phase) {
    al::ByamlIter fileIter;
    al::ByamlIter demoIter;

    if (al::tryGetActorInitFileIter(&fileIter, this, "InitDemo", nullptr)) {
        fileIter.tryGetIterByKey(&demoIter, "Demo");
        demoIter.tryGetIntByKey(&mFadeFrame, isRetreatPhase() ?
                                                 "Night To Day Transition Frame (Retreat)" :
                                                 "Night To Day Transition Frame (Defeat)");
    }

    if (phase < 3 || isRetreatPhase()) {
        mDemoCutscene->registerFrameHook(DarkBowserFunctor(this, &DarkBowser::setSkyToNight), 0);
        mDemoCutscene->registerFrameHook(DarkBowserFunctor(this, &DarkBowser::startDemoFade),
                                         mFadeFrame);
        mDemoCutscene->registerFrameHook(
            DarkBowserFunctor(this, &DarkBowser::endDemoFade),
            mFadeFrame +
                al::getActionFrameMax(mDisasterModeLayout, "DisasterStartInPlain", nullptr));
    }

    if (mFinalCutscene != nullptr) {
        mDemoCutscene->registerFrameHook(
            DarkBowserFunctor(this, &DarkBowser::preFinalBowserTransform), 1499);
        mDemoCutscene->registerEndDemoHook(
            DarkBowserFunctor(this, &DarkBowser::startFinalBowserTransform));
    }
}

/** @brief Registers the off-screen guide once every actor is placed. */
void DarkBowser::initAfterPlacement() {
    mBattle->registerGuideFrameOut();
}

/** @brief Appears and starts the appearance demo. */
void DarkBowser::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvDarkBowserDemoAppear);
    al::startMclAnim(this, "ColorEmission");
    al::setMclAnimFrameAndStop(this, 1.0f);
}

/** @brief Ends the health bar, removes Fury Bowser and ends the battle camera. */
void DarkBowser::kill() {
    mHealthBar->end();
    al::LiveActor::kill();
    mBattle->endCamera();
}

/** @brief Stops the battle music, applies gravity and keeps Fury Bowser above the floor. */
void DarkBowser::control() {
    if (mStopBgmTimer >= 0) {
        if (mStopBgmTimer-- == 0) {
            al::stopBgm(this, mPhaseName.cstr(), 5, -1);
        }
    }

    if (mIsApplyGravity) {
        f32 velocityScale;

        if (al::isOnGround(this, 0, 0.0f)) {
            velocityScale = 0.85f;
        } else {
            al::addVelocityToGravity(this, 14.0f);
            velocityScale = 0.98f;
        }

        al::scaleVelocity(this, velocityScale);
    }

    mShellLandWarningPos = al::getTrans(this);
    mBlurPos.set(mShellLandWarningPos.x, 2000.0f, mShellLandWarningPos.z);
    mShellLandWarningPos.y = 0.0f;

    sead::Vector3f trans = al::getTrans(this);

    if (trans.y < -3000.0f) {
        trans.y = 0.0f;
        al::resetPosition(this, trans, false);
    }
}

/**
 * @brief Passes messages to the battle state once the battle has started.
 * @param pMsg Received message.
 * @param pSelf Own sensor.
 * @param pOther Sensor of the sender.
 * @return Whether the message was handled.
 */
bool DarkBowser::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                            al::HitSensor* pOther) {
    if (mIsFinalHit) {
        return false;
    }

    if (al::isNerve(this, &NrvDarkBowserDemoAppear) && al::isMsgPlayerDisregard(pMsg)) {
        return true;
    }

    if (al::isNerve(this, &NrvDarkBowserBattle) && mBattle->receiveMsg(pMsg, pSelf, pOther)) {
        return true;
    }

    return false;
}

/**
 * @brief Pushes Bowser Jr. away from the body and passes other attacks to the battle state.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void DarkBowser::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mIsFinalHit) {
        return;
    }

    if (al::isSensorKoopaJr(pOther)) {
        if (al::isSensorEnemyBody(pSelf) || al::isSensorName(pSelf, "SoftBelly") ||
            al::isSensorName(pSelf, "SideBelly")) {
            al::sendMsgPushVeryStrong(pOther, pSelf);
            al::sendMsgGigaEnemyAttack(pOther, pSelf);
        }
    } else if (al::isSensorName(pSelf, "LeftArm") || al::isSensorName(pSelf, "RightArm") ||
               al::isSensorName(pSelf, "LeftWrist") || al::isSensorName(pSelf, "RightWrist") ||
               al::isSensorName(pSelf, "Head") || al::isSensorName(pSelf, "Tail1") ||
               al::isSensorName(pSelf, "Tail2") || al::isSensorName(pSelf, "Tail3") ||
               al::isSensorName(pSelf, "BodyDM") || al::isSensorName(pSelf, "BodyForKoopaJr")) {
        return;
    }

    if (al::isNerve(this, &NrvDarkBowserBattle)) {
        mBattle->attackSensor(pSelf, pOther);
    }
}

/** @brief Places the player and the Giga Bells, then plays the appearance demo. */
void DarkBowser::exeDemoAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::tryStartMtpAnimIfExist(this, "DarkBowser");
        mPlayer = al::findNearestPlayerActor(this);
        SingleModeDataFunction::resetBossPlayTime(this);

        if (mPlayer != nullptr) {
            setPlayerSpawnPos();
            rc::setStayInGigaMario(mPlayer, true);

            if (mSpawnInfoNum != 0) {
                al::faceToTarget(this, mSpawnInfos[mSpawnInfoIndex].mTrans);
            }

            mStateDemo->setBaseMtx(mPlayer->getBaseMtx());
        }

        setGigaBellPos();
    }

    al::updateNerveStateAndNextNerve(this, &NrvDarkBowserBattle);
}

/** @brief Moves the player to the spawn point closest to them. */
void DarkBowser::setPlayerSpawnPos() {
    if (mPlayer == nullptr || mSpawnInfoNum < 1) {
        return;
    }

    const sead::Vector3f& playerTrans = al::getTrans(mPlayer);
    f32 minDistance = sead::Mathf::maxNumber();
    s32 index = 0;

    for (s32 i = 0; i < mSpawnInfoNum; i++) {
        const SpawnInfo& spawnInfo = mSpawnInfos[i];
        f32 dx = playerTrans.x - spawnInfo.mTrans.x;
        f32 dz = playerTrans.z - spawnInfo.mTrans.z;
        f32 distance = dx * dx + dz * dz;

        if (distance < minDistance) {
            minDistance = distance;
            index = i;
        }
    }

    const SpawnInfo& spawnInfo = mSpawnInfos[index];
    mSpawnInfoIndex = index;
    rc::setPlayerTrans(mPlayer, spawnInfo.mTrans);
    rc::setPlayerFrontVec(mPlayer, spawnInfo.mFront);
}

/** @brief Places a Giga Bell at every spawn point, hiding the one at the player's spawn. */
void DarkBowser::setGigaBellPos() {
    bool isRespawn = !mIsV2;

    for (s32 i = 0; i < mSpawnInfoNum; i++) {
        GigaBellItem* gigaBell = mGigaBellGroup->getDeriveActor(i);
        const SpawnInfo& spawnInfo = mSpawnInfos[i];
        gigaBell->setIsRespawn(isRespawn);
        al::resetPosition(gigaBell, spawnInfo.mGigaBellTrans, false);
        al::setQuat(gigaBell, spawnInfo.mGigaBellQuat);

        if (i != mSpawnInfoIndex) {
            gigaBell->appearPopUpAboveSilent();
        } else if (mIsV2) {
            gigaBell->kill();
        } else {
            gigaBell->appearHidden();
        }
    }
}

/** @brief Starts the phase music and runs the battle until Fury Bowser is beaten. */
void DarkBowser::exeBattle() {
    if (al::isFirstStep(this)) {
        al::startBgm(this, mPhaseName.cstr(), -1, 0, -1, -1);
    }

    al::updateNerveStateAndNextNerve(this, &NrvDarkBowserDemoEnd);
}

/** @brief Freezes Fury Bowser after the final hit and starts the defeat cutscene. */
void DarkBowser::exeDemoEnd() {
    if (al::isFirstStep(this)) {
        al::startHitReaction(this, "FinalHit");
        mHealthBar->setEndAfterDamage();
        saveHitPoints();
        al::setSklAnimFrameRate(this, 0.0f, 0);
        al::setVelocityZero(this);
        mIsApplyGravity = false;

        for (s32 i = 0; i < cRingBeamNum; i++) {
            if (!al::isDead(mRingBeams[i])) {
                mRingBeams[i]->cutSoundEffects();
            }
        }

        auto* sceneLayout = al::tryGetSceneObj<SingleModeSceneLayout>(
            this, SceneObjID_SingleModeSceneLayout);

        if (sceneLayout != nullptr) {
            sceneLayout->startDemo(true, true);
        }

        return;
    }

    alSeFunction::stopAllSeExcept(mAudioDirector, 10, sSeExceptList, 1);

    if (al::isStep(this, 34)) {
        al::requestCaptureScreenCover(this, 4);

        if (rc::isActiveDemo(this)) {
            rc::requestEndDemoInGameCutscene(this);
        }

        rc::setPlayerTrans(mPlayer, sead::Vector3f(0.0f, 900000.0f, 0.0f));

        for (s32 i = 0; i < mGigaBellGroup->getActorCount(); i++) {
            mGigaBellGroup->getDeriveActor(i)->stopForCutscene();
        }

        return;
    }

    if (!al::isStep(this, 35)) {
        return;
    }

    forceKillObjects();

    if (!SingleModeDataFunction::beginPlayReport(this, this, static_cast<preport::KeyEventType>(21),
                                                 3, 0)) {
        SingleModeDataFunction::setPlayReportData(this, static_cast<preport::Key>(19),
                                                  SingleModeDataFunction::getIs2PAssistMode(this));
        SingleModeDataFunction::setPlayReportData(
            this, static_cast<preport::Key>(46),
            static_cast<s32>(SingleModeDataFunction::getBossPlayTime(this)));
        SingleModeDataFunction::setPlayReportData(this, static_cast<preport::Key>(47), 0);
        SingleModeDataFunction::endPlayReport(this);
    }

    if (mDemoCutscene != nullptr) {
        al::stopAllPadRumble(this);
        mDemoCutscene->setKillAllEffects(false);
        mDemoCutscene->startDemo();

        if (mRainEffect != nullptr) {
            mRainEffect->startAppear();
        }

        al::tryOffStageSwitchInstant(this, "SwitchNormalSkyOn");
        DarkBowserUtil::hideDarkBowser(this);
    } else {
        SingleModeDataFunction::setPhaseEnd(this, true);
    }

    kill();
}

/** @brief Saves the hit points the next battle of this phase starts with. */
void DarkBowser::saveHitPoints() const {
    s32 hitPoint = mHealthStage == 0 ? 100 : 0;

    s32 phase = SingleModeDataFunction::getUnlockedPhase(this) == rc::SingleModePhases::PHASE4_BOSS ?
                    4 :
                    mPhase;

    switch (phase) {
    case 1:
        SingleModeDataFunction::setPhase1DarkBowserHitPoint(this, hitPoint);
        break;
    case 2:
        SingleModeDataFunction::setPhase2DarkBowserHitPoint(this, hitPoint);
        break;
    case 3:
        SingleModeDataFunction::setPhase3DarkBowserHitPoint(this, hitPoint);
        break;
    case 4:
        SingleModeDataFunction::setPhase4DarkBowserHitPoint(this, hitPoint);
        break;
    }
}

/** @brief Kills every projectile and effect Fury Bowser left behind. */
void DarkBowser::forceKillObjects() {
    mInkPuddleGroup->killAll();
    mFireballGroup->killAll();
    mInkBombGroup->killAll();

    for (s32 i = 0; i < cRingBeamNum; i++) {
        mRingBeams[i]->kill();
    }

    mEffectSystem->getPtclSystem()->KillAllEmitterSet();

    if (mRainEffect != nullptr) {
        mRainEffect->startDisappear();
    }
}

/**
 * @brief Creates a cutscene from a placement link.
 * @param rInfo Actor initialization information.
 * @param pLinkName Name of the placement link the cutscene is placed with.
 * @param pName Cutscene actor name.
 * @param isUseBaseMtx Whether the cutscene is placed relative to Fury Bowser.
 * @return The cutscene, or nullptr if it is not placed.
 */
DemoCutscene* DarkBowser::initCutscene(const al::ActorInitInfo& rInfo, const char* pLinkName,
                                       const char* pName, bool isUseBaseMtx) const {
    if (al::calcLinkChildNum(rInfo, pLinkName) < 1) {
        return nullptr;
    }

    al::PlacementInfo placementInfo;
    al::getLinksInfoByIndex(&placementInfo, rInfo, pLinkName, 0);
    al::ActorInitInfo cutsceneInitInfo;
    cutsceneInitInfo.initNoViewId(&placementInfo, rInfo);
    auto demoType = static_cast<alSeFunction::DemoType>(isRetreatPhase() ? 2 : 0);
    auto* cutscene = new DemoCutscene(pName, demoType);
    cutscene->setPlacementBaseMtx(isUseBaseMtx ? &mBaseMtx : &sead::Matrix34f::ident);
    cutscene->init(cutsceneInitInfo);
    return cutscene;
}

/**
 * @brief Queues a hit reaction to start on the next collider update.
 * @param pName Hit reaction name.
 * @return False if another hit reaction is already queued.
 */
bool DarkBowser::requestHitReaction(const char* pName) {
    if (!mHitReactionName.isEmpty()) {
        return false;
    }

    mHitReactionName = pName;
    return true;
}

/**
 * @brief Emits a small radial blur.
 * @param pPos Blur center, or nullptr to use the default position above Fury Bowser.
 */
void DarkBowser::requestSmallBlur(const sead::Vector3f* pPos) {
    if (pPos != nullptr) {
        al::emitRadialBlur(this, pPos, 1200.0f, 5000.0f, 30, -1);
    } else {
        al::emitRadialBlur(this, mBlurPos, 1200.0f, 5000.0f, 30, -1);
    }
}

/**
 * @brief Emits a medium radial blur.
 * @param pPos Blur center, or nullptr to use the default position above Fury Bowser.
 */
void DarkBowser::requestMediumBlur(const sead::Vector3f* pPos) {
    if (pPos != nullptr) {
        al::emitRadialBlur(this, pPos, 13000.0f, 20000.0f, 90, -1);
    } else {
        al::emitRadialBlur(this, mBlurPos, 13000.0f, 20000.0f, 90, -1);
    }
}

/**
 * @brief Emits a large radial blur.
 * @param pPos Blur center, or nullptr to use the default position above Fury Bowser.
 */
void DarkBowser::requestLargeBlur(const sead::Vector3f* pPos) {
    if (pPos != nullptr) {
        al::emitRadialBlur(this, pPos, 15000.0f, 23000.0f, 45, -1);
    } else {
        al::emitRadialBlur(this, mBlurPos, 15000.0f, 23000.0f, 45, -1);
    }
}

/**
 * @brief Aims both joint chains at a target without interpolation.
 * @param rTarget Position to aim at.
 */
void DarkBowser::jointAim(const sead::Vector3f& rTarget) {
    DarkBowserUtil::jointChaimAimNoInterpolate(mJointChainA, rTarget);
    DarkBowserUtil::jointChaimAimNoInterpolate(mJointChainB, rTarget);
}

/**
 * @brief Sets how strongly both joint chains follow their aim.
 * @param rate Power rate.
 */
void DarkBowser::jointSetPower(f32 rate) {
    DarkBowserUtil::jointChainSetPowerRate(mJointChainA, rate);
    DarkBowserUtil::jointChainSetPowerRate(mJointChainB, rate);
}

/**
 * @brief Adds velocity while Fury Bowser stands on the ground.
 * @param rVelocity Velocity to add.
 */
void DarkBowser::requestAddVelocity(const sead::Vector3f& rVelocity) {
    if (al::isOnGround(this, 0, 0.0f)) {
        al::addVelocity(this, rVelocity);
    }
}

/** @brief Updates the position where Fury Bowser touches the water surface. */
void DarkBowser::updateWaterSurface() {
    mWaterSurfacePos = al::getTrans(this);
    al::AreaObj* waterArea = rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea, mWaterSurfacePos);

    if (waterArea == nullptr) {
        return;
    }

    al::AreaShape* areaShape = waterArea->getAreaShape();
    sead::Vector3f top = mWaterSurfacePos + al::getColliderRadius(this) * sead::Vector3f::ey;
    areaShape->checkArrowCollision(&mWaterSurfacePos, nullptr, top, mWaterSurfacePos);
}

/**
 * @brief Takes damage, stopping at the end of the current health bar section.
 * @param damage Damage to take.
 */
void DarkBowser::requestDamage(s32 damage) {
    mHitPoint -= damage;

    if (!mIsFinalBattle) {
        s32 minHitPoint = (sead::Mathi::clamp(mPhase, 0, 2) - mHealthStage - 1) * 100;

        if (mHitPoint <= minHitPoint) {
            if (mPhase == 4 || mIsV2) {
                mHitPoint = sead::Mathi::max(sead::Mathi::min(mHitPoint, 1000), 0);
            } else {
                mHitPoint = minHitPoint;
            }

            return;
        }
    }

    al::stopScene(this, 8, 4, true, false);
}

/** @brief Starts the final hit cutscene once. */
void DarkBowser::requestFinalHit() {
    if (mIsFinalHit) {
        return;
    }

    mStopBgmTimer = 0;
    mIsFinalHit = true;

    if (rc::isActiveDemo(this) && rc::isActiveDemoInGameCutscene(this)) {
        return;
    }

    if (rc::requestStartDemoInGameCutscene(this)) {
        rc::addDemoActor(this);
    }
}

/**
 * @brief Calculates how much of the health bar is left.
 * @return Remaining health between 0 and 1.
 */
f32 DarkBowser::getCurHitCountPercent() const {
    f32 hitPoint = mHitPoint;
    f32 rate;

    if (mIsV2) {
        rate = hitPoint / 335.0f;
    } else {
        rate = hitPoint / (sead::Mathi::clamp(mPhase, 1, 2) * 100);
    }

    return sead::Mathf::clamp(rate, 0.0f, 1.0f);
}

/** @brief Shows the health bar, starting its appear animation first if needed. */
void DarkBowser::showHealthBar() {
    if (!mHealthBar->isAlive()) {
        mHealthBar->startAppear(false);
    }

    mHealthBar->show();
}

/** @brief Hides the health bar. */
void DarkBowser::hideHealthBar() {
    mHealthBar->hide();
}

/**
 * @brief Moves the health bar to its normal or offset position.
 * @param isOffset Whether to use the offset position.
 */
void DarkBowser::setHealthBarState(bool isOffset) {
    mHealthBar->setState(static_cast<SuperBowserHealthBar::OffsetState>(isOffset), -1.0f);
}

/**
 * @brief Takes the next ink puddle and quickly ends the oldest one.
 * @return The ink puddle to use.
 */
InkPuddle* DarkBowser::getDeadInkPuddle() {
    InkPuddle* inkPuddle = mInkPuddleGroup->getDeriveActor(mInkPuddleIndex);
    mInkPuddleIndex = (mInkPuddleIndex + 1) % cInkPuddleNum;
    mInkPuddleGroup->getDeriveActor(mInkPuddleIndex)->quickEnd();
    return inkPuddle;
}

/**
 * @brief Finds an unused fireball.
 * @return An unused fireball, or the first one if all are in use.
 */
SuperBowserFireFlame* DarkBowser::getDeadFireball() {
    SuperBowserFireFlame* fireball = mFireballGroup->tryFindDeadDeriveActor();

    if (fireball != nullptr) {
        return fireball;
    }

    return mFireballGroup->getDeriveActor(0);
}

/**
 * @brief Finds an unused ink bomb.
 * @return An unused ink bomb, or the first one if all are in use.
 */
InkBomb* DarkBowser::getDeadInkBomb() {
    InkBomb* inkBomb = mInkBombGroup->tryFindDeadDeriveActor();

    if (inkBomb != nullptr) {
        return inkBomb;
    }

    return mInkBombGroup->getDeriveActor(0);
}

/**
 * @brief Finds an unused ring beam.
 * @return An unused ring beam, or the first one if all are in use.
 */
DarkBowserRingBeam* DarkBowser::getDeadRingBeam() const {
    for (s32 i = 0; i < cRingBeamNum; i++) {
        if (al::isDead(mRingBeams[i])) {
            return mRingBeams[i];
        }
    }

    return mRingBeams[0];
}

/** @brief Updates the collider, the water state, the floor material and queued hit reactions. */
void DarkBowser::updateCollider() {
    al::LiveActor::updateCollider();
    mIsInWater = rc::isInWaterArea(this);
    updateWaterSurface();

    if (!al::isOnGround(this, 0, 0.0f)) {
        al::setMaterialCode(this, "Air");
    } else if (mIsInWater && !al::isEqualString("Ink", al::getCollidedFloorMaterialCodeName(this))) {
        al::setMaterialCode(this, "Water");
    }

    if (!mHitReactionName.isEmpty()) {
        al::startHitReaction(this, mHitReactionName.cstr());
        mHitReactionName.clear();
    }
}

/** @brief Stops all audio when the final transformation cutscene is skipped. */
void DarkBowser::cancelFinalBowserTransform() {
    al::stopAllBgm(this, 30);
    alAudioSystemFunction::stopAllSeAfterDemoSkip(mAudioDirector, 30);
}

/** @brief Switches the sky to night. */
void DarkBowser::setSkyToNight() {
    DarkBowserUtil::setSkyToNight(this, mSkyLake, mSkyDisaster, false);
}

/**
 * @brief Health bar callback returning the remaining health.
 * @param pActor Fury Bowser.
 * @return Remaining health between 0 and 1.
 */
static f32 getHitCountRate(const al::LiveActor* pActor) {
    return static_cast<const DarkBowser*>(pActor)->getCurHitCountPercent();
}

/** @brief Starts fading in the disaster mode layout. */
void DarkBowser::startDemoFade() {
    mDisasterModeLayout->appear();
    al::tryStartAction(mDisasterModeLayout, "DisasterStartInPlain", nullptr);
}

/** @brief Switches the sky back to day, stops the rain and fades out the disaster layout. */
void DarkBowser::endDemoFade() {
    DarkBowserUtil::setSkyToDay(this, mSkyLake, mSkyDisaster, false);
    al::tryOnStageSwitchInstant(this, "RainOff");

    if (mRainEffect != nullptr) {
        mRainEffect->startDisappear();
    }

    al::tryOnStageSwitchInstant(this, "SwitchNormalSkyOn");
    al::tryOffStageSwitchInstant(mDemoCutscene, "SwitchAppearOn");
    al::tryStartAction(mDisasterModeLayout, "DisasterStartOutPlain", nullptr);
}

/** @brief Covers the screen right before the final transformation cutscene. */
void DarkBowser::preFinalBowserTransform() {
    al::requestCaptureScreenCover(this, 4);
}

/** @brief Starts the final transformation cutscene after the defeat cutscene. */
void DarkBowser::startFinalBowserTransform() {
    mDemoCutscene->cancelSE();
    al::requestCaptureScreenCover(this, 4);
    al::startBgm(this, "FinalBowserTransform", -1, 374, -1, -1);
    mFinalCutscene->setKillAllEffects(false);
    mFinalCutscene->startDemo();

    if (mDisasterModeLayout->isAlive()) {
        mDisasterModeLayout->kill();
    }
}
