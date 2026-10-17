#include "NPC/GhostPlayerDirector.hpp"
#include "NPC/GhostPlayerPlayer.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataFile.hpp"
#include "System/GameDataFunction.hpp"
#include "System/Data/StageDataHolder.hpp"

/** @brief Registers the ghost scene service and creates its recording state. */
GhostPlayerDirector::GhostPlayerDirector(const al::IUseSceneObjHolder* pUser,
    const al::SceneInitInfo& rInfo, al::ExecuteDirector* pExecutor,
    al::PlayerHolder* pPlayers, const GameDataHolder* pGameData) : mGameData(pGameData) {
    al::setSceneObj(pUser, this, 9);
    al::registerExecutorUser(this, pExecutor, getSceneObjName());
    if (rInfo.mGameSystemInfo->_20)
        mNetworkSystem = static_cast<al::NetworkSystem*>(rInfo.mGameSystemInfo->_20);
    mRecorder = new GhostPlayerRecorder(pUser, rInfo, mGameData, pPlayers);
}

void GhostPlayerDirector::execute() {
    if (mRecorder)
        mRecorder->update();
    if (mPlayer)
        mPlayer->update();
}

bool GhostPlayerDirector::tryStartRecord() { return mRecorder->tryStartRecord(); }
bool GhostPlayerDirector::isEndRecord() const { return !mRecorder->isRecording(); }

/** @brief Ends recording and stores the measured or reference clear time. */
bool GhostPlayerDirector::tryEndRecord(bool useBaseTime) {
    mRecorder->endRecord();
    const StageDataHolder* pStage = mGameData->getPlayingFile()->getStageDataHolder();
    int courseId = pStage->getCourseId();
    int time = pStage->calcTimeAttackCount();
    if (useBaseTime) {
        GameDataHolderAccessor accessor(const_cast<GameDataHolder*>(mGameData));
        time = GameDataFunction::findGhostBaseTime(accessor,
                                                  GameDataFunction::findStageName(accessor, courseId));
    }
    mRecorder->setClearTime(time);
    return true;
}

bool GhostPlayerDirector::trySaveRecord() { return mRecorder->saveRecord(); }
bool GhostPlayerDirector::tryStartPlay() { return mPlayer && mPlayer->tryStartPlay(); }
bool GhostPlayerDirector::tryEndPlay() { return mPlayer && mPlayer->tryEndPlay(); }
const char* GhostPlayerDirector::getSceneObjName() const { return "ゴーストディレクター"; }
