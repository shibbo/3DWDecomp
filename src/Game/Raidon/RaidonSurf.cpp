#include "Raidon/RaidonSurf.hpp"

#include <gfx/seadCamera.h>
#include <math/seadMathCalcCommon.h>
#include <prim/seadSafeString.h>

#include "Boss/SuperBowser.hpp"
#include "Camera/CameraPoserFollowLimit.hpp"
#include "Camera/DummyCameraTarget.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/Audio/System/AudioKeeper.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraPoserFunction.hpp"
#include "Library/Camera/CameraTicket.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/SePlayObj.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "NPC/IUseNekoModeActor.hpp"
#include "NPC/NpcFunction.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/WaterUtil.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/OceanWave/OceanWaveUserInfo.hpp"
#include "Raidon/RaidonPuppeteer.hpp"
#include "Raidon/RaidonSurfAnimState.hpp"
#include "Raidon/RaidonSurfStartState.hpp"
#include "Raidon/RaidonSurfWaitState.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
/// A nerve whose execute function is shared with another nerve.
#define RAIDON_SURF_NERVE_DECL_(Action, ActionFunc)                                                \
    class RaidonSurfNrv##Action : public al::Nerve {                                               \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<RaidonSurf>()->exe##ActionFunc();                                   \
        }                                                                                          \
    };

NERVE_DECL(RaidonSurf, Wait)
NERVE_DECL(RaidonSurf, WaitInWater)
NERVE_DECL(RaidonSurf, WaitInWaterReaction)
NERVE_DECL(RaidonSurf, FirstSeenDemo)
NERVE_DECL(RaidonSurf, Start)
NERVE_DECL(RaidonSurf, Ride)
NERVE_DECL(RaidonSurf, GetOn)
NERVE_DECL(RaidonSurf, End)
NERVE_DECL(RaidonSurf, CollectItem)
RAIDON_SURF_NERVE_DECL_(DeSpawnForce, DeSpawn)
NERVE_DECL(RaidonSurf, GetOnInWater)
NERVE_DECL(RaidonSurf, Rebind)
NERVE_DECL(RaidonSurf, GetOff)
NERVE_DECL(RaidonSurf, Goal)
NERVE_DECL(RaidonSurf, GetOffOnWater)
NERVE_DECL(RaidonSurf, Abyss)
NERVE_DECL(RaidonSurf, DelayWaitInWater)
NERVE_DECL(RaidonSurf, Land)
NERVE_DECL(RaidonSurf, Fall)
RAIDON_SURF_NERVE_DECL_(DeSpawnEnd, Death)
RAIDON_SURF_NERVE_DECL_(DieLava, Die)
NERVE_DECL(RaidonSurf, Death)
NERVE_DECL(RaidonSurf, Spawn)
NERVE_DECL(RaidonSurf, DeSpawn)
NERVE_DECL(RaidonSurf, Die)
NERVE_DECL(RaidonSurf, GetOffInAir)
NERVES_MAKE_NOSTRUCT(RaidonSurf, Wait, WaitInWater, WaitInWaterReaction, FirstSeenDemo, Start, Ride,
                     GetOn, End, CollectItem, DeSpawnForce, GetOnInWater, Rebind, GetOff, Goal,
                     GetOffOnWater, Abyss, DelayWaitInWater, Land, Fall, DeSpawnEnd, DieLava, Death,
                     Spawn, DeSpawn, Die, GetOffInAir)

/// Joints the riding players are attached to, by seat index.
const char* const cPlayerJointNames[] = {"Player1", "Player2", "Player3", "Player4"};

/// Effects emitted where the riding cats leave Plessie, by seat index.
const char* const cNekoPoofEffectNames[] = {"NekoRidePoof1", "NekoRidePoof2", "NekoRidePoof3"};

/// Joints the riding NPCs are attached to, by seat index.
const char* const cNpcJointNames[] = {"Player2", "Player3", "Player4"};

/// Blend weights of the five-way input animation (neutral, -X, +X, +Y, -Y).
struct InputBlendWeight {
    f32 neutral;
    f32 minusX;
    f32 plusX;
    f32 plusY;
    f32 minusY;

    /** @brief Splits a two-axis input into the five blend weights.
     * @param x Horizontal input.
     * @param y Vertical input.
     */
    InputBlendWeight(f32 x, f32 y) {
        f32 sum = sead::Mathf::abs(x) + sead::Mathf::abs(y);
        if (sum > 1.0f) {
            x /= sum;
            y /= sum;
            sum = 1.0f;
        }

        neutral = 1.0f - sum;
        minusX = x < 0.0f ? -x : 0.0f;
        plusX = sead::Mathf::max(x, 0.0f);
        plusY = sead::Mathf::max(y, 0.0f);
        minusY = y < 0.0f ? -y : 0.0f;
    }
};

/**
 * @param pPoser Camera poser to check.
 * @return Whether the poser is one of the follow camera posers.
 */
inline bool isFollowCameraPoser(const al::CameraPoser_RS* pPoser) {
    return al::isEqualString(pPoser->getName(), "Follow") ||
           al::isEqualString(pPoser->getName(), "Parallel") ||
           al::isEqualString(pPoser->getName(), "Parallel2D");
}

/**
 * @param pActor Actor checking the water areas.
 * @param rStart Start of the checked line.
 * @param rEnd End of the checked line.
 * @return Whether the line crosses a water area.
 */
inline bool isInWaterAreaBetween(const al::LiveActor* pActor, const sead::Vector3f& rStart,
                                 const sead::Vector3f& rEnd) {
    sead::Vector3f hitPos;
    sead::Vector3f hitNormal;
    return rc::isInWaterArea(pActor, rStart, rEnd, &hitPos, &hitNormal);
}

/**
 * @param pActor Actor whose player water effect is turned on again.
 * @param pHolder Player holder of the scene.
 */
inline void validateFirstPlayerWaterEffect(al::LiveActor* pActor, const al::PlayerHolder* pHolder) {
    if (pHolder != nullptr) {
        if (al::LiveActor* player = al::tryFindAlivePlayerActorFirst(pActor)) {
            rc::validatePlayerWaterEffect(player);
        }
    }
}

/**
 * @param pActor Actor whose player water effect is turned off.
 * @param pHolder Player holder of the scene.
 */
inline void invalidateFirstPlayerWaterEffect(al::LiveActor* pActor,
                                             const al::PlayerHolder* pHolder) {
    if (pHolder != nullptr) {
        if (al::LiveActor* player = al::tryFindAlivePlayerActorFirst(pActor)) {
            rc::invalidatePlayerWaterEffect(player);
        }
    }
}
}  // namespace

/** @return The follow camera poser of the current camera, if one is active. */
inline CameraPoserFollowLimit* RaidonSurf::tryGetFollowCameraPoser() const {
    al::CameraTicket* ticket = mActorSceneInfo->cameraDirector->getCurrentTicket();
    if (ticket == nullptr || ticket->getPoser() == nullptr) {
        return nullptr;
    }

    al::CameraPoser_RS* poser = ticket->getPoser();
    if (!isFollowCameraPoser(poser)) {
        return nullptr;
    }

    return static_cast<CameraPoserFollowLimit*>(poser);
}

/** @brief Lets the riding NPCs get off once all players stand on the ground nearby. */
inline void RaidonSurf::tryEndNpcPuppetBindNearPlayer() {
    if (mIsNpcPuppetBinding && mNpcPuppets.size() != 0 && rc::isAllPlayerOnGround(this) &&
        al::isNearPlayer(this, 1200.0f)) {
        endNpcPuppetBindAll(static_cast<NpcPuppetBindEndType>(0));
    }
}

/** @param pName Actor name. */
RaidonSurf::RaidonSurf(const char* pName)
    : RaidonBase(pName), mTrampleComboCounter(new al::ComboCounter()),
      mInvincibleComboCounter(new al::ComboCounter()) {
    mNpcPuppets.allocBuffer(3, nullptr);
}

/**
 * @brief Reads a spawn point from the placement; the first starting point is kept separately.
 * @param rInfo Placement of the spawn point.
 * @return Whether the spawn point was added.
 */
bool RaidonSurf::addSpawnPoint(const al::ActorInitInfo& rInfo) {
    s32 islandId;
    s32 scenarioId;
    sead::Vector3f trans;
    bool isTunnel = false;
    const al::PlacementInfo& rPlacementInfo = rInfo.getPlacementInfo();
    al::tryGetTrans(&trans, rPlacementInfo);

    bool isStartingPoint = false;
    al::tryGetArg(&islandId, rInfo, "IslandID");
    al::tryGetArg(&scenarioId, rInfo, "ScenarioID");
    al::tryGetArg(&isStartingPoint, rInfo, "StartingPoint");
    al::tryGetArg(&isTunnel, rInfo, "IsTunnel");
    if (islandId > 0 && scenarioId > 0) {
        SingleModeDataFunction::isScenarioComplete(this, islandId - 1, scenarioId - 1);
    }

    if (mStartingPoint == nullptr && isStartingPoint) {
        mStartingPoint = new SpawnPoint();
        al::tryGetTrans(&mStartingPoint->trans, rPlacementInfo);
        al::tryGetFront(&mStartingPoint->front, rPlacementInfo);
        return true;
    }

    if (mSpawnPoints.isFull()) {
        return false;
    }

    SpawnPoint* spawnPoint = mSpawnPoints.emplaceBack();
    al::tryGetTrans(&spawnPoint->trans, rPlacementInfo);
    al::tryGetFront(&spawnPoint->front, rPlacementInfo);
    spawnPoint->islandId = islandId;
    spawnPoint->scenarioId = scenarioId;
    spawnPoint->isTunnel = isTunnel;
    spawnPoint->isEnable = false;
    return true;
}

/** @brief Makes the camera follow Plessie through the dummy camera target. */
void RaidonSurf::startFollowCamera() {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        IUsePlayerPuppet* puppet = mPuppeteers[i].mPuppet;
        if (puppet != nullptr) {
            sead::Vector3f trans = rc::getPuppetTrans(puppet);
            trans.y += 170.0f;
            mCameraTarget->setTrans(trans);
            break;
        }
    }

    mCameraTarget->onTarget();
    mCameraTarget->setRequestDistance(1500.0f);

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    mCameraTarget->setRotateY(sead::Mathf::rad2deg(atan2f(frontDir.x, frontDir.z)));
    al::setCameraReset(this, false);
    mCameraStopStep = 0.0f;
    mIsCameraBetweenLegs = false;
    mIsCameraLimitAngle = false;
    mActorSceneInfo->cameraDirector->setPlessieCamera(true);
}

/** @brief Moves the camera target and adjusts the follow camera to the situation. */
void RaidonSurf::updateFollowCamera() {
    CameraPoserFollowLimit* poser = tryGetFollowCameraPoser();
    if (poser == nullptr) {
        return;
    }

    if (mCameraArea != nullptr &&
        getCameraDirector_RS()->findCameraAreaTicket(mCameraArea)->getPoser() != poser) {
        al::CameraPoser_RS* areaPoser =
            getCameraDirector_RS()->findCameraAreaTicket(mCameraArea)->getPoser();
        areaPoser->setEye(poser->getEye());
        areaPoser->setAt(poser->getAt());
    }

    sead::Vector3f targetTrans = al::getTrans(this);
    bool isUnderwater = false;
    if (isInWater() && targetTrans.y <= mWaterSurfaceY + -100.0f) {
        targetTrans.y = mWaterSurfaceY + -100.0f + 170.0f;
        isUnderwater = true;
    } else if (mIsCameraLimitAngle) {
        f32 rate = 1.0f;
        if (mCameraDistanceStep < 120) {
            rate = mCameraDistanceStep++ / 60;
        }

        mCameraDistance =
            al::lerpValue(0.05f, mCameraDistance, (1.0f - rate) * 500.0f + rate * 800.0f);
        targetTrans.y += mCameraDistance;
    } else {
        targetTrans.y += 170.0f;
    }

    mCameraTarget->setTrans(targetTrans);
    f32 distance = poser->getVerticalAngle();
    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    f32 speedH = sead::Mathf::sqrt(pVelocity->x * pVelocity->x + pVelocity->z * pVelocity->z);
    if (speedH < 30.0f) {
        if (mCameraStopStep < 90.0f) {
            mCameraStopStep += 1.0f;
        }
    } else {
        mCameraStopStep = 0.0f;
    }

    if (!mIsCameraBetweenLegs) {
        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr && controller->isDisasterMode() &&
            controller->getSuperBowser()->isPlayerInBetweenLegsArea()) {
            mIsCameraBetweenLegs = true;
            mHiDegreeLimit = distance;
            mActorSceneInfo->cameraDirector->freezeCameraInput(true);
            if (mHiDegreeLimit > 0.0f) {
                poser->setHiDegreeLimit(0.0f);
            }
        }
    }

    bool isRequestDistance = false;
    if (rc::isInPlessieChaseV2SpecialCamera(this, al::getTrans(this))) {
        if (!mIsCameraLimitAngle) {
            DisasterModeController* controller = DisasterModeController::tryGetController(this);
            if (controller != nullptr &&
                controller->getSuperBowser()->isPlessieChaseV2SpecialAttack()) {
                mCameraDistance = 170.0f;
                mCameraDistanceStep = 0;
                isRequestDistance = true;
                mIsChaseSpecialCamera = true;
                mChaseSpecialCameraTimer = 75;
                poser->setTargetAngleV(10.0f, 45);
                mIsCameraLimitAngle = isRequestDistance;
                distance = 2000.0f;
            }
        } else if (mIsChaseSpecialCamera) {
            DisasterModeController* controller = DisasterModeController::tryGetController(this);
            if (controller != nullptr &&
                controller->getSuperBowser()->isPlessieChaseV2SpecialAttack()) {
                isRequestDistance = true;
                distance = 2000.0f;
            } else if (mChaseSpecialCameraTimer != 0) {
                mChaseSpecialCameraTimer--;
                isRequestDistance = true;
                mIsCameraLimitAngle = isRequestDistance;
                distance = 2000.0f;
            } else {
                isRequestDistance = false;
                mIsCameraLimitAngle = false;
            }
        }
    } else {
        mIsCameraLimitAngle = false;
        sead::Vector3f cameraPos = al::getCameraPos_RS(this, 0);
        poser->clearLowDegreeLimit();
        if (!mIsCameraBetweenLegs) {
            if (isInWaterAreaBetween(this, cameraPos, targetTrans) || isOnDiveJump()) {
                if (poser->getVerticalAngle() < 12.0f) {
                    poser->setTargetAngleV(12.0f);
                }

                poser->setLowDegreeLimit(12.0f);
                isRequestDistance = true;
            } else {
                sead::Vector3f dir = targetTrans - cameraPos;
                al::CollisionPartsFilterActor filter(this);
                isRequestDistance =
                    alCollisionUtil::checkStrikeArrow(this, cameraPos, dir, &filter, nullptr) != 0 &&
                    speedH > 20.0f;
            }
        }

        if (mIsCameraBetweenLegs) {
            DisasterModeController* controller = DisasterModeController::tryGetController(this);
            if (controller == nullptr || !controller->isDisasterMode() ||
                !controller->getSuperBowser()->isPlayerInBetweenLegsArea()) {
                mActorSceneInfo->cameraDirector->freezeCameraInput(false);
                mIsCameraBetweenLegs = false;
                poser->clearHiDegreeLimit();
            }

            distance = 400.0f;
            isRequestDistance = true;
        } else if (mIsAnyPuppetJump) {
            distance = 1800.0f;
            if (isUnderwater && poser->getVerticalAngle() < -5.0f) {
                poser->setTargetAngleV(-5.0f);
            }
        } else if (mDashTimer > 0) {
            distance = 3000.0f;
        } else {
            f32 rate = (distance + -15.0f) / 75.0f;
            f32 currentAngleV = poser->getVerticalAngle();
            f32 baseDistance = rate * 3000.0f + (1.0f - rate) * 1500.0f;
            sead::Vector3f checkPos = al::getTrans(this);
            checkPos.y += -15.0f;
            distance = baseDistance;
            if (!rc::isInWaterArea(this, checkPos)) {
                if (!isOnGroundRaidon() && currentAngleV <= 0.0f && speedH < 5.0f) {
                    distance = 550.0f;
                    isRequestDistance = true;
                } else if (speedH < 30.0f) {
                    isRequestDistance = true;
                    f32 stopRate = mCameraStopStep / 90.0f;
                    distance = stopRate * 750.0f + (1.0f - stopRate) * baseDistance;
                }
            }
        }
    }

    f32 waterHeight;
    if (al::calcWaterDistanceCheck(this, targetTrans, -1000.0f, &waterHeight)) {
        poser->setWaterHeight(waterHeight);
    }

    poser->setPauseMoveLimit(!mIsInCameraHeightLimitArea);
    setForceRequestDistance(isRequestDistance);
    mCameraTarget->setRequestDistance(distance);
}

/** @return Whether Plessie is diving out of the water with a jump. */
bool RaidonSurf::isOnDiveJump() {
    if (al::isNerve(this, &NrvRaidonSurfRide)) {
        return mAnimState->isDiveJump();
    }

    return false;
}

/** @param isForce Whether the follow camera must use the requested distance. */
void RaidonSurf::setForceRequestDistance(bool isForce) {
    if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
        poser->setPriorRequestDistance(isForce);
    }
}

/** @brief Keeps the camera above the water while Plessie dives. */
void RaidonSurf::updateFollowDiveCamera() {
    sead::Vector3f targetTrans = al::getTrans(this);
    if (isInWater() && targetTrans.y < mWaterSurfaceY + -100.0f) {
        targetTrans.y = al::getTrans(mCameraTarget).y;
    } else {
        targetTrans.y += 170.0f;
    }

    mCameraTarget->setTrans(targetTrans);
    sead::Vector3f cameraPos = al::getCameraPos_RS(this, 0);
    if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
        if (rc::isInWaterArea(this, cameraPos)) {
            poser->setTargetAngleV(12.0f);
        } else if (poser->getVerticalAngle() < 0.0f) {
            poser->setTargetAngleV(0.0f);
        }

        if (mCameraTarget->getRequestDistance() > 1800.0f) {
            mCameraTarget->setRequestDistance(1000.0f);
        }
    }
}

/** @brief Gives the camera back to the players. */
void RaidonSurf::stopFollowCamera() {
    setPlessieMode(false);
    mCameraTarget->offTarget();
    mCameraTarget->setRequestDistance(-1.0f);
    if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
        poser->clearHiDegreeLimit();
        poser->clearLowDegreeLimit();
        poser->setPauseMoveLimit(false);
    }

    if (mIsCameraBetweenLegs) {
        mActorSceneInfo->cameraDirector->freezeCameraInput(false);
    }

    mIsCameraBetweenLegs = false;
    mIsCameraLimitAngle = false;
    mActorSceneInfo->cameraDirector->setPlessieCamera(false);
}

/** @param isPlessie Whether the follow camera uses its Plessie angle range. */
void RaidonSurf::setPlessieMode(bool isPlessie) {
    if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
        if (isPlessie) {
            poser->setPlessieCameraOn(-15.0f, 90.0f);
        } else {
            poser->setPlessieCameraOff();
        }
    }
}

/** @brief Makes the follow camera keep the requested distance over water. */
void RaidonSurf::turnOnWaterCameraDistance() {
    if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
        poser->setPriorRequestDistance(true);
    }
}

/** @brief Lets the follow camera choose its distance again. */
void RaidonSurf::turnOffWaterCameraDistance() {
    if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
        poser->setPriorRequestDistance(false);
    }
}

/** @param isFriction Whether the follow camera freezes its horizontal angle. */
void RaidonSurf::updateCameraAngleFriction(bool isFriction) {
    if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
        poser->setPlessieFreezeAngleH(isFriction);
    }
}

/** @return Whether Plessie waits for a player to get on. */
bool RaidonSurf::isWaiting() {
    return (al::isNerve(this, &NrvRaidonSurfWait) && isOnGroundRaidon()) ||
           al::isNerve(this, &NrvRaidonSurfWaitInWater) ||
           al::isNerve(this, &NrvRaidonSurfWaitInWaterReaction) ||
           al::isNerve(this, &NrvRaidonSurfFirstSeenDemo);
}

/**
 * @brief Creates the states, the camera target, the collision filters and the spawn points.
 * @param rInfo Actor init info.
 */
void RaidonSurf::init(const al::ActorInitInfo& rInfo) {
    s32 phase = SingleModeDataFunction::getUnlockedPhase(rInfo.getActorSceneInfo().sceneObjHolder);
    RaidonActor::init(rInfo, phase > 7 ? "RaidonSurfFur" : "RaidonSurf");
    al::setSceneObj(this, this, SceneObjID_RaidonSurf);
    mIsPlessieChase = rc::isPlessieChase(phase);
    bool isSeenCutscene = SingleModeDataFunction::hasSeenCutscene(this, 14);
    if ((phase == 3 && !isSeenCutscene) || mIsPlessieChase) {
        SingleModeDataFunction::resetPlayRidon(GameDataHolderAccessor(this));
        mIsFirstRide = true;
    } else {
        SingleModeDataFunction::setPlayRidon(GameDataHolderAccessor(this));
        mIsFirstRide = false;
    }

    if (mIsPlessieChase) {
        mIsNoGuideWindow = true;
    }

    mPadRumbleKeeper = al::createPadRumbleKeeper(this, 0);
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    al::setEffectFollowMtxPtr(this, "ScreenWet", &mScreenWetMtx);
    rc::createInvincibleUbo(this);
    al::hideInvincibleModel(this);
    mBaseQuat = al::getQuat(this);
    mBaseTrans = al::getTrans(this);
    al::calcFrontDir(&mBaseFrontDir, this);
    al::calcSideDir(&mBaseSideDir, this);

    mPuppeteerNumMax = 1;
    mPuppeteers = new RaidonPuppeteer[mPuppeteerNumMax];
    for (s32 i = 0; i < mPuppeteerNumMax; i++) {
        mPuppeteers[i].init(rInfo);
    }

    mStateSupportStroke = new ActorStateSupportStroke(this);
    mStateSupportStroke->resetFlags();
    mStateSupportStroke->appear();

    mWaitState = new RaidonSurfWaitState("WaitState", this, mStateSupportStroke);
    mStartState = new RaidonSurfStartState("StartState", this, rInfo);
    mAnimState = new RaidonSurfAnimState("RideState", this);
    mFirstSeenState = new RaidonSurfWaitState("FirstSeenState", this, nullptr);

    al::initNerve(this, &NrvRaidonSurfWait, 6);
    al::initNerveState(this, mWaitState, &NrvRaidonSurfWait, "WaitState");
    al::initNerveState(this, mStartState, &NrvRaidonSurfStart, "StartState");
    al::initNerveState(this, mAnimState, &NrvRaidonSurfRide, "RideState");
    al::initNerveState(this, mFirstSeenState, &NrvRaidonSurfFirstSeenDemo, "FirstSeenState");
    offSpringControl();

    mFallAreaChecker = new al::AudioGeneralPurposeAreaChecker("RaidonFallTriggeredArea");
    mFallAreaChecker->init(getAreaObjDirector());
    mFallAreaChecker->setPlayerHolder(rInfo.getActorSceneInfo().playerHolder);
    mColliderOffsetY = al::getColliderOffsetY(this);
    makeActorAppeared();
    mPlayerHolder = rInfo.getActorSceneInfo().playerHolder;

    mSpawnPoints.allocBuffer(800, nullptr);
    addSpawnPoint(rInfo);
    al::tryGetArg(&mRespawnWaitTime, rInfo, "RespawnWaitTime");
    if (mRespawnWaitTime < 0) {
        mRespawnWaitTime = 3;
    }

    mCameraArea = al::tryFindPlessieAreaObj(this, "CameraArea");
    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }

    mIsRespawnRequested = false;
    mLife = 7;
    al::invalidateClipping(this);

    auto* inkLimitFilter = new al::CollisionPartsFilterSpecialPurpose("InkLimit");
    mPlayerOnlyFilter = new al::CollisionPartsFilterMergePair(
        new al::CollisionPartsFilterSpecialPurpose("PlayerOnly"),
        new al::CollisionPartsFilterSpecialPurpose("RaidonOnly"));
    auto* transparentWallFilter = new TransparentWallFilter(mPlayerOnlyFilter);
    mTransparentWallFilter = transparentWallFilter;
    mColliderFilter = new al::CollisionPartsFilterMergePair(inkLimitFilter, transparentWallFilter);
    al::setColliderFilterCollisionParts(this, mColliderFilter);
    mTriangleFilter = new TriangleFloorFilter();
    al::setColliderFilterTriangle(this, mTriangleFilter);

    mRainActor = new al::LiveActor("RainActor");
    al::initActorWithArchiveName(mRainActor, rInfo, "RaidonRain", nullptr);
    mRainActor->makeActorDead();

    mCameraTarget = new DummyCameraTarget("PuppetDummy");
    mCameraTarget->init(rInfo);
    mCameraTarget->setFollowExact(true);

    mSoundActor = new SePlayObj("PlessieSoundActor");
    mSoundActor->initAttached(rInfo);
    al::updatePoseMtx(mSoundActor, getBaseMtx());
    al::invalidateClipping(mSoundActor);

    mBindSensorRadius = al::getSensorRadius(this, "Bind");
    mBindSensorOffset = al::getSensorFollowPosOffset(this, "Bind");
}

/** @brief Enables the spawn points outside tunnels and starts waiting. */
void RaidonSurf::initAfterPlacement() {
    updateSpawns(false);
    if (mIsFirstRide && mStartingPoint != nullptr) {
        if (mIsPlessieChase) {
            mFirstSeenState->setFirstSeen();
        }

        al::setNerve(this, &NrvRaidonSurfFirstSeenDemo);
    } else {
        al::setNerve(this, &NrvRaidonSurfWaitInWater);
    }
}

/** @param isTunnel Whether the spawn points inside tunnels are enabled instead of the others. */
void RaidonSurf::updateSpawns(bool isTunnel) {
    for (s32 i = 0; i < mSpawnPoints.size(); i++) {
        SpawnPoint* spawnPoint = mSpawnPoints[i];
        s32 islandIndex = spawnPoint->islandId - 1;
        bool isTunnelSpawn = spawnPoint->isTunnel;
        bool isUnlocked = true;
        if (islandIndex >= 0) {
            s32 scenarioIndex = spawnPoint->scenarioId - 1;
            if (scenarioIndex >= 0) {
                isUnlocked =
                    SingleModeDataFunction::isScenarioComplete(this, islandIndex, scenarioIndex);
            }
        }

        spawnPoint->isEnable = isUnlocked && isTunnel == isTunnelSpawn;
    }
}

/** @brief Pushes, tramples or attacks what Plessie runs into.
 * @param pSelf Plessie's sensor.
 * @param pOther Touched sensor.
 */
void RaidonSurf::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorMapObj(pSelf)) {
        if (al::isNerve(this, &NrvRaidonSurfWait) || al::isNerve(this, &NrvRaidonSurfGetOn) ||
            al::isSensorKoopaJr(pOther)) {
            if (al::sendMsgPush(pOther, pSelf)) {
                return;
            }
        }
    }

    if (!al::isSensorRide(pSelf)) {
        return;
    }

    if (al::isSensorMapObj(pOther)) {
        if (!rc::isSensorKinopioBrigadeNpc(pOther) && al::sendMsgPush(pOther, pSelf)) {
            return;
        }

        if (rc::sendMsgBobsledBodyAttack(pOther, pSelf)) {
            return;
        }
    }

    if (al::isSensorName(pSelf, "Body") || al::isSensorName(pSelf, "Head")) {
        if (al::isSensorEnemy(pOther)) {
            if (mFirstPlayerSensor == nullptr && al::sendMsgPush(pOther, pSelf)) {
                return;
            }
        } else if (al::isSensorPlayer(pOther)) {
            f32 playerY = al::getPlayerPos(al::getSensorHost(pOther), 0).y;
            f32 transY = al::getTrans(this).y;
            if (!isGetOffNerve() && playerY - transY < 200.0f && al::sendMsgPush(pOther, pSelf)) {
                return;
            }
        }
    }

    if (al::sendMsgPlayerGiantTouch(pOther, pSelf)) {
        if (rc::sendMsgRaidonBreakLightReaction(pOther, pSelf)) {
            playHitReactionHitEffect(this, "キック甲羅ヒット[反射]", pOther, pSelf);
        }

        return;
    }

    if (al::isSensorEnemy(pOther)) {
        if (isOnGroundOrWaterRaidon()) {
            if (rc::sendMsgBobsledBodyAttack(pOther, pSelf) ||
                al::sendMsgPlayerFireBallAttack(pOther, pSelf)) {
                playHitReactionHitEffect(this, "キック甲羅ヒット[反射]", pOther, pSelf);
                return;
            }

            if (al::sendMsgPush(pOther, pSelf)) {
                return;
            }
        } else if (rc::sendMsgBobsledTrample(pOther, pSelf, mTrampleComboCounter)) {
            al::getVelocityPtr(this)->y = 55.0f;
            mAnimState->requestBound();
            playHitReactionHitEffect(this, "キック甲羅ヒット[反射]", pOther, pSelf);
            return;
        } else if (!rc::sendMsgBobsledBodyAttack(pOther, pSelf) && al::sendMsgPush(pOther, pSelf)) {
            return;
        }
    }

    if (mFirstPlayerSensor != nullptr) {
        if (al::sendMsgKickKouraReflect(pOther, pSelf)) {
            return;
        }

        if (al::isSensorName(pSelf, "Body") || al::isSensorName(pSelf, "PlayerSensor") ||
            al::isSensorBindableAll(pOther)) {
            if (al::sendMsgPlayerItemGet(pOther, mFirstPlayerSensor)) {
                return;
            }

            al::getSensorHost(mFirstPlayerSensor)->attackSensor(mFirstPlayerSensor, pOther);
        }

        if (rc::sendMsgPlayerCheckpointTouch(pOther, pSelf)) {
            playHitReactionHitEffect(this, "WallHit", pOther, pSelf);
            return;
        }

        if (rc::isPlayerInvincible(this, mFirstPlayerSensor) &&
            al::sendMsgPlayerInvincibleAttack(pOther, pSelf, mInvincibleComboCounter)) {
            return;
        }

        if (al::isSensorNpc(pOther)) {
            al::HitSensor* pAttacker = npc::isSensorNeko(pOther) ? pSelf : mFirstPlayerSensor;
            if (al::sendMsgPlayerAttackTrample(pOther, pAttacker, mTrampleComboCounter)) {
                return;
            }
        }

        if (al::isNerve(this, &NrvRaidonSurfRide) &&
            rc::sendMsgAskBobsledDashPanel(pOther, pSelf)) {
            if (mDashTimer < 40) {
                mAnimState->requestDash();
            }

            sead::Vector3f dir = al::getVelocity(this);
            dir.y = 0.0f;
            if (al::normalizeOrZero(&dir)) {
                al::calcFrontDir(&dir, this);
            }

            sead::Vector3f* pVelocity = al::getVelocityPtr(this);
            al::verticalizeVec(pVelocity, dir, *pVelocity);
            al::addVelocityToDirection(this, dir, 80.0f);
            mDashTimer = 50;
            return;
        }
    } else {
        if (al::isSensorNpc(pOther)) {
            if (al::isNerve(this, &NrvRaidonSurfWait)) {
                return;
            }

            if (npc::isSensorNeko(pOther)) {
                return;
            }

            if (al::sendMsgPlayerAttackTrample(pOther, pSelf, mTrampleComboCounter)) {
                return;
            }
        }

        if (al::isSensorKickKoura(pOther) && al::sendMsgKickKouraReflect(pOther, pSelf)) {
            return;
        }
    }

    if (al::HitSensor* pGroundSensor = al::tryGetCollidedGroundSensor(this)) {
        al::sendMsgPlessieFloorTouch(pGroundSensor, pSelf);
    }
}

/** @return Whether the players are getting off. */
bool RaidonSurf::isGetOffNerve() {
    return al::isNerve(this, &NrvRaidonSurfGetOff) ||
           al::isNerve(this, &NrvRaidonSurfGetOffInAir) ||
           al::isNerve(this, &NrvRaidonSurfGetOffOnWater);
}

/**
 * @brief Plays a hit reaction unless one was played just before.
 * @param pActor Actor playing the reaction.
 * @param pName Hit reaction name.
 * @param pOther Touched sensor.
 * @param pSelf Plessie's sensor.
 */
void RaidonSurf::playHitReactionHitEffect(const al::LiveActor* pActor, const char* pName,
                                          const al::HitSensor* pOther,
                                          const al::HitSensor* pSelf) {
    if (mHitReactionTimer != 0) {
        return;
    }

    al::startHitReactionHitEffect(pActor, pName, pOther, pSelf);
    mHitReactionTimer = 30;
}

/** @brief Handles binding the riders, attacks, pushes and the messages forwarded to the states.
 * @param pMsg Received message.
 * @param pOther Sender sensor.
 * @param pSelf Plessie's sensor.
 * @return Whether the message was handled.
 */
bool RaidonSurf::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                            al::HitSensor* pSelf) {
    if (rc::isMsgIsEnableIslandWarp(pMsg)) {
        if (!isOnGroundOrWaterRaidon()) {
            return false;
        }

        return mAnimState->isWaitDoDive();
    }

    if (al::isMsgBindCancel(pMsg)) {
        if (al::isNerve(this, &NrvRaidonSurfEnd)) {
            return true;
        }

        if (mFirstPlayerSensor != nullptr) {
            rc::cancelRequestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
        }

        for (s32 i = 0; i < mPuppeteerNum; i++) {
            if (mPuppeteers[i].mPuppet != nullptr) {
                mPuppeteers[i].cancelBind();
                mPuppeteers[i].mPuppet = nullptr;
            }
        }

        stopFollowCamera();
        if (mCameraArea != nullptr) {
            mCameraArea->invalidate();
            turnOffWaterCameraDistance();
        }

        mPuppeteerNum = 0;
        mFirstPlayerSensor = nullptr;
        mIsBindCanceled = true;
        if (al::isMsgBindCancelForGoal(pMsg)) {
            al::setSensorRadius(this, "Bind", 3000.0f);
            al::setNerve(this, &NrvRaidonSurfCollectItem);
            return true;
        }

        if (al::isMsgBindCancelForWarp(pMsg)) {
            al::tryOffStageSwitch(this, "SwitchRideOn");
            al::stopBgm(this, "RaidonSurf", 90, 180);
            mIsFastBgm = false;
            if (rc::isActiveDemo(this)) {
                rc::addDemoActor(this);
            }

            validateFirstPlayerWaterEffect(this, mPlayerHolder);
            al::setNerve(this, &NrvRaidonSurfDeSpawnForce);
            if (rc::isGuideGameWindowActive(this) && rc::isCurrentGuideGameWindowUser(this)) {
                rc::disappearGuideGameWindow(this);
            }

            mIsShowGuideWindow = true;
            return true;
        }

        mAnimState->endMove();
        mLife = 0;
        dieFromDamage(false);
        return true;
    }

    if (al::isMsgCutsceneStart(pMsg) && mFirstPlayerSensor == nullptr &&
        !al::isNerve(this, &NrvRaidonSurfDeSpawnForce) &&
        !al::isNerve(this, &NrvRaidonSurfEnd)) {
        al::hideModelIfShow(this);
        al::setNerve(this, &NrvRaidonSurfDeSpawnForce);
        return true;
    }

    if (al::isNerve(this, &NrvRaidonSurfWait) && mWaitState->receiveMsg(pMsg, pOther, pSelf)) {
        return true;
    }

    if (al::isSensorMapObj(pSelf) &&
        (al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg))) {
        rc::requestHitReactionToAttackerNpc(pSelf, pOther);
        return true;
    }

    if (mFirstPlayerSensor != nullptr) {
        if (al::isMsgLaserAttack(pMsg) || al::isMsgExplosion(pMsg) || al::isMsgEnemyAttack(pMsg) ||
            al::isMsgEnemyAttackBoomerang(pMsg) || rc::isMsgFireRollerAttack(pMsg) ||
            al::isMsgDisasterSpikeAttack(pMsg)) {
            if (!al::isSensorRide(pSelf)) {
                return false;
            }

            if (al::isMsgLaserAttack(pMsg) && mAnimState->isUnderwater()) {
                return false;
            }

            if ((rc::isMsgFireRollerAttack(pMsg) || al::isSensorName(pSelf, "PlayerSensor")) &&
                !isUnderwater() && !al::isSensorName(pOther, "WallHit")) {
                bool isDamaged = false;
                for (s32 i = 0; i < mPuppeteerNum; i++) {
                    if (mPuppeteers[i].mPuppet != nullptr) {
                        rc::tryDamagePuppet(mPuppeteers[i].mPuppet);
                        isDamaged = true;
                    }
                }

                if (!isDamaged) {
                    return false;
                }

                playHitReactionHitEffect(this, "HitDamage", pOther, pSelf);
                mAnimState->requestDamage();
                return true;
            }
        }

        if (rc::isMsgAskControlUserId(pMsg, mFirstPlayerSensor)) {
            return true;
        }

        if (al::isNerve(this, &NrvRaidonSurfRide) && rc::isMsgDashPanel(pMsg)) {
            if (mActorSceneInfo->isSingleMode) {
                al::CameraTicket* ticket = mActorSceneInfo->cameraDirector->getCurrentTicket();
                if (ticket != nullptr && alCameraPoserFunction::isSnapShotMode(ticket->getPoser())) {
                    return false;
                }
            }

            if (mDashTimer < 40) {
                mAnimState->requestDash();
            }

            sead::Vector3f dir = al::getVelocity(this);
            dir.y = 0.0f;
            if (al::normalizeOrZero(&dir)) {
                al::calcFrontDir(&dir, this);
            }

            sead::Vector3f* pVelocity = al::getVelocityPtr(this);
            al::verticalizeVec(pVelocity, dir, *pVelocity);
            al::addVelocityToDirection(this, dir, 80.0f);
            mDashTimer = 50;
            mDashAccel = 0.5f;
            return true;
        }
    } else if (al::isMsgLaserAttack(pMsg) || rc::isMsgFireRollerAttack(pMsg) ||
               rc::isMsgMeraWanwanAttack(pMsg) || al::isMsgBowserPush(pMsg)) {
        dieFromDamage(false);
        return true;
    }

    if ((al::isMsgPushStrong(pMsg) || al::isMsgBowserPush(pMsg)) &&
        al::isSensorName(pSelf, "Body")) {
        if (mFirstPlayerSensor != nullptr && rc::isPlayerInvincible(this, mFirstPlayerSensor) &&
            !al::isMsgBowserPush(pMsg)) {
            return false;
        }

        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        f32 speed = al::getVelocity(this).dot(dir);
        if (speed < 0.0f) {
            f32 pushSpeed = sead::Mathf::abs(speed) < 25.0f ? 25.0f : speed * -1.7f;
            mAnimState->requestHit();
            playHitReactionHitEffect(this, "WallHit", pOther, pSelf);
            al::addVelocityToDirection(this, dir, pushSpeed);
            al::scaleVelocityHV(this, 0.8f, 1.0f);
            mHitTimer = 50;
            return true;
        }
    }

    if (al::isMsgPush(pMsg) && al::isSensorName(pSelf, "Body")) {
        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        sead::Vector3f moveDir = al::getVelocity(this);
        if (al::isNearZero(moveDir, 0.001f) ||
            sead::Mathf::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z) != 0.0f) {
            al::calcFrontDir(&moveDir, this);
        }

        if (moveDir.dot(dir) <= 0.0f) {
            f32 pushSpeed = 12.0f;
            if (!rc::isSensorKinopioBrigadeNpc(pOther)) {
                pushSpeed = 25.0f;
                playHitReactionHitEffect(this, "WallHit", pOther, pSelf);
            }

            mAnimState->requestHit();
            al::addVelocityToDirection(this, dir, pushSpeed);
            al::scaleVelocityHV(this, 0.8f, 1.0f);
            mHitTimer = 50;
            return true;
        }
    }

    if (al::isMsgNekoPush(pMsg)) {
        al::pushAndAddVelocityH(this, pOther, pSelf, 8.0f);
        if (al::isSensorName(pSelf, "Body")) {
            sead::Vector3f dir;
            al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
            sead::Vector3f moveDir = al::getVelocity(this);
            if (al::isNearZero(moveDir, 0.001f) ||
                sead::Mathf::sqrt(moveDir.x * moveDir.x + moveDir.z * moveDir.z) != 0.0f) {
                al::calcFrontDir(&moveDir, this);
            }

            if (moveDir.dot(dir) <= 0.0f || al::isNerve(this, &NrvRaidonSurfWait)) {
                if (al::isNerve(this, &NrvRaidonSurfRide)) {
                    mAnimState->requestHit();
                }

                if (al::isNerve(this, &NrvRaidonSurfWait)) {
                    mWaitState->setReactionNerve(60);
                }

                playHitReactionHitEffect(this, "WallHit", pOther, pSelf);
                al::addVelocityToDirection(this, dir, 20.0f);
                al::scaleVelocityHV(this, 0.4f, 1.0f);
                mHitTimer = 50;
                return true;
            }
        }
    }

    if (al::isMsgGigaBellPush(pMsg)) {
        if (al::isNerve(this, &NrvRaidonSurfWait)) {
            dieFromDamage(false);
            return true;
        }

        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        f32 speed = al::getVelocity(this).dot(dir);
        if (speed < 0.0f) {
            f32 pushSpeed = sead::Mathf::abs(speed) < 25.0f ? 25.0f : speed * -1.7f;
            mAnimState->requestHit();
            playHitReactionHitEffect(this, "WallHit", pOther, pSelf);
            al::addVelocityToDirection(this, dir, pushSpeed);
            al::scaleVelocityHV(this, 0.8f, 1.0f);
            mHitTimer = 50;
            return true;
        }
    }

    if (al::isMsgDisasterSpikePush(pMsg) && al::isSensorName(pSelf, "Body")) {
        sead::Vector3f dir;
        al::calcDirBetweenSensorsH(&dir, pOther, pSelf);
        al::addVelocityToDirection(this, dir, 25.0f);
        al::scaleVelocityHV(this, 0.8f, 1.0f);
        return true;
    }

    if (rc::isMsgInkTouch(pMsg)) {
        playHitReactionHitEffect(this, "HitInk", pOther, pSelf);
    }

    if (al::isMsgBindStart(pMsg)) {
        if (rc::isAnyActiveDemo(this)) {
            return false;
        }

        if (isWaiting() || al::isNerve(this, &NrvRaidonSurfGetOn) ||
            al::isNerve(this, &NrvRaidonSurfStart) || al::isNerve(this, &NrvRaidonSurfRide) ||
            al::isNerve(this, &NrvRaidonSurfCollectItem)) {
            if (al::isNerve(this, &NrvRaidonSurfFirstSeenDemo) ||
                al::isNerve(this, &NrvRaidonSurfCollectItem)) {
                return true;
            }

            al::LiveActor* player = al::getSensorHost(pOther);
            if (!rc::isPlayerOnGroundOrWater(player) && !rc::isPlayerInRouteDokanOrDokan(player)) {
                return true;
            }

            return false;
        }
    }

    if (al::isMsgBindInit(pMsg)) {
        if (mFirstPlayerSensor == nullptr) {
            mFirstPlayerSensor = pOther;
        }

        IUsePlayerPuppet* puppet = rc::startPuppet(pSelf, pOther);
        mPadRumbleKeeper->setPort(rc::getPuppetInputPort(puppet));
        rc::hidePuppetShadow(puppet);
        rc::forceEndSubActionPuppet(puppet);
        bool isCollectItem = al::isNerve(this, &NrvRaidonSurfCollectItem);
        mPuppeteers[mPuppeteerNum].startGetOn(
            puppet, al::getJointMtxPtr(this, cPlayerJointNames[mPuppeteerNum]), isCollectItem);
        mPuppeteerNum++;
        al::sendMsgHoldCancel(pOther, pSelf);
        if (rc::isPlayerEquipHeadgear(pOther)) {
            rc::removePlayerEquipHeadgear(pOther, false);
        }

        al::onCollide(this);
        if (al::isNerve(this, &NrvRaidonSurfWait) ||
            al::isNerve(this, &NrvRaidonSurfFirstSeenDemo)) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvRaidonSurfGetOn);
        } else if (al::isNerve(this, &NrvRaidonSurfWaitInWater) ||
                   al::isNerve(this, &NrvRaidonSurfWaitInWaterReaction)) {
            al::invalidateClipping(this);
            al::setNerve(this, &NrvRaidonSurfGetOnInWater);
        } else if (isCollectItem) {
            al::invalidateClipping(this);
            al::setVelocity(this, mPrevVelocity);
            al::setNerve(this, &NrvRaidonSurfRebind);
        }

        al::setSensorRadius(this, "Bind", mBindSensorRadius);
        mBindSensorTimer = 0;
        rc::tryRequestClearFlingPoleDashFlag(al::getSensorHost(mFirstPlayerSensor));
        rc::tryRequestClearDashFlag(al::getSensorHost(mFirstPlayerSensor));
        return true;
    }

    if (rc::isMsgNpcBindInit(pMsg) && !mNpcPuppets.isFull()) {
        auto* neko = static_cast<IUseNekoModeActor*>(al::getSensorHost(pOther));
        neko->startBindNpc(al::getHitSensor(this, "Bind"), pOther);
        mNpcPuppets.pushBack(neko);
        if (mFirstPlayerSensor != nullptr) {
            mIsNpcPuppetBinding = true;
        }

        return true;
    }

    if (rc::isMsgNpcBindCancel(pMsg)) {
        endNpcPuppetBindAll(static_cast<NpcPuppetBindEndType>(5));
    }

    if (!al::isMsgPlayerBodyAttackReflect(pMsg) && !al::isMsgPlayerClimbAttack(pMsg) &&
        trySetReactionNerve(pMsg, -1)) {
        if (isMsgNpcAttackerHitReaction(pMsg)) {
            rc::requestHitReactionToAttackerNpc(pSelf, pOther);
            playHitReactionHitEffect(this, "ＮＰＣヒット", pOther, pSelf);
        } else {
            playHitReactionHitEffect(this, "キック甲羅ヒット[反射]", pOther, pSelf);
        }

        return true;
    }

    return false;
}

/** @param isInWater Whether Plessie burns in lava instead of dying normally. */
void RaidonSurf::dieFromDamage(bool isInWater) {
    if (isDeadState() || al::isNerve(this, &NrvRaidonSurfSpawn) ||
        al::isNerve(this, &NrvRaidonSurfCollectItem)) {
        return;
    }

    if (mLife <= 0) {
        al::updatePoseMtx(mSoundActor, getBaseMtx());
        if (isInWater) {
            al::setNerve(this, &NrvRaidonSurfDieLava);
        } else {
            al::setNerve(this, &NrvRaidonSurfDie);
        }

        mLife = 7;
    } else {
        mLife--;
    }
}

/** @return Whether Plessie is deep below the surface of the water area she is in. */
bool RaidonSurf::isUnderwater() {
    al::AreaObj* area = rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea, al::getTrans(this));
    if (area == nullptr) {
        return false;
    }

    f32 halfHeight = area->getAreaShape()->mScale.y * 500.0f;
    f32 surfaceY = area->_28.m[1][3] + halfHeight;
    if (al::getTrans(this).y < surfaceY + -150.0f) {
        return true;
    }

    return false;
}

/** @param type How the riding NPCs leave Plessie. */
void RaidonSurf::endNpcPuppetBindAll(NpcPuppetBindEndType type) {
    mIsNpcPuppetBinding = false;
    if (mNpcPuppets.size() == 0) {
        return;
    }

    for (s32 i = 0; i < mNpcPuppets.size(); i++) {
        if (IUseNpcPuppet* puppet = mNpcPuppets[i]) {
            puppet->endBindNpc(type);
            if (type == 5) {
                al::tryEmitEffect(this, cNekoPoofEffectNames[i], nullptr);
            }
        }
    }

    mNpcPuppets.clear();
}

/**
 * @brief Makes Plessie react to an attack while she waits in the water.
 * @param pMsg Received message, or none to react after the given step.
 * @param step Step after which a running reaction restarts.
 * @return Whether the reaction was started.
 */
bool RaidonSurf::trySetReactionNerve(const al::SensorMsg* pMsg, s32 step) {
    s32 reactionStep = -1;
    if (isMsgNpcAttackerHitReaction(pMsg) || al::isMsgTouchAssistTrigNoPat(pMsg) ||
        al::isMsgEnemyAttackBoomerang(pMsg) || al::isMsgEnemyAttack(pMsg) ||
        al::isMsgKickKouraAttack(pMsg) || al::isMsgKickKouraReflect(pMsg) ||
        al::isMsgExplosion(pMsg)) {
        reactionStep = 60;
    }

    if (pMsg == nullptr) {
        reactionStep = step;
    }

    if (reactionStep < 0) {
        return false;
    }

    if (al::isNerve(this, &NrvRaidonSurfWaitInWater)) {
        al::setNerve(this, &NrvRaidonSurfWaitInWaterReaction);
        return true;
    }

    if (al::isNerve(this, &NrvRaidonSurfWaitInWaterReaction) &&
        al::isGreaterEqualStep(this, reactionStep)) {
        al::setNerve(this, &NrvRaidonSurfWaitInWaterReaction);
        return true;
    }

    return false;
}

/**
 * @param pMsg Received message.
 * @return Whether the message is a player attack the attacking NPC reacts to.
 */
bool RaidonSurf::isMsgNpcAttackerHitReaction(const al::SensorMsg* pMsg) {
    return al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
           al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerBoomerangReflect(pMsg) ||
           al::isMsgPlayerBodyAttackReflect(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
           al::isMsgPlayerHipDropAll(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
           al::isMsgNekoAttack(pMsg) || al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg) ||
           al::isMsgKeyThrow(pMsg) || al::isMsgExplosion(pMsg);
}

/** @brief Forwards touch screen messages to the waiting state.
 * @param pMsg Received message.
 * @param pPointer Screen pointer.
 * @param pTarget Touched target.
 * @return Whether the message was handled.
 */
bool RaidonSurf::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvRaidonSurfWait)) {
        return mWaitState->receiveMsgScreenPoint(pMsg, pPointer, pTarget);
    }

    return al::isMsgTouchAssist(pMsg);
}

/** @brief Updates the collider and the floor material while Plessie is alive. */
void RaidonSurf::updateCollider() {
    al::LiveActor::updateCollider();
    if (isDeadState()) {
        return;
    }

    updateMatrialCode();
}

/** @return Whether Plessie is dying or dead. */
bool RaidonSurf::isDeadState() {
    return al::isNerve(this, &NrvRaidonSurfDeath) || al::isNerve(this, &NrvRaidonSurfDie) ||
           al::isNerve(this, &NrvRaidonSurfDieLava);
}

/** @brief Updates stroking, damage floors, the guide window, the invincibility model and timers. */
void RaidonSurf::control() {
    if (isDeadState()) {
        return;
    }

    mStateSupportStroke->update();
    if (mStateSupportStroke->isTrigStroke()) {
        sead::Vector3f itemPos = al::getTrans(this) + sead::Vector3f(0.0f, 200.0f, 0.0f);
        al::appearItemTiming(this, "撫でる", itemPos, sead::Vector3f::ey);
    }

    f32 colliderOffsetY;
    if (al::isNerve(this, &NrvRaidonSurfRide) && mAnimState->isDive()) {
        f32 sensorY = al::getSensorPos(al::getHitSensor(this, "Body")).y;
        colliderOffsetY = sead::Mathf::max(
            sensorY - al::getTrans(this).y + (mColliderOffsetY + -100.0f), -300.0f);
    } else {
        colliderOffsetY = mColliderOffsetY;
    }

    al::setColliderOffsetY(this, colliderOffsetY);
    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    mBaseFrontDir = frontDir;

    if (checkIsMaterial("DamageFire") || checkIsMaterial("Lava") || checkIsMaterial("LavaRed")) {
        mLife = 0;
        dieFromDamage(true);
    } else if (mIsOnInk && mCollisionType == 1 && !isInkWall()) {
        mLife = 0;
        dieFromDamage(false);
    }

    if (mFirstPlayerSensor != nullptr && checkIsMaterial("Needle") && !isUnderwater()) {
        bool isDamaged = false;
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            if (mPuppeteers[i].mPuppet != nullptr) {
                rc::tryDamagePuppet(mPuppeteers[i].mPuppet);
                isDamaged = true;
            }
        }

        if (isDamaged) {
            mAnimState->requestDamage();
        }
    }

    if (!mIsNoGuideWindow && al::isNerve(this, &NrvRaidonSurfRide) && !al::isFirstStep(this)) {
        if (mFirstPlayerSensor != nullptr &&
            rc::isInAreaObjPlayerOne(this, rc::AreaObjType::RaidonUIArea, mPlayerHolder)) {
            if (!mIsInGuideArea) {
                mIsInGuideArea = true;
                if (mGuideState == GuideState_None) {
                    mGuideState = showGameWindow(true);
                }
            }
        } else {
            if (mGuideState == GuideState_AreaRideGuide) {
                mGuideState = GuideState_Failed;
            } else if (mGuideState == GuideState_RideGuide) {
                mGuideState = GuideState_None;
            }

            if (mFirstPlayerSensor == nullptr || mGuideState == GuideState_DismountGuide) {
                mGuideState = showGameWindow(false);
            }

            mIsInGuideArea = false;
        }
    }

    if (mFirstPlayerSensor != nullptr) {
        mIsInCameraHeightLimitArea = rc::isInAreaObjInGroup(
            this, rc::AreaObjType::RaidonCameraHeightLimitArea, al::getTrans(this));
        if (!rc::isPlayerInvincible(this, mFirstPlayerSensor)) {
            mInvincibleComboCounter->reset();
        }

        bool isAppear = rc::isPlayerInvincibleModelAppear(mFirstPlayerSensor);
        bool isHidden = al::isInvincibleModelHidden(this);
        if (isAppear) {
            if (isHidden) {
                al::showInvincibleModel(this);
                al::tryEmitEffect(this, "SuperStar", nullptr);
            }

            rc::setInvincibleColor(this, rc::getPlayerInvincibleColor(mFirstPlayerSensor));
        } else if (!isHidden) {
            sead::Vector3f velocity = al::getVelocity(this);
            al::hideInvincibleModel(this);
            al::setVelocity(this, velocity);
            al::tryDeleteEffect(this, "SuperStar");
        }
    } else if (!al::isInvincibleModelHidden(this)) {
        al::tryDeleteEffect(this, "SuperStar");
        al::hideInvincibleModel(this);
    }

    if (mRainActor != nullptr) {
        sead::Vector3f rainTrans = al::getTrans(this);
        al::calcJointPos(&rainTrans, this, "Spine1");
        al::setTrans(mRainActor, rainTrans);
        sead::Quatf rainQuat;
        al::calcQuat(&rainQuat, this);
        al::setQuat(mRainActor, rainQuat);
        if (DisasterModeController* controller = DisasterModeController::tryGetController(this)) {
            if (controller->isRainEffectsOn()) {
                bool isNoRain = rc::isInAreaObj(this, rc::AreaObjType::NoRainArea);
                bool isDead = al::isDead(mRainActor);
                if (!isNoRain) {
                    if (isDead) {
                        mRainActor->appear();
                    }
                } else if (!isDead) {
                    mRainActor->kill();
                }
            } else if (!al::isDead(mRainActor)) {
                mRainActor->kill();
            }
        }
    }

    const sead::Vector3f& rTrans = al::getTrans(this);
    mDashBlurCenter = mBaseFrontDir * 3000.0f + rTrans;

    if (mDashTimer > 0) {
        mDashTimer--;
        if (mDashTimer == 0) {
            mDashAccel = 0.0f;
        }
    }

    if (mMultiJumpTimer > 0) {
        mMultiJumpTimer--;
    }

    if (mHitTimer > 0) {
        mHitTimer--;
    }

    if (mHitReactionTimer > 0) {
        mHitReactionTimer--;
    }

    if (mJumpInterval > 0) {
        mJumpInterval--;
    }

    mPrevVelocity = al::getVelocity(this);
    if (mBindSensorTimer > 0) {
        mBindSensorTimer--;
        if (mBindSensorTimer == 0) {
            al::setSensorRadius(this, "Bind", mBindSensorRadius);
        }
    }

    mIsInWater = isInWater();
    if (!mIsInWater && al::isEffectEmitting(this, "SwimNeutralRipple")) {
        al::tryDeleteEffect(this, "SwimNeutralRipple");
    }
}

/**
 * @param pName Material name.
 * @return Whether the touched wall or floor has the material.
 */
bool RaidonSurf::checkIsMaterial(const char* pName) {
    bool isMaterial = mMaterialCodeName != nullptr && al::isEqualString(mMaterialCodeName, pName);
    if (mFloorCodeName != nullptr) {
        isMaterial |= al::isEqualString(mFloorCodeName, pName);
    }

    return isMaterial;
}

/** @return Whether the touched wall is an ink wall. */
bool RaidonSurf::isInkWall() {
    return mWallCodeName != nullptr && al::isEqualString(mWallCodeName, "InkWall");
}

/**
 * @param isShow Whether the riding guide is shown or hidden.
 * @return The new guide state.
 */
RaidonSurf::GuideState RaidonSurf::showGameWindow(bool isShow) {
    if (isShow) {
        if (mFirstPlayerSensor != nullptr &&
            rc::isInAreaObjPlayerOne(this, rc::AreaObjType::RaidonUIArea, mPlayerHolder)) {
            GuideState state = mGuideState;
            bool isSingle = al::isPadTypeJoySingle(al::getMainControllerPort());
            if (state == GuideState_RideGuide) {
                return rc::appearGuideGameWindowWithPriority(
                           this, "SingleMode_GuideMessage",
                           isSingle ? "PlessiGuide_SingleJoycons" : "PlessiGuide_DualJoycons",
                           static_cast<GuideMessagePriority>(4), -1, 0.0f) ?
                           GuideState_AreaRideGuide :
                           GuideState_RideGuide;
            }

            return rc::appearGuideGameWindowWithPriority(
                       this, "SingleMode_GuideMessage",
                       isSingle ? "PlessiDismountGuide_SingleJoycons" :
                                  "PlessiDismountGuide_DualJoycons",
                       static_cast<GuideMessagePriority>(4), -1, 0.0f) ?
                       GuideState_DismountGuide :
                       GuideState_None;
        }

        bool isSingle = al::isPadTypeJoySingle(al::getMainControllerPort());
        return rc::appearGuideGameWindowWithPriority(
                   this, "SingleMode_GuideMessage",
                   isSingle ? "PlessiGuide_SingleJoycons" : "PlessiGuide_DualJoycons",
                   static_cast<GuideMessagePriority>(4), -1, 0.0f) ?
                   GuideState_Failed :
                   GuideState_None;
    }

    rc::disappearGuideGameWindow(this);
    if (mFirstPlayerSensor != nullptr &&
        rc::isInAreaObjPlayerOne(this, rc::AreaObjType::RaidonUIArea, mPlayerHolder) &&
        mGuideState == GuideState_AreaRideGuide) {
        return GuideState_RideGuide;
    }

    return GuideState_None;
}

/** @brief Makes the players get off. */
void RaidonSurf::forcePlayerOff() {
    al::setNerve(this, &NrvRaidonSurfGetOff);
}

/** @brief Moves the riding players and NPCs along with their joints. */
void RaidonSurf::calcAnim() {
    al::LiveActor::calcAnim();
    if (al::isDead(this)) {
        return;
    }

    if (al::isNerve(this, &NrvRaidonSurfGetOn) ||
        al::isNerve(this, &NrvRaidonSurfGetOnInWater) ||
        al::isNerve(this, &NrvRaidonSurfStart) || al::isNerve(this, &NrvRaidonSurfRide) ||
        al::isNerve(this, &NrvRaidonSurfGoal) || al::isNerve(this, &NrvRaidonSurfGetOff) ||
        al::isNerve(this, &NrvRaidonSurfGetOffOnWater)) {
        setPuppetQT();
    }

    setNpcPuppetQT();
}

/** @brief Places every rider on their seat joint, easing them in while they get on. */
void RaidonSurf::setPuppetQT() {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            sead::Matrix34f mtx;
            al::normalizeMtxScale(&mtx, *al::getJointMtxPtr(this, cPlayerJointNames[i]));
            if (al::isNerve(this, &NrvRaidonSurfGetOnInWater) ||
                al::isNerve(this, &NrvRaidonSurfGetOn)) {
                sead::Vector3f trans;
                sead::Vector3f jointTrans;
                mtx.getTranslation(jointTrans);
                sead::Vector3f puppetTrans = rc::getPuppetTrans(mPuppeteers[i].mPuppet);
                if (jointTrans != puppetTrans) {
                    al::lerpVec(&trans, puppetTrans, jointTrans, al::easeByType(0.33333334f, 0));
                    mtx.setTranslation(trans);
                }
            }

            rc::setPuppetMtx(mPuppeteers[i].mPuppet, &mtx);
        }
    }
}

/** @brief Places every riding NPC on their seat joint. */
void RaidonSurf::setNpcPuppetQT() {
    for (s32 i = 0; i < mNpcPuppets.size(); i++) {
        if (IUseNpcPuppet* puppet = mNpcPuppets[i]) {
            puppet->setMtx(al::getJointMtxPtr(this, cNpcJointNames[i]));
            if (mPuppeteers[0].mPuppet != nullptr) {
                f32 stickX = rc::getPuppetStickX(mPuppeteers[0].mPuppet);
                puppet->setPlayerPuppetInputTurnStick(stickX, getPuppetInputStickY());
            } else {
                puppet->setPlayerPuppetInputTurnStick(0.0f, 0.0f);
            }
        }
    }
}

/**
 * @param isCheckCamera Whether a Plessie in view of the camera stays where she is.
 * @return Whether Plessie was left behind and respawns near the player.
 */
bool RaidonSurf::shouldRespawn(bool isCheckCamera) {
    mIsRespawnRequested = false;
    if (mIsRespawnWaiting) {
        if (mRespawnWaitStep++ >= mRespawnWaitTime * 60) {
            mIsRespawnWaiting = false;
        }

        return false;
    }

    sead::Vector3f trans = al::getTrans(this);
    al::LiveActor* player = rc::getActivePlayer(this);
    if (player == nullptr) {
        return false;
    }

    bool isOutOfView = false;
    if (isCheckCamera) {
        CameraPoserFollowLimit* poser = tryGetFollowCameraPoser();
        if (poser == nullptr) {
            isOutOfView = true;
        } else {
            sead::Vector3f cameraDir = poser->getAt() - poser->getEye();
            sead::Vector3f toTrans = trans - poser->getEye();
            toTrans.normalize();
            cameraDir.normalize();
            isOutOfView = toTrans.dot(cameraDir) <= mRespawnCameraDot;
        }
    }

    f32 distanceSq = (al::getTrans(player) - trans).squaredLength();
    if (distanceSq >= 2500.0f * 2500.0f) {
        s32 closestIndex = getClosestSpawnIndex(al::getPlayerPos(player, 0), false);
        if (closestIndex <= 0) {
            mIsRespawnRequested = true;
        } else {
            sead::Vector3f spawnTrans = mSpawnPoints[closestIndex]->trans;
            // The distance is computed but never used.
            (trans - spawnTrans).length();
            if (!al::isNear(trans, spawnTrans, 500.0f)) {
                mIsRespawnRequested =
                    !isCheckCamera || isOutOfView || distanceSq >= 10000.0f * 10000.0f;
            }
        }
    }

    return mIsRespawnRequested;
}

/**
 * @param pos Position to search the spawn point closest to.
 * @param isCheckCamera Whether spawn points out of the camera view are skipped.
 * @return Index of the closest enabled spawn point, or -1.
 */
s32 RaidonSurf::getClosestSpawnIndex(sead::Vector3f pos, bool isCheckCamera) const {
    if (mSpawnPoints.size() == 0) {
        return 0;
    }

    s32 closestIndex = -1;
    f32 closestDistanceSq = -1.0f;
    for (s32 i = 0; i < mSpawnPoints.size(); i++) {
        SpawnPoint* spawnPoint = mSpawnPoints[i];
        if (!spawnPoint->isEnable) {
            continue;
        }

        spawnPoint->isCandidate = false;
        if (isCheckCamera) {
            if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
                sead::Vector3f cameraDir = poser->getAt() - poser->getEye();
                sead::Vector3f toSpawn = spawnPoint->trans - poser->getEye();
                toSpawn.normalize();
                cameraDir.normalize();
                if (toSpawn.dot(cameraDir) <= mSpawnCameraDot) {
                    continue;
                }
            }
        }

        f32 dx = spawnPoint->trans.x - pos.x;
        f32 dz = spawnPoint->trans.z - pos.z;
        f32 spawnDistanceSq = dx * dx + dz * dz;
        if (al::isNerve(this, &NrvRaidonSurfFirstSeenDemo)) {
            f32 sx = mStartingPoint->trans.x - pos.x;
            f32 sz = mStartingPoint->trans.z - pos.z;
            if (spawnDistanceSq > 5500.0f * 5500.0f || spawnDistanceSq > sx * sx + sz * sz) {
                continue;
            }
        } else if (spawnDistanceSq > 5500.0f * 5500.0f) {
            continue;
        }

        if (closestDistanceSq == -1.0f || spawnDistanceSq < closestDistanceSq) {
            closestDistanceSq = spawnDistanceSq;
            closestIndex = i;
        }

        spawnPoint->isCandidate = true;
    }

    return closestIndex;
}

/**
 * @param isCheckCamera Whether spawn points out of the camera view are skipped.
 * @return Whether a spawn point near the player was chosen.
 */
bool RaidonSurf::isReadyToSpawn(bool isCheckCamera) {
    al::LiveActor* player = rc::getActivePlayer(this);
    if (player == nullptr) {
        return false;
    }

    s32 index = getClosestSpawnIndex(al::getPlayerPos(player, 0), isCheckCamera);
    if (index < 0) {
        return false;
    }

    mSpawnIndex = index;
    sead::Vector3f trans = al::getTrans(this);
    mIsRespawnRequested = false;
    mRespawnWaitStep = 0;
    mIsRespawnWaiting = true;
    return true;
}

/**
 * @param isCheckCamera Whether spawn points out of the camera view are skipped.
 * @return Whether a spawn point near the player was chosen.
 */
bool RaidonSurf::moveToClosestSpawnPosition(bool isCheckCamera) {
    al::LiveActor* player = rc::getActivePlayer(this);
    if (player == nullptr) {
        return false;
    }

    s32 index = getClosestSpawnIndex(al::getPlayerPos(player, 0), isCheckCamera);
    if (index < 0) {
        return false;
    }

    mSpawnIndex = index;
    return true;
}

/** @brief Waits at the starting point during the demo of the first meeting. */
void RaidonSurf::exeFirstSeenDemo() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::resetPosition(this, mStartingPoint->trans, false);
        al::faceToDirection(this, mStartingPoint->front);
        al::setSensorFollowPosOffset(this, "Bind", sead::Vector3f::zero);
        mIsRespawnRequested = false;
        mIsFirstRide = false;
    }

    if (al::isStep(this, 1050) && rc::isActiveDemo(this)) {
        al::startSe(this, "PgFirstSeenDemo");
    }

    if ((!rc::isActiveDemo(this) || !rc::isActiveDemoMovingCamera(this)) &&
        al::isGreaterEqualStep(this, 200) &&
        !rc::isInAreaObjPlayerOne(this, rc::AreaObjType::RaidonStartArea, mPlayerHolder)) {
        forceSpawn(false);
    }

    updateOnGround();
    al::updateNerveState(this);
}

/** @param isForce Whether Plessie vanishes at once instead of despawning normally. */
void RaidonSurf::forceSpawn(bool isForce) {
    mIsRespawnWaiting = false;
    mIsRespawnRequested = true;
    if (isDespawnState() || isDeadState()) {
        return;
    }

    if (mFirstPlayerSensor != nullptr) {
        al::setNerve(this, &NrvRaidonSurfDie);
    } else if (isForce) {
        al::setNerve(this, &NrvRaidonSurfDeSpawnForce);
    } else {
        al::setNerve(this, &NrvRaidonSurfDeSpawn);
    }
}

/** @brief Waits on the ground for the players to get on. */
void RaidonSurf::exeWait() {
    if (al::LiveActor* player = rc::tryFindNearestActivePlayerActorInSphere(this, 6000.0f)) {
        turnHead(al::getTrans(player));
    } else {
        backHead();
    }

    if (isInWater()) {
        al::setNerve(this, &NrvRaidonSurfWaitInWater);
        return;
    }

    if (rc::isInAreaObj(this, rc::AreaObjType::RaidonDisappearArea)) {
        forceSpawn(false);
        return;
    }

    tryEndNpcPuppetBindNearPlayer();
    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    if (!mIsFirstRide) {
        whileWait(true);
    }

    al::updateNerveState(this);
}

/**
 * @brief Moves Plessie on and in the water.
 * @param accel Acceleration to the front.
 * @param brake Velocity scale, used instead of the acceleration when positive.
 * @param extraAccel Additional acceleration, for example while dashing.
 */
void RaidonSurf::updateWaterPosVelocity(f32 accel, f32 brake, f32 extraAccel) {
    f32 hitRate = al::lerpValue(mHitTimer, 10.0f, 30.0f, 1.0f, 0.0f);
    sead::Vector3f surfacePos;
    sead::Vector3f checkPos = al::getTrans(this);
    checkPos.y += 100.0f;
    mDiveDepthLimit = 300.0f;
    mDiveDepth = rc::calcWaterSinkDepth(this, checkPos);
    if (WaterUtil::checkWaterSurface(this, &surfacePos, checkPos, 100.0f, nullptr)) {
        mWaterSurfaceY = surfacePos.y;
    }

    if (al::isNerve(this, &NrvRaidonSurfWait)) {
        if (al::isOnGround(this, 0, 0.0f)) {
            al::getVelocityPtr(this)->y = 0.0f;
        } else {
            al::addVelocityToGravity(this, 1.6f);
        }
    }

    if (!mIsInWater && isInWater() && isFloating()) {
        al::getVelocityPtr(this)->y = 0.0f;
    }

    f32 totalAccel = accel + extraAccel;
    if (isInWater()) {
        if (isFloating()) {
            if (al::getVelocityPtr(this)->y < 0.0f) {
                al::getVelocityPtr(this)->y = 0.0f;
            }

            if (!al::isOnGround(this, 0, 0.0f)) {
                if (mDiveDepth < 100.0f) {
                    checkPos.y = mWaterSurfaceY + -100.0f;
                    al::setTrans(this, checkPos);
                    al::getVelocityPtr(this)->y = 0.0f;
                } else {
                    al::addVelocityToGravity(this, -0.9999999f);
                }
            }
        } else {
            al::addVelocityToGravity(this, -0.9999999f);
        }
    } else {
        al::addVelocityToGravity(this, 1.6f);
    }

    sead::Vector3f groundNormal = sead::Vector3f::zero;
    sead::Vector3f frontDir = mBaseFrontDir;
    f32 speed = totalAccel * hitRate;
    if (al::isCollidedGround(this)) {
        al::getCollidedGroundPos(this);
        groundNormal = al::getCollidedGroundNormal(this);
        al::verticalizeVec(&frontDir, groundNormal, mBaseFrontDir);
        if (speed <= 0.0f && mFirstPlayerSensor == nullptr) {
            if (mIsOnSlideGround) {
                al::addVelocityToGravity(this, 2.8f);
            } else if ((al::isNerve(this, &NrvRaidonSurfWait) && !mWaitState->isReacting()) ||
                       al::isNerve(this, &NrvRaidonSurfEnd) ||
                       al::isNerve(this, &NrvRaidonSurfGetOff)) {
                sead::Vector3f velocity;
                al::parallelizeVec(&velocity, groundNormal, al::getVelocity(this));
                al::setVelocity(this, velocity);
            }
        } else if (frontDir.y > 0.0f && al::getVelocityPtr(this)->y < -1.6f) {
            al::getVelocityPtr(this)->y = 0.0f;
            al::addVelocityToGravity(this, 1.6f);
        }
    }

    if (al::getVelocityPtr(this)->y < -20.0f) {
        al::getVelocityPtr(this)->y = -20.0f;
    }

    if (brake > 0.0f) {
        al::scaleVelocity(this, brake);
    } else {
        al::addVelocityToDirection(this, frontDir, speed);
    }

    if (al::isCollidedWall(this)) {
        if (mIsOnInk) {
            al::reboundVelocityFromEachCollision(this, 0.0f, 0.5f, 0.0f, 0.0f);
        } else if (mIsOnSlideGround) {
            if (isInWater()) {
                al::reboundVelocityFromEachCollision(this, 0.0f, 1.0f, 0.0f, 50.0f);
            } else {
                al::reboundVelocityFromEachCollision(this, 0.0f, 1.0f, 0.0f, 0.0f);
            }
        } else {
            al::reboundVelocityFromEachCollision(this, 0.0f, 0.5f, 0.0f, 50.0f);
        }
    }

    if (al::isOnGround(this, 0, 0.0f) && mGroundCount > 90 && !mIsOnSlideGround) {
        al::scaleVelocity(this, 0.97f);
    } else {
        al::scaleVelocity(this, 0.985f);
    }

    sead::Quatf* pQuat = al::getQuatPtr(this);
    al::turnQuatYDirRadian(pQuat, *pQuat, mGroundUpVec, sead::Mathf::deg2rad(5.0f));
}

/** @param isCheckCamera Whether Plessie in view of the camera stays where she is. */
void RaidonSurf::whileWait(bool isCheckCamera) {
    updateGroundUpVec();
    updateOnGround();
    if (shouldRespawn(isCheckCamera)) {
        mAnimState->endMove();
        al::setNerve(this, &NrvRaidonSurfDeSpawn);
    }

    if (!isOnGroundOrWaterRaidon()) {
        al::setNerve(this, &NrvRaidonSurfFall);
    }
}

/** @brief Lets the player get on and starts once they are bound. */
void RaidonSurf::exeGetOn() {
    if (al::isFirstStep(this)) {
        al::BgmPlayingRequest request("RaidonSurf");
        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        bool isInvincible;
        if (controller != nullptr &&
            (controller->isDisasterMode() || controller->isDisasterForeshadow())) {
            isInvincible = true;
        } else {
            isInvincible = rc::isPlayerInvincible(al::getSensorHost(mFirstPlayerSensor));
        }

        s32 fadeOutFrames;
        s32 startDelayFrames;
        if (SingleModeDataFunction::isAlreadyPlayRidon(this)) {
            al::startAction(this, "GetOnRunIdle");
            fadeOutFrames = 36;
            startDelayFrames = 26;
        } else {
            al::startAction(this, "GetOn");
            fadeOutFrames = 70;
            startDelayFrames = 60;
        }

        request.startDelayFrames = isInvincible ? 0 : startDelayFrames;
        request.fadeOutFrames = fadeOutFrames;
        request._18 = isInvincible ? 91000 : -1;
        request.fadeInFrames = isInvincible ? 60 : -1;
        rc::disappearCameraChangeLayoutAndResetCameraMode(this);
        al::startBgm(this, request);
        const sead::Vector3f& rVelocity = al::getVelocity(this);
        if (sead::Mathf::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z) >= 36.0f) {
            al::changeBgmSituation(this, "RaidonSurfFast");
        } else {
            al::changeBgmSituation(this, "RaidonSurfStart");
        }

        if (mNpcPuppets.size() != 0) {
            mIsNpcPuppetBinding = true;
        }

        startFollowCamera();
    }

    backHead();
    al::turnQuatFrontToDirDegreeH(this, mBaseFrontDir, 1.0f);
    if (al::isGreaterEqualStep(this, 120)) {
        rc::requestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
    }

    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    invalidateFirstPlayerWaterEffect(this, mPlayerHolder);
    if (rc::isAllPlayerBinded(this, al::getHitSensor(this, "Bind")) && al::isActionEnd(this)) {
        rc::setDisableReviveBubbleForAllPlayer(this);
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            IUsePlayerPuppet* puppet = mPuppeteers[i].mPuppet;
            if (puppet != nullptr) {
                al::setCameraLookAtPosPtr(this, rc::getPuppetSensor(puppet),
                                          al::getTransPtr(this));
            }
        }

        al::validateHitSensor(this, "PlayerSensor");
        mStartState->start();
        al::setNerve(this, &NrvRaidonSurfStart);
        return;
    }

    updateFollowCamera();
}

/** @brief Runs the ride start state. */
void RaidonSurf::exeStart() {
    addSpringControlRate(0.05f);
    rc::requestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
    if (al::isFirstStep(this)) {
        if (mFirstPlayerSensor != nullptr) {
            auto* player = static_cast<PlayerActor*>(al::getSensorHost(mFirstPlayerSensor));
            rc::invalidatePlayerDamage(player, 120);
            rc::invalidatePlayerFlash(player);
        }

        if (mIsFirstRide) {
            mIsFirstRide = false;
        }
    }

    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    updateFollowCamera();
    if (al::updateNerveState(this)) {
        if (!SingleModeDataFunction::isAlreadyPlayRidon(this)) {
            SingleModeDataFunction::setPlayRidon(GameDataHolderAccessor(this));
            al::setSensorFollowPosOffset(this, "Bind", mBindSensorOffset);
        }

        rc::resetDisableReviveBubbleForAllPlayer(this);
        al::setNerve(this, &NrvRaidonSurfRide);
        mStickOffStep = 90;
        mAnimState->requestIdle();
    }
}

/** @brief Swims by the riders' input. */
void RaidonSurf::exeRide() {
    addSpringControlRate(0.05f);
    if (mIsRequestPlessieMode) {
        setPlessieMode(true);
        mIsRequestPlessieMode = false;
    }

    if (al::isFirstStep(this)) {
        al::onCollide(this);
        al::startAction(this, "SwimWait");
        if (mCameraArea != nullptr && DisasterModeController::tryGetController(this) != nullptr) {
            mCameraArea->validate();
            if (CameraPoserFollowLimit* poser = tryGetFollowCameraPoser()) {
                auto* areaPoser = static_cast<CameraPoserFollowLimit*>(
                    getCameraDirector_RS()->findCameraAreaTicket(mCameraArea)->getPoser());
                f32 waterHeight;
                if (al::calcWaterDistanceCheck(this, al::getTrans(this), -1000.0f,
                                               &waterHeight)) {
                    areaPoser->setWaterHeight(waterHeight);
                    areaPoser->setSnapShotRollFollow(true);
                }

                areaPoser->setEye(poser->getEye());
                areaPoser->setAt(poser->getAt());
                mIsRequestPlessieMode = true;
            }
        }

        mFallAreaChecker->reset();
        al::tryOnStageSwitch(this, "SwitchRideOn");
        if (mIsShowGuideWindow && !mIsNoGuideWindow) {
            bool isAppear;
            if (al::isPadTypeJoySingle(al::getMainControllerPort())) {
                isAppear = rc::appearGuideGameWindowWithPriority(
                    this, "SingleMode_GuideMessage", "PlessiGuide_SingleJoycons",
                    static_cast<GuideMessagePriority>(4), -1, 0.0f);
            } else {
                isAppear = rc::appearGuideGameWindowWithPriority(
                    this, "SingleMode_GuideMessage", "PlessiGuide_DualJoycons",
                    static_cast<GuideMessagePriority>(4), -1, 0.0f);
            }

            if (isAppear) {
                mIsShowGuideWindow = false;
                if (rc::isInAreaObjPlayerOne(this, rc::AreaObjType::RaidonUIArea, mPlayerHolder)) {
                    mGuideState = GuideState_AreaRideGuide;
                } else {
                    mGuideState = GuideState_Failed;
                }
            }
        }

        mFastBgmCount = 0;
        mSlowBgmCount = 0;
    }

    mFallAreaChecker->update(-1);
    if (mFallAreaChecker->isEnteredArea()) {
        startPuppetSe("LongDiving");
    }

    if (isOnGroundOrWaterRaidon() && rc::isInAreaObj(this, rc::AreaObjType::RaidonDisappearArea)) {
        forceSpawn(false);
        return;
    }

    if (rc::isInDeathArea(this)) {
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            if (mPuppeteers[i].mPuppet != nullptr) {
                rc::endBindForceAbyssAndPuppetNull(&mPuppeteers[i].mPuppet);
            }
        }

        mPuppeteerNum = 0;
        mFirstPlayerSensor = nullptr;
        al::setNerve(this, &NrvRaidonSurfAbyss);
        return;
    }

    if (al::HitSensor* pGroundSensor = al::tryGetCollidedGroundSensor(this)) {
        rc::sendMsgRaidonAttack(pGroundSensor, al::getHitSensor(this, "Body"));
    }

    if (al::HitSensor* pWallSensor = al::tryGetCollidedWallSensor(this)) {
        rc::sendMsgRaidonAttack(pWallSensor, al::getHitSensor(this, "Body"));
    }

    rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, "Bind"));
    mIsAnyPuppetJump = false;
    bool isStickOn = false;
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
        if (mPuppeteers[i].isJump()) {
            mIsAnyPuppetJump = true;
        }

        isStickOn |= rc::isPuppetStickOn(mPuppeteers[i].mPuppet);
    }

    if (isStickOn) {
        mStickOffStep--;
    } else {
        mStickOffStep = 90;
    }

    if (!mIsNoGuideWindow) {
        if (mStickOffStep < 1) {
            if (mGuideState == GuideState_Failed || mGuideState == GuideState_AreaRideGuide) {
                mGuideState = showGameWindow(false);
            }
        } else if (mGuideState <= GuideState_None) {
            mGuideState = showGameWindow(true);
        }
    }

    updateCameraAngleFriction(mIsAnyPuppetJump);
    if (mAnimState->isDive()) {
        updateFollowDiveCamera();
    } else {
        updateFollowCamera();
    }

    updatePuppetInput();
    updateHandleAndAccel();
    updateRide();
    al::updateNerveState(this);

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    mCameraTarget->setRotateY(sead::Mathf::rad2deg(atan2f(frontDir.x, frontDir.z)));
    checkBgmChange();
}

/** @brief Switches between the fast and the slow riding BGM by the speed. */
void RaidonSurf::checkBgmChange() {
    if (mIsBindCanceled) {
        return;
    }

    const sead::Vector3f& rVelocity = al::getVelocity(this);
    f32 speedH = sead::Mathf::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z);
    bool isStickOn = rc::isPuppetStickOn(mPuppeteers[0].mPuppet);
    if (!isStickOn && !mIsFastBgm) {
        mFastBgmCount = 0;
    }

    if (isOnGroundRaidon()) {
        mOnGroundBgmStep++;
        if (mOnGroundBgmStep > 0) {
            mFastBgmSpeed = 36.0f;
            mSlowBgmSpeed = 36.0f;
            mOnGroundBgmStep = 0;
        }

        if (mInWaterBgmStep != 0) {
            mInWaterBgmStep = 0;
        }
    } else if (isInWater()) {
        mInWaterBgmStep++;
        if (mInWaterBgmStep > 20) {
            mSlowBgmSpeed = 55.0f;
            mInWaterBgmStep = 0;
            mFastBgmSpeed = 18.0f;
        }

        if (mOnGroundBgmStep != 0) {
            mOnGroundBgmStep = 0;
        }
    }

    if (speedH > mFastBgmSpeed && !mIsFastBgm) {
        mFastBgmCount++;
        al::changeBgmSituation(this, "RaidonSurfFast");
        mIsFastBgm = true;
        mFastBgmCount = 0;
        return;
    }

    if ((isOnGroundRaidon() || !isInWater()) && speedH <= mSlowBgmSpeed && mIsFastBgm) {
        mSlowBgmCount++;
        al::changeBgmSituation(this, "RaidonSurfSlow");
        mIsFastBgm = false;
        mSlowBgmCount = 0;
        return;
    }

    if (!isOnGroundRaidon() && speedH < mSlowBgmSpeed && mIsFastBgm && !isStickOn) {
        mSlowBgmCount++;
        al::changeBgmSituation(this, "RaidonSurfSlow");
        mIsFastBgm = false;
        mSlowBgmCount = 0;
        return;
    }

    mFastBgmCount = 0;
    mSlowBgmCount = 0;
}

/** @brief Falls after dropping into a death area. */
void RaidonSurf::exeAbyss() {
    al::addVelocityToGravity(this, 1.6f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    al::scaleVelocity(this, 0.985f);
}

/** @brief Lets the players get off on land or on the water. */
void RaidonSurf::exeGoal() {
    if (isInWater()) {
        al::setNerve(this, &NrvRaidonSurfGetOffOnWater);
    } else {
        al::setNerve(this, &NrvRaidonSurfGetOff);
    }
}

/**
 * @brief Throws the players off Plessie.
 * @param isInAir Whether Plessie is in the air.
 * @param isDie Whether Plessie dies, throwing the players backwards.
 */
void RaidonSurf::startSharedGetOff(bool isInAir, bool isDie) {
    stopFollowCamera();
    if (mAnimState != nullptr) {
        mAnimState->endMove();
    }

    al::invalidateHitSensor(this, "PlayerSensor");
    const sead::Vector3f& rVelocity = al::getVelocity(this);
    f32 speedH = sead::Mathf::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z);
    updatePuppetInput();
    updateHandleAndAccel();

    sead::Vector3f dir;
    if (mAccel * mAccel + mHandle * mHandle < 0.01f) {
        al::calcFrontDir(&dir, this);
    } else {
        dir = getInputDirection();
    }

    f32 speed;
    if (isDie) {
        dir.x = -dir.x;
        dir.z = -dir.z;
        speed = 0.5f;
    } else {
        speed = sead::Mathf::max(sead::Mathf::min(speedH, 15.0f), 8.0f);
    }

    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            al::setCameraLookAtPosPtr(this, rc::getPuppetSensor(mPuppeteers[i].mPuppet), nullptr);
            sead::Vector3f getOffDir(dir.x * speed, speed * 0.0f, dir.z * speed);
            dir = getOffDir;
            mPuppeteers[i].startGetOff(dir, i * 15, true, isInAir);
            rc::startPuppetSe(mPuppeteers[i].mPuppet, "JumpVoice");
        }
    }

    mRespawnWaitStep = 0;
    mIsRespawnWaiting = true;
    mSpawnIndex = -1;
    rc::disappearGuideGameWindow(this);
    mIsShowGuideWindow = true;
    if (mCameraArea != nullptr) {
        mCameraArea->invalidate();
    }
}

/** @return Horizontal direction of the riders' stick input relative to the camera. */
sead::Vector3f RaidonSurf::getInputDirection() {
    sead::Vector3f lookDir;
    sead::Vector3f rightDir;
    al::getLookAtCamera(this, 0).getRightVectorByMatrix(&rightDir);
    rightDir.normalize();

    al::getLookAtCamera(this, 0).getLookVectorByMatrix(&lookDir);
    lookDir.y = 0.0f;
    lookDir.normalize();

    sead::Vector3f accelDir(lookDir.x * mAccel, lookDir.y * mAccel, lookDir.z * mAccel);
    sead::Vector3f dir = mHandle * rightDir - accelDir;
    dir.normalize();
    return dir;
}

/** @return Whether all players got off. */
bool RaidonSurf::updateSharedGetOff() {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    validateFirstPlayerWaterEffect(this, mPlayerHolder);
    if (isAllGetOffPlayer()) {
        mPuppeteerNum = 0;
        mFirstPlayerSensor = nullptr;
        return true;
    }

    return false;
}

/** @brief Lets the players get off on land. */
void RaidonSurf::exeGetOff() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "RunBrake");
        startSharedGetOff(false, false);
        if (!isNotChangeBgm()) {
            al::startBgm(this, al::BgmPlayingRequest("RaidonWait", 120, 0, 150));
        }
    }

    subSpringControlRate(0.05f);
    updateFollowCamera();
    al::scaleVelocity(this, 0.1f);
    if (updateSharedGetOff()) {
        al::tryOffStageSwitch(this, "SwitchRideOn");
        al::stopBgm(this, "RaidonSurf", 90, 180);
        mIsFastBgm = false;
        al::setNerve(this, &NrvRaidonSurfEnd);
    }
}

/** @brief Lets the players get on while Plessie swims. */
void RaidonSurf::exeGetOnInWater() {
    if (al::isFirstStep(this)) {
        if (!SingleModeDataFunction::isAlreadyPlayRidon(this)) {
            SingleModeDataFunction::setPlayRidon(GameDataHolderAccessor(this));
            al::setSensorFollowPosOffset(this, "Bind", mBindSensorOffset);
        }

        al::startAction(this, "SwimGetOn");
        rc::disappearCameraChangeLayoutAndResetCameraMode(this);
        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        bool isInvincible;
        if (controller != nullptr &&
            (controller->isDisasterMode() || controller->isDisasterForeshadow())) {
            isInvincible = true;
        } else {
            isInvincible = rc::isPlayerInvincible(al::getSensorHost(mFirstPlayerSensor));
        }

        al::BgmPlayingRequest request("RaidonSurf");
        request.startDelayFrames = isInvincible ? 0 : 26;
        request.fadeOutFrames = 36;
        request._18 = isInvincible ? 91000 : -1;
        request.fadeInFrames = isInvincible ? 60 : -1;
        al::startBgm(this, request);
        const sead::Vector3f& rVelocity = al::getVelocity(this);
        if (sead::Mathf::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z) >= 18.0f) {
            al::changeBgmSituation(this, "RaidonSurfFast");
        } else {
            al::changeBgmSituation(this, "RaidonSurfStart");
        }

        if (mNpcPuppets.size() != 0) {
            mIsNpcPuppetBinding = true;
        }

        startFollowCamera();
    }

    updateFollowCamera();
    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    invalidateFirstPlayerWaterEffect(this, mPlayerHolder);
    if (al::isGreaterEqualStep(this, 120)) {
        rc::requestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
    }

    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    if (rc::isAllPlayerBinded(this, al::getHitSensor(this, "Bind"))) {
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            IUsePlayerPuppet* puppet = mPuppeteers[i].mPuppet;
            if (puppet != nullptr) {
                al::setCameraLookAtPosPtr(this, rc::getPuppetSensor(puppet),
                                          al::getTransPtr(this));
            }
        }

        al::validateHitSensor(this, "PlayerSensor");
        if (al::isActionEnd(this)) {
            mStartState->start();
            al::setNerve(this, &NrvRaidonSurfRide);
            mAnimState->requestIdle();
            mIsBindCanceled = false;
        }
    }
}

/** @brief Lets the players get off while Plessie swims. */
void RaidonSurf::exeGetOffOnWater() {
    if (al::isFirstStep(this)) {
        const sead::Vector3f& rVelocity = al::getVelocity(this);
        if (sead::Mathf::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z) <= 5.0f) {
            al::startAction(this, "SwimWaitBrake");
        } else {
            al::startAction(this, "SwimBrake");
        }

        mAnimState->updateSwimSound();
        startSharedGetOff(false, false);
        al::tryOffStageSwitch(this, "SwitchRideOn");
        al::stopBgm(this, "RaidonSurf", 90, 180);
        mIsFastBgm = false;
    }

    updateWaterPosVelocity(0.0f, 0.1f, 0.0f);
    if (updateSharedGetOff() && al::isActionEnd(this)) {
        al::setNerve(this, &NrvRaidonSurfDelayWaitInWater);
    }
}

/** @brief Lets the players get off while Plessie is in the air. */
void RaidonSurf::exeGetOffInAir() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AirBrake");
        mAnimState->updateSwimSound();
        startSharedGetOff(false, false);
        al::tryOffStageSwitch(this, "SwitchRideOn");
        al::stopBgm(this, "RaidonSurf", 90, 180);
        mIsFastBgm = false;
        updateWaterPosVelocity(0.0f, 0.1f, 0.0f);
    } else {
        updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
        updateOnGround();
    }

    if (updateSharedGetOff()) {
        if (isOnGroundOrWaterRaidon()) {
            al::setNerve(this, &NrvRaidonSurfLand);
        } else if (al::isActionEnd(this)) {
            al::setNerve(this, &NrvRaidonSurfFall);
        }
    }
}

/** @brief Waits a moment in the water after the players got off. */
void RaidonSurf::exeDelayWaitInWater() {
    if (al::isOnGround(this, 0, 0.0f)) {
        al::setNerve(this, &NrvRaidonSurfEnd);
        return;
    }

    if (al::isFirstStep(this)) {
        al::startAction(this, "SwimWaitSolo");
        mAnimState->endMove();
    }

    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    tryEndNpcPuppetBindNearPlayer();
    if (al::isGreaterStep(this, 120)) {
        al::setNerve(this, &NrvRaidonSurfWaitInWater);
    }
}

/** @brief Waits in the water for the players to get on. */
void RaidonSurf::exeWaitInWater() {
    if (!isInWater() && isOnGroundRaidon()) {
        al::setNerve(this, &NrvRaidonSurfWait);
    } else if (al::tryStartActionIfNotPlaying(this, "SwimWaitSolo")) {
        al::tryEmitEffect(this, "SwimNeutralRipple", nullptr);
    }

    if (al::LiveActor* player = rc::tryFindNearestActivePlayerActorInSphere(this, 6000.0f)) {
        turnHead(al::getTrans(player));
    } else {
        backHead();
    }

    tryEndNpcPuppetBindNearPlayer();
    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    whileWait(true);
}

/** @brief Reacts to an attack while waiting in the water. */
void RaidonSurf::exeWaitInWaterReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "SwimMoveReaction");
        al::tryEmitEffect(this, "SwimNeutralRipple", nullptr);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvRaidonSurfWaitInWater);
        return;
    }

    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    whileWait(true);
}

/** @brief Waits a moment after the players got off. */
void RaidonSurf::exeEnd() {
    if (al::isFirstStep(this)) {
        rc::appearCameraChangeLayout(this);
    }

    tryEndNpcPuppetBindNearPlayer();
    if (al::isGreaterStep(this, 30)) {
        al::setNerve(this, &NrvRaidonSurfWait);
    }
}

/** @brief Waits while the players collect the goal item. */
void RaidonSurf::exeCollectItem() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "GoalItemGet");
        al::setVelocity(this, sead::Vector3f::zero);
    }

    if (al::isActionEnd(this)) {
        al::startAction(this, "SwimWaitSolo");
        al::tryEmitEffect(this, "SwimNeutralRipple", nullptr);
        mBindSensorTimer = 15;
    }

    if (al::isGreaterStep(this, 360)) {
        if (isOnGroundOrWaterRaidon()) {
            al::setNerve(this, &NrvRaidonSurfWaitInWater);
        } else {
            al::setNerve(this, &NrvRaidonSurfFall);
        }
    }
}

/** @brief Appears at the chosen spawn point. */
void RaidonSurf::exeSpawn() {
    al::getTrans(this);
    if (al::isFirstStep(this) && mSpawnIndex >= 0) {
        al::showModelIfHide(this);
        al::validateHitSensors(this);
        sead::Vector3f surfacePos;
        sead::Vector3f spawnTrans = mSpawnPoints[mSpawnIndex]->trans;
        if (WaterUtil::checkWaterSurface(this, &surfacePos, spawnTrans, 100.0f, nullptr)) {
            mWaterSurfaceY = surfacePos.y;
            spawnTrans.y = mWaterSurfaceY + -100.0f;
        }

        al::resetPosition(this, spawnTrans, false);
        al::faceToDirection(this, mSpawnPoints[mSpawnIndex]->front);
        al::turnVecToVecDegree(&mGroundUpVec, mGroundUpVec, sead::Vector3f::ey, 1.0f);
        al::startAction(this, "Spawn");
    }

    f32 alpha = al::getActionFrame(this) / 30.0f;
    mGlobalAlphaLastFrame = alpha > 1.0f ? 1.0f : alpha;
    if (mSpawnIndex >= 0 && al::isActionEnd(this)) {
        mGlobalAlphaLastFrame = 1.0f;
        al::setNerve(this, &NrvRaidonSurfWaitInWater);
    }
}

/** @brief Shows Plessie and turns her collision back on. */
void RaidonSurf::reactivate() {
    al::showModelIfHide(this);
    al::validateHitSensors(this);
}

/** @brief Disappears, either diving away or with a jump. */
void RaidonSurf::exeDeSpawn() {
    if (al::isFirstStep(this)) {
        offSpringControl();
        al::setVelocity(this, sead::Vector3f::zero);
        al::updatePoseMtx(mSoundActor, getBaseMtx());
        if (al::isNerve(this, &NrvRaidonSurfDeSpawnForce) || !isInWater()) {
            al::startAction(this, "DeSpawnJump");
            al::addVelocityJump(this, 12.0f);
        } else {
            al::startAction(this, "Despawn");
            al::startSe(mSoundActor, "PgDeSpawn");
        }

        endNpcPuppetBindAll(static_cast<NpcPuppetBindEndType>(2));
    }

    if (!al::isNerve(this, &NrvRaidonSurfDeSpawnForce) &&
        al::isActionPlaying(this, "Despawn")) {
        mGlobalAlphaLastFrame =
            1.0f - al::getActionFrame(this) / al::getActionFrameMax(this, "Despawn");
    }

    if ((al::isActionOneTime(this, al::getActionName(this)) && al::isActionEnd(this)) ||
        al::isNerve(this, &NrvRaidonSurfDeSpawnForce)) {
        deactivate();
        if (al::isActionPlaying(this, "DeSpawnJump") ||
            al::isNerve(this, &NrvRaidonSurfDeSpawnForce)) {
            if (!al::isEffectEmitting(this, "Poof")) {
                al::tryEmitEffect(this, "Poof", nullptr);
            }
        }

        if (!al::isDead(mRainActor)) {
            mRainActor->kill();
        }

        al::setVelocityY(this, 0.0f);
        al::setNerve(this, &NrvRaidonSurfDeSpawnEnd);
    }
}

/** @brief Hides Plessie and turns her collision off. */
void RaidonSurf::deactivate() {
    al::hideModelIfShow(this);
    if (!al::isInvincibleModelHidden(this)) {
        al::tryDeleteEffect(this, "SuperStar");
        al::hideInvincibleModel(this);
    }

    al::offCollide(this);
    al::invalidateHitSensors(this);
}

/** @brief Falls until Plessie lands. */
void RaidonSurf::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FallSolo");
    }

    if (isOnGroundOrWaterRaidon()) {
        al::setNerve(this, &NrvRaidonSurfLand);
        return;
    }

    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    updateOnGround();
}

/** @brief Lands on the ground or in the water. */
void RaidonSurf::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, isInWater() ? "AirEnd" : "AirEndGround");
        if (isInWater()) {
            al::startOceanWave(this, "JumpEnd");
            f32 velocityY = al::getVelocity(this).y;
            if (isInWater() && velocityY < -40.0f) {
                al::startHitReaction(this, "BigLand");
            }
        }
    }

    bool isWater = isInWater();
    bool isEnd = al::isActionEnd(this);
    if (isWater) {
        if (isEnd) {
            al::setNerve(this, &NrvRaidonSurfWaitInWater);
            return;
        }
    } else if (isEnd) {
        if (al::isActionPlaying(this, "AirEnd") || al::isActionPlaying(this, "AirEndGround")) {
            al::startAction(this, "RunBrake");
        } else if (al::isActionPlaying(this, "RunBrake")) {
            al::setNerve(this, &NrvRaidonSurfWait);
        }

        return;
    }

    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    updateOnGround();
}

/** @brief Binds the players again after collecting the goal item. */
void RaidonSurf::exeRebind() {
    if (al::isFirstStep(this)) {
        startFollowCamera();
        rc::requestBindAllPlayer(this, al::getHitSensor(this, "Bind"));
    }

    updateFollowCamera();
    updateWaterPosVelocity(0.0f, 0.0f, 0.0f);
    invalidateFirstPlayerWaterEffect(this, mPlayerHolder);
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateNerve();
    }

    if (rc::isAllPlayerBinded(this, al::getHitSensor(this, "Bind"))) {
        for (s32 i = 0; i < mPuppeteerNum; i++) {
            IUsePlayerPuppet* puppet = mPuppeteers[i].mPuppet;
            if (puppet != nullptr) {
                al::setCameraLookAtPosPtr(this, rc::getPuppetSensor(puppet),
                                          al::getTransPtr(this));
            }
        }

        al::validateHitSensor(this, "PlayerSensor");
        al::setNerve(this, &NrvRaidonSurfRide);
        mAnimState->requestIdle();
    }
}

/** @brief Dies, throwing the players off and vanishing. */
void RaidonSurf::exeDie() {
    if (al::isFirstStep(this)) {
        offSpringControl();
        al::setVelocity(this, sead::Vector3f::zero);
        if (mFirstPlayerSensor != nullptr) {
            mAnimState->updateSwimSound();
            startSharedGetOff(false, al::isNerve(this, &NrvRaidonSurfDieLava));
            al::tryOffStageSwitch(this, "SwitchRideOn");
            al::stopBgm(this, "RaidonSurf", 90, 180);
            mIsFastBgm = false;
        }

        al::updatePoseMtx(mSoundActor, getBaseMtx());
        if (!al::isNerve(this, &NrvRaidonSurfDieLava) &&
            (isInWater() || (mIsOnInk && mCollisionType == 1))) {
            al::startAction(this, "Despawn");
            al::startSe(mSoundActor, "PgDeSpawnDie");
        } else {
            al::startAction(this, "AirEndGround");
        }

        endNpcPuppetBindAll(static_cast<NpcPuppetBindEndType>(2));
        return;
    }

    if (mFirstPlayerSensor != nullptr) {
        updateSharedGetOff();
    }

    if (al::isActionPlaying(this, "Despawn")) {
        mGlobalAlphaLastFrame =
            1.0f - al::getActionFrame(this) / al::getActionFrameMax(this, "Despawn");
    } else {
        al::hideModelIfShow(this);
        al::tryEmitEffect(this, "Poof", nullptr);
        al::startSe(mSoundActor, "PgVanish");
    }

    if ((al::isActionPlaying(this, "Despawn") && al::isActionEnd(this)) ||
        al::isHideModel(this)) {
        deactivate();
        if (!al::isDead(mRainActor)) {
            mRainActor->kill();
        }

        al::setNerve(this, &NrvRaidonSurfDeath);
    }
}

/** @brief Stays dead until Plessie can respawn near the player. */
void RaidonSurf::exeDeath() {
    if (al::isFirstStep(this)) {
        if (al::isEffectEmitting(this, "SwimNeutralRipple")) {
            al::tryDeleteEffect(this, "SwimNeutralRipple");
        }

        mGlobalAlphaLastFrame = 0.0f;
    }

    if ((al::isNerve(this, &NrvRaidonSurfDeath) && al::isGreaterEqualStep(this, 300)) ||
        al::isNerve(this, &NrvRaidonSurfDeSpawnEnd)) {
        if (!rc::isPlayerInCloudBonus(this) && isReadyToSpawn(true)) {
            al::setNerve(this, &NrvRaidonSurfSpawn);
        }
    }
}

/** @brief Ends the bind of every rider.
 * @param pParam How the players leave Plessie.
 */
void RaidonSurf::endBind(const PlayerBindEndParam* pParam) {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            rc::endBindAndPuppetNull(&mPuppeteers[i].mPuppet, pParam);
        }
    }

    mPuppeteerNum = 0;
    mFirstPlayerSensor = nullptr;
}

/** @return Whether Plessie is despawning or despawned. */
bool RaidonSurf::isDespawnState() {
    return al::isNerve(this, &NrvRaidonSurfDeSpawn) ||
           al::isNerve(this, &NrvRaidonSurfDeSpawnForce) ||
           al::isNerve(this, &NrvRaidonSurfDeath) ||
           al::isNerve(this, &NrvRaidonSurfDeSpawnEnd);
}

/** @return Horizontal stick input of the first rider. */
f32 RaidonSurf::getPuppetInputStickX() {
    return rc::getPuppetStickX(mPuppeteers[0].mPuppet);
}

/** @brief Reads the input of every rider. */
void RaidonSurf::updatePuppetInput() {
    bool isEnableInput = !rc::isInAreaObj(this, rc::AreaObjType::RaidonNoInputArea) &&
                         !rc::isInAreaObj(this, rc::AreaObjType::BobsledGoalArea);
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mPuppeteers[i].updateInput(isInWater(), isEnableInput);
    }
}

/** @brief Averages the steering and acceleration input of the riders. */
void RaidonSurf::updateHandleAndAccel() {
    mAccel = 0.0f;
    mHandle = 0.0f;
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        mHandle += mPuppeteers[i].getHandle();
        mAccel += mPuppeteers[i].getAccel();
    }

    if (mPuppeteerNum > 0) {
        mHandle /= mPuppeteerNum;
        mAccel /= mPuppeteerNum;
    }
}

/** @brief Turns the ground up vector towards the ground normal below Plessie. */
void RaidonSurf::updateGroundUpVec() {
    if (al::isOnGround(this, 5, 0.0f)) {
        sead::Vector3f normal = al::getOnGroundNormal(this, 5);
        al::verticalizeVec(&normal, mBaseSideDir, normal);
        if (!al::normalizeOrZero(&normal)) {
            al::turnVecToVecDegree(&mGroundUpVec, mGroundUpVec, normal, 1.5f);
        }

        return;
    }

    al::turnVecToVecDegree(&mGroundUpVec, mGroundUpVec, sead::Vector3f::ey, 1.0f);
}

/** @brief Counts the frames Plessie spends on the ground. */
void RaidonSurf::updateOnGround() {
    if (al::isOnGround(this, 0, 0.0f)) {
        mTrampleComboCounter->reset();
        mGroundGraceCount = 5;
        mMultiJumpTimer = 0;
        mGroundCount++;
    } else {
        if (mGroundGraceCount > 0) {
            mGroundGraceCount--;
        }

        mGroundCount = 0;
    }
}

/** @brief Reads the materials of the touched floor and walls. */
void RaidonSurf::updateMatrialCode() {
    mCollisionType = 0;
    mWallCodeName = nullptr;
    mFloorCodeName = nullptr;
    mMaterialCodeName = nullptr;
    bool isAir = true;
    if (isInWater()) {
        al::setMaterialCode(this, "NoCode");
        isAir = false;
    }

    if (al::isCollidedGround(this)) {
        mFloorCodeName = al::getCollidedFloorCodeName(this);
        mWallCodeName = al::getCollidedFloorWallCodeName(this);
        isAir = false;
        mCollisionType = 1;
    }

    if (al::isCollidedWall(this)) {
        mMaterialCodeName = al::getCollidedWallMaterialCodeName(this);
        mWallCodeName = al::getCollidedWallCodeName(this);
        mCollisionType = 2;
    } else if (isAir) {
        al::setMaterialCode(this, "Air");
    }

    mIsOnInk = checkIsMaterial("Ink") || checkIsMaterial("InkSlow") || checkIsMaterial("InkWall");
    if (checkIsMaterial("Skate") || checkIsMaterial("PlessieSkate")) {
        if (!mIsOnSlideGround) {
            mSlideGroundTimer = 5;
            mIsOnSlideGround = true;
        }
    } else if (mIsOnSlideGround) {
        if (mSlideGroundTimer == 0) {
            mIsOnSlideGround = false;
        }

        mSlideGroundTimer--;
    }
}

/** @return Name of the material of the touched wall. */
const char* RaidonSurf::getMaterialCode() {
    return mMaterialCodeName;
}

/** @brief Starts an animation on every rider.
 * @param pActionName Animation name.
 */
void RaidonSurf::startPuppetActionAll(const char* pActionName) {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            rc::startPuppetAction(mPuppeteers[i].mPuppet, pActionName);
        }
    }
}

/** @brief Blends every rider's animation by their own stick input. */
void RaidonSurf::setPuppetInputBlendAnimWeight() {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        RaidonPuppeteer& rPuppeteer = mPuppeteers[i];
        if (rPuppeteer.mPuppet != nullptr) {
            InputBlendWeight weight(rPuppeteer.getStickX(), rPuppeteer.getStickY());
            rc::setPuppetBlendAnimWeight(rPuppeteer.mPuppet, weight.neutral, weight.minusX,
                                         weight.plusX, weight.plusY, weight.minusY, 0.0f);
        }
    }
}

/** @brief Blends Plessie's animation by the averaged input. */
void RaidonSurf::setInputBlendAnimWeight() {
    InputBlendWeight weight(mHandle, sead::Mathf::abs(mAccel));
    al::setSklAnimBlendWeightFivefold(this, weight.neutral, weight.minusX, weight.plusX,
                                      weight.plusY, weight.minusY);
}

/** @return Whether no player rides Plessie anymore. */
bool RaidonSurf::isAllGetOffPlayer() const {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            return false;
        }
    }

    return true;
}

/** @brief Plays a sound on every rider.
 * @param pName Sound name.
 */
void RaidonSurf::startPuppetSe(const char* pName) {
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].mPuppet != nullptr) {
            rc::startPuppetSe(mPuppeteers[i].mPuppet, pName);
        }
    }
}

/** @brief Moves Plessie forward during the ride start. */
void RaidonSurf::updateStart() {
    updateGroundUpVec();
    updateOnGround();
    updateMatrialCode();

    sead::Vector3f frontDir;
    al::calcFrontDir(&frontDir, this);
    al::addVelocityToDirection(this, frontDir, 0.8f);
    al::addVelocityToGravity(this, 1.6f);
    al::reboundVelocityFromCollision(this, 0.0f, 0.0f, 1.0f);
    al::scaleVelocity(this, 0.985f);
}

/** @brief Turns, jumps, dives and gets off by the riders' input. */
void RaidonSurf::updateRide() {
    f32 inputSq = mAccel * mAccel + mHandle * mHandle;
    f32 input = inputSq < 0.01f ? 0.0f : inputSq;
    f32 dashAccel = 0.0f;
    if (mDashTimer > 0) {
        dashAccel = al::lerpValue(mDashTimer, 30.0f, 0.0f, mDashAccel, 0.0f);
    }

    const sead::Vector3f& rVelocity = al::getVelocity(this);
    f32 speedH = sead::Mathf::sqrt(rVelocity.x * rVelocity.x + rVelocity.z * rVelocity.z);
    if (input > 0.0f) {
        sead::Vector3f dir = getInputDirection();
        sead::Vector3f lookAtPos = dir * 5.0f + al::getTrans(this);
        turnHead(lookAtPos);
        mTurnSpeed = speedH > 20.0f ? 2.5f : 5.0f;
        al::turnToDirection(this, dir, mTurnSpeed);
    }

    u32 jumpNum = 0;
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        if (mPuppeteers[i].isRequestJump()) {
            jumpNum++;
        }
    }

    s32 squatNum = 0;
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        squatNum += rc::isPuppetTrigSquatButton(mPuppeteers[i].mPuppet);
    }

    s32 dashNum = 0;
    for (s32 i = 0; i < mPuppeteerNum; i++) {
        dashNum += rc::isPuppetTrigDashButton(mPuppeteers[i].mPuppet);
    }

    if (dashNum != 0) {
        mAnimState->requestDive();
    }

    if (jumpNum != 0 && (isOnGroundOrWaterRaidon() || mGroundGraceCount > 0)) {
        if (!(al::isNerve(this, &NrvRaidonSurfRide) && mAnimState->isJumping()) &&
            !al::isCollidedCeiling(this) && mAnimState->isEnableJump() && mJumpInterval == 0 &&
            mAnimState->requestJump()) {
            mJumpInterval = 18;
        }
    }

    if (squatNum != 0 && !mAnimState->isDive()) {
        al::startSe(this, "GetOff");
        if (isInWater()) {
            al::setNerve(this, &NrvRaidonSurfGetOffOnWater);
        } else if (isOnGroundRaidon()) {
            al::setNerve(this, &NrvRaidonSurfGetOff);
        } else {
            al::setNerve(this, &NrvRaidonSurfGetOffInAir);
        }
    }

    if (jumpNum >= 2 && mMultiJumpTimer > 0) {
        al::getVelocityPtr(this)->y += 15.0f;
        al::startHitReaction(this, "マルチジャンプ成功");
        mMultiJumpTimer = 0;
    }

    if (al::isCollidedWall(this)) {
        f32 wallSpeed = al::getVelocity(this).dot(al::getCollidedWallNormal(this));
        if (checkIsMaterial("Lava")) {
            mLife = 0;
            dieFromDamage(true);
        } else {
            f32 hitSpeed = !mIsOnInk ? -50.0f : -25.0f;
            const char* pHitName = !mIsOnInk ? "WallHit" : "HitInk";
            if (wallSpeed <= hitSpeed && !mAnimState->isHit()) {
                if (!isHittingTorpedoSpikeWall()) {
                    al::startHitReactionHitEffect(this, pHitName, al::getCollidedWallPos(this));
                }

                mAnimState->requestHit();
            }
        }
    }

    if (al::isNerve(this, &NrvRaidonSurfRide) && mAnimState->isJumping() &&
        al::isCollidedCeiling(this)) {
        if (al::getVelocityPtr(this)->y > 0.0f) {
            al::getVelocityPtr(this)->y = 0.0f;
        }

        mAnimState->requestFall();
    }

    if (mAnimState->isDive() && al::isOnGround(this, 0, 0.0f)) {
        if (al::getCollidedGroundNormal(this).dot(mBaseFrontDir) > -0.68f) {
            mAnimState->requestIdle();
        } else {
            mAnimState->requestHit();
        }
    }

    updateGroundUpVec();
    updateOnGround();
    updateWaterPosVelocity(inputSq, 0.0f, dashAccel);
}

/** @return Whether Plessie hits the wall of a torpedo spike. */
bool RaidonSurf::isHittingTorpedoSpikeWall() const {
    const al::CollisionParts* parts = al::tryGetCollidedWallCollisionParts(this);
    if (parts == nullptr || parts->getConnectedHost() == nullptr) {
        return false;
    }

    return al::isEqualString(parts->getConnectedHost()->getName(), "DisasterSpikeTorpedo");
}

/** @param isPerfect Whether the jump was timed perfectly, giving a dash. */
void RaidonSurf::doJump(bool isPerfect) {
    f32 jumpSpeed;
    if (isPerfect) {
        mDashTimer = 50;
        mDashAccel = 0.5f;
        jumpSpeed = 54.0f;
    } else {
        jumpSpeed = mDashTimer > 0 ? 54.0f : 36.0f;
    }

    mGroundGraceCount = 0;
    mMultiJumpTimer = 8;
    f32 velocityY = al::getVelocityPtr(this)->y;
    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    if (velocityY < 0.0f) {
        pVelocity->y = jumpSpeed;
    } else {
        pVelocity->y += jumpSpeed;
    }
}

/** @param isDash Whether Plessie dives with a dash. */
void RaidonSurf::doDive(bool isDash) {
    if (isDash) {
        mDashTimer = 50;
    }

    f32 diveSpeed = mDiveSpeed;
    if (al::getVelocityPtr(this)->y > 0.0f || isInWater()) {
        al::getVelocityPtr(this)->y = -diveSpeed;
    } else {
        al::getVelocityPtr(this)->y -= diveSpeed;
    }
}

/** @param isStrong Whether the jump panel launches Plessie higher. */
void RaidonSurf::doJumpPanel(bool isStrong) {
    f32 jumpSpeed = isStrong ? 80.0f : 54.0f;
    mGroundGraceCount = 0;
    f32 velocityY = al::getVelocityPtr(this)->y;
    sead::Vector3f* pVelocity = al::getVelocityPtr(this);
    if (velocityY < 0.0f) {
        pVelocity->y = jumpSpeed;
    } else {
        pVelocity->y += jumpSpeed;
    }
}

/** @return Puppet of the last rider, if any. */
IUsePlayerPuppet* RaidonSurf::getPuppet() const {
    if (mPuppeteerNum < 1) {
        return nullptr;
    }

    return mPuppeteers[mPuppeteerNum - 1].mPuppet;
}

/** @brief Enables every spawn point. */
void RaidonSurf::enableSpawns() {
    for (s32 i = 0; i < mSpawnPoints.size(); i++) {
        mSpawnPoints(i)->isEnable = true;
    }
}

/** @return Whether Plessie floats on the water surface. */
bool RaidonSurf::isFloating() {
    if (al::isNerve(this, &NrvRaidonSurfStart) || al::isNerve(this, &NrvRaidonSurfLand) ||
        al::isNerve(this, &NrvRaidonSurfWaitInWater) ||
        al::isNerve(this, &NrvRaidonSurfWaitInWaterReaction) ||
        al::isNerve(this, &NrvRaidonSurfDelayWaitInWater)) {
        return true;
    }

    if (al::isNerve(this, &NrvRaidonSurfCollectItem) && isInWater()) {
        return true;
    }

    if (al::isNerve(this, &NrvRaidonSurfGetOffOnWater) ||
        al::isNerve(this, &NrvRaidonSurfGetOnInWater)) {
        return true;
    }

    return mAnimState->isWaitDoDive();
}

/** @brief Makes Plessie count as airborne. */
void RaidonSurf::clearGroundCount() {
    mGroundGraceCount = 0;
    mGroundCount = 0;
}

/** @return Vertical stick input of the first rider. */
f32 RaidonSurf::getPuppetInputStickY() {
    return rc::getPuppetStickY(mPuppeteers[0].mPuppet);
}

/** @return Whether Plessie is in a water area. */
bool RaidonSurf::isInWater() const {
    return rc::isInWaterArea(this);
}

/** @return Whether Plessie stands on the ground or swims. */
bool RaidonSurf::isOnGroundOrWaterRaidon() const {
    return isOnGroundRaidon() || isInWater();
}

/** @return Whether Plessie is in an area she can dive in. */
bool RaidonSurf::isInDiveArea() const {
    return mIsInDiveArea;
}

/** @param isShow Whether the guide game window is shown again or hidden. */
void RaidonSurf::toggleGameWindow(bool isShow) {
    if (isShow) {
        rc::unHideGuideGameWindow(this);
    } else {
        rc::hideGuideGameWindow(this);
    }
}

/** @return Whether the players are getting on. */
bool RaidonSurf::isGetOnNerve() {
    return al::isNerve(this, &NrvRaidonSurfGetOn) ||
           al::isNerve(this, &NrvRaidonSurfGetOnInWater);
}

/** @param speed New dive speed. */
void RaidonSurf::increaseDiveSpeed(s32 speed) {
    mDiveSpeed = speed;
}

/** @brief Toggles drawing the spawn points. */
void RaidonSurf::toggleSpawnDebugDraw() {
    mIsSpawnDebugDraw = !mIsSpawnDebugDraw;
}

/** @param pSensor Sensor of the bell Plessie ran into during the chase. */
void RaidonSurf::plessieChaseHitBells(al::HitSensor* pSensor) {
    sead::Vector3f dir;
    al::calcDirBetweenSensorsH(&dir, pSensor, al::getHitSensor(this, "Body"));
    f32 speed = al::getVelocity(this).dot(dir);
    speed = speed >= 0.0f ? 0.0f : speed;
    f32 pushSpeed = sead::Mathf::abs(speed) < 25.0f ? 25.0f : speed * -1.7f;
    al::addVelocityToDirection(this, dir, pushSpeed);
    al::addVelocityToDirection(this, sead::Vector3f::ey, 105.0f);
    al::scaleVelocityHV(this, 0.8f, 1.0f);
    mHitTimer = 50;
    mAnimState->requestPlessieChaseBellHit();
}

/** @return Whether a jump was started. */
bool RaidonSurf::forceJump() {
    if (!al::isNerve(this, &NrvRaidonSurfRide)) {
        return false;
    }

    if (!isInWater() && !isOnGroundRaidon()) {
        return false;
    }

    if (mAnimState->isRequestJump()) {
        return false;
    }

    if (!mAnimState->isEnableJump()) {
        return false;
    }

    mAnimState->requestJump();
    return true;
}

/**
 * @param pos Position to search the spawn point closest to.
 * @param pSpawnPos Position of the closest spawn point.
 * @param pSpawnFront Front direction of the closest spawn point.
 */
void RaidonSurf::getClosestSpawnPosFront(sead::Vector3f pos, sead::Vector3f* pSpawnPos,
                                         sead::Vector3f* pSpawnFront) {
    s32 index = getClosestSpawnIndex(pos, false);
    if (index == -1) {
        return;
    }

    *pSpawnPos = mSpawnPoints(index)->trans;
    *pSpawnFront = mSpawnPoints[index]->front;
}
