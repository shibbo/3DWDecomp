#include "MapObj/CloudBonusLauncher.hpp"

#include <attributes.h>

#include "Boss/SuperBowserShell.hpp"
#include "Layout/IslandMap.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/CloudBonusLauncherBindPuppeteer.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/Fury/CloudBonusWatcher.hpp"
#include "MapObj/Fury/GameSkyProjection.hpp"
#include "MapObj/WarpObjUtil.hpp"
#include "NPC/ActorStateSupportStroke.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Player/Giga/PlayerActionObserver.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Player/PlayerBindEndParam.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaObjGroup.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/IslandDataList.hpp"
#include "System/ScenarioInfo.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(CloudBonusLauncher, Wait)
NERVE_DECL(CloudBonusLauncher, Nothing)
NERVE_DECL(CloudBonusLauncher, Reaction)
NERVE_DECL(CloudBonusLauncher, InForce)
NERVE_DECL(CloudBonusLauncher, In)
NERVE_DECL(CloudBonusLauncher, End)
NERVE_DECL(CloudBonusLauncher, BonusEndCameraEnd)
NERVE_DECL(CloudBonusLauncher, ReactionSlide)
NERVE_DECL(CloudBonusLauncher, InWait)
NERVE_DECL(CloudBonusLauncher, WaitAllBind)
NERVE_DECL(CloudBonusLauncher, InForceIn)
NERVE_DECL(CloudBonusLauncher, LaunchSign)
NERVE_DECL(CloudBonusLauncher, LaunchStart)
NERVE_DECL(CloudBonusLauncher, LaunchEnd)
NERVE_DECL(CloudBonusLauncher, BonusStartWarpStart)
NERVE_DECL(CloudBonusLauncher, BonusStartWarpEnd)
NERVE_DECL(CloudBonusLauncher, Bonus)
NERVE_DECL(CloudBonusLauncher, WaitAllBindBonusEnd)
NERVE_DECL(CloudBonusLauncher, BonusEndWarpStart)
NERVE_DECL(CloudBonusLauncher, BonusEndWarpEnd)

NERVES_MAKE_NOSTRUCT(CloudBonusLauncher, Wait, Nothing, Reaction, InForce, In, End,
                     BonusEndCameraEnd, ReactionSlide, InWait, WaitAllBind, InForceIn, LaunchSign,
                     LaunchStart, LaunchEnd, BonusStartWarpStart, BonusStartWarpEnd, Bonus,
                     WaitAllBindBonusEnd, BonusEndWarpStart, BonusEndWarpEnd)

/// How the bind of the players ends when they come out of the launcher.
const PlayerBindEndParam cBindEndParam = {{}, 0, 0, false, false, false, 0, 1.2f, 0, false, {}};

/// Name of the sensor the players are bound to.
constexpr const char* cBindSensorName = "Bind";

/**
 * @brief Start the placed object camera, or else a programable camera that keeps its current
 * offset but looks at an actor.
 * @param pActor The actor to look at.
 * @param pObjCamera The placed object camera, or nullptr.
 * @param pCamera The programable camera.
 */
ALWAYS_INLINE inline void startObjOrLookAtCamera(al::LiveActor* pActor,
                                                 const al::CameraInfo* pObjCamera,
                                                 al::CameraInfo* pCamera) {
    const al::IUseCamera* user = pActor;
    if (pObjCamera != nullptr) {
        al::startCamera(user, pObjCamera, -1);
    } else {
        al::startCamera(user, pCamera, -1);
        sead::Vector3f trans = al::getTrans(pActor);
        sead::Vector3f offset = al::getCameraPos(user) - al::getCameraLookAt(user);
        al::setCameraLookAtPos(pCamera, trans);
        al::setCameraPos(pCamera, offset + trans);
    }
}

/**
 * @brief Check whether a player is bound by the bind sensor of an actor.
 * @param pPlayer The player actor.
 * @param pBinder The actor that should bind the player.
 * @return Whether the player is bound, and by that actor if it has a bind sensor.
 */
inline bool isPlayerBindedBy(const al::LiveActor* pPlayer, const al::LiveActor* pBinder) {
    auto* player = static_cast<const PlayerActor*>(pPlayer);
    if (!player->getActionObserver()->isInBind()) {
        return false;
    }

    al::HitSensor* bindSensor = al::getHitSensor(pBinder, cBindSensorName);
    return bindSensor == nullptr || player->getBindSensor() == bindSensor;
}
}  // namespace

/**
 * @brief Construct the launcher.
 * @param pName The actor name.
 */
CloudBonusLauncher::CloudBonusLauncher(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the launcher, its puppeteers, cameras and bonus area links.
 * @param rInfo The actor init info.
 */
void CloudBonusLauncher::init(const al::ActorInitInfo& rInfo) {
    al::initActorChangeModel(this, rInfo);
    al::initNerve(this, &NrvCloudBonusLauncherWait, 1);
    al::tryGetArg(&mIsAllowRestart, rInfo, "AllowRestart");
    al::getLinksMatrix(&mDestMtx, rInfo, "DestCloudBonusLauncher");
    al::getLinksMatrix(&mEndMtx, rInfo, "EndCloudBonusLauncher");

    mPuppeteerGroup =
        new BindPuppeteerGroup("雲ボーナス大砲バインド操作グループ", al::getPlayerNumMax(this));
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNumMax(); i++) {
        mPuppeteerGroup->registerPuppeteer(
            new CloudBonusLauncherBindPuppeteer("雲ボーナス大砲バインド操作", rInfo));
    }

    mBindPuppeteers.allocBuffer(al::getPlayerNumMax(this), nullptr);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
    if (mIsSingleMode) {
        if (al::isExistLinkChild(rInfo.getPlacementInfo(), "BonusAreaSky", 0)) {
            mSky = new GameSkyProjection("CloudBonusSky");
            al::initLinksActor(mSky, rInfo, "BonusAreaSky", 0);
            mSky->kill();
        }

        s32 quadrant = 0;
        al::tryGetArg(&quadrant, rInfo, "Quadrant");
        mQuadrant = IslandDataFunction::getQuadrantIndexFromParam(quadrant);
    }

    bool isUseInWaitCamera = false;
    al::tryGetArg(&isUseInWaitCamera, rInfo, "IsUseInWaitCamera");
    if (isUseInWaitCamera && !mIsSingleMode) {
        mInWaitObjCamera = al::initObjectCamera(this, rInfo, "InWait");
    } else if (mIsSingleMode) {
        mInWaitCameraRS = al::initProgramableCamera_RS(this, rInfo, "CloudInWait",
                                                       &mInWaitCameraPos, &mInWaitCameraLookAt,
                                                       nullptr);
    } else {
        mInWaitCamera = al::initProgramableCamera(this, rInfo, "InWait");
    }

    bool isUseLaunchCamera = false;
    al::tryGetArg(&isUseLaunchCamera, rInfo, "IsUseLaunchCamera");
    if (isUseLaunchCamera && !mIsSingleMode) {
        mLaunchObjCamera = al::initObjectCamera(this, rInfo, "Launch");
    } else if (mIsSingleMode) {
        mLaunchCameraRS = al::initProgramableCamera_RS(this, rInfo, "Launch", &mLaunchCameraPos,
                                                       &mLaunchCameraLookAt, nullptr);
    } else {
        mLaunchCamera = al::initProgramableCamera(this, rInfo, "Launch");
    }

    bool isUseEndWarpCamera = false;
    al::tryGetArg(&isUseEndWarpCamera, rInfo, "IsUseEndWarpCamera");
    if (isUseEndWarpCamera) {
        if (mIsSingleMode) {
            mEndCameraRS = al::initObjectCamera_RS(this, rInfo, nullptr);
        } else {
            mEndObjCamera = al::initObjectCamera(this, rInfo, "EndBonus");
        }
    }

    if (!al::tryGetStringArg(&mBgmName, rInfo, "BgmPlayInfoName")) {
        mBgmName = "Bonus";
    }

    al::trySetShadowLength(this, rInfo, nullptr);
    mEndAreaGroup = al::createLinkAreaGroup(this, rInfo, "CloudBonusEndArea",
                                            "子供エリアグループ", "子供エリア");
    if (al::calcLinkChildNum(rInfo, "CloudBonusEndCameraArea") == 1) {
        al::PlacementInfo areaPlacementInfo;
        al::getLinksInfoByIndex(&areaPlacementInfo, rInfo.getPlacementInfo(),
                                "CloudBonusEndCameraArea", 0);
        al::AreaInitInfo areaInitInfo(areaPlacementInfo, rInfo.getStageSwitchDirector());
        mEndCameraArea = new al::AreaObj("CameraArea");
        mEndCameraArea->init(areaInitInfo);
        mEndCameraArea->mIsSnapToCamera = true;
        mEndCameraArea->mIsSpawnEntranceCamera = true;
        mEndCameraArea->_70 = true;
        mEndCameraArea->invalidate();

        al::AreaObjGroup* cameraAreaGroup = al::tryFindAreaObjGroup(this, "CameraArea");
        if (cameraAreaGroup != nullptr) {
            cameraAreaGroup->resisterAreaObj(mEndCameraArea);
        }
    }

    mWipe = new al::WipeSimple("雲ボーナス大砲ワイプ", "WipeFadeWhite",
                               al::getLayoutInitInfo(rInfo), nullptr);
    mSupportStroke = new ActorStateSupportStroke(this);

    al::PlacementInfo destInfo;
    al::getLinksInfo(&destInfo, al::getPlacementInfo(rInfo), "DestCloudBonusLauncher");
    al::tryGetPlacementID(mDestPlacementId, destInfo);

    al::PlacementInfo endInfo;
    al::getLinksInfo(&endInfo, al::getPlacementInfo(rInfo), "EndCloudBonusLauncher");
    al::tryGetPlacementID(mEndPlacementId, endInfo);

    if (mIsSingleMode) {
        sead::Vector3f destTrans;
        mDestMtx.getTranslation(destTrans);
        auto* watcher = al::getSceneObj<CloudBonusWatcher>(this, SceneObjID_CloudBonusWatcher);
        if (watcher != nullptr) {
            watcher->tryRegisterCloudBonusLauncher(this, destTrans);
        }
    }

    makeActorAppeared();
}

/**
 * @brief Register the cloud bonus Power Moon icon on the island map.
 */
void CloudBonusLauncher::initAfterPlacement() {
    if (!mIsSingleMode) {
        return;
    }

    auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
    if (islandMap == nullptr) {
        return;
    }

    ScenarioInfo scenarioInfo = {-1, -1};
    SingleModeDataFunction::getFirstCloudScenarioByQuadrant(this, mQuadrant, &scenarioInfo);
    if (scenarioInfo.mScenarioIndex == -1) {
        return;
    }

    islandMap->addSpecialShineLocation(this, al::getTrans(this), scenarioInfo);
    islandMap->setSpecialShineIconComplete(
        this, SingleModeDataFunction::allCloudScenariosComplete(this, mQuadrant));
}

/**
 * @brief Update the puppeteers and the stroke state, and react to the microphone.
 */
void CloudBonusLauncher::control() {
    mSupportStroke->update();
    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        if (!al::isNerve(mBindPuppeteers[i], &NrvCloudBonusLauncherNothing)) {
            mBindPuppeteers[i]->updateNerve();
        }
    }

    al::holdSe(this, "PgWait", nullptr);
    if (al::isMicInputOn(this) && al::isNerve(this, &NrvCloudBonusLauncherWait) &&
        al::isGreaterEqualStep(this, 10)) {
        al::setNerve(this, &NrvCloudBonusLauncherReaction);
    }
}

/**
 * @brief Dispatch a message to the launcher's handlers.
 * @param pMsg The message.
 * @param pOther The sensor that sent the message.
 * @param pSelf The sensor of the launcher that received it.
 * @return Whether the message was handled.
 */
bool CloudBonusLauncher::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                    al::HitSensor* pSelf) {
    if (mIsInBonus) {
        if (al::isSensorBindableAllPlayer(pSelf)) {
            return receiveMsgCloudBonusEnd(pMsg, pOther, pSelf);
        }

        return false;
    }

    if (al::isSensorEnemyBody(pSelf)) {
        return receiveMsgCloudBonusLauncher(pMsg, pOther, pSelf);
    }

    if (al::isSensorBindableAllPlayer(pSelf)) {
        return receiveMsgCloudBonusStart(pMsg, pOther, pSelf);
    }

    return false;
}

/**
 * @brief Handle the binds of the players getting into the launcher.
 * @param pMsg The message.
 * @param pOther The sensor of the player.
 * @param pSelf The bind sensor of the launcher.
 * @return Whether the message was handled.
 */
bool CloudBonusLauncher::receiveMsgCloudBonusStart(const al::SensorMsg* pMsg,
                                                   al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgBindStart(pMsg)) {
        return true;
    }

    if (al::isMsgBindInit(pMsg)) {
        auto* puppeteer =
            mPuppeteerGroup->getPuppeteerByPlayerIndex<CloudBonusLauncherBindPuppeteer>(pOther);
        mBindPuppeteers.pushBack(puppeteer);
        if (mIsSingleMode && rc::isPlayerHoldingSomething(pOther)) {
            rc::addDemoActor(rc::getPlayerHoldingActor(al::getSensorHost(pOther)));
        }

        mLastBindPlayerSensor = pOther;

        al::sendMsgWarpStart(pOther, pSelf);
        if (al::isNerve(this, &NrvCloudBonusLauncherInForce)) {
            puppeteer->startBindForce(pOther, pSelf,
                                      al::getTrans(this) + sead::Vector3f(0.0f, 50.0f, 0.0f));
            mForceBindNum++;
            return true;
        }

        puppeteer->startBindInStart(pOther, pSelf);
        if (mIsStarted) {
            al::startAction(this, "In");
        } else {
            mIsStarted = true;
            al::invalidateClipping(this);
            al::setNerve(this, &NrvCloudBonusLauncherIn);
        }

        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
            rc::showPuppet(mBindPuppeteers[i]->getPlayerPuppet());
        }

        mWipe->tryStartOpen(60);
        WarpObjUtil::restartStageTimer(this);
        rc::cancelRequestBindAndResetDisableReviveBubbleForAllPlayer(
            this, al::getHitSensor(this, cBindSensorName));
        mLastBindPlayerSensor = nullptr;
        al::setNerve(this, &NrvCloudBonusLauncherEnd);
        return true;
    }

    return false;
}

/**
 * @brief Handle the binds of the players leaving the cloud bonus area.
 * @param pMsg The message.
 * @param pOther The sensor of the player.
 * @param pSelf The bind sensor of the launcher.
 * @return Whether the message was handled.
 */
bool CloudBonusLauncher::receiveMsgCloudBonusEnd(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                                 al::HitSensor* pSelf) {
    if (al::isMsgBindStart(pMsg)) {
        return true;
    }

    if (al::isMsgBindInit(pMsg)) {
        al::setCameraCalcTargetFlag(al::getSensorHost(pOther), false);
        auto* puppeteer =
            mPuppeteerGroup->getPuppeteerByPlayerIndex<CloudBonusLauncherBindPuppeteer>(pOther);
        mBindPuppeteers.pushBack(puppeteer);
        al::sendMsgWarpStart(pOther, pSelf);
        puppeteer->startBonusEndBind(pOther, pSelf);
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
            rc::showPuppet(mBindPuppeteers[i]->getPlayerPuppet());
        }

        mLastBindPlayerSensor = nullptr;
        mWipe->tryStartOpen(60);
        WarpObjUtil::restartStageTimer(this);
        rc::cancelRequestBindAndResetDisableReviveBubbleForAllPlayer(
            this, al::getHitSensor(this, cBindSensorName));
        al::setNerve(this, &NrvCloudBonusLauncherBonusEndCameraEnd);
        return true;
    }

    return false;
}

/**
 * @brief Make the launcher react to attacks.
 * @param pMsg The message.
 * @param pOther The sensor of the attacker.
 * @param pSelf The body sensor of the launcher.
 * @return Whether the attack was handled.
 */
bool CloudBonusLauncher::receiveMsgCloudBonusLauncher(const al::SensorMsg* pMsg,
                                                      al::HitSensor* pOther,
                                                      al::HitSensor* pSelf) {
    if (al::isMsgEnemyAttackFire(pMsg) || al::isMsgEnemyRouteDokanFire(pMsg) ||
        al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
        al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerHipDropAll(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
        al::isMsgPlayerCooperationHipDrop(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
        al::isMsgKickKouraAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg) ||
        al::isMsgBallAttack(pMsg) || al::isMsgBallTrample(pMsg) || al::isMsgExplosion(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg) ||
        (GameDataFunction::isSingleMode(this) && al::isMsgKeyThrow(pMsg)) ||
        (GameDataFunction::isSingleMode(this) && al::isMsgLaserAttack(pMsg))) {
        if (al::isNerve(this, &NrvCloudBonusLauncherWait) &&
            (!al::isMsgExplosion(pMsg) || !al::isLessStep(this, 45))) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
            al::setNerve(this, &NrvCloudBonusLauncherReaction);
        }

        return !al::isMsgKickKouraAttack(pMsg);
    }

    return false;
}

/**
 * @brief Make the launcher react to touches and strokes on the touch screen.
 * @param pMsg The message.
 * @param pPointer The screen pointer.
 * @param pTarget The screen point target of the launcher.
 * @return Whether the message was handled.
 */
bool CloudBonusLauncher::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                               al::ScreenPointer* pPointer,
                                               al::ScreenPointTarget* pTarget) {
    if (mIsInBonus) {
        return false;
    }

    if (!al::isNerve(this, &NrvCloudBonusLauncherWait) &&
        !al::isNerve(this, &NrvCloudBonusLauncherReaction) &&
        !al::isNerve(this, &NrvCloudBonusLauncherReactionSlide)) {
        return false;
    }

    if (al::isMsgTouchAssistTrig(pMsg)) {
        if (!al::isNerve(this, &NrvCloudBonusLauncherReaction)) {
            al::setNerve(this, &NrvCloudBonusLauncherReaction);
        }

        return true;
    }

    if (mSupportStroke->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (!al::isNerve(this, &NrvCloudBonusLauncherReactionSlide)) {
            al::setNerve(this, &NrvCloudBonusLauncherReactionSlide);
        }

        return true;
    }

    return false;
}

/**
 * @brief Release every bound player.
 */
void CloudBonusLauncher::endBindAllPuppet() {
    rc::resetDisableReviveBubbleForAllPlayer(this);
    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        IUsePlayerPuppet* puppet = mBindPuppeteers[i]->getPlayerPuppet();
        al::sendMsgWarpEnd(rc::getPuppetSensor(puppet), al::getHitSensor(this, cBindSensorName));
        rc::showPuppet(puppet);
        mBindPuppeteers.unsafeAt(i)->endBind(&cBindEndParam);
    }

    if (mLastBindPlayerSensor != nullptr &&
        rc::isPlayerInvincible(this, mLastBindPlayerSensor)) {
        rc::trySetPlayerInvicibleBgmState(mLastBindPlayerSensor, false);
    }

    mLastBindPlayerSensor = nullptr;
    mBindPuppeteers.clear();
}

/**
 * @brief Wait for the players.
 */
void CloudBonusLauncher::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/**
 * @brief React to an attack or a touch.
 */
void CloudBonusLauncher::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvCloudBonusLauncherWait);
    }
}

/**
 * @brief React while being stroked on the touch screen.
 */
void CloudBonusLauncher::exeReactionSlide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "ReactionSlide");
    }

    if (!mSupportStroke->isTouch()) {
        al::setNerve(this, &NrvCloudBonusLauncherWait);
    }
}

/**
 * @brief Start a cutscene camera placed behind and above an actor.
 * @param pActor The actor the camera looks at.
 * @param pTicket The camera.
 * @param pCameraPos Where to store the camera position.
 * @param pCameraLookAt Where to store the camera look-at position.
 */
void CloudBonusLauncher::startCloudCamera(const al::LiveActor* pActor, al::CameraTicket* pTicket,
                                          sead::Vector3f* pCameraPos,
                                          sead::Vector3f* pCameraLookAt) {
    al::startCamera_RS(pActor, pTicket, -1);
    mActiveCameraRS = pTicket;
    sead::Vector3f trans = al::getTrans(pActor);
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    *pCameraLookAt = trans;
    *pCameraPos = front * 1000.0f + trans;
    pCameraPos->y += 500.0f;
}

/**
 * @brief Take the players in.
 */
void CloudBonusLauncher::exeIn() {
    if (al::isFirstStep(this)) {
        if (mIsSingleMode) {
            al::invalidateHitSensors(this);
            if (!rc::requestStartDemoPlayerCutscene(this)) {
                al::setNerve(this, &NrvCloudBonusLauncherIn);
                return;
            }

            rc::setDemoFullEffectUpdate(this, true);
            rc::setDemoAudioType(this, alSeFunction::DemoType(4));
            if (mLastBindPlayerSensor != nullptr &&
                rc::isPlayerInvincible(this, mLastBindPlayerSensor)) {
                rc::trySetPlayerInvicibleBgmState(mLastBindPlayerSensor, true);
            }

            al::startAction(this, "In");
            startCloudCamera(this, mInWaitCameraRS, &mInWaitCameraPos, &mInWaitCameraLookAt);
            rc::addDemoActor(this);
        } else {
            al::startAction(this, "In");
            startObjOrLookAtCamera(this, mInWaitObjCamera, mInWaitCamera);
        }

        WarpObjUtil::stopStageTimer(this);
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvCloudBonusLauncherInWait);
    }
}

/**
 * @brief Wait for the other players to get in.
 */
void CloudBonusLauncher::exeInWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "InWait");
    }

    if (!al::isGreaterEqualStep(this, 0)) {
        return;
    }

    if (mIsSingleMode && !al::isGreaterEqualStep(this, 30)) {
        return;
    }

    s32 freePlayerNum = 0;
    s32 bubblePlayerNum = 0;
    for (s32 i = 0; i < al::getPlayerNumMax(this); i++) {
        al::LiveActor* player = al::getPlayerActor(this, i);
        bool isBubble = rc::isPlayerBubble(player);
        if (isBubble || rc::isPlayerDead(player)) {
            bubblePlayerNum += isBubble;
        } else {
            freePlayerNum += !rc::isPlayerBinded(player);
        }
    }

    if (freePlayerNum == 0 && bubblePlayerNum == 0) {
        if (rc::checkAllPlayerBindedAndDisableReviveBubble(
                this, al::getHitSensor(this, cBindSensorName))) {
            al::setNerve(this, &NrvCloudBonusLauncherWaitAllBind);
            return;
        }
    } else if (bubblePlayerNum != 0 && freePlayerNum == 0) {
        al::setNerve(this, &NrvCloudBonusLauncherInForce);
        return;
    }

    if (al::isGreaterEqualStep(this, 70)) {
        al::setNerve(this, &NrvCloudBonusLauncherInForce);
    }
}

/**
 * @brief Force the remaining players into the launcher.
 */
void CloudBonusLauncher::exeInForce() {
    al::HitSensor* bindSensor = al::getHitSensor(this, cBindSensorName);
    rc::requestBindAllPlayer(this, bindSensor);
    if (!rc::checkAllPlayerBindedAndDisableReviveBubble(this, bindSensor)) {
        return;
    }

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        if (!mBindPuppeteers.unsafeAt(i)->isInWait()) {
            return;
        }
    }

    if (mForceBindNum > 0) {
        al::setNerve(this, &NrvCloudBonusLauncherInForceIn);
    } else {
        al::setNerve(this, &NrvCloudBonusLauncherWaitAllBind);
    }
}

/**
 * @brief Play the take-in animation for the forced players.
 */
void CloudBonusLauncher::exeInForceIn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "In");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvCloudBonusLauncherWaitAllBind);
    }
}

/**
 * @brief Wait until every player is bound.
 */
void CloudBonusLauncher::exeWaitAllBind() {
    al::HitSensor* bindSensor = al::getHitSensor(this, cBindSensorName);
    rc::requestBindAllPlayer(this, bindSensor);
    if (rc::checkAllPlayerBindedAndDisableReviveBubble(this, bindSensor)) {
        al::setNerve(this, &NrvCloudBonusLauncherLaunchSign);
        if (mIsSingleMode) {
            al::endCamera_RS(this, mInWaitCameraRS, -1, false);
        }
    }
}

/**
 * @brief Get ready to fire.
 */
void CloudBonusLauncher::exeLaunchSign() {
    if (al::isFirstStep(this)) {
        if (mIsSingleMode) {
            if (mLaunchCameraRS != nullptr) {
                al::startAction(this, "LaunchSign");
                startCloudCamera(this, mLaunchCameraRS, &mLaunchCameraPos, &mLaunchCameraLookAt);
            }
        } else {
            al::startAction(this, "LaunchSign");
            startObjOrLookAtCamera(this, mLaunchObjCamera, mLaunchCamera);
        }
    }

    if (al::isActionEnd(this)) {
        mLaunchIndex = 0;
        al::setNerve(this, &NrvCloudBonusLauncherLaunchStart);
    }
}

/**
 * @brief Fire the players one after the other.
 */
void CloudBonusLauncher::exeLaunchStart() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LaunchStart");
        mBindPuppeteers[mLaunchIndex]->launchStart(50.0f);
        mLaunchIndex++;
    }

    if (!al::isGreaterEqualStep(this, 10)) {
        return;
    }

    if (mLaunchIndex == mBindPuppeteers.size()) {
        al::setNerve(this, &NrvCloudBonusLauncherLaunchEnd);
    } else {
        al::setNerve(this, &NrvCloudBonusLauncherLaunchStart);
    }
}

/**
 * @brief Finish firing and fade the screen out.
 */
void CloudBonusLauncher::exeLaunchEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "LaunchEnd");
    }

    if (al::isStep(this, 0)) {
        al::pauseBgm(this, "Stage", 120);
    }

    if (al::isGreaterEqualStep(this, 15)) {
        for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
            mBindPuppeteers[i]->launchEnd();
        }

        mWipe->startClose(60);
        if (mIsSingleMode) {
            SingleModeDataFunction::setMapEnabled(GameDataHolderAccessor(this), false);
            auto* sceneLayout = al::tryGetSceneObj<SingleModeSceneLayout>(
                this, SceneObjID_SingleModeSceneLayout);
            if (sceneLayout != nullptr) {
                sceneLayout->setDisablePause(true);
            }
        }

        al::setNerve(this, &NrvCloudBonusLauncherBonusStartWarpStart);
    }
}

/**
 * @brief Move the players into the cloud bonus area once the screen faded out.
 */
void CloudBonusLauncher::exeBonusStartWarpStart() {
    if (!mWipe->isCloseEnd()) {
        return;
    }

    mWipe->startOpen(60);
    if (mIsSingleMode) {
        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            rc::addDemoActor(controller);
            controller->forceDisasterForeshadowOff(true, 0);
            auto* shell = static_cast<SuperBowserShell*>(controller->getDemoSubActor());
            if (shell != nullptr) {
                shell->setEffectsOn(false);
            }
        }

        auto* watcher = al::getSceneObj<CloudBonusWatcher>(this, SceneObjID_CloudBonusWatcher);
        if (watcher != nullptr) {
            watcher->setPlayerInCloudBonusStage(true);
        }

        auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
        if (islandMap != nullptr) {
            islandMap->updatePlayerTrackerForBonusArea(al::getTrans(this));
            islandMap->setIsPlayerInBonusArea(true);
        }

        if (mLaunchCameraRS != nullptr) {
            al::endCamera_RS(this, mLaunchCameraRS, -1, false);
            rc::requestEndDemoPlayerCutscene(this);
        }

        PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            koopaJr->startCloudBonusLauncher();
        }
    } else {
        al::endCamera(this, mLaunchObjCamera != nullptr ? mLaunchObjCamera : mLaunchCamera, -1);
    }

    al::requestCancelInterpole(this);
    if (mIsSingleMode) {
        al::requestCancelCameraInterpole(this, 0);
    }

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        rc::startPuppetAction(mBindPuppeteers[i]->getPlayerPuppet(), "Jump");
    }

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        mBindPuppeteers[i]->startBonusStartWarp(mDestMtx, i, mBindPuppeteers.size());
    }

    al::startBgm(this, mBgmName, -1, 0, -1, -1);
    al::setNerve(this, &NrvCloudBonusLauncherBonusStartWarpEnd);
}

/**
 * @brief Wait until the players landed in the cloud bonus area.
 */
void CloudBonusLauncher::exeBonusStartWarpEnd() {
    if (al::isFirstStep(this)) {
        al::hideModel(this);
        if (rc::isExistGhostPlayerRecorder(this)) {
            rc::setPlacementIdObjGhostPlayerRecorder(this, mDestPlacementId, false);
            rc::tryStartFromObjGhostPlayerRecorder(this, mDestPlacementId);
        }

        if (mIsSingleMode) {
            al::tryOnStageSwitchInstant(this, "SwitchBonusOn");
        }

        if (mSky != nullptr && mIsSingleMode) {
            DisasterModeController* controller = DisasterModeController::tryGetController(this);
            if (controller != nullptr) {
                controller->setSkyEnable(false);
                mSky->appear();
            }
        }
    }

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        if (mBindPuppeteers[i]->isBonusStartWarp()) {
            return;
        }
    }

    endBindAllPuppet();
    WarpObjUtil::restartStageTimer(this);
    mIsInBonus = true;
    al::setNerve(this, &NrvCloudBonusLauncherBonus);
}

/**
 * @brief Disappear, or get ready again when the launcher can be reused.
 */
void CloudBonusLauncher::exeEnd() {
    if (mIsAllowRestart) {
        restartLauncher(true);
    } else {
        kill();
    }
}

/**
 * @brief Reset the launcher so it can be used again.
 * @param isRestartWait Whether to also make the launcher wait for the players again.
 */
void CloudBonusLauncher::restartLauncher(bool isRestartWait) {
    mIsInBonus = false;
    mIsStarted = false;
    mForceBindNum = 0;
    mLaunchIndex = 0;
    al::showModelIfHide(this);
    if (isRestartWait) {
        al::validateClipping(this);
        al::setNerve(this, &NrvCloudBonusLauncherWait);
    }
}

/**
 * @brief Wait in the cloud bonus area until every player stepped on the exit.
 */
void CloudBonusLauncher::exeBonus() {
    if (mIsSingleMode && al::isFirstStep(this)) {
        auto* watcher = al::getSceneObj<CloudBonusWatcher>(this, SceneObjID_CloudBonusWatcher);
        if (watcher != nullptr) {
            watcher->tryUpdateLastCloudBonusFileID(this);
        }

        PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            koopaJr->endCloudBonusLauncher();
        }

        auto* raidonSurf = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
        if (raidonSurf != nullptr) {
            raidonSurf->forceSpawn(false);
        }

        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            controller->setDisasterProgressEnable(false);
        }

        SingleModeDataFunction::setMapEnabled(GameDataHolderAccessor(this), true);
        auto* sceneLayout =
            al::tryGetSceneObj<SingleModeSceneLayout>(this, SceneObjID_SingleModeSceneLayout);
        if (sceneLayout != nullptr) {
            sceneLayout->setDisablePause(false);
        }
    }

    for (s32 i = 0; i < al::getPlayerNumMax(this); i++) {
        if (al::isPlayerDead(this, i) || rc::isPlayerBinded(this, i)) {
            continue;
        }

        if (al::tryIsInAreaObj(mEndAreaGroup, al::getPlayerPos(this, i))) {
            continue;
        }

        rc::requestPlayerBind(al::getHitSensor(al::getPlayerActor(this, i), "Body"),
                              al::getHitSensor(this, cBindSensorName));
    }

    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        IUsePlayerPuppet* puppet = mBindPuppeteers[i]->getPlayerPuppet();
        if (rc::isPlayerHoldingSomething(rc::getPuppetSensor(puppet))) {
            continue;
        }

        al::LiveActor* player = al::getSensorHost(rc::getPuppetSensor(puppet));
        if (rc::isPlayerHoldingSomething(al::getHitSensor(player, "Body"))) {
            continue;
        }

        mBindPuppeteers[i]->tryTemporaryWarp();
    }

    for (s32 i = 0; i < al::getPlayerNumMax(this); i++) {
        al::LiveActor* player = al::getPlayerActor(this, i);
        if (rc::isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (!isPlayerBindedBy(player, this)) {
            return;
        }
    }

    WarpObjUtil::stopStageTimer(this);
    al::setNerve(this, &NrvCloudBonusLauncherWaitAllBindBonusEnd);
}

/**
 * @brief Wait until every living player is bound to leave the cloud bonus area.
 */
void CloudBonusLauncher::exeWaitAllBindBonusEnd() {
    rc::requestBindAllPlayer(this, al::getHitSensor(this, cBindSensorName));
    for (s32 i = 0; i < al::getPlayerNumMax(this); i++) {
        al::LiveActor* player = al::getPlayerActor(this, i);
        if (rc::isPlayerBubble(al::getPlayerActor(this, i))) {
            continue;
        }

        if (rc::isPlayerDead(al::getPlayerActor(this, i))) {
            if (!al::isDead(player) && rc::isPlayingDeadAnim(player)) {
                return;
            }

            continue;
        }

        if (!isPlayerBindedBy(player, this)) {
            return;
        }
    }

    al::setNerve(this, &NrvCloudBonusLauncherBonusEndWarpStart);
}

/**
 * @brief Fade out and move the players back to the exit launcher.
 */
void CloudBonusLauncher::exeBonusEndWarpStart() {
    if (al::isFirstStep(this)) {
        if (mIsSingleMode) {
            SingleModeDataFunction::setMapEnabled(this, false);
            auto* sceneLayout = al::tryGetSceneObj<SingleModeSceneLayout>(
                this, SceneObjID_SingleModeSceneLayout);
            if (sceneLayout != nullptr) {
                sceneLayout->setDisablePause(true);
            }
        }

        mWipe->startClose(60);
    }

    if (!mWipe->isCloseEnd()) {
        return;
    }

    mWipe->startOpen(60);
    for (s32 i = 0; i < mBindPuppeteers.size(); i++) {
        IUsePlayerPuppet* puppet = mBindPuppeteers[i]->getPlayerPuppet();
        rc::startPuppetAction(puppet, "Fall");
        rc::setPuppetMtx(puppet, &mEndMtx);
        sead::Vector3f trans = rc::getPuppetTrans(puppet);
        sead::Vector3f offset;
        WarpObjUtil::getJumpOutLocalTrans(&offset, i, mBindPuppeteers.size());
        offset.rotate(mEndMtx);
        rc::setPuppetTrans(puppet, trans + offset);
        rc::setPuppetVelocity(puppet, sead::Vector3f::zero);
        rc::resetPuppetDynamics(puppet);
        al::setCameraCalcTargetFlag(al::getSensorHost(rc::getPuppetSensor(puppet)), true);
    }

    if (mIsSingleMode) {
        auto* watcher = al::getSceneObj<CloudBonusWatcher>(this, SceneObjID_CloudBonusWatcher);
        if (watcher != nullptr) {
            watcher->tryResetLastCloudBonusStage();
            watcher->setPlayerInCloudBonusStage(false);
        }

        PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            koopaJr->startCloudBonusLauncher();
        }

        DisasterModeController* controller = DisasterModeController::tryGetController(this);
        if (controller != nullptr) {
            controller->setDisasterProgressEnable(true);
            auto* shell = static_cast<SuperBowserShell*>(controller->getDemoSubActor());
            if (shell != nullptr) {
                shell->setEffectsOn(true);
            }
        }
    } else {
        PlayerAliveWatcher* aliveWatcher = PlayerAliveWatcher::tryGetPlayerAliveWatcher(this);
        if (aliveWatcher != nullptr) {
            aliveWatcher->addBubbleDelayTime(-1);
        }
    }

    al::requestCancelInterpole(this);
    al::setNerve(this, &NrvCloudBonusLauncherBonusEndWarpEnd);
}

/**
 * @brief Release the players at the exit launcher.
 */
void CloudBonusLauncher::exeBonusEndWarpEnd() {
    if (al::isFirstStep(this)) {
        al::stopBgm(this, mBgmName, -1, -1);
        if (rc::isExistGhostPlayerRecorder(this)) {
            rc::setPlacementIdObjGhostPlayerRecorder(this, mEndPlacementId, false);
            rc::tryStartFromObjGhostPlayerRecorder(this, mEndPlacementId);
        }

        if (mIsSingleMode) {
            al::validateHitSensors(this);
            restartLauncher(false);
            al::tryOffStageSwitchInstant(this, "SwitchBonusOn");
        }

        if (mSky != nullptr && mIsSingleMode) {
            DisasterModeController* controller = DisasterModeController::tryGetController(this);
            if (controller != nullptr) {
                controller->setSkyEnable(true);
                mSky->kill();
            }
        }
    }

    if (!al::isGreaterEqualStep(this, 2)) {
        return;
    }

    endBindAllPuppet();
    if (mIsSingleMode) {
        if (mEndCameraRS != nullptr) {
            if (!rc::requestStartDemoPlayerCutscene(this)) {
                al::setNerve(this, &NrvCloudBonusLauncherBonusEndWarpEnd);
                return;
            }

            al::startCamera_RS(this, mEndCameraRS, -1);
            rc::addDemoActor(this);
        }
    } else if (mEndObjCamera != nullptr) {
        al::startCamera(this, mEndObjCamera, -1);
    }

    if (mIsSingleMode) {
        if (mEndCameraRS != nullptr) {
            al::endCamera_RS(this, mEndCameraRS, -1, false);
            rc::requestEndDemoPlayerCutscene(this);
        }

        if (mEndCameraArea != nullptr) {
            mEndCameraArea->validate();
        }
    }

    al::setNerve(this, &NrvCloudBonusLauncherBonusEndCameraEnd);
}

/**
 * @brief Wait until every living player landed, then end the cloud bonus.
 */
void CloudBonusLauncher::exeBonusEndCameraEnd() {
    for (s32 i = 0; i < al::getPlayerNumMax(this); i++) {
        if (rc::isPlayerDeadOrBubble(this, i)) {
            continue;
        }

        if (!rc::isPlayerOnGround(this, i)) {
            return;
        }
    }

    WarpObjUtil::restartStageTimer(this);
    if (mIsSingleMode) {
        auto* islandMap = al::tryGetSceneObj<IslandMap>(this, SceneObjID_IslandMap);
        if (islandMap != nullptr) {
            islandMap->setIsPlayerInBonusArea(false);
            if (SingleModeDataFunction::allCloudScenariosComplete(this, mQuadrant)) {
                islandMap->setSpecialShineIconComplete(this, true);
            }
        }

        SingleModeDataFunction::setMapEnabled(GameDataHolderAccessor(this), true);
        auto* sceneLayout =
            al::tryGetSceneObj<SingleModeSceneLayout>(this, SceneObjID_SingleModeSceneLayout);
        if (sceneLayout != nullptr) {
            sceneLayout->setDisablePause(false);
        }

        PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            koopaJr->endCloudBonusLauncher();
        }
    } else if (mEndObjCamera != nullptr) {
        al::endCamera(this, mEndObjCamera, -1);
    }

    if (mIsAllowRestart) {
        restartLauncher(true);
    } else {
        kill();
    }
}
