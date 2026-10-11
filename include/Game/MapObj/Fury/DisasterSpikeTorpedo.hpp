#pragma once

#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class DashPanel;
class DisasterSpikeDirector;
class GigaBellManager;
class SuperBowser;

/// Spike Fury Bowser shoots through the ocean at Plessie.
class DisasterSpikeTorpedo : public al::LiveActor {
public:
    explicit DisasterSpikeTorpedo(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void setDisasterSpikeDirector(DisasterSpikeDirector* pDirector);
    void appearSetup();
    void appear(SuperBowser* pBowser, s32 index, bool isFirst);
    void calcAppearStartPos();
    void setUpDashPanel();
    void appearReckless(SuperBowser* pBowser, sead::Vector3f pos);
    void kill() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool canDamage();
    void damage(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;

    void exeWait();
    void exeAppear();
    void updateShootInfo();
    void updateDashPanel();
    void checkTorpedoSpikeRestrictionAreas(bool isCheckSink, bool isCheckExplode);
    void exeFall();
    void exeShoot();
    void enforceFollowAngleLimit();
    void moveForward();
    void exeSink();
    void exeCrumble();
    void updateCrumble();
    void exeCrumbleLaser();

    f32 getSideOffsetScale();
    f32 getFrontOffsetScale();
    f32 getShootSpeed();
    void calcShootStartPos();
    void calcShootStartQuat();
    sead::Vector3f calcTorpedoSpikeRestrictionAreaCheckPos() const;

    /**
     * @brief Get the id given to the torpedo when it was shot.
     * @return The torpedo id.
     */
    s32 getID() const { return mID; }

private:
    DisasterSpikeDirector* mDirector = nullptr;                // 0x148
    GigaBellManager* mGigaBellManager = nullptr;               // 0x150
    SuperBowser* mBowser = nullptr;                            // 0x158
    s32 mID = -1;                                              // 0x160
    bool mIsFirst = false;                                     // 0x164
    sead::Vector3f mShootDir = sead::Vector3f::ez;             // 0x168
    sead::Vector3f mAppearStartPos = sead::Vector3f::zero;     // 0x174
    sead::Vector3f mShootStartPos = sead::Vector3f::zero;      // 0x180
    sead::Quatf mShootStartQuat = sead::Quatf::unit;           // 0x18c
    sead::Vector3f mSinkStartPos = sead::Vector3f::zero;       // 0x19c
    s32 mSinkFrames = 120;                                     // 0x1a8
    DashPanel* mDashPanel = nullptr;                           // 0x1b0
    f32 mFallSpeed;                                            // 0x1b8
    s32 mAppearFrames = 0;                                     // 0x1bc
    s32 mShootFrames = 0;                                      // 0x1c0
    sead::Vector3f mBowserPos = sead::Vector3f::zero;          // 0x1c4
    f32 mMinDistanceFromBowser = 0.0f;                         // 0x1d0
};

static_assert(sizeof(DisasterSpikeTorpedo) == 0x1d8);
