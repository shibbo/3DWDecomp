#pragma once

#include <container/seadObjArray.h>
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/TriangleFilterBase.hpp"
#include "Raidon/RaidonBase.hpp"

namespace al {
class AreaObj;
class AudioGeneralPurposeAreaChecker;
class CameraPoser_RS;
class ComboCounter;
class HitSensor;
class PadRumbleKeeper;
class PlayerHolder;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

class ActorStateSupportStroke;
class CameraPoserFollowLimit;
class DummyCameraTarget;
class IUseNpcPuppet;
class IUsePlayerPuppet;
class PlayerBindEndParam;
class RaidonPuppeteer;
class RaidonSurfAnimState;
class RaidonSurfStartState;
class RaidonSurfWaitState;
class SePlayObj;
enum NpcPuppetBindEndType : s32;

/// Plessie in her surfing form; registered as a scene object while she exists.
class RaidonSurf : public RaidonBase, public al::ISceneObj {
public:
    /// A place Plessie can respawn at after she was left behind.
    struct SpawnPoint {
        sead::Vector3f trans;
        sead::Vector3f front;
        s32 islandId;
        s32 scenarioId;
        bool isTunnel;
        bool isCandidate;
        bool isEnable;
    };

    static_assert(sizeof(SpawnPoint) == 0x24);

    /// Ignores the transparent ocean floor walls and everything the wrapped filter ignores.
    class TransparentWallFilter : public al::CollisionPartsFilterBase {
    public:
        /** @param pFilter Filter that decides about every other collision. */
        TransparentWallFilter(al::CollisionPartsFilterBase* pFilter) : mFilter(pFilter) {}

        /**
         * @param rParts Collision to check.
         * @return Whether Plessie passes through the collision.
         */
        bool isInvalidParts(const al::CollisionParts& rParts) const override {
            if (al::isEqualSubString(rParts.getConnectedHost()->getName(),
                                     "TransparentWallOceanFloor")) {
                return true;
            }

            return mFilter->isInvalidParts(rParts);
        }

    private:
        al::CollisionPartsFilterBase* mFilter;
    };

    /// Ignores the floors Plessie must not stand on.
    class TriangleFloorFilter : public al::TriangleFilterBase {
    public:
        /**
         * @param rTriangle Triangle to check.
         * @return Whether Plessie passes through the triangle.
         */
        bool isInvalidTriangle(const al::Triangle& rTriangle) const override {
            return al::isFloorCode("PlessieIgnore", rTriangle) ||
                   al::isMaterialCode("OceanFloor", rTriangle);
        }
    };

    /// Which game window guide is currently shown.
    enum GuideState : s32 {
        GuideState_RideGuide = 0,
        GuideState_None = 1,
        GuideState_DismountGuide = 2,
        GuideState_Failed = 3,
        GuideState_AreaRideGuide = 4,
    };

    RaidonSurf(const char* pName);

    bool addSpawnPoint(const al::ActorInitInfo& rInfo);
    void startFollowCamera();
    void updateFollowCamera();
    bool isOnDiveJump();
    void setForceRequestDistance(bool isForce);
    void updateFollowDiveCamera();
    void stopFollowCamera();
    void setPlessieMode(bool isPlessie);
    void turnOnWaterCameraDistance();
    void turnOffWaterCameraDistance();
    void updateCameraAngleFriction(bool isFriction);
    bool isWaiting();
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void updateSpawns(bool isTunnel);
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool isGetOffNerve();
    void playHitReactionHitEffect(const al::LiveActor* pActor, const char* pName,
                                  const al::HitSensor* pOther, const al::HitSensor* pSelf);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void dieFromDamage(bool isInWater);
    bool isUnderwater();
    void endNpcPuppetBindAll(NpcPuppetBindEndType type);
    bool trySetReactionNerve(const al::SensorMsg* pMsg, s32 step);
    bool isMsgNpcAttackerHitReaction(const al::SensorMsg* pMsg);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    void updateCollider() override;
    bool isDeadState();
    void control() override;
    bool checkIsMaterial(const char* pName);
    bool isInkWall();
    GuideState showGameWindow(bool isShow);
    void forcePlayerOff();
    void calcAnim() override;
    void setPuppetQT();
    void setNpcPuppetQT();
    bool shouldRespawn(bool isCheckCamera);
    s32 getClosestSpawnIndex(sead::Vector3f pos, bool isCheckCamera) const;
    bool isReadyToSpawn(bool isCheckCamera);
    bool moveToClosestSpawnPosition(bool isCheckCamera);

    /**
     * Sets whether the binding of the player is canceled (set by the scene during demos).
     * @param isCanceled Whether the binding is canceled.
     */
    void setBindCanceled(bool isCanceled) { mIsBindCanceled = isCanceled; }
    void exeFirstSeenDemo();
    void forceSpawn(bool isForce);
    void exeWait();
    void updateWaterPosVelocity(f32 accel, f32 brake, f32 extraAccel);
    void whileWait(bool isCheckCamera);
    void exeGetOn();
    void exeStart();
    void exeRide();
    void checkBgmChange();
    void exeAbyss();
    void exeGoal();
    void startSharedGetOff(bool isInAir, bool isDie);
    sead::Vector3f getInputDirection();
    bool updateSharedGetOff();
    void exeGetOff();
    void exeGetOnInWater();
    void exeGetOffOnWater();
    void exeGetOffInAir();
    void exeDelayWaitInWater();
    void exeWaitInWater();
    void exeWaitInWaterReaction();
    void exeEnd();
    void exeCollectItem();
    void exeSpawn();
    void reactivate();
    void exeDeSpawn();
    void deactivate();
    void exeFall();
    void exeLand();
    void exeRebind();
    void exeDie();
    void exeDeath();
    void endBind(const PlayerBindEndParam* pParam);
    bool isDespawnState();
    f32 getPuppetInputStickX();
    void updatePuppetInput() override;
    void updateHandleAndAccel() override;
    void updateGroundUpVec() override;
    void updateOnGround() override;
    void updateMatrialCode() override;
    const char* getMaterialCode() override;
    void startPuppetActionAll(const char* pActionName) override;
    void setPuppetInputBlendAnimWeight() override;
    void setInputBlendAnimWeight() override;
    bool isAllGetOffPlayer() const override;
    void startPuppetSe(const char* pName) override;
    void updateStart() override;
    void updateRide() override;
    bool isHittingTorpedoSpikeWall() const;
    void doJump(bool isPerfect);
    void doDive(bool isDash);
    void doJumpPanel(bool isStrong);
    IUsePlayerPuppet* getPuppet() const;
    void enableSpawns();
    bool isFloating();
    void clearGroundCount() override;
    f32 getPuppetInputStickY() override;
    bool isInWater() const override;
    bool isOnGroundOrWaterRaidon() const override;
    bool isInDiveArea() const;
    void toggleGameWindow(bool isShow);
    bool isGetOnNerve();
    void increaseDiveSpeed(s32 speed);
    void toggleSpawnDebugDraw();
    void plessieChaseHitBells(al::HitSensor* pSensor);
    bool forceJump();
    void getClosestSpawnPosFront(sead::Vector3f pos, sead::Vector3f* pSpawnPos,
                                 sead::Vector3f* pSpawnFront);

    /** @return Whether Plessie touched the ground within the last few frames. */
    bool isOnGroundRaidon() const override { return mGroundCount > 0; }

    /** @return Center of the screen blur used while dashing. */
    const sead::Vector3f& getDashBlurCenter() const override { return mDashBlurCenter; }

    /** @return Front direction Plessie swims to. */
    const sead::Vector3f& getBaseFrontDir() const override { return mBaseFrontDir; }

    /** @return Up direction of the ground below Plessie. */
    const sead::Vector3f& getGroundUpVec() const override { return mGroundUpVec; }

    /** @return Rotation Plessie was placed with. */
    const sead::Quatf& getBaseQuat() const override { return mBaseQuat; }

    /** @return Plessie has no goal position. */
    const sead::Vector3f& getGoalPosition() const override { return sead::Vector3f::zero; }

    /** @return Plessie has no goal position. */
    bool isEnableGoalPosition() const override { return false; }

    /** @return Averaged steering input of the riders. */
    f32 getHandle() const override { return mHandle; }

    /** @return Averaged acceleration input of the riders. */
    f32 getAccel() const override { return mAccel; }

    /** @return Current Y rotation offset in degrees. */
    f32 getRotateY() const override { return mRotateY; }

    /** @return Whether the stage BGM must be kept while riding. */
    bool isNotChangeBgm() const override { return mIsNotChangeBgm; }

    /** @return Whether Plessie is standing on ground she slides down. */
    bool isOnSlideGround() const { return mIsOnSlideGround; }

    /** @return Height of the water surface Plessie is swimming in. */
    s32 getWaterSurfaceY() const { return mWaterSurfaceY; }

    /** @return Depth below which Plessie starts surfacing again. */
    f32 getDiveDepthLimit() const { return mDiveDepthLimit; }

    /** @return Current depth below the water surface. */
    f32 getDiveDepth() const { return mDiveDepth; }

private:
    inline CameraPoserFollowLimit* tryGetFollowCameraPoser() const;

    inline void tryEndNpcPuppetBindNearPlayer();

    RaidonSurfWaitState* mWaitState = nullptr;                    // 0x178
    RaidonSurfStartState* mStartState = nullptr;                  // 0x180
    RaidonSurfAnimState* mAnimState = nullptr;                    // 0x188
    void* _190 = nullptr;
    RaidonSurfWaitState* mFirstSeenState = nullptr;               // 0x198
    RaidonPuppeteer* mPuppeteers = nullptr;                       // 0x1a0
    ActorStateSupportStroke* mStateSupportStroke;                 // 0x1a8
    s32 mPuppeteerNumMax = 0;                                     // 0x1b0
    s32 mPuppeteerNum = 0;                                        // 0x1b4
    al::HitSensor* mFirstPlayerSensor = nullptr;                  // 0x1b8
    al::ComboCounter* mTrampleComboCounter;                       // 0x1c0
    al::ComboCounter* mInvincibleComboCounter;                    // 0x1c8
    sead::Matrix34f mScreenWetMtx = sead::Matrix34f::ident;       // 0x1d0
    const char* mMaterialCodeName = nullptr;                      // 0x200
    const char* mFloorCodeName = nullptr;                         // 0x208
    const char* mWallCodeName = nullptr;                          // 0x210
    sead::Quatf mBaseQuat = sead::Quatf::unit;                    // 0x218
    sead::Vector3f mBaseTrans = sead::Vector3f::zero;             // 0x228
    sead::Vector3f mBaseFrontDir = sead::Vector3f::ez;            // 0x234
    sead::Vector3f mBaseSideDir = sead::Vector3f::ex;             // 0x240
    sead::Vector3f mGroundUpVec = sead::Vector3f::ey;             // 0x24c
    sead::Vector3f mDashBlurCenter = sead::Vector3f::zero;        // 0x258
    sead::Vector3f _264 = sead::Vector3f::zero;
    f32 mHandle = 0.0f;                                           // 0x270
    f32 mRotateY = 0.0f;                                          // 0x274
    f32 mAccel = 0.0f;                                            // 0x278
    f32 mDashAccel = 0.0f;                                        // 0x27c
    f32 mDiveSpeed = 30.0f;                                       // 0x280
    s32 mGroundGraceCount = 3;                                    // 0x284
    s32 mGroundCount = 0;                                         // 0x288
    s32 mLife = 3;                                                // 0x28c
    s32 mDashTimer = 0;                                           // 0x290
    s32 mHitTimer = 0;                                            // 0x294
    s32 mMultiJumpTimer = 0;                                      // 0x298
    bool mIsOnInk = false;                                        // 0x29c
    bool mIsOnSlideGround = false;                                // 0x29d
    s32 mSlideGroundTimer = 0;                                    // 0x2a0
    bool mIsInWater = false;                                      // 0x2a4
    bool mIsNotChangeBgm = true;                                  // 0x2a5
    al::AudioGeneralPurposeAreaChecker* mFallAreaChecker = nullptr;  // 0x2a8
    const al::PlayerHolder* mPlayerHolder = nullptr;              // 0x2b0
    DummyCameraTarget* mCameraTarget;                             // 0x2b8
    sead::ObjArray<SpawnPoint> mSpawnPoints;                      // 0x2c0
    SpawnPoint* mStartingPoint = nullptr;                         // 0x2e0
    s32 mRespawnWaitStep = 0;                                     // 0x2e8
    bool mIsRespawnWaiting = false;                               // 0x2ec
    bool mIsCameraLimitAngle = false;                             // 0x2ed
    bool mIsChaseSpecialCamera = false;                           // 0x2ee
    f32 mCameraDistance = 170.0f;                                 // 0x2f0
    s32 mCameraDistanceStep;                                      // 0x2f4
    s32 mChaseSpecialCameraTimer = 0;                             // 0x2f8
    s32 mSpawnIndex = -1;                                         // 0x2fc
    bool mIsRespawnRequested = false;                             // 0x300
    bool mIsRequestPlessieMode = false;                           // 0x301
    bool mIsAnyPuppetJump = false;                                // 0x302
    bool mIsCameraBetweenLegs;                                    // 0x303
    f32 mHiDegreeLimit;                                           // 0x304
    bool mIsNoGuideWindow = false;                                // 0x308
    bool mIsShowGuideWindow = true;                               // 0x309
    GuideState mGuideState = GuideState_None;                     // 0x30c
    s32 mRespawnWaitTime = -1;                                    // 0x310
    sead::Vector3f mPrevVelocity = sead::Vector3f::zero;          // 0x314
    f32 mTurnSpeed = 0.0f;                                        // 0x320
    al::AreaObj* mCameraArea = nullptr;                           // 0x328
    bool mIsFirstRide = false;                                    // 0x330
    bool mIsPlessieChase = false;                                 // 0x331
    bool mIsFastBgm = false;                                      // 0x332
    u32 mFastBgmCount = 0;                                        // 0x334
    u32 mSlowBgmCount = 0;                                        // 0x338
    u32 mOnGroundBgmStep = 0;                                     // 0x33c
    u32 mInWaterBgmStep = 0;                                      // 0x340
    f32 mFastBgmSpeed = 18.0f;                                    // 0x344
    f32 mSlowBgmSpeed = 55.0f;                                    // 0x348
    bool mIsInDiveArea = false;                                   // 0x34c
    bool mIsInCameraHeightLimitArea = false;                      // 0x34d
    f32 mColliderOffsetY = 0.0f;                                  // 0x350
    s32 mWaterSurfaceY = 0;                                       // 0x354
    f32 mDiveDepthLimit = 0.0f;                                   // 0x358
    f32 mDiveDepth = 0.0f;                                        // 0x35c
    f32 mCameraStopStep = 0.0f;                                   // 0x360
    s32 mStickOffStep = 0;                                        // 0x364
    s32 mJumpInterval = 0;                                        // 0x368
    al::PadRumbleKeeper* mPadRumbleKeeper = nullptr;              // 0x370
    bool mIsInGuideArea = false;                                  // 0x378
    s32 mHitReactionTimer = 0;                                    // 0x37c
    s32 mBindSensorTimer = 0;                                     // 0x380
    SePlayObj* mSoundActor = nullptr;                             // 0x388
    bool mIsSpawnDebugDraw = false;                               // 0x390
    f32 mSpawnCameraDot = 0.83f;                                  // 0x394
    f32 mRespawnCameraDot = 0.83f;                                // 0x398
    al::CollisionPartsFilterBase* mPlayerOnlyFilter = nullptr;    // 0x3a0
    TransparentWallFilter* mTransparentWallFilter = nullptr;      // 0x3a8
    al::CollisionPartsFilterBase* mColliderFilter = nullptr;      // 0x3b0
    TriangleFloorFilter* mTriangleFilter = nullptr;               // 0x3b8
    bool mIsBindCanceled = false;                                 // 0x3c0
    sead::PtrArray<IUseNpcPuppet> mNpcPuppets;                    // 0x3c8
    bool mIsNpcPuppetBinding = false;                             // 0x3d8
    al::LiveActor* mRainActor = nullptr;                          // 0x3e0
    s32 mCollisionType = 0;                                       // 0x3e8
    f32 mBindSensorRadius = 0.0f;                                 // 0x3ec
    sead::Vector3f mBindSensorOffset;                             // 0x3f0
};

static_assert(sizeof(RaidonSurf) == 0x400);
