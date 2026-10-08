#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "NPC/NpcTargetFinder.hpp"

namespace al {
class ByamlIter;
class HitSensor;
class Nerve;
class PlacementInfo;
class SensorMsg;
}  // namespace al

class IUseNekoModeActor;
class NpcHeadController;
struct NekoAttachReason;

namespace neko {

/**
 * @brief A spot a cat can be brought to (a cat parent's drop target).
 */
class Target {
public:
    Target(const al::PlacementInfo& rInfo, s32 parentId, s32 index, const al::LiveActor* pHost);

    /** @brief Construct an unused target. */
    Target() { mTrans = sead::Vector3f::zero; }

    const sead::Vector3f* tryGetHostTrans() const;

    sead::Vector3f mTrans;
    s32 mIndex = -1;
    bool mIsActive = true;
    s32 mParentId = 0;
    s32 mId = 0;
    f32 mRange = 0.0f;
    const al::LiveActor* mHost = nullptr;
    s32 _28 = 7;
};

static_assert(sizeof(Target) == 0x30);

NpcHeadController* makeHeadController(IUseNekoModeActor* pActor, al::ByamlIter iter,
                                      NpcTargetFinder* pTargetFinder,
                                      npc::NpcFindTargetType targetType);
bool trySetNerve(al::IUseNerve* pUser, const al::Nerve* pNerve);
bool isSensorEnemyReactAttack(const al::HitSensor* pSensor);
bool isMsgNpcAttackerHitReaction(const IUseNekoModeActor* pActor, const al::SensorMsg* pMsg,
                                 const al::HitSensor* pOther, const al::HitSensor* pSelf);
bool isMsgHitReaction(const IUseNekoModeActor* pActor, const al::SensorMsg* pMsg,
                      const al::HitSensor* pOther, const al::HitSensor* pSelf);
bool isMsgMeraWanwanTrackAttack(const IUseNekoModeActor* pActor, const al::SensorMsg* pMsg,
                                const al::HitSensor* pOther, const al::HitSensor* pSelf);
void setAnimationRate(IUseNekoModeActor* pActor);
bool isActive(const IUseNekoModeActor* pActor, f32 range);
bool isInChaseRange(const IUseNekoModeActor* pActor, const al::LiveActor* pTarget);
bool isInChaseRange(const IUseNekoModeActor* pActor, const sead::Vector3f& rPos);
bool isInRange(const al::LiveActor* pActor, const sead::Vector3f& rCenter, f32 range);
bool checkGround(const al::LiveActor* pActor);
void scaleHitSensors(al::LiveActor* pActor, f32 scale);
s32 calcUID(al::PlacementHolder holder);
bool attackSensor(const al::HitSensor* pSelf, const al::HitSensor* pOther);
bool isSensorMapObjReactAttack(const al::HitSensor* pSensor);
bool isHitSensorRadius(f32 radius, const al::HitSensor* pSelf, const al::HitSensor* pOther);
bool tryReceiveMsgPushAndAddVelocityH(al::LiveActor* pActor, const al::SensorMsg* pMsg,
                                      const al::HitSensor* pOther, const al::HitSensor* pSelf,
                                      f32 pushSpeed, f32 radius, bool isCheckFall,
                                      bool isCheckAvoidArea);
bool isInPuddle(const al::LiveActor* pActor);
bool isInMotion(const al::LiveActor* pActor);
void setOnGroundFlag(const al::LiveActor* pActor, bool& rIsOnGround);

}  // namespace neko

/**
 * @brief Host actor of a cat; its behavior is implemented by IUseNekoModeActor modes.
 *
 * Every cat owns a regular mode actor (NekoNormal, or NekoParent for a mother cat) and a
 * disaster mode actor (NekoDisaster) and switches between them when Fury Bowser shows up.
 */
class Neko : public al::LiveActor, public DisasterModeController::IUseEventReceiver {
public:
    /**
     * @brief Behavior modes of a cat.
     */
    enum Mode : s32 {
        Mode_Normal = 0,
        Mode_Parent = Mode_Normal,  ///< A mother cat uses the regular mode slot.
        Mode_Disaster = 1,
        Mode_None = 2,
    };

    Neko(const char* pName, Neko* pRideNeko = nullptr);

    void init(const al::ActorInitInfo& rInfo) override;

    void initAsModelName(const al::ActorInitInfo& rInfo, const char* pModelName);
    void startAppearNormal();
    void tryStartClipped();
    bool isHidden() const;
    void tryEndClipped();
    void movementPaused(bool isPaused) override;
    bool isMode(Mode mode) const;
    void attachNormal(const NekoAttachReason& rReason);
    void attachDisaster(const NekoAttachReason& rReason);
    void exeWait();
    bool tryStartHide();
    void exeHide();
    void exeRespawn();
    bool isActiveInView() const;
    Mode getMode() const;
    al::LiveActor* tryGetNearestPlayerInRange(f32 range);
    void startKill();
    void startSeekTarget(const neko::Target* pTarget, bool isForce);
    void setActivePosition(const sead::Vector3f& rTrans);
    void setActiveFace(const sead::Vector3f& rDir);
    void onDisasterModeStateChange(DisasterModeController::State state) override;

    s32 getUID() const { return mUID; }

    IUseNekoModeActor* getNormalModeActor() const { return mNormalModeActor; }

    IUseNekoModeActor* getDisasterModeActor() const { return mDisasterModeActor; }

    IUseNekoModeActor* getModeActor() const { return mModeActor; }

    Neko* getRideNeko() const { return mRideNeko; }

    void setRideNeko(Neko* pNeko) { mRideNeko = pNeko; }

    void setRequestHide(bool isRequest) { mIsRequestHide = isRequest; }

private:
    /**
     * @brief Bits of mFlags.
     */
    enum Flag : u32 {
        Flag_Disaster = 1 << 0,  ///< Fury Bowser is rampaging, the cat should be in disaster mode.
        Flag_DisasterAnticipation = 1 << 3,  ///< Fury Bowser is about to show up.
        Flag_AttachedNormal = 1 << 5,
        Flag_AttachedDisaster = 1 << 6,
    };

    inline void initDisasterModeActor(const al::ActorInitInfo& rInfo, s32 colorType);
    inline void initNormalModeActor(const al::ActorInitInfo& rInfo, IUseNekoModeActor* pModeActor,
                                    s32 colorType);

    Neko* mRideNeko;  // 0x150
    s32 mUID = 0;  // 0x158
    IUseNekoModeActor* mNormalModeActor = nullptr;  // 0x160
    IUseNekoModeActor* mDisasterModeActor = nullptr;  // 0x168
    IUseNekoModeActor* mModeActor = nullptr;  // 0x170
    u32 mFlags = 0;  // 0x178
    NpcTargetFinder* mTargetFinder;  // 0x180
    sead::Vector3f mPlacementTrans = sead::Vector3f::zero;  // 0x188
    s32 mHideCheckTime = 0;  // 0x194
    bool mIsRequestHide = false;  // 0x198
};

static_assert(sizeof(Neko) == 0x1a0);
