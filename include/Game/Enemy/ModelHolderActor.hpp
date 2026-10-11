#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class ModelHolderActor : public al::LiveActor {
public:
    ModelHolderActor(const char* pName, al::LiveActor* pOwner);
    /** @brief Releases the model holder actor. */
    ~ModelHolderActor() override = default;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

private:
    al::LiveActor* mOwner;
};
