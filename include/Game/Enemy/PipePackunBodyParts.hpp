#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class PipePackun;

/** @brief One segment of a Pipe Piranha Plant stem. */
class PipePackunBodyParts : public al::LiveActor {
public:
    PipePackunBodyParts(PipePackun* pHost, const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    bool isStraight() const;
    bool isHitToBody(al::HitSensor* pSelf, al::HitSensor* pOther, sead::Vector3f* pHitPos, sead::Vector3f* pNormal);
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void exeWait();
    void killSwitch();
    f32 getLength() const;
    void setTotalLength(f32 length);
    void setTurnPattern(const PipePackunBodyParts* pPrevious);
    void calcPos(sead::Vector3f* pPos, f32 rate) const;
    void calcDir(sead::Vector3f* pDir, f32 rate) const;
    void setCoord(f32 coord);
    f32 calcNearCoord(f32* pDistance, const sead::Vector3f& rPos);
    /** @brief Gets the cumulative endpoint coordinate. @return Coordinate at the segment end. */
    f32 getTotalLength() const { return mTotalLength; }
private:
    PipePackun* mHost;
    void* mUnused = nullptr;
    sead::Vector3f mDirection = sead::Vector3f::ez;
    f32 mTotalLength = 0.0f;
    f32 mLength = 0.0f;
    f32 mCoordRate = 0.0f;
    bool mIsStraight = false;
    bool mTurnPattern = false;
};
static_assert(sizeof(PipePackunBodyParts) == 0x178);
