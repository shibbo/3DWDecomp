#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
}  // namespace al

class GameDataHolder;

/**
 * @brief List of the collected stamps shown from the map menu.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ListStamp {
public:
    ListStamp(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder);

    void startAppear(s32 port, bool isTransition, bool isRight);
    void startEnd(bool isTransition, bool isRight);
    bool isInTransition() const;
    bool isEnd() const;

private:
    u8 _0[0x138];
};

static_assert(sizeof(ListStamp) == 0x138);
