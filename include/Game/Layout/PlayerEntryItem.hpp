#pragma once

#include <basis/seadTypes.h>
#include <controller/nin/seadNinJoyNpadDevice.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class GameDataHolder;
class PlayerEntryMini;
class PlayerKeyConfig;

/// One controller user's slot in the player-entry window (character select and join/leave).
class PlayerEntryItem : public al::LayoutActor {
public:
    PlayerEntryItem(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                    PlayerEntryMini* pEntryMini, s32 userId, const GameDataHolder* pHolder);

    void appear() override;
    s32 getPortNum() const;
    void control() override;
    void end();
    bool isNotActive() const;
    bool isDecided() const;
    void startShuffle();
    void startEntryEnd();
    void setPlayerActive(bool isActive);
    bool isShuffleEnd() const;
    void startDemo();
    void endDemo();
    void hide();
    bool isShowLayout() const;
    bool isLayoutVisible() const;
    void setChangeSrcUser();
    void setChangeDstUser();

    void exePlayAppear();
    void exePlayWait();
    void exePlayEnd();
    void exeHide();
    void exeWaitEntry();
    void exeEntryAppear();
    void exeEntryWait();
    void exeEntryEnd();
    void exeHiddenAppear();
    void exeSelectAppear();
    void exeSelectWait();
    void exeSelectEnd();
    void exeCursorMove();
    void exeDecide();
    void exeDecideEnd();
    void exeShuffle();
    void exeShuffleEnd();
    void exeEnd();

private:
    bool isWaitingSingleJoyConnect() const;

    PlayerEntryMini* mEntryMini;
    PlayerKeyConfig* mKeyConfig = nullptr;
    s32 mUserId;
    s32 mCharacterType;
    s32 mIdleFrame = 0;
    bool mIsDemo = false;
    const GameDataHolder* mGameDataHolder;
    bool mIsPlayerActive = false;
    sead::NinJoyNpadDevice::Style mControllerStyle = sead::NinJoyNpadDevice::cStyle_Invalid;
};
static_assert(sizeof(PlayerEntryItem) == 0x158);
