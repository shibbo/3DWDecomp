#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/Collision/CollisionMultiSphereBase.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerCollisionPartsArray.hpp"
#include "Player/IUsePlayerSnapWallInfo.hpp"

namespace al {
class CollisionParts;
class LiveActor;
}  // namespace al

class IUseCollisionPartsMtx;
class IUsePlayerAnimator;
class IUsePlayerCollisionCheckArrow;
class IUsePlayerCollisionCheckSphere;
class IUsePlayerCollisionCheckSphereMove;
class IUsePlayerInput;
class PlayerConstParam;
class PlayerFigureDirector;
class PlayerGroundFollower;
class PlayerSnapWallInfo;
struct PlayerProperty;

/// What the collider recorded about one side (floor, ceiling, a wall) this frame: the deepest hit.
class PlayerCollisionInfoBase {
public:
    virtual void clear() = 0;
    virtual void record(const sead::Vector3f& rNormal, f32 depth, const al::CollisionParts* pParts,
                        const char* pMapCode, const char* pWallCode,
                        const char* pMaterialCode) = 0;
    virtual bool isValid() const = 0;
    virtual const sead::Vector3f& getNormal() const = 0;
    virtual f32 getDepth() const = 0;
    virtual const al::CollisionParts* getCollisionParts() const = 0;
    virtual const char* getMapCodeName() const = 0;
    virtual const char* getWallCodeName() const = 0;
    virtual const char* getMaterialCodeName() const = 0;
};

/// Moves the player through the map: casts the legs and spheres, pushes the player out of
/// walls and floors and remembers what it touched.
class PlayerCollider : public IUsePlayerCollision,
                       public IUsePlayerCollisionPartsArray,
                       public IUsePlayerSnapWallInfo {
public:
    using Sphere = al::CollisionMultiSphereBase::Sphere;

    /// The extra spheres checked while an animation (e.g. climbing) is playing.
    struct ExSphereInfo {
        s32 mSphereNum;         // 0x0
        Sphere* mSpheres;       // 0x8, [0] bounds all the others
        const char* mAnimName;  // 0x10
    };

    PlayerCollider(al::LiveActor* pActor, IUseCollisionPartsMtx* pCollisionPartsMtx,
                   const PlayerConstParam* pConstParam, const IUsePlayerAnimator* pAnimator,
                   bool isLongStep);

    void setProperty(PlayerProperty* pProperty);
    void setFigureDirector(const PlayerFigureDirector* pFigureDirector);
    void clear() override;
    void clearCollisionInfo();
    void moveSimple(bool isClear) override;
    void updateExSphere();
    void clearExPush();
    void solveAir() override;
    void applyVelocity(const sead::Vector3f& rVel, f32 legOffset, bool isSkipLeg);
    void solveAirNoFloor() override;
    void snapGround() override;
    bool isForceFollowing() const;
    void snapWall(bool isBack) override;
    void calcFollowVec(sead::Vector3f* pOut, const sead::Vector3f& rPos,
                       const al::CollisionParts* pParts);
    void checkSnapWall(const sead::Vector3f& rPos, const sead::Vector3f& rDir, bool isBack);
    void push(const sead::Vector3f& rPush) override;
    void calcMinMax(sead::Vector3f* pMin, sead::Vector3f* pMax, const sead::Vector3f& rVec) const;
    void clearPush() override;
    void arrangeJumpFollowVel() override;
    void clearJumpFollowVel() override;
    bool isOnFloor() const override;
    bool isOnFrontWall() const override;
    bool isOnBackWall() const override;
    bool isOnCeiling() const override;
    bool isOnRightWall() const override;
    bool isOnLeftWall() const override;
    bool isHeadOnFrontWall() const override;
    bool isOnAnyWall() const override;
    void getFloorInfo(Info* pInfo) const override;
    void getFrontWallInfo(Info* pInfo) const override;
    void getBackWallInfo(Info* pInfo) const override;
    void getCeilingInfo(Info* pInfo) const override;
    void getRightWallInfo(Info* pInfo) const override;
    void getLeftWallInfo(Info* pInfo) const override;
    void shrinkBody() override;
    void growBody() override;
    void cutVelocity(u32 flags) override;
    bool isSnapWallExist() const override;
    const sead::Vector3f& getSnapWallLastNormal() const override;
    const sead::Vector3f& getSnapWallNormal() const override;
    const sead::Vector3f& getSnapWallPos() const override;
    void calcBodyPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans, bool isAddHover) const;
    void calcHeadPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans, bool isAddHover) const;
    f32 calcSquatRate() const;
    f32 calcLegLength() const;
    void applyVelocityCore(sead::Vector3f& rTrans, const sead::Vector3f& rVel, bool isUnused,
                           const sead::Vector3f& rLegDir, f32 legOffset,
                           const sead::Vector3f& rSide, bool isSkipLeg, bool isSwimSlow);
    void keepOutRestrictedArea();
    void calcLegPos(sead::Vector3f* pOut, const sead::Vector3f& rTrans) const;
    bool checkLegArrow(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                       sead::Vector3f* pSidePush, sead::Vector3f* pMovingSidePush,
                       const sead::Vector3f& rStart, const sead::Vector3f& rDir, f32 legOffset);
    void checkBodySphere(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                         const sead::Vector3f& rPos, const sead::Vector3f& rSide, bool isCheckFloor,
                         bool isCheckFloorBack);
    void checkHeadSphere(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                         sead::Vector3f* pSidePush, sead::Vector3f* pMovingSidePush,
                         const sead::Vector3f& rPos, const sead::Vector3f& rSide);
    void checkExSphere(sead::Vector3f* pPush, sead::Vector3f* pMovingPush,
                       const sead::Vector3f& rTrans, const sead::Vector3f& rSide);
    u32 findNearestLegCollision(const sead::Vector3f& rPos) const;
    bool isGround(const sead::Vector3f& rNormal) const;
    void recordFloorCollisionInfo(u32 index, f32 depth);
    void registerPartsArray(CollisionPartsArray* pArray, const al::CollisionParts* pParts);
    bool isWall(const sead::Vector3f& rNormal) const;
    void calcWallInfo(sead::Vector3f& rMin, sead::Vector3f& rMax, sead::Vector3f& rMovingMin,
                      sead::Vector3f& rMovingMax, u32 index, const sead::Vector3f& rSide);
    bool isWallForHead(const sead::Vector3f& rNormal) const;
    void recordWallCollisionInfo(u32 index, const sead::Vector3f& rSide);
    bool isCeiling(const sead::Vector3f& rNormal) const;
    void recordCeilingCollisionInfo(u32 index);
    bool isBlockBorder(const sead::Vector3f& rPos, const sead::Vector3f& rNormal) const;
    void recordWallCollisionInfoCommon(const sead::Vector3f& rPos, const sead::Vector3f& rNormal,
                                       f32 depth, const al::CollisionParts* pParts,
                                       const char* pMapCode, const char* pWallCode,
                                       const char* pMaterialCode, const sead::Vector3f& rSide);
    bool isLeftWall(const sead::Vector3f& rNormal, const sead::Vector3f& rSide) const;
    bool isRightWall(const sead::Vector3f& rNormal, const sead::Vector3f& rSide) const;
    bool isCenterOnFloor() const override;
    void setDisableLegCheck(bool isDisable) override;
    void setDisableLegCheckForce(bool isDisable) override;
    const CollisionPartsArray* getFloorPartsArray() const override;
    const CollisionPartsArray* getCeilingPartsArray() const override;
    const CollisionPartsArray* getFrontPartsArray() const override;
    const CollisionPartsArray* getFrontHemispherePartsArray() const override;
    const CollisionPartsArray* getBackHemispherePartsArray() const override;

    const PlayerProperty* getProperty() const { return mProperty; }

    IUsePlayerCollisionCheckArrow* getCheckArrow() const { return mCheckArrow; }

    PlayerCollisionInfoBase* getFloorCollisionInfo() const { return mFloorInfo; }

    CollisionPartsArray* getFloorPartsArrayMutable() const { return mFloorPartsArray; }

    void setCheckSphere(IUsePlayerCollisionCheckSphere* pCheckSphere) {
        mCheckSphere = pCheckSphere;
    }

    void setCheckSphereMove(IUsePlayerCollisionCheckSphereMove* pCheckSphereMove) {
        mCheckSphereMove = pCheckSphereMove;
    }

    void setCheckArrow(IUsePlayerCollisionCheckArrow* pCheckArrow) { mCheckArrow = pCheckArrow; }

    void setInput(const IUsePlayerInput* pInput) { mInput = pInput; }

private:
    al::LiveActor* mActor;                                           // 0x18
    PlayerProperty* mProperty = nullptr;                             // 0x20
    IUsePlayerCollisionCheckSphere* mCheckSphere = nullptr;          // 0x28
    IUsePlayerCollisionCheckSphereMove* mCheckSphereMove = nullptr;  // 0x30
    IUsePlayerCollisionCheckArrow* mCheckArrow = nullptr;            // 0x38
    const IUsePlayerInput* mInput = nullptr;                         // 0x40
    bool mIsShrink = false;                                          // 0x48
    bool mIsDisableLegCheck = false;                                 // 0x49
    bool mIsDisableLegCheckForce = false;                            // 0x4a
    u32 mGrowTimer = 0;                                              // 0x4c
    const IUsePlayerAnimator* mAnimator;                             // 0x50
    ExSphereInfo* mExSphere = nullptr;                               // 0x58
    u32 mExSphereFrame = 0;                                          // 0x60
    u32 mHeadLowerTimer = 0;                                         // 0x64
    u32 mHeadLowerFrame = 0;                                         // 0x68
    bool mIsLongStep;                                                // 0x6c
    PlayerCollisionInfoBase* mFloorInfo;                             // 0x70
    PlayerCollisionInfoBase* mCeilingInfo;                           // 0x78
    PlayerCollisionInfoBase* mFrontWallInfo;                         // 0x80
    PlayerCollisionInfoBase* mBackWallInfo;                          // 0x88
    PlayerCollisionInfoBase* mLeftWallInfo;                          // 0x90
    PlayerCollisionInfoBase* mRightWallInfo;                         // 0x98
    PlayerCollisionInfoBase* mFrontWallAnyInfo;                      // 0xa0
    PlayerGroundFollower* mGroundFollower;                           // 0xa8
    IUseCollisionPartsMtx* mCollisionPartsMtx;                       // 0xb0
    sead::Vector3f mPushMin;                                         // 0xb8
    sead::Vector3f mPushMax;                                         // 0xc4
    sead::Vector3f mJumpFollowVel;                                   // 0xd0
    sead::Vector3f mFollowVel;                                       // 0xdc
    sead::Vector3f mFollowRotate;                                    // 0xe8
    sead::Vector3f mFollowFront;                                     // 0xf4
    CollisionPartsArray* mFloorPartsArray;                           // 0x100
    CollisionPartsArray* mFrontPartsArray;                           // 0x108
    CollisionPartsArray* mCeilingPartsArray;                         // 0x110
    CollisionPartsArray* mFrontHemispherePartsArray;                 // 0x118
    CollisionPartsArray* mBackHemispherePartsArray;                  // 0x120
    PlayerSnapWallInfo* mSnapWallInfo;                               // 0x128
    bool mIsHeadOnFrontWall = false;                                 // 0x130
    bool mIsCenterOnFloor = false;                                   // 0x131
    sead::Vector3f mPushVec;                                         // 0x134
    sead::Vector3f mMovingPushVec;                                   // 0x140
    const PlayerConstParam* mConstParam;                             // 0x150
    sead::Matrix34f mExSphereMtx;                                    // 0x158
    bool mIsCheckSubLeg = true;                                      // 0x188
    bool mIsValidExSphere = true;                                    // 0x189
};

static_assert(sizeof(PlayerCollider) == 0x190);
