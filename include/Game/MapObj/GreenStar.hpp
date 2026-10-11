#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadQuat.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace al {
    class MtxConnector;
    class OccludedEffectRequestInfo;
}  // namespace al

class ActorStateDemoCamera;
class ActorStateDemoCameraParam;
class ItemBubble;
class ItemStateAssistRotate;
class ItemStatePopUpFront;

/**
 * @brief A green star, either placed in the course, attached to an item bubble or spawned as an
 * item.
 */
class GreenStar : public al::LiveActor {
public:
    explicit GreenStar(const char* pName, ItemBubble* pBubble = nullptr, bool isAttach = false);
    ~GreenStar() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void switchKill();
    void initAfterPlacement() override;
    void makeActorAppeared() override;
    void makeActorDead() override;
    void control() override;
    void appearWithPos(const sead::Vector3f& rPos);
    bool isEnableMsgItemGet(const al::SensorMsg* pMsg) const;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    void doGet(al::HitSensor* pOther, al::HitSensor* pSelf);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void setNoConnect();
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
    void exePopUpFront();

    bool isAcquiredInScene() const { return mIsAcquiredInScene; }
    al::HitSensor* getAcquirerSensor() const { return mAcquirerSensor; }

private:
    inline void updateAppearMove(f32 rate);

    ActorStateDemoCamera* mDemoCamera = nullptr;                  // 0x148
    ActorStateDemoCameraParam* mDemoCameraParam = nullptr;        // 0x150
    sead::FixedSafeString<32>* mDemoCameraName = nullptr;         // 0x158
    ItemStateAssistRotate* mAssistRotate = nullptr;               // 0x160
    ItemStatePopUpFront* mPopUpFront = nullptr;                   // 0x168
    al::MtxConnector* mConnector = nullptr;                       // 0x170
    ItemBubble* mBubble;                                          // 0x178
    al::HitSensor* mAcquirerSensor = nullptr;                     // 0x180
    sead::Quatf mBaseQuat = sead::Quatf::unit;                    // 0x188
    f32 mRotateY = 0.0f;                                          // 0x198
    sead::Vector3f mAppearTrans = sead::Vector3f::zero;           // 0x19c
    s32 mGreenStarId = 0;                                         // 0x1a8
    bool mIsAcquired = false;                                     // 0x1ac
    bool mIsAcquiredInScene = false;                              // 0x1ad
    bool mIsInvalidClipping = false;                              // 0x1ae
    bool mIsConnectOnlyTrans = false;                             // 0x1af
    bool mIsPlacementInRouteDokan = false;                        // 0x1b0
    bool mIsDropShadowActorDown = false;                          // 0x1b1
    bool mIsNotUseLpp = false;                                    // 0x1b2
    bool mIsForceWaitAppear = false;                              // 0x1b3
    bool mUsingOccludedEffect = true;                             // 0x1b4
    s32 mSeType = 0;                                              // 0x1b8
    sead::Vector3f mClippingShadowExpand = sead::Vector3f::zero;  // 0x1bc
    bool mIsSpinSeStarted = false;                                // 0x1c8
    bool mIsAttach;                                               // 0x1c9
    bool mDisconnectWhenGot = false;                              // 0x1ca
    al::OccludedEffectRequestInfo* mOccludedEffect = nullptr;     // 0x1d0
};

static_assert(sizeof(GreenStar) == 0x1d8);
