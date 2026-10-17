#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class ChorobonNode;
class ChorobonItem;

/** @brief Coordinates a train of Fuzzies along a rail. */
class ChorobonHolder : public al::LiveActor {
public:
    explicit ChorobonHolder(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    f32 calcNearestCoordOfPoint(const al::LiveActor* pActor);
    void exeWatch();
    ChorobonNode* tryGetNodeStart();
    ChorobonNode* tryGetNodeEnd();
private:
    ChorobonNode** mNodes = nullptr;
    ChorobonItem* mItemNode = nullptr;
    s32 mCount = 0;
    f32 mSpeed = 5.0f;
};
static_assert(sizeof(ChorobonHolder) == 0x160);
