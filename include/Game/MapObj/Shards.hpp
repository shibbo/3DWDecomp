#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace al {
    class MtxConnector;
    class PlacementInfo;
}  // namespace al

class ActorStateDemoCamera;
class ActorStateDemoCameraParam;
class ItemStateAssistRotate;
class ItemStatePopUpFront;
class ShardsWatcher;

/**
 * @brief A green star shard ("ShardPiece"); collecting all shards of a ShardsWatcher spawns its
 * goal item.
 */
class Shards : public al::LiveActor {
public:
    explicit Shards(const char* pName, bool isAttach = false);

    void init(const al::ActorInitInfo& rInfo) override;
    void sharedInit(const al::ActorInitInfo& rInfo);
    void switchKill();
    void initAfterPlacement() override;
    void respawn() override;
    void respawnShard();
    void makeActorAppeared() override;
    void startAction(const char* pActionName);
    void makeActorDead() override;
    void startClipped() override;
    void endClipped() override;
    void control() override;
    void appearWithPos(const sead::Vector3f& rPos);
    void appearBySwitch();
    bool isEnableMsgItemGet(const al::SensorMsg* pMsg, al::HitSensor* pOther) const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void doGet(al::HitSensor* pOther, al::HitSensor* pSelf);
    bool isActionEnd();
    bool isGot() const;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void updateLinkedTrans(const sead::Vector3f& rTrans) override;
    void startDemoActor(s32 demoType) override;
    void endDemoActor(s32 demoType) override;
    bool isFinalShard() const;
    void setNoConnect();
    void setShardId(const al::PlacementInfo& rInfo);
    void rotate(f32 speed);
    void exeAppear();
    void exeDirectAppear();
    void exeWait();
    void exeAttached();
    void exeThrow();
    void exeSpinDrc();
    void exeDemoAppear();
    void exeDirectDisappear();
    void exeDisappear();
    void exeGot();
    void exeGotLast();
    void exePopUpFront();

    void setWatcher(ShardsWatcher* pWatcher) { mWatcher = pWatcher; }
    s32 getIslandId() const { return mIslandId; }
    void setIslandId(s32 islandId) { mIslandId = islandId; }
    void setFromWatcher() { mFromWatcher = true; }
    bool isCollected() const { return mCollected; }
    bool isCollectionFinished() const { return mCollectionFinished; }
    bool isCollecting() const { return mCollecting; }
    bool isGoalPending() const { return mGoalPending; }
    bool isCollectedBySensor() const { return mCollectedBySensor; }
    al::HitSensor* getCollectSensor() const { return mCollectSensor; }
    const sead::Vector3f& getGoalFront() const { return mGoalFront; }
    bool isUseFrontAngle() const { return mUseFrontAngle; }
    f32 getFrontAngle() const { return mFrontAngle; }

private:
    inline void invalidateClippingAll();
    inline void validateClippingAll();
    inline void updateDemoEffect();
    inline void updateAppearMove(f32 rate);

    ShardsWatcher* mWatcher;                                // 0x148
    ActorStateDemoCamera* mDemoCamera = nullptr;            // 0x150
    ActorStateDemoCameraParam* mDemoCameraParam = nullptr;  // 0x158
    sead::FixedSafeString<32>* mDemoCameraName = nullptr;   // 0x160
    ItemStateAssistRotate* mAssistRotate = nullptr;         // 0x168
    ItemStatePopUpFront* mPopUpFront = nullptr;             // 0x170
    al::MtxConnector* mConnector = nullptr;                 // 0x178
    al::HitSensor* mCollectSensor = nullptr;                // 0x180
    sead::Quatf mBaseQuat = sead::Quatf::unit;              // 0x188
    f32 mRotateY = 0.0f;                                    // 0x198
    sead::Vector3f mAppearTrans = sead::Vector3f::zero;     // 0x19c
    s32 mShardId = 0;                                       // 0x1a8
    s32 mIslandId = 0;                                      // 0x1ac
    bool mCollected = false;                                // 0x1b0
    bool mCollectionFinished = false;                       // 0x1b1
    bool mIsInvalidClipping = false;                        // 0x1b2
    bool mIsConnectOnlyTrans = false;                       // 0x1b3
    bool mIsPlacementInRouteDokan = false;                  // 0x1b4
    bool mIsDropShadowActorDown = false;                    // 0x1b5
    bool mIsNotUseLpp = false;                              // 0x1b6
    bool mIsForceWaitAppear = false;                        // 0x1b7
    bool mUsingOccludedEffect = true;                       // 0x1b8
    s32 mUnknown1bc;                                        // 0x1bc
    sead::Vector3f mClippingShadowExpand = sead::Vector3f::zero;  // 0x1c0
    bool mIsSpinSeStarted = false;                          // 0x1cc
    bool mIsAttach;                                         // 0x1cd
    bool mDisconnectWhenGot = false;                        // 0x1ce
    const char* mModelName = nullptr;                       // 0x1d0
    bool mIsSwitchAppear = false;                           // 0x1d8
    bool mIsKeepDeadAfterPlacement = false;                 // 0x1d9
    bool mIsInitialized = false;                            // 0x1da
    bool mCollecting = false;                               // 0x1db
    bool mGoalPending = false;                              // 0x1dc
    bool mIgnoreIslandOffset = false;                       // 0x1dd
    bool mFromWatcher = false;                              // 0x1de
    bool mCollectedBySensor = false;                        // 0x1df
    bool mIsSpinByAttack = false;                           // 0x1e0
    bool mIsClippedInDemo = false;                          // 0x1e1
    bool mIsDemoEffectStopped = false;                      // 0x1e2
    bool mUseFrontAngle = false;                            // 0x1e3
    f32 mFrontAngle = 0.0f;                                 // 0x1e4
    sead::Vector3f mGoalFront;                              // 0x1e8
    al::LiveActor* mEmptyActor = nullptr;                   // 0x1f8
};

static_assert(sizeof(Shards) == 0x200);
