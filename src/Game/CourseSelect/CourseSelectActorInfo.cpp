#include "CourseSelect/CourseSelectActorInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

/**
 * @brief Resolve a course-select actor's stage metadata from its placement.
 * @param pActor Actor providing the scene's game data.
 * @param rInfo Placement containing optional WorldID and StageID arguments.
 */
CourseSelectActorInfo::CourseSelectActorInfo(al::LiveActor* pActor, const al::ActorInitInfo& rInfo)
    : mpActor(pActor), mpStageInfo(nullptr), mWorldId(0), mStageId(0), mCourseId(0) {
    al::tryGetArg(&mWorldId, rInfo, "WorldID");
    al::tryGetArg(&mStageId, rInfo, "StageID");
    if (mWorldId != 0 || mStageId != 0) {
        mCourseId = GameDataFunction::tryCalcCourseId(GameDataHolderAccessor(mpActor), mWorldId, mStageId);
        mpStageInfo = GameDataFunction::findStageDatabaseInfo(GameDataHolderAccessor(mpActor), mCourseId);
    }
}

/**
 * @brief Check whether the resolved stage uses saved course information.
 * @return Whether the stage uses course information; stage metadata must be resolved.
 */
bool CourseSelectActorInfo::isUseCourseInfo() const {
    return mpStageInfo->isUseCourseInfo();
}

/**
 * @brief Check whether this actor enters a gatekeeper stage.
 * @return Whether the resolved stage is a gatekeeper stage.
 */
bool CourseSelectActorInfo::isEnterGateKeeper() const {
    return mpStageInfo->isGateKeeper();
}

/**
 * @brief Check whether this actor enters a Captain Toad stage.
 * @return Whether the resolved stage is a Kinopio Brigade stage.
 */
bool CourseSelectActorInfo::isEnterKinopioBrigade() const {
    return mpStageInfo->isKinopioBrigade();
}

/**
 * @brief Check whether this actor enters a hidden Toad House.
 * @return Whether the resolved stage is a hidden Toad House.
 */
bool CourseSelectActorInfo::isEnterHide() const {
    return mpStageInfo->isKinopioHouseHide();
}
