#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class RouteDokanEntrance;

/**
 * @brief Carries one player through the pipes of a RouteDokan.
 */
class RouteDokanRider : public al::LiveActor {
public:
    RouteDokanRider(const char* pName, s32 type);

    void setMoveSpeed(f32 speed);
    bool isActive(s32 playerIndex) const;
    bool canBindSingleMode();
    void startBind(RouteDokanEntrance* pEntrance, al::HitSensor* pSender,
                   al::HitSensor* pReceiver);
    void tryCancelBind(al::HitSensor* pSender);
    bool damage(al::HitSensor* pSender);

private:
    u8 _148[0xc8];
};

static_assert(sizeof(RouteDokanRider) == 0x210);
