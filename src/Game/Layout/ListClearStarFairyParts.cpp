#include "Layout/ListClearStarFairyParts.hpp"

#include "Layout/LayoutFontUtil.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/LayoutUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(ListClearStarFairyParts, Hide);
NERVE_DECL(ListClearStarFairyParts, Show);
NERVES_MAKE_NOSTRUCT(ListClearStarFairyParts, Hide, Show)
}  // namespace

/**
 * @brief Creates a hidden course-completion list entry.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout.
 * @param pGameData Game data used to format the course name.
 */
ListClearStarFairyParts::ListClearStarFairyParts(const al::LayoutInitInfo& rInfo,
    const char* pName, const char* pPartsName, al::LayoutActor* pParent,
    const GameDataHolder* pGameData)
    : al::LayoutActor(pName), mParent(pParent), mGameData(pGameData) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvListClearStarFairyPartsHide, 0);
}

/** @brief Starts the visible entry animation. */
void ListClearStarFairyParts::exeShow() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Main");
    }
}

/** @brief Starts the hidden entry animation. */
void ListClearStarFairyParts::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Main");
    }
}

/** @brief Hides this list entry. */
void ListClearStarFairyParts::setHide() {
    al::setNerve(this, &NrvListClearStarFairyPartsHide);
}

/**
 * @brief Shows a course's world name and collected-stamp indicator.
 * @param courseId Course whose completion data is displayed.
 * @param worldLabelId World number used to select the message label.
 */
void ListClearStarFairyParts::setShow(int courseId, int worldLabelId) {
    int worldId = 1;
    int stageId = 1;
    GameDataFunction::calcWorldAndStageId(mParent, &worldId, &stageId, courseId);
    al::StringTmp<32> label("ListClearStar_World%d", worldLabelId);
    rc::setPaneWorldStageString(mParent, this, "TxtWorld", "ListClearStar", label.cstr(),
                               worldId, stageId, mGameData, nullptr, false);
    if (CourseInfoFunction::isAcquireIllustItem(mParent, courseId, false, -1)) {
        al::setPaneString(this, "TxtStamp", LayoutFontUtil::getIconFontIllustItem(), 0, -1);
    } else {
        al::setPaneString(this, "TxtStamp", u" ", 0, -1);
    }
    al::setNerve(this, &NrvListClearStarFairyPartsShow);
}
