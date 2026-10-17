#include "Scene/IslandWarpState.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "NPC/NekoParent.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/HitSensor/SensorMsg.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/TimerManager.hpp"
#include "Project/AreaObj/AreaObjDirector.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/SceneObjID.hpp"
#include "Scene/SingleModeScene.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Layout/GuideGameWindow.hpp"
namespace {
NERVE_DECL(IslandWarpState, BindWait)
NERVE_DECL(IslandWarpState, DoWarp)
NERVE_DECL(IslandWarpState, FadeWait)
NERVES_MAKE_NOSTRUCT(IslandWarpState, BindWait, DoWarp, FadeWait)
} // namespace
IslandWarpState::IslandWarpState(SingleModeScene* scene)
    : StageStartEventBase("IslandWarpState"), mScene(scene) {}
void IslandWarpState::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, info);
    initHitSensor(1);
    al::addHitSensorBindableGoal(this, info, "Bindable", 0.0f, 0, sead::Vector3f::zero);
    al::initNerve(this, &NrvIslandWarpStateBindWait, 0);
    al::initExecutorUpdate(this, info, "デモオブジェクト");
    makeActorDead();
    mPuppeteers = new BindPuppeteerGroup("IslandWarpPuppeteers", al::getPlayerNumMax(this));
    for (s32 i = 0; i < mPuppeteers->getPuppeteerNumMax(); i++) {
        BindPuppeteer* puppet = new BindPuppeteer("Phase0BindPuppeteer");
        mPuppeteers->registerPuppeteer(puppet);
    }
}
void IslandWarpState::appear() {
    al::LiveActor::appear();
    rc::forceDisappearGuideGameWindow(this);
    al::setNerve(this, &NrvIslandWarpStateBindWait);
    rc::requestBindAllPlayerAcceptReviveBubble(this, al::getHitSensor(this, nullptr));
    if (auto* timer = TimerManager::tryGetTimerManager(this))
        timer->forceCancelCurrent(true, true);
}
void IslandWarpState::kill() { al::LiveActor::kill(); }
void IslandWarpState::setIslandWarpDest(s32 islandId) { mIslandId = islandId; }
bool IslandWarpState::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                                 al::HitSensor* receiver) {
    if (!al::isNerve(this, &NrvIslandWarpStateBindWait))
        return false;
    if (al::isMsgBindStart(msg))
        return !mPuppeteers->getPuppeteerByPlayerIndex(sender)->isBind();
    if (al::isMsgBindInit(msg)) {
        BindPuppeteer* puppet = mPuppeteers->getPuppeteerByPlayerIndex(sender);
        puppet->startBind(sender, receiver);
        rc::setPuppetAlphaCtrl(puppet->getPlayerPuppet(), false);
        al::deactivateAudioEventController(al::getSensorHost(sender));
        al::setNerve(this, &NrvIslandWarpStateDoWarp);
        return true;
    }
    if (al::isMsgBindCancel(msg)) {
        mPuppeteers->getPuppeteerByPlayerIndex(sender)->cancelBind();
        if (mPuppeteers->isEndBindAll())
            al::setNerve(this, &NrvIslandWarpStateFadeWait);
        return true;
    }
    return false;
}
bool IslandWarpState::isEndDemo() const { return al::isDead(this); }
bool IslandWarpState::isEnableOpenStartWipe() const {
    return al::isNerve(this, &NrvIslandWarpStateFadeWait);
}
void IslandWarpState::exeBindWait() { al::isFirstStep(this); }
void IslandWarpState::exeDoWarp() {
    if (al::isFirstStep(this)) {
        IslandKeeper* keeper = IslandKeeper::tryGetIslandKeeper(this);
        if (keeper) {
            keeper->setIslandStartPosArea(mIslandId);
            keeper->getIslandStartPos(mIslandId, mDestination, mFront);
        }
        for (s32 i = 0; i < mPuppeteers->getPuppeteerNum(); i++) {
            if (mPuppeteers->isBinding(i)) {
                IUsePlayerPuppet* puppet = mPuppeteers->getPlayerPuppet(i);
                auto* player =
                    static_cast<PlayerActor*>(al::getSensorHost(rc::getPuppetSensor(puppet)));
                rc::removeAllEquipFromPlayerSilent(player);
                player->doIslandWarp();
                if (player->getHoldingSensor())
                    player->cancelAndRelease(true);
                rc::setPuppetTrans(puppet, mDestination);
                rc::setPuppetFrontVec(puppet, mFront);
                rc::setPuppetUpVec(puppet, sead::Vector3f::ey);
                al::activateAudioEventController(player);
                player->mActorSceneInfo->cameraDirector->setJustWarped();
                player->mActorSceneInfo->cameraDirector->freezeCameraInput(true);
            }
        }
        al::resetClippingDistanceStates(this);
        if (auto* plessie = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf))
            plessie->forceSpawn(false);
        if (keeper)
            keeper->resetCurrentIsland();
        if (auto* parents =
                al::tryGetSceneObj<NekoParentHolder>(this, SceneObjID_NekoParentHolder)) {
            for (s64 i = 0; i < parents->getParentNum(); i++)
                if (auto* parent = parents->getParent(i))
                    parent->forceHideGuideBalloon();
        }
    }
    al::setNerve(this, &NrvIslandWarpStateFadeWait);
}
void IslandWarpState::exeFadeWait() {
    if (al::isFirstStep(this)) {
        al::resetClippingDistanceStates(this);
        mScene->getLiveActorKit()->getAreaObjDirector()->setEnableAll(true);
        mScene->getLiveActorKit()->getGraphicsSystemInfo()->getGraphicsAreaDirector()->unlockArea();
        for (s32 i = 0; i < mPuppeteers->getPuppeteerNum(); i++) {
            if (mPuppeteers->isBinding(i)) {
                IUsePlayerPuppet* puppet = mPuppeteers->getPuppeteer(i)->getPlayerPuppet();
                rc::startPuppetAction(puppet, "Wait");
                rc::setPuppetAlphaCtrl(puppet, true);
            }
        }
        al::requestCancelCameraInterpole(this, 0);
    } else if (al::isLessEqualStep(this, 4)) {
        al::resetClippingDistanceStates(this);
        if (auto* jr = PlayerKoopaJr::tryGetPlayerKoopaJr(this))
            jr->doIslandWarp();
    }
    if (al::isStep(this, 10)) {
        for (s32 i = 0; i < mPuppeteers->getPuppeteerNum(); i++) {
            if (mPuppeteers->isBinding(i)) {
                BindPuppeteer* puppet = mPuppeteers->getPuppeteer(i);
                puppet->getPlayerPuppet();
                puppet->endBindOnGround();
            }
        }
        kill();
    }
}
