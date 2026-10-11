#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class NpcStateWait;

/**
 * @brief A Toad (キノピオ) NPC that just stands around, reacting to the player.
 *
 * The coat color is picked from the "KinopioColor" placement argument.
 */
class KinopioNpc : public al::LiveActor {
public:
    KinopioNpc(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    void exeWait();

private:
    NpcStateWait* mStateWait = nullptr;
};

static_assert(sizeof(KinopioNpc) == 0x150);
