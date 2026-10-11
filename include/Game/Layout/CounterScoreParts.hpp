#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}
class GameDataHolder;

class CounterScoreParts : public al::LayoutActor {
public:
    CounterScoreParts(const al::LayoutInitInfo& rInfo, const char* pName,
                      const char* pPartsName, al::LayoutActor* pParent,
                      const GameDataHolder* pGameDataHolder);

    void exeAppear();
    void exeWait();

private:
    s32 mScore = 0;
    const GameDataHolder* mGameDataHolder;
};
