#include "Layout/ListStampParts.hpp"
#include "Layout/ListStampDemoParts.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace rc {
StampList* createStampList();
int getStampSize(StampList* pList);
const char* tryGetStampNameByIndex(StampList* pList, int index, const GameDataHolder* pGameData);
bool isAcquiredStampByIndex(StampList* pList, int index, const GameDataHolder* pGameData);
const char* tryGetStampNameByCourseId(StampList* pList, int courseId);
const char* tryGetCharacterCompleteStampName(StampList* pList, int characterId);
}
namespace {
NERVE_DECL(ListStampParts, Wait);
NERVE_DECL(ListStampParts, Decide);
NERVES_MAKE_NOSTRUCT(ListStampParts, Wait, Decide)
}
/** @brief Creates the stamp collection and optional acquisition presentation.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Layout parts name.
 * @param pParent Parent layout, if present.
 * @param pGameData Save data containing acquired stamps. */
ListStampParts::ListStampParts(const al::LayoutInitInfo& rInfo, const char* pName,
                             const char* pPartsName, al::LayoutActor* pParent,
                             const GameDataHolder* pGameData)
    : al::LayoutActor(pName), mGameData(pGameData), mAcquiredStampName("") {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    mStampList = rc::createStampList();
    initNerve(&NrvListStampPartsWait, 0);
    if (pParent)
        mDemo = new ListStampDemoParts(rInfo, "ハンコ決定パーツ", "ParStampDemo", pParent, this);
    appear();
}
/** @brief Refreshes each stamp while keeping the newly acquired stamp hidden. */
void ListStampParts::appear() {
    al::LayoutActor::appear();
    int size = rc::getStampSize(mStampList);
    for (int i = 0; i < size; ++i) {
        const char* pName = rc::tryGetStampNameByIndex(mStampList, i, mGameData);
        if (!pName)
            continue;
        if ((mAcquiredStampName.isEmpty() ||
             !al::isEqualString(mAcquiredStampName.cstr(), pName)) &&
            rc::isAcquiredStampByIndex(mStampList, i, mGameData))
            startOn(pName);
        else
            startOff(pName);
    }
}
/** @brief Hides the named stamp. @param pName Stamp action group. */
void ListStampParts::startOff(const char* pName) { al::tryStartAction(this, "Off", pName); }
/** @brief Shows the named stamp. @param pName Stamp action group. */
void ListStampParts::startOn(const char* pName) { al::tryStartAction(this, "On", pName); }
/** @brief Starts the acquired stamp's presentation at its picture pane. */
void ListStampParts::startDecide() {
    if (!mDemo)
        return;
    al::StringTmp<64> paneName("Pic%s", mAcquiredStampName.cstr());
    sead::Vector3f pos = sead::Vector3f::zero;
    if (al::isExistPane(this, paneName.cstr())) {
        al::calcPaneTrans(&pos, this, paneName.cstr());
        mDemo->startDecide(mAcquiredStampName.cstr(), pos, paneName.cstr());
        al::setNerve(this, &NrvListStampPartsDecide);
    }
}
/** @brief Checks whether the acquisition presentation has finished.
 * @return Whether no presentation remains. */
bool ListStampParts::isDecideEnd() const {
    if (!mDemo || !al::getLayoutPaneGroup(this, mAcquiredStampName.cstr()))
        return true;
    return al::isNerve(this, &NrvListStampPartsWait);
}
/** @brief Selects a course stamp for presentation. @param courseId Course identifier. */
void ListStampParts::setAcquiredStampNameByCourseId(int courseId) {
    const char* pName = rc::tryGetStampNameByCourseId(mStampList, courseId);
    if (pName) {
        mHasNoAcquiredStamp = false;
        mAcquiredStampName = pName;
        startOff(pName);
    }
}
/** @brief Selects a character completion stamp. @param characterId Character identifier. */
void ListStampParts::setAcquiredStampNameByCharacterComplete(int characterId) {
    const char* pName = rc::tryGetCharacterCompleteStampName(mStampList, characterId);
    if (pName) {
        mHasNoAcquiredStamp = false;
        mAcquiredStampName = pName;
        startOff(pName);
    }
}
/** @brief Holds the stamp collection's idle state. */
void ListStampParts::exeWait() {}
/** @brief Reveals the stamp after its acquisition presentation completes. */
void ListStampParts::exeDecide() {
    if (mDemo->isDecideEnd()) {
        startOn(mAcquiredStampName.cstr());
        al::setNerve(this, &NrvListStampPartsWait);
    }
}
