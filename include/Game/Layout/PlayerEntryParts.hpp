#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class PlayerEntry;
class PlayerEntryPlayer;

/// One player's panel in the character select window (join, pick a character, decide).
class PlayerEntryParts : public al::LayoutActor {
public:
    PlayerEntryParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                     s32 userId, PlayerEntry* pEntry);

    void appear() override;
    virtual bool processDisconnects();

    bool isUserPadConnected() const;
    void cancel();
    void closeEntry();
    bool isDecided() const;
    void resetButtonIcons();
    s32 getPadPortCurrentUser() const;
    bool isEntry() const;
    bool isActive() const;
    void requestStartDecision();
    bool isEnableStartDecision() const;
    bool isEndPlayerDecision() const;
    bool isUserPadTrigJoin() const;
    s32 getInitialCharacter() const;
    void setCharacter(s32 characterType);
    void updateArrow();
    bool isUserPadTrigDecide() const;
    bool isCancelDecideWait() const;
    bool isUserPadTrigCancel() const;
    s32 getCharacter() const;

    void exeEntry();
    void exeAppear();
    void exeSelect();
    void exeMoveOut();
    void exeMoveIn();
    void exeDecide();
    void exeDecideWait();
    void exeDecideEnd();
    void exeRetire();
    void exeBackEntry();
    void exeHide();
    void exeDisable();
    void exeEntryEnd();

private:
    PlayerEntry* mEntry;
    PlayerEntryPlayer* mPlayer = nullptr;
    s32 mUserId;
    s32 mCharacterType = 0;
    bool mIsShowArrow = true;
    bool mIsMoving = false;
    bool mIsDisableInput = false;
};

static_assert(sizeof(PlayerEntryParts) == 0x148);
