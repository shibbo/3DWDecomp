#include "CourseSelect/CourseSelectNodeParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
NERVE_DECL(CourseSelectNodeParts, Deactive);
NERVE_DECL(CourseSelectNodeParts, Wait);
NERVE_DECL(CourseSelectNodeParts, Grow);
NERVES_MAKE_NOSTRUCT(CourseSelectNodeParts, Deactive, Wait, Grow)
}
/** @brief Create a node part with opening and idle actions.
 * @param pName Actor name. @param pGrowAction Opening action. @param pWaitAction Idle action.
 */
CourseSelectNodeParts::CourseSelectNodeParts(const char* pName, const char* pGrowAction, const char* pWaitAction)
    : al::LiveActor(pName), mpGrowAction(pGrowAction), mpWaitAction(pWaitAction) {}
/** @brief Initialize a hidden node part from its model archive.
 * @param rInfo Actor initialization data. @param pArchiveName Model archive.
 */
void CourseSelectNodeParts::initPartsWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchiveName) {
    al::initActorWithArchiveName(this, rInfo, pArchiveName, nullptr);
    al::initNerve(this, &NrvCourseSelectNodePartsDeactive, 0);
    al::hideModelIfShow(this);
    mIsRoadCurve = al::isEqualString(pArchiveName, "CourseSelectRoadCurve");
    makeActorDead();
}
/** @brief Open the part with its growth animation unless already open. */
void CourseSelectNodeParts::startOpen() {
    if (al::isNerve(this, &NrvCourseSelectNodePartsWait)) return;
    appear();
    al::setNerve(this, &NrvCourseSelectNodePartsGrow);
    al::startAction(this, mpGrowAction);
    al::invalidateClipping(this);
}
/** @brief Show the fully opened part immediately. */
void CourseSelectNodeParts::startOpenImmediately() {
    appear();
    al::setNerve(this, &NrvCourseSelectNodePartsWait);
    al::showModelIfHide(this);
    al::invalidateClipping(this);
}
/** @brief Check whether growth reached its final frame. @return Whether the part has finished growing. */
bool CourseSelectNodeParts::isEndGrow() const {
    if (al::isNerve(this, &NrvCourseSelectNodePartsDeactive)) return false;
    if (al::isNerve(this, &NrvCourseSelectNodePartsWait)) return true;
    if (al::isAlive(this)) {
        float endFrame = al::getActionFrameMax(this, al::getActionName(this)) - 1.0f;
        if (endFrame <= al::getActionFrame(this)) return true;
    }
    return false;
}
/** @brief Remain inactive until opened. */
void CourseSelectNodeParts::exeDeactive() {}
/** @brief Reveal the growing model on the second update and wait for its action to end. */
void CourseSelectNodeParts::exeGrow() {
    if (al::isStep(this, 1)) al::showModelIfHide(this);
    al::setNerveAtActionEnd(this, &NrvCourseSelectNodePartsWait);
}
/** @brief Start idle animation, restore clipping, and settle the model updates. */
void CourseSelectNodeParts::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mpWaitAction);
        al::validateClipping(this);
    }
    if (al::isStep(this, 3)) al::setNeedSetBaseMtxAndCalcAnimFlag(this, false);
}
/** @brief Destroy the node part. */
CourseSelectNodeParts::~CourseSelectNodeParts() = default;
