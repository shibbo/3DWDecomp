#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
}  // namespace al

class GameDataHolder;

/**
 * @brief List of the collected green stars shown from the map menu.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ListClearStar {
public:
    ListClearStar(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder);

    void startAppear(s32 port, s32 worldId, bool isTransition, bool isRight);
    void startEnd(bool isTransition, bool isRight);
    bool isInTransition() const;
    bool isEnd() const;

private:
    u8 _0[0x160];
};

static_assert(sizeof(ListClearStar) == 0x160);
