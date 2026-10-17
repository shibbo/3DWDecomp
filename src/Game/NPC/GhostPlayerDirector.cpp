#include "NPC/GhostPlayerDirector.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "NPC/GhostPlayerPlayer.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/Data/StageDataHolder.hpp"
/** @brief Registers recording and playback execution. @param pUser Scene object access. @param
 * rInfo Scene initialization. @param pExecute Execution director. @param pPlayers Player holder.
 * @param pData Game data. */
GhostPlayerDirector::GhostPlayerDirector(const al::IUseSceneObjHolder* pUser,
                                         const al::SceneInitInfo& rInfo,
                                         al::ExecuteDirector* pExecute, al::PlayerHolder* pPlayers,
                                         const GameDataHolder* pData)
    : mNetwork(nullptr), mData(pData), mRecorder(nullptr), mPlayer(nullptr) {
    al::setSceneObj(pUser, this, 9);
    al::registerExecutorUser(this, pExecute, getSceneObjName());
    if (rInfo.mGameSystemInfo->_20) {
        mNetwork = rInfo.mGameSystemInfo->_20;
    }
    mRecorder = new GhostPlayerRecorder(pUser, rInfo, mData, pPlayers);
}
/** @brief Updates active recording and playback. */
void GhostPlayerDirector::execute() {
    if (mRecorder) {
        mRecorder->update();
    }
    if (mPlayer) {
        mPlayer->update();
    }
}
/** @brief Attempts to start recording. @return Whether recording started. */
bool GhostPlayerDirector::tryStartRecord() { return mRecorder->tryStartRecord(); }
/** @brief Reports the recording state. @return Whether recording has ended. */
bool GhostPlayerDirector::isEndRecord() const { return mRecorder->isEndRecord(); }
/** @brief Ends recording and sets its clear time. @param useBaseTime Use the course reference time.
 * @return True after processing the request. */
bool GhostPlayerDirector::tryEndRecord(bool useBaseTime) {
    mRecorder->endRecord();
    const StageDataHolder* pStage = mData->getPlayingFile()->getStageDataHolder();
    int courseId = pStage->getCourseId();
    int clearTime = pStage->calcTimeAttackCount();
    if (useBaseTime) {
        GameDataHolderAccessor data(const_cast<GameDataHolder*>(mData));
        clearTime = GameDataFunction::findGhostBaseTime(
            data, GameDataFunction::findStageName(data, courseId));
    }
    mRecorder->setClearTime(clearTime);
    return true;
}
/** @brief Attempts to save the recording. @return Recorder save result. */
bool GhostPlayerDirector::trySaveRecord() { return mRecorder->saveRecord(); }
/** @brief Starts playback if a player is registered. */
void GhostPlayerDirector::tryStartPlay() {
    if (mPlayer) {
        mPlayer->tryStartPlay();
    }
}
/** @brief Ends playback if a player is registered. */
void GhostPlayerDirector::tryEndPlay() {
    if (mPlayer) {
        mPlayer->tryEndPlay();
    }
}
/** @brief Gets the execution and scene object name. @return Scene object name. */
const char* GhostPlayerDirector::getSceneObjName() const { return "GhostPlayerDirector"; }
