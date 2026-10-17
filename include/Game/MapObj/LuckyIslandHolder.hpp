#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
class LuckyIslandHolder : public al::ISceneObj {
public:
    sead::PtrArray<al::LiveActor> mIslands;
    al::LiveActor* mActiveIsland;
};
