#pragma once

#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>

namespace al { class PlayerHolder; }
class IslandArea;

class IslandAreaWatcher : public al::ISceneObj {
public:
    explicit IslandAreaWatcher(al::PlayerHolder* pPlayerHolder);
    void registerIslandArea(IslandArea* pArea);
    IslandArea* getActiveIsland();
    int getActiveIslandIndex();
    int getCurrentIslandID();
    bool isEmpty() const;

private:
    sead::PtrArray<IslandArea> mAreas;
    IslandArea* mAreaBuffer[99];
    int mCurrentIsland;
    al::PlayerHolder* mPlayerHolder;
};
