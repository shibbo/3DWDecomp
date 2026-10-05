#include "Sequence/ProductStageStartParam.hpp"

#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

/**
 * @brief Initialize the stage selection to world zero, stage zero.
 * @param pGameDataHolder Game data used to resolve course names.
 */
ProductStageStartParam::ProductStageStartParam(GameDataHolder* pGameDataHolder)
    : mpGameDataHolder(pGameDataHolder) {}

/** @brief Perform the empty stage-parameter initialization hook. */
void ProductStageStartParam::init() {}

/**
 * @brief Resolve the selected course's stage data name.
 * @return Stage name from the game-data course database.
 */
const char* ProductStageStartParam::getStageDataName() const {
    int courseId = GameDataFunction::calcCourseId(GameDataHolderAccessor(mpGameDataHolder),
                                                 mWorldId, mStageId);
    return GameDataFunction::findStageName(GameDataHolderAccessor(mpGameDataHolder), courseId);
}

/**
 * @brief Get the selected world.
 * @return World ID.
 */
int ProductStageStartParam::getWorldId() const { return mWorldId; }

/**
 * @brief Get the selected stage within the world.
 * @return Stage ID.
 */
int ProductStageStartParam::getStageId() const { return mStageId; }

/**
 * @brief Select a world.
 * @param worldId World ID to store.
 */
void ProductStageStartParam::setWorldId(int worldId) { mWorldId = worldId; }

/**
 * @brief Select a stage within the world.
 * @param stageId Stage ID to store.
 */
void ProductStageStartParam::setStageId(int stageId) { mStageId = stageId; }

/**
 * @brief Format the stage-launch command for the current selection.
 * @param pCommand Destination buffer for the command string.
 */
void ProductStageStartParam::createCommandString(sead::BufferedSafeString* pCommand) {
    const char* pStageName = getStageDataName();
    pCommand->format("X Stage=%s WorldId=%d StageId=%d", pStageName, mWorldId, mStageId);
}
