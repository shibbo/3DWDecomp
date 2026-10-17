#pragma once

#include "Library/Layout/LayoutActor.hpp"
#include "System/ScenarioInfo.hpp"

namespace al {
class LayoutInitInfo;
class LiveActor;
class PlayerHolder;
class WipeSimple;
}  // namespace al

class SingleModeSceneLayout;

/**
 * @brief Bowser's Fury HUD banner showing the name of the island / scenario the player enters.
 */
class AreaNameParts : public al::LayoutActor {
public:
    AreaNameParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                  al::LayoutActor* pParent, const al::PlayerHolder* pPlayerHolder);

    void changeName(const ScenarioInfo& rInfo);
    bool isFadeOut();
    void updateTextBoxes();
    void fadeOut(bool isForce);
    void forceEndAppear(bool isPhaseStart);
    void setWipeFadeWhite(al::WipeSimple* pWipe);
    void handleIslandWarp();
    void endDemo(bool isAppear);

    void exeAppear();
    void exeWait();
    void exeWaitFadeWhite();
    void exeChangeName();
    void exeFadeOut();

private:
    const al::PlayerHolder* mPlayerHolder;  // 0x128
    al::LiveActor* mPlayerActor;            // 0x130
    SingleModeSceneLayout* mParent;         // 0x138
    ScenarioInfo mScenarioInfo;             // 0x140
    bool mIsFadeOutRequested;               // 0x148
    bool mIsForceEndAppear;                 // 0x149
    bool mIsForceEndAppearPhaseStart;       // 0x14A
    u8 _14b[0x158 - 0x14b];
    al::WipeSimple* mWipeFadeWhite;  // 0x158
};

static_assert(sizeof(AreaNameParts) == 0x160);
