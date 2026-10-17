#include "Demo/DemoSceneActorHolder.hpp"

#include "Camera/DemoSceneDarkBowser.hpp"
#include "Demo/DemoSceneActor.hpp"
#include "Demo/DemoSceneActorPlayer.hpp"
#include "Demo/DemoScenePlayer.hpp"
#include "Demo/DemoScenePlayerModel.hpp"
#include "Demo/DemoSceneCamera.hpp"
#include "Demo/DemoEffectObj.hpp"
#include "Demo/DemoLayoutObj.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Library/Controller/PadRumbleDirector.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include <prim/seadSafeString.h>

/** @brief Creates an empty demo scene. @param maxPlayers Maximum individually placed players. */
DemoSceneActorHolder::DemoSceneActorHolder(int maxPlayers) : mMaxPlayers(maxPlayers) {
    mBaseMtx.makeIdentity();
}

/** @brief Creates actors described by the demo placement. @param rPlacement Object list.
 * @param rInfo Parent initialization. @param pSelector Retargeting selector.
 * @param pBaseMtx Optional parent transform. @param isUseFigure Whether to limit players to Super. */
void DemoSceneActorHolder::initPlacementDemoActor(const al::PlacementInfo& rPlacement,
    const al::ActorInitInfo& rInfo, PlayerRetargettingSelector* pSelector,
    const sead::Matrix34f* pBaseMtx, bool isUseFigure) {
    mGraphics = rInfo.mActorSceneInfo.graphicsSystemInfo;
    int count = al::getCountPlacementInfo(rPlacement);
    if (!count || mActors)
        return;
    mActors = new DemoSceneActor*[count];
    mActorCount = 0;
    auto* rumble = new al::PadRumbleKeeper(-1);
    for (int i = 0; i < count; i++) {
        al::PlacementInfo placement;
        al::getPlacementInfoByIndex(&placement, rPlacement, i);
        al::ActorInitInfo info;
        info.initViewIdSelf(&placement, rInfo);
        const char* name = nullptr;
        al::getObjectName(&name, placement);
        if (al::isEqualString(name, "DemoActor") || al::isEqualString(name, "DemoMap") ||
            al::isEqualString(name, "DemoActorBowser")) {
            const char* modelName = nullptr;
            if (!alPlacementFunction::tryGetModelName(&modelName, placement))
                modelName = "Unknown_Model";
            auto* actorName = new sead::FixedSafeString<64>();
            actorName->format("DemoActor[%s]", modelName);
            DemoSceneActor* actor;
            if (al::isEqualString(name, "DemoActorBowser"))
                actor = new DemoSceneDarkBowser(actorName->cstr(), this);
            else
                actor = new DemoSceneActor(actorName->cstr(), this);
            actor->initDemoSceneActor(info, rInfo, pBaseMtx);
            al::trySetPadRumbleKeeper(actor, rumble);
            mActors[mActorCount++] = actor;
        } else if (al::isEqualString(name, "DemoActorPlayer")) {
            if (!mPlayers) {
                mPlayers = new DemoSceneActorPlayer*[mMaxPlayers];
                for (int j = 0; j < mMaxPlayers; j++)
                    mPlayers[j] = nullptr;
                mPlayerCount = 0;
            }
            mPlayers[mPlayerCount] = new DemoSceneActorPlayer("デモアクタープレイヤー", this, pSelector);
            mPlayers[mPlayerCount]->initDemoSceneActor(info, rInfo, pBaseMtx);
            mPlayerCount++;
        } else if (al::isEqualString(name, "DemoPlayer")) {
            if (mScenePlayer)
                continue;
            mScenePlayer = new DemoScenePlayer("デモプレイヤー", this, pSelector);
            mScenePlayer->initDemoSceneActor(info, rInfo, pBaseMtx, isUseFigure);
        } else if (al::isEqualString(name, "DemoCamera")) {
            if (mCamera)
                continue;
            mCamera = new DemoSceneCamera(GameDataFunction::getGameDataHolder(rInfo.mActorSceneInfo.sceneObjHolder));
            mCamera->initDemoSceneActor(info, rInfo, pBaseMtx);
        } else if (al::isEqualString(name, "DemoLayoutObj")) {
            if (mLayout)
                continue;
            mLayout = new DemoLayoutObj();
            mLayout->initDemoSceneLayout(info, nullptr);
        } else if (al::isEqualString(name, "DemoEffectObj")) {
            if (!mEffects) {
                mEffects = new DemoEffectObj*[16];
                for (int j = 0; j < 16; j++)
                    mEffects[j] = nullptr;
                mEffectCount = 0;
            } else if (mEffectCount >= 16) {
                continue;
            }
            const char* displayName;
            al::getDisplayName(&displayName, info);
            auto* effect = new DemoEffectObj(displayName);
            effect->initDemoSceneActor(info, rInfo, pBaseMtx);
            mEffects[mEffectCount++] = effect;
        }
    }
}

/** @brief Creates a layout-only demo. @param pName Layout name. @param rInfo Initialization data. */
void DemoSceneActorHolder::initPlacementDemoLayout(const char* pName, const al::ActorInitInfo& rInfo) {
    mGraphics = rInfo.mActorSceneInfo.graphicsSystemInfo;
    mActors = new DemoSceneActor*[1];
    mActorCount = 0;
    mLayout = new DemoLayoutObj();
    mLayout->initDemoSceneLayout(rInfo, pName);
}

/** @brief Starts the camera, layout, and demo actors. */
void DemoSceneActorHolder::appear() {
    mIsFullyStarted = true;
    if (mCamera) mCamera->startDemo(mBaseMtx);
    if (mLayout) mLayout->startDemo();
    if (mScenePlayer) mScenePlayer->startDemo(mBaseMtx);
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i]) mActors[i]->startDemo(mBaseMtx);
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++)
            if (mPlayers[i]) mPlayers[i]->startDemo(mBaseMtx);
    if (!mFindGraphicsByPlayer)
        mGraphics->setAreaTarget(al::GraphicsAreaTarget::CameraLookAt);
}
/** @brief Leaves per-frame work to the contained actors. */
void DemoSceneActorHolder::update() {}

/** @brief Deletes actor effects and stops their sound effects. */
void DemoSceneActorHolder::killAllEffect() {
    if (mScenePlayer) mScenePlayer->killAllEffect();
    for (int i = 0; i < mActorCount; i++) {
        auto* actor = mActors[i];
        if (actor) {
            if (actor->getEffectKeeper()) al::tryDeleteEmitterAndParticleAll(actor);
            if (actor->getAudioKeeper()) al::stopAllSeFromUser(actor, 0);
        }
    }
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++)
            if (mPlayers[i]) mPlayers[i]->killAllEffect();
}

/** @brief Ends playback and restores graphics-area selection. */
void DemoSceneActorHolder::kill() {
    if (!mIsFullyStarted) return;
    mIsFullyStarted = false;
    if (mCamera) mCamera->endDemo();
    if (mLayout) mLayout->endDemo();
    if (mScenePlayer) mScenePlayer->endDemo();
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i]) mActors[i]->endDemo();
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++)
            if (mPlayers[i]) mPlayers[i]->endDemo();
    if (!mFindGraphicsByPlayer)
        mGraphics->setAreaTarget(al::GraphicsAreaTarget::Player);
}
/** @brief Checks for a Koopa Jr. actor. @return Whether one is present. */
bool DemoSceneActorHolder::isContainKoopaJr() const {
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i] && mActors[i]->mIsKoopaJr) return true;
    return false;
}
/** @brief Sets participant selection. @param pInfo Player information provider. */
void DemoSceneActorHolder::setDemoScenePlayerInfo(IDemoScenePlayerInfo* pInfo) { mScenePlayer->setDemoScenePlayerInfo(pInfo); }
/** @brief Sets the camera exit blend. @param frames Interpolation duration. */
void DemoSceneActorHolder::setEndCameraInterpolateFrame(int frames) { if (mCamera) mCamera->setInterpolateFrame(frames); }
/** @brief Sets camera field of view. @param fovy Vertical angle in degrees. */
void DemoSceneActorHolder::setCameraFovyDegree(float fovy) { if (mCamera) mCamera->setCameraFovyDegree(fovy); }
/** @brief Keeps graphics-area selection tied to player position. */
void DemoSceneActorHolder::setGraphicsAreaFindByPlayerPos() { mFindGraphicsByPlayer = true; }

/** @brief Starts an indexed action. @param index Action index. @param loop Whether to skip camera startup. */
void DemoSceneActorHolder::startAction(int index, bool loop) {
    if (!loop && mCamera) mCamera->startCamera(index);
    if (mScenePlayer) mScenePlayer->startAction(index);
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i]) mActors[i]->startAction(index);
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++)
            if (mPlayers[i]) mPlayers[i]->startAction(index);
}
/** @brief Starts a named camera and actor action. @param pName Action name. */
void DemoSceneActorHolder::tryStartActionByName(const char* pName) {
    if (mCamera) mCamera->tryStartActionByName(pName);
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i]) mActors[i]->tryStartActionByName(pName);
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++)
            if (mPlayers[i]) mPlayers[i]->tryStartActionByName(pName);
}
/** @brief Cancels playback effects and optionally advances the camera. @param flag Whether to seek to the end. */
void DemoSceneActorHolder::tryCancel(bool flag) {
    if (flag) {
        int frame = getMaxCameraFrame();
        if (mCamera) mCamera->setCurrentFrame(frame);
    }
    tryCancelAudio(0, false);
    tryCancelEffect();
}
/** @brief Gets playback duration. @return Camera or layout duration, or zero. */
int DemoSceneActorHolder::getMaxCameraFrame() const {
    if (mCamera) return mCamera->getMaxFrame();
    if (mLayout) return mLayout->getMaxFrame();
    return 0;
}
/** @brief Stops demo audio. @param frames Fade duration. @param flag Whether to stop music too. */
void DemoSceneActorHolder::tryCancelAudio(int frames, bool flag) {
    for (int i = 0; i < mActorCount; i++) {
        if (mActors[i] && mActors[i]->getAudioKeeper()) {
            al::stopAllSeFromUser(mActors[i], frames);
            if (flag && al::getCurPlayingBgmPlayName(mActors[i])) {
                al::stopAllBgm(mActors[i], frames);
                al::stopAllSequenceBgm(mActors[i], frames);
            }
        }
    }
    if (mPlayers) {
        for (int i = 0; i < mPlayerCount; i++) {
            if (mPlayers[i]->getAudioKeeper()) {
                al::stopAllSeFromUser(mPlayers[i], frames);
                if (flag && al::getCurPlayingBgmPlayName(mPlayers[i])) {
                    al::stopAllBgm(mPlayers[i], frames);
                    al::stopAllSequenceBgm(mPlayers[i], frames);
                }
            }
        }
    }
}
/** @brief Kills active actor emitters and particles. */
void DemoSceneActorHolder::tryCancelEffect() {
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i] && mActors[i]->getEffectKeeper()) al::tryKillEmitterAndParticleAll(mActors[i]);
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++)
            if (mPlayers[i]->getEffectKeeper()) al::tryKillEmitterAndParticleAll(mPlayers[i]);
}
/** @brief Checks camera or layout completion. @param frames Frame threshold. @return Whether playback ended. */
bool DemoSceneActorHolder::isActionEndCamera(int frames) const {
    if (mCamera) return mCamera->isEndAnimCamera(frames);
    if (mLayout) return mLayout->isEnd(frames);
    return false;
}
/** @brief Checks camera presence. @return Whether a camera exists. */
bool DemoSceneActorHolder::isExistCamera() const { return mCamera != nullptr; }
/** @brief Checks layout presence. @return Whether a layout exists. */
bool DemoSceneActorHolder::isExistLayout() const { return mLayout != nullptr; }
/** @brief Checks camera and actor completion. @return Whether all checked actions have ended. */
bool DemoSceneActorHolder::isAllActionEnd() const {
    if (!isActionEndCamera(-1)) return false;
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i] && !al::isActionEnd(mActors[i])) return false;
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++)
            if (mPlayers[i] && !mPlayers[i]->isModelActionEnd()) return false;
    return true;
}
/** @brief Finds a placed model by name. @param pName Model name. @return Actor or null. */
al::LiveActor* DemoSceneActorHolder::tryFindDemoSceneActor(const char* pName) {
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i] && al::isEqualString(pName, al::getModelName(mActors[i]))) return mActors[i];
    return nullptr;
}
/** @brief Gets a placed actor. @param index Actor index. @return Actor. */
al::LiveActor* DemoSceneActorHolder::getDemoSceneActor(int index) const { return mActors[index]; }
/** @brief Gets an individually placed player. @param index Player index. @return Player actor. */
al::LiveActor* DemoSceneActorHolder::getDemoScenePlayerActor(int index) const { return mPlayers[index]; }
/** @brief Gets a placed player's model. @param index Player index. @return Model actor. */
al::LiveActor* DemoSceneActorHolder::getDemoScenePlayerActorModel(int index) const { return mPlayers[index]->mModel->getModel(); }
/** @brief Replaces parent placement for every demo object. @param pMtx Parent transform. */
void DemoSceneActorHolder::overrideBaseMtx(const sead::Matrix34f* pMtx) {
    for (int i = 0; i < mActorCount; i++)
        if (mActors[i]) mActors[i]->setPlacementBaseMtx(pMtx);
    for (int i = 0; i < mPlayerCount; i++)
        if (mPlayers[i]) mPlayers[i]->setPlacementBaseMtx(pMtx);
    if (mScenePlayer) mScenePlayer->setPlacementBaseMtx(pMtx);
    if (mCamera) mCamera->setPlacementBaseMtx(pMtx);
    for (int i = 0; i < mEffectCount; i++)
        if (mEffects[i]) mEffects[i]->setPlacementBaseMtx(pMtx);
}
/** @brief Finds a coordinated player model. @param index Participant index. @return Model or null. */
al::LiveActor* DemoSceneActorHolder::tryFindPlayerModel(int index) { return mScenePlayer ? mScenePlayer->getPlayerModel(index) : nullptr; }
/** @brief Gets playback position. @return Camera or layout frame, or zero. */
int DemoSceneActorHolder::getCurFrame() const {
    if (mCamera) return mCamera->getCurrentFrame();
    if (mLayout) return mLayout->getCurrentFrame();
    return 0;
}
/** @brief Forces placed player actors to retain their configured characters. */
void DemoSceneActorHolder::setForceIgnoreCharId() {
    if (mPlayers)
        for (int i = 0; i < mPlayerCount; i++) mPlayers[i]->mIgnoreSingleModeCharID = true;
}
