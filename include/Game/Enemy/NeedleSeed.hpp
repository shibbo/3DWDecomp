#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class NeedleSeed : public al::LiveActor {
public:
    NeedleSeed(const char* pName);
    /** @brief Releases the spike seed actor. */
    ~NeedleSeed() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void exeWait();
    void exeBreak();

private:
    bool _144 = false;
    const char* mItemType = nullptr;
};
