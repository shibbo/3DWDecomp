#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadPtrArray.h>
class BombHei;

/** @brief Launches Bob-ombs from a reusable pool. */
class BombHeiLauncher : public al::LiveActor {
public:
    explicit BombHeiLauncher(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void start();
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void exeWaitStart();
    void exeWait();
    BombHei* tryGetDeadBombHei() const;
    void exeLaunchInterval();
    void exeLaunchReady();
    BombHei* getDeadBombHei() const;
    void exeLaunch();
private:
    sead::PtrArray<BombHei> mBombs;
    BombHei* mLaunchBomb = nullptr;
};
static_assert(sizeof(BombHeiLauncher) == 0x160);
