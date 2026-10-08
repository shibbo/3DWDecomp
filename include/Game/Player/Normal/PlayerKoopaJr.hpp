#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include "Player/Normal/PlayerProperty.hpp"

namespace al {
class PadRumbleKeeper;
}  // namespace al

/// Bowser Jr. when controlled by a second player in Bowser's Fury.
class PlayerKoopaJr : public al::LiveActor, public al::ISceneObj {
public:
    explicit PlayerKoopaJr(const char* pName);

    static PlayerKoopaJr* tryGetPlayerKoopaJr(const al::IUseSceneObjHolder* pUser);
    bool isWaitingForOptionsMenu();
    void endOptionsIntro();
    bool canDoIslandWarp() const;
    bool tryPraiseReaction(int);
    void hideForDemo();
    void startMidBossDemo();
    void showFromDemo();
    void startCloudBonusLauncher();
    void endCloudBonusLauncher();
    void resetTransformPostCutscene(const sead::Vector3f& rTrans, const sead::Vector3f& rFront);
    PlayerProperty* getProperty() { return &mProperty; }
    bool isThrowingItem() const;
    void startGoalItemDemo(const sead::Vector3f& rTrans);
    void endGoalItemDemo();
    bool tryThrowStockItem(int itemType, const char* pItemName, al::LiveActor* pPlayer);
    bool tryStartAmiiboAttack();
    void startMysteryBox();
    void endMysteryBox();

    /**
     * @brief Check whether Bowser Jr. is driven by the AI instead of a second player.
     * @return True if the AI moves Bowser Jr.
     */
    bool isAIMovement() const { return mIsUseAIMovement; }

    /**
     * @brief Get the rumble keeper of Bowser Jr.'s controller.
     * @return The pad rumble keeper.
     */
    al::PadRumbleKeeper* getPadRumbleKeeper() const { return mPadRumbleKeeper; }

private:
    u8 _150[0x158 - 0x150];
    al::PadRumbleKeeper* mPadRumbleKeeper;  // 0x158
    u8 _160[0x170 - 0x160];
    PlayerProperty mProperty;  // 0x170
    u8 _1ec[0x2dc - 0x1ec];
    bool mIsUseAIMovement;  // 0x2dc
};
