#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerInputArranger.hpp"
#include "Player/IUsePlayerJumpButtonControl.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"

namespace al {
    class LiveActor;
}

class IUsePlayerCollision;

/// The player's controller input, read from its pad port.
class PlayerInput : public IUsePlayerInput,
                    public IUsePlayerInputArranger,
                    public IUsePlayerJumpButtonControl {
public:
    PlayerInput(const IUsePlayerKeyConfig* pKeyConfig);

    void setActorForAreaSearch(al::LiveActor* pActor);
    void update(const sead::Matrix34f& rCameraMtx);
    void clearAll();
    bool calcFrontAndSide(sead::Vector3f* pFront, sead::Vector3f* pSide,
                          const sead::Matrix34f& rCameraMtx);
    void snapStick(sead::Vector2f* pStick);
    void snapWideX(sead::Vector2f* pStick);
    void snapNormal(sead::Vector2f* pStick);
    void arrangeRouteDependence(sead::Vector2f* pStick);

    void setCollision(const IUsePlayerCollision* pCollision) { mCollision = pCollision; }

    s32 getPort() const override { return mKeyConfig->getPort(); }

    bool isStickOn() const override { return mIsStickOn; }

    const sead::Vector3f& getMoveVec() const override { return mMoveVec; }

    const sead::Vector3f& getMoveVecNoArrange() const override { return mMoveVecNoArrange; }

    f32 getStickX() const override { return mStick.x; }

    f32 getStickY() const override { return mStick.y; }

    bool isJumpTrigOn() const override;
    bool isJumpButtonTrigOn() const override;

    s32 getFrameFromLastJumpTrig() const override { return mFrameFromLastJumpTrig; }

    bool isJumpButtonOn() const override { return mIsJumpButtonOn; }

    bool isDashTrigOn() const override { return mIsDashTrigOn; }

    bool isDashButtonOn() const override { return mIsDashButtonOn; }

    bool isDashButtonReleased() const override { return mIsDashButtonReleased; }

    bool isSquatTrigOn() const override { return mIsSquatTrigOn; }

    bool isSquatButtonOn() const override { return mIsSquatButtonOn; }

    bool isHipDropTrigOn() const override { return mIsHipDropTrigOn; }

    bool isHipDropButtonOn() const override { return mIsHipDropButtonOn; }

    bool isFireBallTrigOn() const override { return mIsFireBallTrigOn; }

    bool isTailAttackTrigOn() const override { return mIsTailAttackTrigOn; }

    bool isPrecedingSwimPaddleTrigOn() const override;

    bool isSwimPaddleTrigOn() const override { return mIsSwimPaddleTrigOn; }

    bool isSwimPaddleButtonOn() const override { return mIsSwimPaddleButtonOn; }

    bool isStoneStatueTrigOn() const override { return mIsStoneStatueTrigOn; }

    bool isStoneStatueSustainButtonOn() const override { return mIsStoneStatueSustainButtonOn; }

    bool isRollingTrigOn() const override;

    bool isRollingButtonOn() const override { return mIsRollingButtonOn; }

    bool isBubbleTrigOn() const override { return mIsBubbleTrigOn; }

    bool isClimbAttackTrigOn() const override { return mIsClimbAttackTrigOn; }

    bool isClimbAttackButtonOn() const override { return mIsClimbAttackButtonOn; }

    bool isHoldButtonOn() const override { return mIsHoldButtonOn; }

    bool isHoldShakeOn() const override { return mIsHoldShakeOn; }

    bool isHoldTrigOn() const override { return mIsHoldTrigOn; }

    bool isReleaseTrigOn() const override { return mIsReleaseTrigOn; }

    bool isSpinAttackTrigOn() const override { return mIsSpinAttackTrigOn; }

    void resetPrecedingJump() override;
    void invalidateFrame(u32 frame) override;

    void disableJumpButton() override { mIsJumpButtonEnabled = false; }

    void enableJumpButton() override { mIsJumpButtonEnabled = true; }

private:
    bool mIsStickOn = false;                                 // 0x18
    sead::Vector3f mMoveVec = {0.0f, 0.0f, 0.0f};            // 0x1c
    sead::Vector3f mMoveVecNoArrange = {0.0f, 0.0f, 0.0f};   // 0x28
    sead::Vector2f mStick;                                   // 0x34
    bool mIsJumpButtonOn = false;                            // 0x3c
    u32 mFrameFromLastJumpTrig = 10;                         // 0x40
    bool mIsJumpButtonEnabled = true;                        // 0x44
    bool mIsDashTrigOn = false;                              // 0x45
    bool mIsDashButtonOn = false;                            // 0x46
    bool mIsDashButtonReleased = false;                      // 0x47
    bool mIsSquatTrigOn = false;                             // 0x48
    bool _49;                                                // 0x49
    bool mIsSquatButtonOn = false;                           // 0x4a
    bool mIsHipDropTrigOn = false;                           // 0x4b
    bool mIsHipDropButtonOn = false;                         // 0x4c
    bool mIsFireBallTrigOn = false;                          // 0x4d
    bool mIsTailAttackTrigOn = false;                        // 0x4e
    u32 mFrameFromLastSwimPaddleTrig = 5;                    // 0x50
    bool mIsSwimPaddleTrigOn = false;                        // 0x54
    bool mIsSwimPaddleButtonOn = false;                      // 0x55
    bool mIsStoneStatueTrigOn = false;                       // 0x56
    bool mIsStoneStatueSustainButtonOn = false;              // 0x57
    bool mIsRollingButtonOn = false;                         // 0x58
    u32 mFrameFromLastRollingTrig = 5;                       // 0x5c
    bool mIsBubbleTrigOn = false;                            // 0x60
    bool mIsClimbAttackTrigOn = false;                       // 0x61
    bool mIsClimbAttackButtonOn = false;                     // 0x62
    bool mIsHoldButtonOn = false;                            // 0x63
    bool mIsHoldShakeOn = false;                             // 0x64
    bool mIsHoldTrigOn = false;                              // 0x65
    bool mIsReleaseTrigOn = false;                           // 0x66
    bool mIsSpinAttackTrigOn = false;                        // 0x67
    bool mIsControlOff = false;                              // 0x68
    f32 mRouteDependenceX = 0.0f;                            // 0x6c
    f32 mRouteDependenceY = 0.0f;                            // 0x70
    u32 mInvalidFrame = 0;                                   // 0x74
    al::LiveActor* mActorForAreaSearch = nullptr;            // 0x78
    const IUsePlayerCollision* mCollision = nullptr;         // 0x80
    f32 mAlongWallRate = 0.0f;                               // 0x88
    s32 mAlongWallKeepFrame = 0;                             // 0x8c
    bool mIsWaitFirstLanding = true;                         // 0x90
    const IUsePlayerKeyConfig* mKeyConfig;                   // 0x98
    bool mIsSingleMode = false;                              // 0xa0
};
