#include "Layout/AreaNameParts.hpp"

#include <math/seadVector.h>
#include <prim/seadSafeString.h>
#include "Layout/LayoutFontUtil.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/IslandDataList.hpp"
#include "Util/PlayerUtil.hpp"

/// Declares a nerve of the area name banner whose state function may differ from the nerve's name.
#define AREA_NAME_PARTS_NERVE_DECL(Action, Func)                                                   \
    class AreaNamePartsNrv##Action : public al::Nerve {                                            \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<AreaNameParts>())->exe##Func();                                    \
        }                                                                                          \
    };

namespace {
AREA_NAME_PARTS_NERVE_DECL(FadeOut, FadeOut)
AREA_NAME_PARTS_NERVE_DECL(Appear, Appear)
AREA_NAME_PARTS_NERVE_DECL(WaitFadeWhite, WaitFadeWhite)
AREA_NAME_PARTS_NERVE_DECL(Wait, Wait)
AREA_NAME_PARTS_NERVE_DECL(FadeOutChangeName, FadeOut)
AREA_NAME_PARTS_NERVE_DECL(FadeOutHide, FadeOut)

NERVES_MAKE_NOSTRUCT(AreaNameParts, FadeOut, Appear, WaitFadeWhite, Wait, FadeOutChangeName,
                     FadeOutHide)

/// Island kinds (from the island's data) that pick a phase-specific banner color.
enum IslandKind : s32 {
    IslandKind_Phase2A = 16,
    IslandKind_Phase2B = 18,
    IslandKind_Phase3A = 19,
    IslandKind_Phase3B = 22,
};

/// Unlocked phases during which no scenario is shown on the banner.
constexpr s32 cPhaseNoScenarioA = 7;
constexpr s32 cPhaseNoScenarioB = 10;

/**
 * @brief Reads the kind of an island found through the island keeper.
 * @param pIsland Island returned by IslandKeeper::findIsland.
 * @return The island kind stored in the island's data.
 */
s32 getIslandKind(const void* pIsland) {
    const u8* data = *reinterpret_cast<u8* const*>(static_cast<const u8*>(pIsland) + 0x118);
    return *reinterpret_cast<const s32*>(data + 0x24);
}
}  // namespace

/**
 * @brief Creates the area name banner.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pParent Parent scene layout.
 * @param pPlayerHolder Holder of the players.
 */
AreaNameParts::AreaNameParts(const al::LayoutInitInfo& rInfo, const char* pName,
                             const char* pPartsName, al::LayoutActor* pParent,
                             const al::PlayerHolder* pPlayerHolder)
    : al::LayoutActor(pName), mPlayerHolder(pPlayerHolder),
      mParent(static_cast<SingleModeSceneLayout*>(pParent)), mScenarioInfo{-1, -1},
      mIsFadeOutRequested(false), mIsForceEndAppear(false), mIsForceEndAppearPhaseStart(false),
      mWipeFadeWhite(nullptr) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    mPlayerActor = rc::findPlayerActorFirstByUserId(mPlayerHolder, 0);
    initNerve(&NrvAreaNamePartsFadeOut, 0);
    al::startAction(this, "ForceHide", nullptr);
    al::startAction(this, "ScenarioBannerForceHide", "ScenarioBanner");
}

/**
 * @brief Switches the banner to another island / scenario.
 * @param rInfo Island and scenario to show.
 */
void AreaNameParts::changeName(const ScenarioInfo& rInfo) {
    if (isAlive() && (al::isNerve(this, &NrvAreaNamePartsAppear) ||
                      al::isNerve(this, &NrvAreaNamePartsWaitFadeWhite))) {
        return;
    }

    if (rInfo.mIslandId == mScenarioInfo.mIslandId) {
        return;
    }

    bool isGigaBellLocked = false;
    if (IslandDataFunction::isGigaBellIsland(rInfo.mIslandId + 1)) {
        isGigaBellLocked =
            SingleModeDataFunction::getGoalItemsCollected(this) <
            SingleModeDataFunction::getGigaBellLockCount(GameDataHolderAccessor(this));
    }

    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (phase != cPhaseNoScenarioA && phase != cPhaseNoScenarioB &&
        (isGigaBellLocked ||
         (rInfo.mScenarioIndex >= 0 &&
          rInfo.mScenarioIndex < SingleModeDataFunction::getScenarioNum(this, rInfo.mIslandId) &&
          !SingleModeDataFunction::isScenarioComplete(this, rInfo.mIslandId,
                                                      rInfo.mScenarioIndex)))) {
        al::showPane(this, "Scenario");
        al::showPane(this, "ScenarioBanner");
        al::startFreezeActionEnd(this, "ScenarioBannerHide", "ScenarioBanner");
    } else {
        al::hidePane(this, "Scenario");
        al::hidePane(this, "ScenarioBanner");
    }

    if (mScenarioInfo.mIslandId < 0 || isFadeOut()) {
        al::LayoutActor::appear();
        mScenarioInfo.mIslandId = rInfo.mIslandId;
        mScenarioInfo.mScenarioIndex = rInfo.mScenarioIndex;
        updateTextBoxes();

        if (mIsForceEndAppear) {
            al::startFreezeActionEnd(this, "Hide", nullptr);

            if (mIsForceEndAppearPhaseStart) {
                mIsForceEndAppearPhaseStart = false;
                al::startAction(this, "ScenarioBannerAppear", "ScenarioBanner");
            } else {
                al::startFreezeActionEnd(this, "ScenarioBannerAppear", "ScenarioBanner");
            }

            mParent->forceShineCounterEndAppear();
            al::setNerve(this, &NrvAreaNamePartsWait);
            mIsForceEndAppear = false;
        } else {
            al::setNerve(this, &NrvAreaNamePartsAppear);
        }
    } else {
        mScenarioInfo.mIslandId = rInfo.mIslandId;
        mScenarioInfo.mScenarioIndex = rInfo.mScenarioIndex;
        al::setNerve(this, &NrvAreaNamePartsFadeOutChangeName);
    }

    al::updateLayoutPaneRecursive(this);

    sead::Vector2f islandLineSize;
    sead::Vector2f scenarioLineSize;
    al::getPaneLocalSize(&islandLineSize, this, "WdwLineIslandName");
    al::getPaneLocalSize(&scenarioLineSize, this, "WdwLineScenarioName");

    if (islandLineSize.x > scenarioLineSize.x) {
        al::showPane(this, "WdwLineIslandName");
        al::hidePane(this, "WdwLineScenarioName");
    } else {
        al::showPane(this, "WdwLineScenarioName");
        al::hidePane(this, "WdwLineIslandName");
    }
}

/**
 * @brief Checks whether the banner is fading out.
 * @return Whether one of the fade-out states is active.
 */
bool AreaNameParts::isFadeOut() {
    return al::isNerve(this, &NrvAreaNamePartsFadeOut) ||
           al::isNerve(this, &NrvAreaNamePartsFadeOutHide) ||
           al::isNerve(this, &NrvAreaNamePartsFadeOutChangeName);
}

/** @brief Refreshes the island name, scenario name and shine icons of the banner. */
void AreaNameParts::updateTextBoxes() {
    auto* islandKeeper = al::tryGetSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper);
    if (islandKeeper != nullptr) {
        const void* island = islandKeeper->findIsland(mScenarioInfo.mIslandId + 1);
        if (island == nullptr) {
            return;
        }

        s32 kind = getIslandKind(island);
        if (kind == IslandKind_Phase2A || kind == IslandKind_Phase2B) {
            al::startAction(this, "SetPhase2", "BaselineColor");
        } else if (kind == IslandKind_Phase3B || kind == IslandKind_Phase3A) {
            al::startAction(this, "SetPhase3", "BaselineColor");
        } else {
            al::startAction(this, "SetPhase1", "BaselineColor");
        }
    } else {
        al::startAction(this, "SetPhase1", "BaselineColor");
    }

    auto* islandName = reinterpret_cast<const char16_t*>(
        IslandDataFunction::getIslandName(this, this, mScenarioInfo.mIslandId + 1));
    if (islandName != nullptr) {
        al::setPaneString(this, "TxtNameLg", islandName);
    }

    if (al::isHidePane(this, "Scenario")) {
        al::startAction(this, "ScenarioNameOff", "LayoutSetup");
    } else {
        const char16_t* scenarioName = nullptr;
        if (IslandDataFunction::isGigaBellIsland(mScenarioInfo.mIslandId + 1)) {
            switch (SingleModeDataFunction::getGigaBellLockCount(GameDataHolderAccessor(this))) {
            case 5:
                scenarioName = al::getSystemMessageString(this, "ScenarioName",
                                                          "GigaBell_Scenario01");
                break;
            case 15:
                scenarioName = al::getSystemMessageString(this, "ScenarioName",
                                                          "GigaBell_Scenario02");
                break;
            case 20:
                scenarioName = al::getSystemMessageString(this, "ScenarioName",
                                                          "GigaBell_Scenario03");
                break;
            case 39:
                scenarioName = al::getSystemMessageString(this, "ScenarioName",
                                                          "GigaBell_Scenario04");
                break;
            case 50:
                scenarioName = al::getSystemMessageString(this, "ScenarioName",
                                                          "GigaBell_Scenario05");
                break;
            default:
                break;
            }
        } else {
            scenarioName = reinterpret_cast<const char16_t*>(
                IslandDataFunction::getIslandScenarioName(this, this, mScenarioInfo.mIslandId + 1,
                                                          mScenarioInfo.mScenarioIndex + 1));
        }

        if (scenarioName != nullptr) {
            al::setPaneString(this, "TxtScenarioCenter", scenarioName);
            al::setPaneString(this, "TxtScenarioCorner", scenarioName);
            SingleModeDataFunction::setCurActiveScenarioNameSeen(GameDataHolderAccessor(this),
                                                                 mScenarioInfo.mIslandId);
        }

        al::startAction(this, "ScenarioNameOn", "LayoutSetup");
    }

    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (phase == cPhaseNoScenarioB || phase == cPhaseNoScenarioA) {
        al::setPaneString(this, "TxtShine", u"");
        al::setPaneString(this, "TxtShine_empty", u"");
        return;
    }

    sead::WFixedSafeString<16> shineIcons;
    sead::WFixedSafeString<16> shineBaseIcons;
    s32 activeIndex =
        SingleModeDataFunction::getCurActiveScenarioIndex(this, mScenarioInfo.mIslandId);
    u64 scenarioFlag = SingleModeDataFunction::getScenarioFlag(this, mScenarioInfo.mIslandId);
    s32 scenarioNum = SingleModeDataFunction::getScenarioNum(this, mScenarioInfo.mIslandId);

    if (scenarioNum != 0) {
        s32 lastFixed = activeIndex < 2 ? activeIndex : 2;
        for (s32 i = 0; i <= lastFixed; i++) {
            if (scenarioFlag & (1ull << i)) {
                shineIcons.append(LayoutFontUtil::getIconFontCatShine());
                shineBaseIcons.append(LayoutFontUtil::getIconFontCatShineBase());
            } else {
                shineIcons.append(LayoutFontUtil::getIconFontCatShineEmpty());
                shineBaseIcons.append(LayoutFontUtil::getIconFontCatShineBase());
            }
        }

        for (s32 i = 3; i < scenarioNum; i++) {
            if (scenarioFlag & (1ull << i)) {
                shineIcons.append(LayoutFontUtil::getIconFontCatShine());
                shineBaseIcons.append(LayoutFontUtil::getIconFontCatShineBase());
            } else if (i == activeIndex) {
                shineIcons.append(LayoutFontUtil::getIconFontCatShineEmpty());
                shineBaseIcons.append(LayoutFontUtil::getIconFontCatShineBase());
            }
        }
    }

    al::setPaneString(this, "TxtShine", shineIcons.cstr());
    al::setPaneString(this, "TxtShine_empty", shineBaseIcons.cstr());
    al::updateLayoutPaneRecursive(this);
}

/**
 * @brief Starts fading the banner out, or defers it until the appear animation ends.
 * @param isForce Whether to forget the current scenario so the next one always reappears.
 */
void AreaNameParts::fadeOut(bool isForce) {
    if (isForce) {
        mScenarioInfo.mIslandId = -2;
    }

    if (isFadeOut() || mIsFadeOutRequested) {
        return;
    }

    if (al::isNerve(this, &NrvAreaNamePartsAppear)) {
        mIsFadeOutRequested = true;
        return;
    }

    al::setNerve(this, &NrvAreaNamePartsFadeOut);
}

/**
 * @brief Makes the next appearance skip its appear animation.
 * @param isPhaseStart Whether the scenario banner should still play its appear action.
 */
void AreaNameParts::forceEndAppear(bool isPhaseStart) {
    mIsForceEndAppear = true;
    mIsForceEndAppearPhaseStart = isPhaseStart;
}

/**
 * @brief Sets the white fade wipe to wait for after an island warp.
 * @param pWipe The white fade wipe.
 */
void AreaNameParts::setWipeFadeWhite(al::WipeSimple* pWipe) {
    mWipeFadeWhite = pWipe;
}

/** @brief Hides the banner while the player warps to another island. */
void AreaNameParts::handleIslandWarp() {
    mScenarioInfo.mIslandId = -1;
    al::startFreezeActionEnd(this, "Hide", nullptr);
    al::startFreezeActionEnd(this, "ScenarioBannerHide", "ScenarioBanner");
    al::setNerve(this, &NrvAreaNamePartsWaitFadeWhite);
}

/**
 * @brief Restores the scenario banner after a demo.
 * @param isAppear Whether to play the scenario banner's appear action.
 */
void AreaNameParts::endDemo(bool isAppear) {
    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (phase == cPhaseNoScenarioA || phase == cPhaseNoScenarioB) {
        return;
    }

    bool isShowScenario;
    if (IslandDataFunction::isGigaBellIsland(mScenarioInfo.mIslandId + 1) &&
        SingleModeDataFunction::getGoalItemsCollected(this) <
            SingleModeDataFunction::getGigaBellLockCount(GameDataHolderAccessor(this))) {
        isShowScenario = true;
    } else {
        s32 scenarioIndex = mScenarioInfo.mScenarioIndex;
        isShowScenario =
            scenarioIndex >= 0 &&
            scenarioIndex < SingleModeDataFunction::getScenarioNum(this, mScenarioInfo.mIslandId) &&
            !SingleModeDataFunction::isScenarioComplete(this, mScenarioInfo.mIslandId,
                                                        mScenarioInfo.mScenarioIndex);
    }

    if (isShowScenario) {
        al::showPane(this, "Scenario");
        al::showPane(this, "ScenarioBanner");
    } else {
        al::hidePane(this, "Scenario");
        al::hidePane(this, "ScenarioBanner");
    }

    if (isAppear) {
        al::startAction(this, "ScenarioBannerAppear", "ScenarioBanner");
    }
}

/** @brief Plays the appear animation, then shows the scenario banner. */
void AreaNameParts::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
    }

    if (!al::isActionEnd(this, nullptr)) {
        return;
    }

    if (mIsFadeOutRequested) {
        al::setNerve(this, &NrvAreaNamePartsFadeOutHide);
        return;
    }

    if (al::isGreaterStep(this, 300)) {
        al::startAction(this, "Hide", nullptr);
        al::startAction(this, "ScenarioBannerAppear", "ScenarioBanner");

        if (!IslandDataFunction::isGigaBellIsland(mScenarioInfo.mIslandId + 1)) {
            mParent->showShineCounter();
        }

        al::setNerve(this, &NrvAreaNamePartsWait);
    }
}

/** @brief Keeps the banner as it is. */
void AreaNameParts::exeWait() {}

/** @brief Waits for the white fade wipe to close. */
void AreaNameParts::exeWaitFadeWhite() {
    if (!mWipeFadeWhite->isAlive()) {
        al::setNerve(this, &NrvAreaNamePartsWait);
    }
}

/** @brief Waits a moment after a name change before idling. */
void AreaNameParts::exeChangeName() {
    if (al::isGreaterStep(this, 60)) {
        al::setNerve(this, &NrvAreaNamePartsWait);
    }
}

/** @brief Plays the hide animation, then reappears with a new name or disappears. */
void AreaNameParts::exeFadeOut() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvAreaNamePartsFadeOutHide)) {
            al::startAction(this, "Hide", nullptr);
        } else {
            al::startAction(this, "ScenarioBannerHide", "ScenarioBanner");
        }
    }

    if (al::isNerve(this, &NrvAreaNamePartsFadeOutHide)) {
        if (!al::isActionEnd(this, "Main")) {
            return;
        }
    } else {
        if (!al::isActionEnd(this, "ScenarioBanner")) {
            return;
        }

        if (al::isNerve(this, &NrvAreaNamePartsFadeOutChangeName) &&
            mScenarioInfo.mIslandId >= 0) {
            updateTextBoxes();
            al::setNerve(this, &NrvAreaNamePartsAppear);
            return;
        }
    }

    mIsFadeOutRequested = false;
    kill();
}
