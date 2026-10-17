#include "MapObj/RideUserInfoKeeper.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

RideUserInfoKeeper::RideUserInfoKeeper() {
    int capacity = rc::getControlUserNumMax();
    mUsers.allocBuffer(capacity, nullptr);
    mActiveUserIds = new int[capacity];
}

void RideUserInfoKeeper::resetInfo(const al::LiveActor* pActor) {
    mPreviousRideUserCount = calcRideUserNum();
    mPreviousRideCount = calcRideNum();
    mUsers.clear();
    int count = rc::findActiveUserIdList(mActiveUserIds, GameDataHolderAccessor(pActor));
    for (int i = 0; i < count; ++i)
        mUsers.pushBack({mActiveUserIds[i], 0});
}

int RideUserInfoKeeper::calcRideUserNum() const {
    if (mUsers.size() < 1)
        return 0;
    int count = 0;
    for (const auto& user : mUsers)
        if (user.rideCount > 0)
            ++count;
    return count;
}

int RideUserInfoKeeper::calcRideNum() const {
    if (mUsers.size() < 1)
        return 0;
    int count = 0;
    for (const auto& user : mUsers)
        count += user.rideCount;
    return count;
}

bool RideUserInfoKeeper::isAllUserRide() const {
    for (int i = 0; i < mUsers.size(); ++i)
        if (mUsers(i).rideCount == 0)
            return false;
    return true;
}

bool RideUserInfoKeeper::isUserRideGreaterEqual(int count) const { return calcRideUserNum() >= count; }
bool RideUserInfoKeeper::isRideGreaterEqual(int count) const { return calcRideNum() >= count; }
bool RideUserInfoKeeper::isRideAtLeastOne() const { return calcRideNum() > 0; }
bool RideUserInfoKeeper::isIncreasedRideUser() const { return mPreviousRideUserCount < calcRideUserNum(); }
bool RideUserInfoKeeper::isIncreasedRider() const { return mPreviousRideCount < calcRideNum(); }

void RideUserInfoKeeper::setUserRide(int userId) {
    int index = -1;
    for (int i = 0; i < mUsers.size(); ++i) {
        if (mUsers(i).userId == userId) {
            index = i;
            break;
        }
    }
    if (index != -1)
        ++mUsers(index).rideCount;
}
