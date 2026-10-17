#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class Moamoa;

class MoamoaHolder : public al::LiveActor {
public:
    explicit MoamoaHolder(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    /** @brief Allows held enemies to track the player. */
    void validateLookCtrl() { mIsLookCtrl = true; }
    /** @brief Stops held enemies from tracking the player. */
    void invalidateLookCtrl() { mIsLookCtrl = false; }
    /** @brief Checks whether player tracking is enabled. @return Whether tracking is enabled. */
    bool isLookCtrl() const { return mIsLookCtrl; }
private:
    Moamoa** mActors = nullptr;
    s32 mCount = 0;
    bool mIsLookCtrl = true;
};
static_assert(sizeof(MoamoaHolder) == 0x158);
