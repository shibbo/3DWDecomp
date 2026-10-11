#pragma once

#include <basis/seadTypes.h>
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class ActorInitInfo;
class LayoutInitInfo;
}  // namespace al
class GameDataHolder;
class PlayerEntryPlayer;

/**
 * @brief Character select layout where every player picks a character before playing.
 * @note Only the members used by already-decompiled callers are declared.
 */
class PlayerEntry : public al::LayoutActor {
public:
    PlayerEntry(const al::LayoutInitInfo& rLayoutInfo, const al::ActorInitInfo& rActorInfo,
                GameDataHolder* pGameDataHolder);

    void startCharacterSelect();
    void appearWithDecidePort(s32 port);
    void resetButtonIcons();
    void hidePlayerAll();
    bool isRequestBack() const;
    bool isFinishBack() const;
    bool isAllPlayerDecided() const;
    bool isAllPlayerDecideStart() const;
    bool isEnablePartsControl() const;
    bool tryEntryPlayer(s32 userId, s32 characterType);
    void retirePlayer(s32 userId);
    PlayerEntryPlayer* getPlayerModel(s32 userId, s32 characterType, s32 index);

    static bool isUse3dPlayer();

    GameDataHolder* getGameDataHolder() const { return mGameDataHolder; }

private:
    u8 _121[0x130 - 0x121];
    GameDataHolder* mGameDataHolder;
    u8 _138[0x150 - 0x138];
};

static_assert(sizeof(PlayerEntry) == 0x150);
