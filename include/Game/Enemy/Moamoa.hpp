#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadVector.h>
class MoamoaHolder;
class ActorJointLookController;

class Moamoa : public al::LiveActor {
public:
    Moamoa(const char* pName, const MoamoaHolder* pHolder);
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    /** @brief Sets the offset within the holder's grid. @param rOffset Local placement offset. */
    void setOffset(const sead::Vector3f& rOffset) { mOffset.set(rOffset); }
    void exeWait();
    void exeDie();
    void exeMicWeakInit();
    void exeWeak();
    void exeWeakWait();
    void exeRevivalSign();
    void exeRevival();
private:
    const MoamoaHolder* mHolder;
    ActorJointLookController* mLookController;
    sead::Vector3f mOffset;
    s32 mMicDelay;
};
static_assert(sizeof(Moamoa) == 0x168);
