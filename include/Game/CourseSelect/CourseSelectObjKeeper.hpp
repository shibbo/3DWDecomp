#pragma once

#include <basis/seadTypes.h>
#include <prim/seadBitFlag.h>

namespace al {
class LiveActor;
}  // namespace al

/**
 * @brief Keeps the objects of the course-select map sorted by world, so only the objects of the
 * active worlds are shown.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectObjKeeper {
public:
    CourseSelectObjKeeper(s32 worldNum, s32 objNumMax);

    void registerObject(al::LiveActor* pActor, s32 worldId);
    void initAfterPlacement();
    void updateActiveWorld(const sead::BitFlag32& rWorldFlag);
    void appearWorldObj(s32 worldId);
    void killWorldObj(s32 worldId);

private:
    u8 _0[0x18];
};

static_assert(sizeof(CourseSelectObjKeeper) == 0x18);
