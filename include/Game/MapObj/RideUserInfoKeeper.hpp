#pragma once
#include <container/seadRingBuffer.h>

namespace al { class LiveActor; }
class RideUserInfoKeeper {
public:
    RideUserInfoKeeper();
    void resetInfo(const al::LiveActor* pActor);
    int calcRideUserNum() const;
    int calcRideNum() const;
    bool isAllUserRide() const;
    bool isUserRideGreaterEqual(int count) const;
    bool isRideGreaterEqual(int count) const;
    bool isRideAtLeastOne() const;
    bool isIncreasedRideUser() const;
    bool isIncreasedRider() const;
    void setUserRide(int userId);
private:
    struct UserInfo { int userId; int rideCount; };
    sead::RingBuffer<UserInfo> mUsers;
    int* mActiveUserIds = nullptr;
    int mPreviousRideUserCount;
    int mPreviousRideCount;
};
