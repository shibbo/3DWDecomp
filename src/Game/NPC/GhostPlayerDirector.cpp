#include "NPC/GhostPlayerDirector.hpp"

#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/ProjectActorFactoryTypes.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"

/**
 * @brief Register the director as a scene object and create the ghost recorder.
 * @param pHolder Scene object holder the director is registered in.
 * @param rInfo Scene initialization info.
 * @param pExecuteDirector Execute director the director is updated by.
 * @param pPlayerHolder Holder of the players to record.
 * @param pGameDataHolder Game data holder of the running game.
 */
GhostPlayerDirector::GhostPlayerDirector(const al::IUseSceneObjHolder* pHolder,
                                         const al::SceneInitInfo& rInfo,
                                         al::ExecuteDirector* pExecuteDirector,
                                         al::PlayerHolder* pPlayerHolder,
                                         const GameDataHolder* pGameDataHolder)
    : mGameDataHolder(pGameDataHolder) {
    al::setSceneObj(pHolder, this, SceneObjID_GhostPlayerDirector);
    al::registerExecutorUser(this, pExecuteDirector, getSceneObjName());

    al::NetworkSystem* pNetworkSystem = rInfo.mGameSystemInfo->getNetworkSystem();
    if (pNetworkSystem != nullptr) {
        mNetworkSystem = pNetworkSystem;
    }

    mRecorder = new GhostPlayerRecorder(pHolder, rInfo, mGameDataHolder, pPlayerHolder);
}

/** @brief Update the recorder and the ghost player, when present. */
void GhostPlayerDirector::execute() {
    if (mRecorder != nullptr) {
        mRecorder->update();
    }

    if (mPlayer != nullptr) {
        mPlayer->update();
    }
}

/**
 * @brief Start recording the player.
 * @return True if the recording started.
 */
bool GhostPlayerDirector::tryStartRecord() {
    return mRecorder->tryStartRecord();
}

/**
 * @brief Check whether the recording has ended.
 * @return True when nothing is being recorded.
 */
bool GhostPlayerDirector::isEndRecord() const {
    return !mRecorder->isRecording();
}

/**
 * @brief End the recording and store its clear time.
 * @param isClear Whether the stage was cleared; the ghost base time of the stage is stored then,
 * otherwise the time attack count of the play.
 * @return Always true.
 */
bool GhostPlayerDirector::tryEndRecord(bool isClear) {
    mRecorder->endRecord();

    const StageDataHolder* pStageDataHolder =
        mGameDataHolder->getPlayingFile()->getStageDataHolder();
    s32 courseId = pStageDataHolder->getCourseId();
    s32 time = pStageDataHolder->calcTimeAttackCount();

    if (isClear) {
        GameDataHolderAccessor accessor(const_cast<GameDataHolder*>(mGameDataHolder));
        const char* pStageName = GameDataFunction::findStageName(accessor, courseId);
        time = GameDataFunction::findGhostBaseTime(accessor, pStageName);
    }

    mRecorder->setClearTime(time);
    return true;
}

/**
 * @brief Save the recorded ghost data.
 * @return True if the data was saved.
 */
bool GhostPlayerDirector::trySaveRecord() {
    return mRecorder->saveRecord();
}

/** @brief Start replaying the ghost, when a ghost player exists. */
void GhostPlayerDirector::tryStartPlay() {
    if (mPlayer != nullptr) {
        mPlayer->tryStartPlay();
    }
}

/** @brief Stop replaying the ghost, when a ghost player exists. */
void GhostPlayerDirector::tryEndPlay() {
    if (mPlayer != nullptr) {
        mPlayer->tryEndPlay();
    }
}

/**
 * @brief Name of the scene object.
 * @return The debug name of the director.
 */
const char* GhostPlayerDirector::getSceneObjName() const {
    return "ゴーストディレクター";
}
