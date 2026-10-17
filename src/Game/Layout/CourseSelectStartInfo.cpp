#include "Layout/CourseSelectStartInfo.hpp"

#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(CourseSelectStartInfo, Appear);
NERVE_DECL(CourseSelectStartInfo, Wait);
NERVES_MAKE_NOSTRUCT(CourseSelectStartInfo, Appear, Wait)
}

/** @brief Creates and shows the course-start curtain.
 * @param rInfo Layout initialization context. */
CourseSelectStartInfo::CourseSelectStartInfo(const al::LayoutInitInfo& rInfo)
    : al::LayoutActor("WipeCurtainStart") {
    al::initLayoutActor(this, rInfo, "WipeCurtainStart", nullptr);
    initNerve(&NrvCourseSelectStartInfoAppear, 0);
    appear();
}

/** @brief Formats the current course number and plays the appearance animation. */
void CourseSelectStartInfo::exeAppear() {
    if (al::isFirstStep(this)) {
        int courseId = GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(this));
        int worldId;
        int stageId;
        GameDataFunction::calcWorldAndStageId(GameDataHolderAccessor(this), &worldId,
                                            &stageId, courseId);
        al::setPaneString(this, "TxtWorld",
                          sead::WFormatFixedSafeString<16>(u"WORLD%d-%d", worldId, stageId).cstr(),
                          0, -1);
        al::startAction(this, "Appear", nullptr);
    }
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvCourseSelectStartInfoWait);
}

/** @brief Starts the idle animation on entry. */
void CourseSelectStartInfo::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait", nullptr);
}

/** @brief Starts the end animation on entry. */
void CourseSelectStartInfo::exeEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "End", nullptr);
}
