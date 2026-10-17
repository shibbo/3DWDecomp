#pragma once
#include <container/seadPtrArray.h>
#include "Library/Scene/ISceneObj.hpp"
class LuckyIsland;
class LuckyIslandHolder : public al::ISceneObj {
public:
    LuckyIslandHolder();
    void registerIsland(LuckyIsland*);
    bool canStartLuckyIslandDemo() const;
    LuckyIsland* startLuckyIslandDemo();
    void queueLuckyIslandDemo();
    void setIslandFlagActive();
    void disableLuckyIslandCollision();
    int getIslandCount() const { return mIslands.size(); }
    LuckyIsland* getIsland(int index) const { return mIslands[index]; }
    LuckyIsland* getCurrentIsland() const { return mCurrentIsland; }
    void setCurrentIsland(LuckyIsland* island) { mCurrentIsland = island; }
private:
    sead::PtrArray<LuckyIsland> mIslands;
    LuckyIsland* mCurrentIsland = nullptr;
};
