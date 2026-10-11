#include "Layout/Switch/CatShineCounterParts.hpp"

#include <prim/seadSafeString.h>

#include "Layout/LayoutFontUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"

namespace {
/** Highest scenario index that is always listed, whether or not it is unlocked yet. */
constexpr s32 cAlwaysShownScenarioMax = 2;
/** First scenario index that is only listed once it is unlocked or collected. */
constexpr s32 cHiddenScenarioStart = 3;

typedef sead::WFixedSafeString<32> ShineIconString;
}  // namespace

/**
 * @brief Creates the counter as parts of its parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent layout actor.
 */
CatShineCounterParts::CatShineCounterParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                           const char* pPartsName, al::LayoutActor* pParent)
    : SingleModeCounterBase(rInfo, pName, pPartsName, pParent) {
    mShineTextInfo.setTextBox(pParent->getLayoutKeeper()->getLayout(), "TxtShine", 2);
    mShineOceanTextInfo.setTextBox(pParent->getLayoutKeeper()->getLayout(), "TxtShine_Ocean", 2);
}

/** @brief Re-applies the text box font fix-ups every frame. */
void CatShineCounterParts::control() {
    mShineTextInfo.applyFix();
    mShineOceanTextInfo.applyFix();
}

/**
 * @brief Rebuilds the row of shine icons for an island.
 * @param islandId Island whose scenarios are shown, or a negative value for a lucky shine.
 * @param scenarioId Scenario whose shine was just collected.
 * @param isHideScenario Whether the shine of scenarioId is drawn as not yet collected.
 */
void CatShineCounterParts::updateCount(s32 islandId, s32 scenarioId, bool isHideScenario) {
    u64 flags = SingleModeDataFunction::getScenarioFlag(this, islandId);
    if (isHideScenario) {
        flags &= ~(1ull << scenarioId);
    }

    ShineIconString shineText;
    ShineIconString baseText;

    if (islandId < 0) {
        if (SingleModeDataFunction::isLuckyShine(this, islandId + 1, scenarioId + 1)) {
            // The remaining count is queried but not used.
            SingleModeDataFunction::getNumRemainingLuckyShines(this);
            if (isHideScenario) {
                shineText.append(LayoutFontUtil::getIconFontCatShineEmpty());
            } else {
                shineText.append(LayoutFontUtil::getIconFontCatShine());
            }

            baseText.append(LayoutFontUtil::getIconFontCatShineBase());
        } else {
            if (isHideScenario) {
                shineText.append(LayoutFontUtil::getIconFontCatShineEmpty());
                baseText.append(LayoutFontUtil::getIconFontCatShineBase());
            } else {
                shineText.append(LayoutFontUtil::getIconFontCatShine());
                baseText.append(LayoutFontUtil::getIconFontCatShineBase());
            }
        }
    } else {
        s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, islandId);
        s32 activeIndex = SingleModeDataFunction::getCurActiveScenarioIndex(this, islandId);

        // The leading scenarios are listed up to the first one not collected yet.
        for (s32 i = 0; i <= sead::Mathi::min(activeIndex, cAlwaysShownScenarioMax); i++) {
            if ((flags & (1ull << i)) == 0) {
                shineText.append(LayoutFontUtil::getIconFontCatShineEmpty());
                baseText.append(LayoutFontUtil::getIconFontCatShineBase());
                break;
            }

            shineText.append(LayoutFontUtil::getIconFontCatShine());
            baseText.append(LayoutFontUtil::getIconFontCatShineBase());
        }

        // Later scenarios only show up once collected or when they are the current target.
        for (s32 i = cHiddenScenarioStart; i < scenarioNum; i++) {
            if ((flags & (1ull << i)) != 0) {
                shineText.append(LayoutFontUtil::getIconFontCatShine());
                baseText.append(LayoutFontUtil::getIconFontCatShineBase());
            } else if (i == scenarioId) {
                shineText.append(LayoutFontUtil::getIconFontCatShineEmpty());
                baseText.append(LayoutFontUtil::getIconFontCatShineBase());
            } else if (activeIndex == 3 && scenarioId == 4) {
                shineText.append(LayoutFontUtil::getIconFontCatShineEmpty());
                baseText.append(LayoutFontUtil::getIconFontCatShineBase());
            }
        }
    }

    al::StringTmp<32> baseName("TxtShineBase");
    al::StringTmp<32> shineName("TxtShine");
    al::StringTmp<32> dsName("TxtShine_ds");

    al::startAction(this, "ShowIslandShines", "ShineGroups");
    al::setPaneString(this, shineName.cstr(), shineText.cstr());
    al::setPaneString(this, baseName.cstr(), baseText.cstr());
    al::setPaneString(this, dsName.cstr(), baseText.cstr());
}
