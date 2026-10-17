#include "Course/LoadingLayout.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolder.hpp"
#include "System/Data/SingleModeData.hpp"

#include <cstdlib>

namespace {
NERVE_DECL(LoadingLayout, Appear);
NERVE_DECL(LoadingLayout, WaitAppear);
NERVE_DECL(LoadingLayout, Loop);
NERVE_DECL(LoadingLayout, End);
NERVES_MAKE_STRUCT(LoadingLayout, Appear, WaitAppear, End)
NERVES_MAKE_NOSTRUCT(LoadingLayout, Loop)
}

/**
 * @brief Creates the loading screen and seeds its previous icon selection.
 * @param rInfo Layout initialization context.
 * @param pGameData Save data used to determine available icons.
 */
LoadingLayout::LoadingLayout(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameData)
    : al::LayoutActor("LoadingLayout"), mpGameData(pGameData) {
    al::initLayoutActor(this, rInfo, "RCS_Loading", nullptr);
    initNerve(&NrvLoadingLayout.Appear, 0);
    mIcon = std::rand() % 4 + 1;
}

/**
 * @brief Displays the loading screen with an immediate or delayed appearance.
 * @param isDefaultIcon True to select the first character icon.
 * @param isDelayAppear True to wait before playing the appear animation.
 */
void LoadingLayout::startDisplay(bool isDefaultIcon, bool isDelayAppear) {
    appear();
    if (isDefaultIcon) {
        mIcon = 1;
        al::startAction(this, "SetDotCharacter1", "DotCharacter");
    } else {
        selectRandomIcon();
    }
    mEndRequested = false;
    al::setNerve(this, isDelayAppear ? static_cast<const al::Nerve*>(&NrvLoadingLayout.WaitAppear)
                                   : &NrvLoadingLayout.Appear);
}

/** @brief Selects an unlocked character icon without repeating the previous icon. */
void LoadingLayout::selectRandomIcon() {
    s32 count = getNumIconsAvailable();
    s32 icon = std::rand() % count + 1;
    if (icon == mIcon) {
        icon = icon == count ? 1 : icon + 1;
    }
    mIcon = icon;
    al::startAction(this, al::StringTmp<32>("SetDotCharacter%d", mIcon).cstr(), "DotCharacter");
}

/** @brief Requests the loading screen's end transition. */
void LoadingLayout::requestEnd() { mEndRequested = true; }

/** @brief Plays the appear action and enters the loop when it finishes. */
void LoadingLayout::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear");
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvLoadingLayoutLoop);
    }
}

/** @brief Waits up to 120 frames before appearing, unless ending is requested. */
void LoadingLayout::exeWaitAppear() {
    if (mEndRequested) {
        al::setNerve(this, &NrvLoadingLayout.End);
        return;
    }
    if (al::isGreaterEqualStep(this, 120)) {
        al::setNerve(this, &NrvLoadingLayout.Appear);
    }
}

/** @brief Honors an end request after at least 90 frames in the loop. */
void LoadingLayout::exeLoop() {
    if (mEndRequested && al::isGreaterEqualStep(this, 90)) {
        al::setNerve(this, &NrvLoadingLayout.End);
    }
}

/** @brief Plays the end animation and kills the layout once it finishes. */
void LoadingLayout::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End");
    }
    if (al::isActionEnd(this)) {
        kill();
    }
}

/** @brief Returns the number of icons unlocked by the current Fury save. */
s32 LoadingLayout::getNumIconsAvailable() {
    const SingleModeData* data = mpGameData->getSingleFile();
    if (data->getUnlockedPhase() >= 3 && data->isFirstPhase2BossDefeated()) {
        return 4;
    }
    return data->hasSeenCutscene(1) ? 3 : 2;
}
