#pragma once

#include <math/seadVector.h>

#include "Player/PlayerDef.hpp"

namespace al {
    class IUseAreaObj;
}

class IUsePlayerAnimator;
class IUsePlayerCollision;
class IUsePlayerInput;
class IUseWaterFlowAccess;
class PlayerConstParam;
class PlayerFigureDirector;
struct PlayerProperty;

/// Movement helpers shared by the player actions.
namespace PlayerActionFunc {
    void calcSideDir(sead::Vector3f* pOut, const PlayerProperty* pProperty);
    f32 brake(f32 speed, u32 frame, f32 maxSpeed);
    f32 accel(f32 speed, f32 maxSpeed, f32 accel);
    void applyGravity(PlayerProperty* pProperty, const IUsePlayerCollision* pCollision, f32 gravity,
                      f32 fallSpeedMax);
    void removeVelocityParallelToUpVec(PlayerProperty* pProperty);
    void forceFaceTo(PlayerProperty* pProperty, const sead::Vector3f& rDir);
    bool isUpperVelocity(const PlayerProperty* pProperty);
    void vertAndNormVec(sead::Vector3f* pVec, const sead::Vector3f& rUp,
                        const sead::Vector3f& rDefault);
    bool isOppositeSide(const sead::Vector3f& rA, const sead::Vector3f& rB);
    bool isOppositeInput(const IUsePlayerInput* pInput, const PlayerProperty* pProperty,
                         const sead::Vector3f& rDir);
    void faceToHorizontalVelocity(PlayerProperty* pProperty);
    void faceToInputDirection(PlayerProperty* pProperty, const IUsePlayerInput* pInput);
    void setupJump(PlayerProperty* pProperty, IUsePlayerCollision* pCollision, f32 jumpPower,
                   bool isResetVelocity);
    f32 calcJumpPow(const sead::Vector3f& rVelocity, const sead::Vector3f& rUp, f32 speedRate,
                    f32 speedMin, f32 speedRange, f32 highRate, f32 highPower, f32 lowPower);
    f32 converge(f32 value, f32 target, f32 step);
    void calcWaterFlowField(const al::IUseAreaObj* pAreaUser, sead::Vector3f* pFlow,
                            const IUseWaterFlowAccess* pAccess, const sead::Vector3f& rPos,
                            f32 keepRate);
    f32 cutOff(f32 speed, f32 threshold);
    void snapGroundOrSolveAir(IUsePlayerCollision* pCollision, const PlayerProperty* pProperty);
    void addAndAdjustVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rDir, f32 accel,
                              f32 speedMax);
    f32 calcSwimVerticalVelocity(f32 speed, const IUsePlayerInput* pInput, f32 gravity,
                                 const PlayerConstParam* pParam);
    f32 calcSwimRiseVelocity(f32 speed, const PlayerConstParam* pParam);
    f32 calcSwimFallVelocity(f32 speed, f32 gravity, const PlayerConstParam* pParam);
    f32 calcBobbleVelocity(f32 speed, f32 gravity, const PlayerConstParam* pParam, s32 frame,
                           s32 unused, EPlayerChara chara);
    void calcSwimWalkVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rDir,
                              const IUsePlayerInput* pInput, const PlayerConstParam* pParam);
    void decayVerticalVec(sead::Vector3f* pVec, const sead::Vector3f& rUp, f32 rate);
    void calcSwimHorizontalVelocity(sead::Vector3f* pVelocity, const IUsePlayerInput* pInput,
                                    const PlayerProperty* pProperty,
                                    const IUsePlayerCollision* pCollision, bool isHighSpeed,
                                    const PlayerConstParam* pParam, bool isNoSink);
    void verticalizeVelocity(sead::Vector3f* pVelocity, const sead::Vector3f& rUp);
    void calcSwimFrontVec(PlayerProperty* pProperty, const IUsePlayerInput* pInput, f32 degreeMax);
    bool isRaccoonDog(const PlayerFigureDirector* pFigureDirector);
    bool isClimb(const PlayerFigureDirector* pFigureDirector);
    bool isClimbGiga(const PlayerFigureDirector* pFigureDirector);
    void scaleVecOfDir(sead::Vector3f* pVec, const sead::Vector3f& rDir, f32 scale);
    void controlDirection(PlayerProperty* pProperty, const IUsePlayerInput* pInput, f32 rate);
    bool setupSquatStartAnim(IUsePlayerAnimator* pAnimator, const char* pAnimName);
    void controlDirectionalVelocity(sead::Vector3f* pVelocity, const PlayerProperty* pProperty,
                                    const IUsePlayerInput* pInput, f32 brakeRate, f32 speedMin,
                                    f32 sideRate);
    void controlSideVelocity(sead::Vector3f* pVelocity, const PlayerProperty* pProperty,
                             const IUsePlayerInput* pInput, f32 rate);
    void updateSquatVelocity(PlayerProperty* pProperty, const IUsePlayerInput* pInput, f32 speed,
                             f32 turnRate, f32 decayRate);
    f32 calcStickPow(f32 stick);
    bool checkMapCode(const IUsePlayerCollision* pCollision, const char* pCode);
    bool checkMaterialCode(const IUsePlayerCollision* pCollision, const char* pCode);
    bool isMapCodeSkate(const IUsePlayerCollision* pCollision);
    f32 calcCenterRate(const PlayerFigureDirector* pFigureDirector);
    void turnHeadUpForce(PlayerProperty* pProperty);
    void calcDownward(sead::Vector3f* pOut, const PlayerProperty* pProperty,
                      const sead::Vector3f& rNormal);
}  // namespace PlayerActionFunc
