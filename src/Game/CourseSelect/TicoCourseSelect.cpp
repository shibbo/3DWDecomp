#include "CourseSelect/TicoCourseSelect.hpp"
#include "Library/ActorUtil.hpp"

/**
 * @brief Create a course-select Luma actor.
 * @param pName Actor name.
 */
TicoCourseSelect::TicoCourseSelect(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Load the Luma model and start its idle animation at a random frame.
 * @param rInfo Actor placement and scene initialization data.
 */
void TicoCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("Tico"), "CourseSelect");
    al::startActionAtRandomFrame(this, "Wait");
    makeActorAppeared();
}

/** @brief Destroy the course-select Luma actor. */
TicoCourseSelect::~TicoCourseSelect() = default;
