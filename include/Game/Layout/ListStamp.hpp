#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al { class LayoutInitInfo; }
class GameDataHolder;
class ListStampParts;

/** @brief Stamp collection page shown from the map menu. */
class ListStamp : public al::LayoutActor {
public:
    ListStamp(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder);
    void exeAppear();
    void exeWait();
    void exeEnd();
    void startAppear(s32 port, bool isTransition, bool isLeft);
    void startEnd(bool isTransition, bool isLeft);
    bool isEnd() const;
    bool isInTransition() const;

private:
    int mPort = -1;
    const GameDataHolder* mGameDataHolder;
    ListStampParts* mStampParts = nullptr;
};

static_assert(sizeof(ListStamp) == 0x138);
