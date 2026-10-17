#pragma once

#include <container/seadBuffer.h>
#include <container/seadPtrArray.h>
#include <prim/seadBitFlag.h>

namespace al { class LiveActor; }

class CourseSelectObjKeeper {
public:
    CourseSelectObjKeeper(int worldNum, int maxObjectsPerWorld);
    void registerObject(al::LiveActor* pActor, int worldId);
    void initAfterPlacement();
    void updateActiveWorld(const sead::BitFlag<u32>& rActiveWorlds);
    void appearWorldObj(int worldId);
    void killWorldObj(int worldId);

private:
    sead::Buffer<sead::PtrArray<al::LiveActor>> mWorldObjects;
    int _10;
    sead::BitFlag<u32> mActiveWorlds;
};

static_assert(sizeof(CourseSelectObjKeeper) == 0x18);
