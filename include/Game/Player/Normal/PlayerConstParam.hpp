#pragma once

#include <basis/seadTypes.h>

namespace al {
    class ByamlIter;
}

/// Every tuning value as X(Type, Name), in getter (vtable) order.
#define PLAYER_CONST_PARAM_LIST(X)                                            \
    X(f32, Gravity)                                                           \
    X(f32, CenterHeight)                                                      \
    X(f32, BodyRadius)                                                        \
    X(f32, CollectInfoRadiusAddition)                                         \
    X(f32, SnapGroundMaxLength)                                               \
    X(f32, SnapWallMaxLength)                                                 \
    X(f32, StickRoundThreshold)                                               \
    X(f32, HeightCheckLength)                                                 \
    X(f32, CutVelLimit)                                                       \
    X(f32, CutVelRate)                                                        \
    X(s32, ThrowInvalidationFrames)                                           \
    X(f32, Tall)                                                              \
    X(f32, ChestRadius)                                                       \
    X(f32, DashCheckRadius)                                                   \
    X(f32, ShadowCheckLength)                                                 \
    X(f32, ShadowLengthMax)                                                   \
    X(s32, PivotFrame)                                                        \
    X(f32, PivotDegree)                                                       \
    X(f32, NormalMaxSpeed)                                                    \
    X(f32, DashMaxSpeed)                                                      \
    X(f32, SuperDashSpeed)                                                    \
    X(s32, SuperDashTimer)                                                    \
    X(s32, SuperDashTimerMini)                                                \
    X(s32, SuperDashTimerFire)                                                \
    X(s32, SuperDashTimerClimb)                                               \
    X(s32, SuperDashTimerRaccoonDog)                                          \
    X(s32, SuperDashTimerBoomerang)                                           \
    X(s32, SuperDashTimerRaccoonDogWhite)                                     \
    X(f32, SuperDashStartAnimRate)                                            \
    X(s32, SuperDashStartAnimFrame)                                           \
    X(s32, BrakeFrame)                                                        \
    X(s32, DashBrakeFrame)                                                    \
    X(s32, StickOnBrakeFrame)                                                 \
    X(s32, BrakeFrameOnIce)                                                   \
    X(s32, AccelFrame)                                                        \
    X(s32, DashAccelFrame)                                                    \
    X(f32, RoundLimitDegreeMax)                                               \
    X(f32, RoundLimitDegreeMin)                                               \
    X(f32, RunAnimRateMax)                                                    \
    X(f32, GiantRunAnimRateMax)                                               \
    X(f32, ShortAnimRateEff)                                                  \
    X(s32, DashStartFrame)                                                    \
    X(s32, ModifiedDashStartFrame)                                            \
    X(s32, DashStartBlendFrame)                                               \
    X(s32, DashInputSuccessFrame)                                             \
    X(f32, DownHillAccelStartDegree)                                          \
    X(f32, DownHillAccelEndDegree)                                            \
    X(f32, DownHillAccelAddRate)                                              \
    X(f32, DashPanelSpeed)                                                    \
    X(f32, ModifiedDashPanelSpeed)                                            \
    X(f32, DashPanelOverRate)                                                 \
    X(s32, DashPanelTimer)                                                    \
    X(f32, FlingPoleSpeed)                                                    \
    X(s32, GroundOffFrame)                                                    \
    X(s32, ClimbToGroundMoveFrame)                                            \
    X(f32, TiltMaxDegree)                                                     \
    X(f32, TiltBlendRate)                                                     \
    X(f32, TiltMaxFrontAngle)                                                 \
    X(f32, HoldingTiltMaxFrontAngle)                                          \
    X(f32, TiltStartSpeed)                                                    \
    X(f32, TiltEndSpeed)                                                      \
    X(f32, ClimbRunAnimRateEff)                                               \
    X(f32, PanelDashAnimRate)                                                 \
    X(f32, ModifiedPanelDashAnimRate)                                         \
    X(f32, SlopeMaxSpeedScale)                                                \
    X(f32, MaxSpeedScale)                                                     \
    X(f32, DashSignAnimRate)                                                  \
    X(f32, DashSignMaxLoop)                                                   \
    X(f32, DashSignMaxSpeed)                                                  \
    X(s32, DashSignAnimFrameMax)                                              \
    X(f32, JumpDirRotLimit)                                                   \
    X(f32, JumpPowLow)                                                        \
    X(f32, JumpPow)                                                           \
    X(s32, JumpPowCountMax)                                                   \
    X(f32, JumpExtensionGravityRate)                                          \
    X(f32, JumpSideVelRate)                                                   \
    X(f32, JumpFrontBrakeRate)                                                \
    X(f32, JumpRotBlendRate)                                                  \
    X(s32, JumpAccelFrame)                                                    \
    X(s32, ReleaseAccelFrame)                                                 \
    X(s32, JumpAccelAddFrame)                                                 \
    X(f32, JumpCancelBrakeRate)                                               \
    X(f32, JumpCancelMinSpeed)                                                \
    X(s32, ContinuousJumpTimer)                                               \
    X(s32, ContinuousJumpCount)                                               \
    X(f32, FallSpeedMax)                                                      \
    X(f32, DashJumpAddition)                                                  \
    X(f32, PunchReflectPower)                                                 \
    X(s32, GlideInhibitFrameAfterPunch)                                       \
    X(f32, WalkMinSpeedRate)                                                  \
    X(s32, JumpHVelSamplingNum)                                               \
    X(f32, FollowFrontDamper)                                                 \
    X(f32, FollowBackDamper)                                                  \
    X(s32, FlightDurationCount)                                               \
    X(f32, FlightDurationRotBlendRate)                                        \
    X(s32, FlightDurationJumpStartInhibitFrame)                               \
    X(f32, FlightDurationSideDamper)                                          \
    X(f32, RaccoonDogFallSpeedMax)                                            \
    X(f32, RaccoonDogFallGravityRate)                                         \
    X(f32, RaccoonDogFallGravityAdd)                                          \
    X(f32, RaccoonDogFirstFallDamper)                                         \
    X(f32, RaccoonDogRotBlendRate)                                            \
    X(f32, RaccoonDogFallSideDamper)                                          \
    X(f32, TrampleJumpGravity)                                                \
    X(f32, TrampleJump)                                                       \
    X(f32, TrampleJumpSideVelRate)                                            \
    X(f32, TrampleHighJumpGravity)                                            \
    X(f32, TrampleHighJump)                                                   \
    X(f32, TrampleHipDropGravity)                                             \
    X(f32, TrampleHipDropJump)                                                \
    X(f32, RisingTrampleJumpGravity)                                          \
    X(f32, RisingTrampleJump)                                                 \
    X(f32, RisingTrampleHighJumpGravity)                                      \
    X(f32, RisingTrampleHighJump)                                             \
    X(f32, RisingTrampleHVelBrakeRate)                                        \
    X(f32, PunchedJumpGravity)                                                \
    X(f32, PunchedJump)                                                       \
    X(f32, RisingPunchedHVelBrakeRate)                                        \
    X(f32, TossedJumpGravity)                                                 \
    X(f32, TossedJump)                                                        \
    X(f32, TossedHighJumpGravity)                                             \
    X(f32, TossedHighJump)                                                    \
    X(f32, TossedHipDropGravity)                                              \
    X(f32, TossedHipDropJump)                                                 \
    X(f32, RisingTossedJumpGravity)                                           \
    X(f32, RisingTossedJump)                                                  \
    X(f32, RisingTossedHighJumpGravity)                                       \
    X(f32, RisingTossedHighJump)                                              \
    X(f32, RisingTossedHVelBrakeRate)                                         \
    X(f32, SquatBrakeEndSpeed)                                                \
    X(f32, SquatShiftSpeedRate)                                               \
    X(f32, SquatAccelRate)                                                    \
    X(s32, SquatNoBrakeFrame)                                                 \
    X(f32, SquatBrakeRate)                                                    \
    X(f32, SquatBrakeRateOnSkate)                                             \
    X(f32, SquatBrakeSideAccel)                                               \
    X(f32, SquatBrakeSideBrakeRate)                                           \
    X(f32, SquatBrakeSideBrakeRateOnSkate)                                    \
    X(f32, SquatBrakeSideMaxSpeedRate)                                        \
    X(f32, SquatWalkSpeed)                                                    \
    X(f32, SquatWalkFrontVecBlend)                                            \
    X(s32, SquatEnergyAccelFrame)                                             \
    X(f32, SquatJumpGravity)                                                  \
    X(f32, SquatJumpPow)                                                      \
    X(f32, SquatHighJumpPow)                                                  \
    X(f32, SquatJumpBackPow)                                                  \
    X(f32, SquatJumpTramplePow)                                               \
    X(f32, HipDropSpeed)                                                      \
    X(s32, HipDropLandCancelFrame)                                            \
    X(f32, HipDropHeight)                                                     \
    X(s32, HipDropMsgInterval)                                                \
    X(f32, HipDropKnockDownRadiusMin)                                         \
    X(f32, HipDropKnockDownRadiusMax)                                         \
    X(f32, HipDropStartAnimRate)                                              \
    X(s32, HipDropKnockDownFrame)                                             \
    X(f32, HipDropJumpPow)                                                    \
    X(s32, HipDropJumpPowCountMax)                                            \
    X(s32, HipDropJumpPermitBeginFrame)                                       \
    X(s32, HipDropJumpPermitEndFrame)                                         \
    X(f32, WallHeightLowLimit)                                                \
    X(f32, WallGravity)                                                       \
    X(f32, WallMaxSpeed)                                                      \
    X(f32, WallApartFrame)                                                    \
    X(f32, WallSnapDistance)                                                  \
    X(f32, WallSlideMaxSpeed)                                                 \
    X(f32, WallSlideAccel)                                                    \
    X(s32, WallInhibitAfterPunch)                                             \
    X(f32, WallJumpGravity)                                                   \
    X(f32, WallJumpHSpeed)                                                    \
    X(f32, WallJumpPow)                                                       \
    X(s32, WallJumpInvalidateInputFrame)                                      \
    X(s32, WallJumpDirEffectiveFrame)                                         \
    X(f32, WallJumpDirLimit)                                                  \
    X(f32, WallJumpLimitPlay)                                                 \
    X(s32, WallClimbReadyFrame)                                               \
    X(s32, WallClimbReadyWaitFrame)                                           \
    X(s32, WallClimbReadyFromSlopeFrame)                                      \
    X(f32, WallClimbAccel)                                                    \
    X(f32, WallClimbMaxSpeed)                                                 \
    X(f32, WallClimbDashMaxSpeed)                                             \
    X(s32, WallClimbFrame)                                                    \
    X(s32, WallClimbFrame1)                                                   \
    X(s32, WallClimbFrame2)                                                   \
    X(s32, WallClimbDashFrame)                                                \
    X(s32, WallClimbDashFrame1)                                               \
    X(s32, WallClimbDashFrame2)                                               \
    X(s32, WallClimbStopFrame)                                                \
    X(f32, WallClimbBrakeRate)                                                \
    X(s32, WallClimbNoWallFrame)                                              \
    X(f32, WallClimbMaxSideSpeed)                                             \
    X(f32, WallClimbDashMaxSideSpeed)                                         \
    X(f32, WallClimbSideAccel)                                                \
    X(f32, WallClimbRetainThreshold)                                          \
    X(f32, WallClimbInvalidSideMoveDegree)                                    \
    X(f32, WallClimbDashAnimRate)                                             \
    X(f32, WallClimbNormalAnimRate)                                           \
    X(s32, WallClimbJumpPowCountMax)                                          \
    X(f32, WallClimbJumpDashAddition)                                         \
    X(f32, WallClimbJumpPowLow)                                               \
    X(f32, WallClimbJumpPow)                                                  \
    X(f32, WallClimbSlideGravity)                                             \
    X(f32, WallClimbSlideMaxSpeed)                                            \
    X(f32, WallClimbSlideSideAccel)                                           \
    X(f32, WallClimbSlideSideMaxSpeed)                                        \
    X(f32, WallClimbSlideStartBrakeRate)                                      \
    X(s32, ClimbAirStopFrame)                                                 \
    X(f32, ClimbAirStopRotateVelMax)                                          \
    X(f32, LongJumpSuccessSpeed)                                              \
    X(f32, LongJumpSpeedMin)                                                  \
    X(f32, LongJumpBrake)                                                     \
    X(f32, LongJumpSideAccel)                                                 \
    X(s32, LongJumpCancelFrame)                                               \
    X(s32, LongJumpFastSuccessFrame)                                          \
    X(f32, LongJumpFastGravity)                                               \
    X(f32, LongJumpFastJumpPow)                                               \
    X(f32, LongJumpFastSpeed)                                                 \
    X(f32, LongJumpSlowGravity)                                               \
    X(f32, LongJumpSlowJumpPow)                                               \
    X(f32, LongJumpSlowSpeed)                                                 \
    X(s32, DashBrakeCommandFrame)                                             \
    X(s32, DashBrakeActionFrame)                                              \
    X(f32, DashBrakeSpeed)                                                    \
    X(f32, TurnJumpGravity)                                                   \
    X(f32, TurnJumpPow)                                                       \
    X(f32, TurnJumpVelH)                                                      \
    X(f32, TurnJumpBrake)                                                     \
    X(f32, TurnJumpAccel)                                                     \
    X(f32, TurnJumpSideAccel)                                                 \
    X(s32, TurnJumpToFlightDurationFrame)                                     \
    X(f32, WaitRollingMinSpeed)                                               \
    X(s32, WaitRollingNoBrakeFrame)                                           \
    X(f32, WaitRollingBrakeRate)                                              \
    X(f32, WaitRollingSideBrakeRate)                                          \
    X(f32, WaitRollingSideAccel)                                              \
    X(f32, WaitRollingSideMaxSpeed)                                           \
    X(f32, NormalRollingMinSpeed)                                             \
    X(s32, NormalRollingNoBrakeFrame)                                         \
    X(f32, NormalRollingBrakeRate)                                            \
    X(f32, NormalRollingSideBrakeRate)                                        \
    X(f32, NormalRollingSideAccel)                                            \
    X(f32, NormalRollingSideMaxSpeed)                                         \
    X(f32, AirRollingMinSpeed)                                                \
    X(s32, AirRollingNoBrakeFrame)                                            \
    X(f32, AirRollingBrakeRate)                                               \
    X(f32, AirRollingSideBrakeRate)                                           \
    X(f32, AirRollingSideAccel)                                               \
    X(f32, AirRollingSideMaxSpeed)                                            \
    X(f32, AirRollingJumpPow)                                                 \
    X(f32, AirRollingGravity)                                                 \
    X(f32, RollingMinSpeed)                                                   \
    X(s32, RollingNoBrakeFrame)                                               \
    X(f32, RollingBrakeRate)                                                  \
    X(f32, RollingSideBrakeRate)                                              \
    X(f32, RollingSideAccel)                                                  \
    X(f32, RollingSideMaxSpeed)                                               \
    X(f32, RaccoonDogWaitRollingMinSpeed)                                     \
    X(s32, RaccoonDogWaitRollingNoBrakeFrame)                                 \
    X(f32, RaccoonDogWaitRollingBrakeRate)                                    \
    X(f32, RaccoonDogWaitRollingSideBrakeRate)                                \
    X(f32, RaccoonDogWaitRollingSideAccel)                                    \
    X(f32, RaccoonDogWaitRollingSideMaxSpeed)                                 \
    X(f32, RaccoonDogNormalRollingMinSpeed)                                   \
    X(s32, RaccoonDogNormalRollingNoBrakeFrame)                               \
    X(f32, RaccoonDogNormalRollingBrakeRate)                                  \
    X(f32, RaccoonDogNormalRollingSideBrakeRate)                              \
    X(f32, RaccoonDogNormalRollingSideAccel)                                  \
    X(f32, RaccoonDogNormalRollingSideMaxSpeed)                               \
    X(f32, RaccoonDogDashRollingMinSpeed)                                     \
    X(s32, RaccoonDogDashRollingNoBrakeFrame)                                 \
    X(f32, RaccoonDogDashRollingBrakeRate)                                    \
    X(f32, RaccoonDogDashRollingSideBrakeRate)                                \
    X(f32, RaccoonDogDashRollingSideAccel)                                    \
    X(f32, RaccoonDogDashRollingSideMaxSpeed)                                 \
    X(f32, RollingTramplePow)                                                 \
    X(f32, WaitRollingAttackJumpGravity)                                      \
    X(f32, WaitRollingAttackJumpPow)                                          \
    X(f32, WaitRollingAttackVelH)                                             \
    X(f32, NormalRollingAttackJumpGravity)                                    \
    X(f32, NormalRollingAttackJumpPow)                                        \
    X(f32, NormalRollingAttackVelH)                                           \
    X(f32, RollingAttackJumpGravity)                                          \
    X(f32, RollingAttackJumpPow)                                              \
    X(f32, RollingAttackVelH)                                                 \
    X(f32, RaccoonDogWaitRollingAttackJumpGravity)                            \
    X(f32, RaccoonDogWaitRollingAttackJumpPow)                                \
    X(f32, RaccoonDogWaitRollingAttackVelH)                                   \
    X(f32, RaccoonDogWaitRollingAttackHighJumpGravity)                        \
    X(f32, RaccoonDogWaitRollingAttackHighJumpPow)                            \
    X(f32, RaccoonDogWaitRollingAttackHighVelH)                               \
    X(f32, CommonRollingAttackSpeedMin)                                       \
    X(f32, CommonRollingAttackBrake)                                          \
    X(f32, CommonRollingAttackSideAccel)                                      \
    X(f32, RollingHitBound)                                                   \
    X(s32, WallHitLandCancelFrame)                                            \
    X(s32, DamageInvalidCount)                                                \
    X(s32, DamageCancelFrame)                                                 \
    X(s32, InvincibleFrame)                                                   \
    X(s32, InvincibleDashFrame)                                               \
    X(f32, InvincibleDashSpeed)                                               \
    X(f32, InvincibleJumpPow)                                                 \
    X(s32, InvincibleJumpPowCountMax)                                         \
    X(s32, TailAttackStart)                                                   \
    X(s32, TailAttackFrame)                                                   \
    X(s32, TailAttackInterval)                                                \
    X(f32, StandSwimRisePower)                                                \
    X(f32, StandSwimRiseSpeedMax)                                             \
    X(f32, StandSwimGravity)                                                  \
    X(f32, StandSwimFallSpeedMax)                                             \
    X(f32, StandSwimHorizontalFloorDashAccel)                                 \
    X(f32, StandSwimHorizontalFloorDashSpeedMax)                              \
    X(f32, StandSwimHorizontalFloorAccel)                                     \
    X(f32, StandSwimHorizontalFloorSpeedMax)                                  \
    X(f32, NoSinkSwimHorizontalHighAccel)                                     \
    X(f32, NoSinkSwimHorizontalHighInputMin)                                  \
    X(f32, NoSinkSwimHorizontalHighSpeedMax)                                  \
    X(f32, NoSinkSwimHorizontalHighSpeedMin)                                  \
    X(f32, StandSwimHorizontalHighAccel)                                      \
    X(f32, StandSwimHorizontalHighSpeedMax)                                   \
    X(f32, StandSwimHorizontalLowAccel)                                       \
    X(f32, StandSwimHorizontalLowSpeedMax)                                    \
    X(f32, StandSwimHorizontalBrakeRate)                                      \
    X(s32, StandSwimHighAccelPermitFrame)                                     \
    X(f32, StandSwimForwardBentDegree)                                        \
    X(f32, StandSwimForwardBentBlend)                                         \
    X(f32, StandSwimFlowFieldBlend)                                           \
    X(f32, StandSwimRotSpeed)                                                 \
    X(f32, StandSwimSurfaceRotSpeed)                                          \
    X(f32, StandSwimSurfaceRotSpeedNoMovement)                                \
    X(f32, StandSwimWalkAnimMinRate)                                          \
    X(f32, StandSwimWalkAnimMaxRate)                                          \
    X(f32, StandSwimWalkMaxSpeed)                                             \
    X(s32, StandSwimPaddleAnimInterval)                                       \
    X(s32, StandSwimPaddleAnimRateIntervalMax)                                \
    X(s32, StandSwimPaddleAnimRateIntervalMin)                                \
    X(f32, StandSwimPaddleAnimMaxRate)                                        \
    X(f32, SwimHRotSpeed)                                                     \
    X(f32, SwimVRotSpeed)                                                     \
    X(f32, SwimPaddleAccel)                                                   \
    X(f32, SwimPaddleSpeedMax)                                                \
    X(s32, SwimPaddleFrame)                                                   \
    X(f32, SwimKickAccel)                                                     \
    X(f32, SwimKickSpeedMax)                                                  \
    X(f32, SwimKickBrake)                                                     \
    X(f32, SwimBrake)                                                         \
    X(f32, SwimSideBrake)                                                     \
    X(s32, StandSwimFromDiveTimer)                                            \
    X(f32, StandSwimFromDiveRisePower)                                        \
    X(f32, StandSwimFromDiveRisePowerClimb)                                   \
    X(f32, SwimDiveStartSpeed)                                                \
    X(f32, SwimDiveBrake)                                                     \
    X(f32, SwimDiveEndSpeed)                                                  \
    X(s32, SwimDiveLandCount)                                                 \
    X(s32, SwimDiveLandCancelFrame)                                           \
    X(s32, SwimDiveButtonValidFrame)                                          \
    X(f32, DiveStartSpeed)                                                    \
    X(f32, DiveBrake)                                                         \
    X(f32, DiveBrakeSingleMode)                                               \
    X(f32, DiveEndSpeed)                                                      \
    X(f32, StandSwimTramplePower)                                             \
    X(f32, DiveTramplePower)                                                  \
    X(f32, DiveTrampleCancelFrame)                                            \
    X(f32, SwimSurfaceStartDist)                                              \
    X(f32, SwimSurfaceEndDist)                                                \
    X(f32, SwimSurfaceStartDistShort)                                         \
    X(f32, SwimSurfaceEndDistShort)                                           \
    X(f32, SwimSurfaceVelDamper)                                              \
    X(f32, SwimSurfaceGravity)                                                \
    X(s32, SwimSurfaceValidDamperFrame)                                       \
    X(s32, SwimSurfaceDamperLerpFrame)                                        \
    X(f32, SwimSurfaceBaseHeight)                                             \
    X(f32, SwimSurfaceBaseHeightShort)                                        \
    X(f32, SwimSurfaceSpring)                                                 \
    X(f32, SwimSurfaceVerticalOffset)                                         \
    X(f32, SwimSurfacePivotRate)                                              \
    X(f32, SwimSurfacePivotCancelAngle)                                       \
    X(f32, SwimSurfaceSpeedThreshold)                                         \
    X(s32, SwimSurfacePivotCounter)                                           \
    X(f32, SwimSurfaceTiltDuringPivotMaxDegree)                               \
    X(f32, SwimSurfaceTiltMaxDegree)                                          \
    X(f32, SwimSurfaceTiltMaxFrontAngle)                                      \
    X(f32, SwimSurfaceClimbAnimationRate)                                     \
    X(f32, SwimSurfaceSpringForSurfaceSwim)                                   \
    X(f32, SwimSurfaceSpringForSurfaceSwimClimb)                              \
    X(f32, SwimJumpPow)                                                       \
    X(s32, SwimSquatInhibitFrame)                                             \
    X(f32, PropellerRisePow)                                                  \
    X(s32, PropellerPowSustain)                                               \
    X(s32, PropellerPowSustainMin)                                            \
    X(s32, PropellerPowRelease)                                               \
    X(f32, PropellerBeforeDropGravity)                                        \
    X(f32, PropellerRiseGravity)                                              \
    X(f32, PropellerAfterDropGravity)                                         \
    X(f32, PropellerFallSpeedMax)                                             \
    X(f32, PropellerButtonOffFallSpeedMax)                                    \
    X(f32, PropellerEngineBrakeVel)                                           \
    X(f32, PropellerEngineBrakeRate)                                          \
    X(f32, PropellerEngineBrakeEndVel)                                        \
    X(f32, PropellerRotBlendRate)                                             \
    X(f32, PropellerSideDamper)                                               \
    X(f32, PropellerStickOffBrakeRate)                                        \
    X(f32, LongFallDistance)                                                  \
    X(s32, StatueFallStartFrame)                                              \
    X(s32, StatueLandFrame)                                                   \
    X(s32, StatueEndFrame)                                                    \
    X(s32, StatueEndAnimStep)                                                 \
    X(f32, StatueFallSpeedInWater)                                            \
    X(f32, SlideSlopeAngle)                                                   \
    X(f32, SlideSlopeEndAngle)                                                \
    X(f32, SlideEndSpeed)                                                     \
    X(f32, SlideAccel)                                                        \
    X(f32, SlideMaxSpeed)                                                     \
    X(f32, SlideSideBrake)                                                    \
    X(f32, SlideSideAccel)                                                    \
    X(f32, SlideSideMaxSpeed)                                                 \
    X(f32, SlideSideAccelOnLevelLand)                                         \
    X(f32, SlideSideMaxSpeedOnLevelLand)                                      \
    X(f32, SlideBrake)                                                        \
    X(f32, ForceSlideBrake)                                                   \
    X(f32, SlidePostureBlendRate)                                             \
    X(f32, ForceSlideSpeed)                                                   \
    X(f32, ForceSlideSpeedUpRate)                                             \
    X(f32, SlideTiltBlendRate)                                                \
    X(f32, SlideTiltMaxDegree)                                                \
    X(s32, SlideInvalidFrame)                                                 \
    X(f32, ForceSlideMaxSpeed)                                                \
    X(s32, SlideFallCancelFrame)                                              \
    X(f32, SlideJumpHVelScale)                                                \
    X(s32, HoldShakeInterval)                                                 \
    X(s32, HoldThrowFrontTiming)                                              \
    X(s32, HoldThrowUpTiming)                                                 \
    X(f32, HoldJumpFrontVel)                                                  \
    X(f32, HoldJumpUpVel)                                                     \
    X(s32, ClimbAttackInterval)                                               \
    X(s32, ClimbAttackWaitInterval)                                           \
    X(s32, ClimbAttackCancelFrame)                                            \
    X(s32, ClimbAttackSensorOnFrame)                                          \
    X(f32, ClimbBodyAttackFrontVel)                                           \
    X(f32, ClimbBodyAttackDownVel)                                            \
    X(s32, ClimbBodyAttackFrame)                                              \
    X(f32, ClimbBodyAttackGravity)                                            \
    X(f32, ClimbBodyAttackFallSpeedMax)                                       \
    X(f32, ClimbBodyAttackHBrakeRate)                                         \
    X(f32, ClimbBodyAttackSideAccel)                                          \
    X(f32, ClimbBodyAttackSideMoveDist)                                       \
    X(f32, SinkSandMoveMaxSpeed)                                              \
    X(f32, SinkSandMoveMaxDashSpeed)                                          \
    X(s32, SinkSandInvalidFrameInJump)                                        \
    X(f32, PushedBrakeRate)                                                   \
    X(f32, PushedBrakeMaxRate)                                                \
    X(f32, PushedJumpCancelSpeed)                                             \
    X(s32, GroundSpinFrame)                                                   \
    X(f32, GroundSpinAccel)                                                   \
    X(f32, GroundSpinBrake)                                                   \
    X(f32, GroundSpinVelMax)                                                  \
    X(f32, SpinJumpGravity)                                                   \
    X(f32, SpinJumpPow)                                                       \
    X(s32, SpinAttackInterval)                                                \
    X(s32, SpinAttackCancelFrame)                                             \
    X(s32, SpinAttackSensorOnFrame)                                           \
    X(f32, SpinAttackJumpPow)                                                 \
    X(f32, SpinAttackJumpGravity)                                             \
    X(f32, SpinAttackGroundBrake)                                             \
    X(f32, SkateJumpGravity)                                                  \
    X(f32, SkateJumpPowLow)                                                   \
    X(f32, SkateJumpPow)                                                      \
    X(s32, SkateJumpPowCountMax)                                              \
    X(f32, SkateJumpThreshold)                                                \
    X(s32, CoopHipDropFrame)                                                  \
    X(f32, CoopHipDropRadiusMin)                                              \
    X(f32, CoopHipDropRadius)                                                 \
    X(s32, GiantHipDropFrame)                                                 \
    X(f32, GiantHipDropRadiusMin)                                             \
    X(f32, GiantHipDropRadiusMax)                                             \
    X(f32, KnockDownVelH)                                                     \
    X(f32, KnockDownVelV)                                                     \
    X(s32, KnockDownCancelFrame)                                              \
    X(f32, ReflectJumpGravity)                                                \
    X(f32, ReflectJump)                                                       \
    X(f32, RisingReflectJumpHVelBrakeRate)                                    \
    X(s32, TossCancelFrame)                                                   \
    X(s32, ManekinekoFallStartFrame)                                          \
    X(s32, ManekinekoLandFrame)                                               \
    X(s32, ManekinekoEndNoticeFrame)                                          \
    X(s32, ManekinekoEndFrame)                                                \
    X(s32, ManekinekoCancelFrame)                                             \
    X(f32, ManekinekoFallSpeedInWater)                                        \
    X(s32, GroomingMaxInterval)                                               \
    X(s32, GroomingMinInterval)                                               \
    X(s32, SePropellerBeginStep)                                              \
    X(f32, SeFootNoteNormalVolMul)                                            \
    X(f32, SeFootNoteNormalPitDec)                                            \
    X(f32, SeFootNoteDashVolAdd)                                              \
    X(f32, SeFootNoteDashPitAdd)                                              \
    X(f32, GigaCommonAnimRate)                                                \
    X(f32, GigaMiniRunAnimRateMax)                                            \
    X(f32, GigaMiniDashAnimRateMax)                                           \
    X(f32, GigaSuperRunAnimRateMax)                                           \
    X(f32, GigaSuperDashAnimRateMax)                                          \
    X(f32, GigaClimbRunAnimRateMax)                                           \
    X(f32, GigaClimbDashAnimRateMax)                                          \
    X(f32, GigaNormalMaxSpeed)                                                \
    X(f32, GigaDashMaxSpeed)                                                  \
    X(f32, GigaSuperDashSpeed)                                                \
    X(f32, GigaInvincibleDashSpeed)                                           \
    X(s32, GigaAccelFrame)                                                    \
    X(f32, GigaSquatWalkSpeed)                                                \
    X(f32, GigaGroundSpinAccel)                                               \
    X(f32, GigaGroundSpinBrake)                                               \
    X(f32, GigaKnockDownVelH)                                                 \
    X(f32, GigaKnockDownVelV)                                                 \
    X(f32, GigaLeftFootHrTime)                                                \
    X(f32, GigaRightFootHrTime)                                               \
    X(f32, GigaClimbLeftWalkHrTime)                                           \
    X(f32, GigaClimbRightWalkHrTime)                                          \
    X(f32, GigaClimbLeftRunHrTime)                                            \
    X(f32, GigaClimbRightRunHrTime)                                           \
    X(f32, GigaRoundLimitDegreeMax)                                           \
    X(f32, GigaRoundLimitDegreeMin)                                           \
    X(f32, GigaNormalRollingMinSpeed)                                         \
    X(f32, GigaNormalGravityAddition)                                         \
    X(f32, GigaSquatBrakeRate)                                                \
    X(f32, GigaGravity)                                                       \
    X(f32, GigaFallSpeedMax)                                                  \
    X(f32, GigaFloatFallSpeedMax)                                             \
    X(f32, GigaJumpPow)                                                       \
    X(f32, GigaJumpPowLow)                                                    \
    X(s32, GigaJumpPowCountMax)                                               \
    X(f32, GigaJumpCancelBrakeRate)                                           \
    X(f32, GigaJumpCancelMinSpeed)                                            \
    X(f32, GigaHipDropSpeed)                                                  \
    X(f32, GigaHipDropAnimRate)                                               \
    X(f32, GigaHipDropJumpPow)                                                \
    X(f32, GigaTrampleJump)                                                   \
    X(f32, GigaLongJumpSlowSpeed)                                             \
    X(f32, GigaLongJumpFastSpeed)                                             \
    X(f32, GigaLongJumpSlowJumpPow)                                           \
    X(f32, GigaLongJumpFastJumpPow)                                           \
    X(f32, GigaLongJumpSlowGravity)                                           \
    X(f32, GigaLongJumpFastGravity)                                           \
    X(f32, GigaSquatJumpGravity)                                              \
    X(f32, GigaSquatJumpPow)                                                  \
    X(f32, GigaSquatHighJumpPow)                                              \
    X(f32, GigaSquatJumpBackPow)                                              \
    X(f32, GigaSpinJumpGravity)                                               \
    X(f32, GigaSpinJumpPow)                                                   \
    X(f32, GigaNormalRollingAttackJumpGravity)                                \
    X(f32, GigaNormalRollingAttackJumpPow)                                    \
    X(f32, GigaNormalRollingAttackVelH)                                       \
    X(f32, GigaWallJumpHSpeed)                                                \
    X(f32, GigaLongJumpBrake)                                                 \
    X(f32, GigaLongJumpSpeedMin)                                              \
    X(f32, GigaLongJumpSideAccel)                                             \
    X(s32, GigaLandFrame)                                                     \
    X(f32, GigaWallClimbMaxSpeed)                                             \
    X(f32, GigaWallClimbDashMaxSpeed)                                         \
    X(f32, GigaWallClimbAccel)                                                \
    X(f32, GigaWallClimbMaxSideSpeed)                                         \
    X(f32, GigaWallClimbDashMaxSideSpeed)                                     \
    X(f32, GigaWallClimbSideAccel)                                            \
    X(f32, GigaWallSnapDistance)                                              \
    X(f32, GigaWallClimbJumpPowLow)                                           \
    X(f32, GigaWallClimbJumpPow)                                              \
    X(f32, GigaWallClimbSlideGravity)                                         \
    X(f32, GigaWallClimbSlideMaxSpeed)                                        \
    X(f32, GigaWallClimbSlideSideAccel)                                       \
    X(f32, GigaWallClimbSlideSideMaxSpeed)                                    \
    X(f32, FlashRangeAttackLengthOffset)                                      \
    X(f32, FlashRangeAttackDegreeScale)                                       \
    X(f32, FlashRangeAttackHeightMax)                                         \
    X(f32, FlashRangeAttackHeightMin)                                         \
    X(s32, IsEnableHeadLightOfx)                                              \
    X(f32, HeadLightOfxOffsetY)                                               \
    X(f32, HeadLightOfxOffsetZ)                                               \
    X(f32, HeadLightPrePassPointLightRadiusScale)                             \
    X(f32, HeadLightPrePassPointLightOffsetY)                                 \
    X(f32, HeadLightPrePassPointLightOffsetZ)

/// Declares the getter of one tuning value. The getter asks the override parameter set
/// instead while one is active.
#define PLAYER_CONST_PARAM_DECLARE_GETTER(Type, Name) virtual Type get##Name() const;

/// The player's tuning values (speeds, jump heights, frame counts, ...), read from
/// PlayerConst.byml. Another set can override it (e.g. for Bowser's Fury's modes).
class PlayerConstParam {
public:
    PlayerConstParam();
    PlayerConstParam(const al::ByamlIter&);

    PLAYER_CONST_PARAM_LIST(PLAYER_CONST_PARAM_DECLARE_GETTER)

    /// Make the getters ask the override parameter set (or stop doing so).
    void setOverride(bool isOverride) { mIsOverride = isOverride; }
    /// Whether the getters currently ask the override parameter set.
    bool isOverride() const { return mIsOverride; }
    /// The parameter set the getters ask while overriding.
    const PlayerConstParam* getOverrideParam() const { return mOverrideParam; }

private:
    f32 mGravity;  // 0x8
    f32 mCenterHeight;  // 0xc
    f32 mBodyRadius;  // 0x10
    f32 mCollectInfoRadiusAddition;  // 0x14
    f32 mSnapGroundMaxLength;  // 0x18
    f32 mSnapWallMaxLength;  // 0x1c
    f32 mStickRoundThreshold;  // 0x20
    f32 mHeightCheckLength;  // 0x24
    f32 mCutVelLimit;  // 0x28
    f32 mCutVelRate;  // 0x2c
    s32 mThrowInvalidationFrames;  // 0x30
    f32 mTall;  // 0x34
    f32 mChestRadius;  // 0x38
    f32 mDashCheckRadius;  // 0x3c
    f32 mShadowCheckLength;  // 0x40
    f32 mShadowLengthMax;  // 0x44
    s32 mPivotFrame;  // 0x48
    f32 mPivotDegree;  // 0x4c
    f32 mNormalMaxSpeed;  // 0x50
    f32 mDashMaxSpeed;  // 0x54
    f32 mSuperDashSpeed;  // 0x58
    s32 mSuperDashTimer;  // 0x5c
    s32 mSuperDashTimerMini;  // 0x60
    s32 mSuperDashTimerFire;  // 0x64
    s32 mSuperDashTimerClimb;  // 0x68
    s32 mSuperDashTimerRaccoonDog;  // 0x6c
    s32 mSuperDashTimerBoomerang;  // 0x70
    s32 mSuperDashTimerRaccoonDogWhite;  // 0x74
    f32 mSuperDashStartAnimRate;  // 0x78
    s32 mSuperDashStartAnimFrame;  // 0x7c
    s32 mBrakeFrame;  // 0x80
    s32 mDashBrakeFrame;  // 0x84
    s32 mStickOnBrakeFrame;  // 0x88
    s32 mBrakeFrameOnIce;  // 0x8c
    s32 mAccelFrame;  // 0x90
    s32 mDashAccelFrame;  // 0x94
    f32 mRoundLimitDegreeMax;  // 0x98
    f32 mRoundLimitDegreeMin;  // 0x9c
    f32 mRunAnimRateMax;  // 0xa0
    f32 mGiantRunAnimRateMax;  // 0xa4
    f32 mShortAnimRateEff;  // 0xa8
    s32 mDashStartFrame;  // 0xac
    s32 mModifiedDashStartFrame;  // 0xb0
    s32 mDashStartBlendFrame;  // 0xb4
    s32 mDashInputSuccessFrame;  // 0xb8
    f32 mDownHillAccelStartDegree;  // 0xbc
    f32 mDownHillAccelEndDegree;  // 0xc0
    f32 mDownHillAccelAddRate;  // 0xc4
    f32 mDashPanelSpeed;  // 0xc8
    f32 mModifiedDashPanelSpeed;  // 0xcc
    f32 mDashPanelOverRate;  // 0xd0
    s32 mDashPanelTimer;  // 0xd4
    f32 mFlingPoleSpeed;  // 0xd8
    s32 mGroundOffFrame;  // 0xdc
    s32 mClimbToGroundMoveFrame;  // 0xe0
    f32 mTiltMaxDegree;  // 0xe4
    f32 mTiltBlendRate;  // 0xe8
    f32 mTiltMaxFrontAngle;  // 0xec
    f32 mHoldingTiltMaxFrontAngle;  // 0xf0
    f32 mTiltStartSpeed;  // 0xf4
    f32 mTiltEndSpeed;  // 0xf8
    f32 mClimbRunAnimRateEff;  // 0xfc
    f32 mPanelDashAnimRate;  // 0x100
    f32 mModifiedPanelDashAnimRate;  // 0x104
    f32 mSlopeMaxSpeedScale;  // 0x108
    f32 mMaxSpeedScale;  // 0x10c
    f32 mDashSignAnimRate;  // 0x110
    f32 mDashSignMaxLoop;  // 0x114
    f32 mDashSignMaxSpeed;  // 0x118
    s32 mDashSignAnimFrameMax;  // 0x11c
    f32 mJumpDirRotLimit;  // 0x120
    f32 mJumpPowLow;  // 0x124
    f32 mJumpPow;  // 0x128
    s32 mJumpPowCountMax;  // 0x12c
    f32 mJumpExtensionGravityRate;  // 0x130
    f32 mJumpSideVelRate;  // 0x134
    f32 mJumpFrontBrakeRate;  // 0x138
    f32 mJumpRotBlendRate;  // 0x13c
    s32 mJumpAccelFrame;  // 0x140
    s32 mReleaseAccelFrame;  // 0x144
    s32 mJumpAccelAddFrame;  // 0x148
    f32 mJumpCancelBrakeRate;  // 0x14c
    f32 mJumpCancelMinSpeed;  // 0x150
    s32 mContinuousJumpTimer;  // 0x154
    s32 mContinuousJumpCount;  // 0x158
    f32 mFallSpeedMax;  // 0x15c
    f32 mDashJumpAddition;  // 0x160
    f32 mPunchReflectPower;  // 0x164
    s32 mGlideInhibitFrameAfterPunch;  // 0x168
    f32 mWalkMinSpeedRate;  // 0x16c
    s32 mJumpHVelSamplingNum;  // 0x170
    f32 mFollowFrontDamper;  // 0x174
    f32 mFollowBackDamper;  // 0x178
    s32 mFlightDurationCount;  // 0x17c
    f32 mFlightDurationRotBlendRate;  // 0x180
    s32 mFlightDurationJumpStartInhibitFrame;  // 0x184
    f32 mFlightDurationSideDamper;  // 0x188
    f32 mRaccoonDogFallSpeedMax;  // 0x18c
    f32 mRaccoonDogFallGravityRate;  // 0x190
    f32 mRaccoonDogFallGravityAdd;  // 0x194
    f32 mRaccoonDogFirstFallDamper;  // 0x198
    f32 mRaccoonDogRotBlendRate;  // 0x19c
    f32 mRaccoonDogFallSideDamper;  // 0x1a0
    f32 mTrampleJumpGravity;  // 0x1a4
    f32 mTrampleJump;  // 0x1a8
    f32 mTrampleJumpSideVelRate;  // 0x1ac
    f32 mTrampleHighJumpGravity;  // 0x1b0
    f32 mTrampleHighJump;  // 0x1b4
    f32 mTrampleHipDropGravity;  // 0x1b8
    f32 mTrampleHipDropJump;  // 0x1bc
    f32 mRisingTrampleJumpGravity;  // 0x1c0
    f32 mRisingTrampleJump;  // 0x1c4
    f32 mRisingTrampleHighJumpGravity;  // 0x1c8
    f32 mRisingTrampleHighJump;  // 0x1cc
    f32 mRisingTrampleHVelBrakeRate;  // 0x1d0
    f32 mPunchedJumpGravity;  // 0x1d4
    f32 mPunchedJump;  // 0x1d8
    f32 mRisingPunchedHVelBrakeRate;  // 0x1dc
    f32 mTossedJumpGravity;  // 0x1e0
    f32 mTossedJump;  // 0x1e4
    f32 mTossedHighJumpGravity;  // 0x1e8
    f32 mTossedHighJump;  // 0x1ec
    f32 mTossedHipDropGravity;  // 0x1f0
    f32 mTossedHipDropJump;  // 0x1f4
    f32 mRisingTossedJumpGravity;  // 0x1f8
    f32 mRisingTossedJump;  // 0x1fc
    f32 mRisingTossedHighJumpGravity;  // 0x200
    f32 mRisingTossedHighJump;  // 0x204
    f32 mRisingTossedHVelBrakeRate;  // 0x208
    f32 mSquatBrakeEndSpeed;  // 0x20c
    f32 mSquatShiftSpeedRate;  // 0x210
    f32 mSquatAccelRate;  // 0x214
    s32 mSquatNoBrakeFrame;  // 0x218
    f32 mSquatBrakeRate;  // 0x21c
    f32 mSquatBrakeRateOnSkate;  // 0x220
    f32 mSquatBrakeSideAccel;  // 0x224
    f32 mSquatBrakeSideBrakeRate;  // 0x228
    f32 mSquatBrakeSideBrakeRateOnSkate;  // 0x22c
    f32 mSquatBrakeSideMaxSpeedRate;  // 0x230
    f32 mSquatWalkSpeed;  // 0x234
    f32 mSquatWalkFrontVecBlend;  // 0x238
    s32 mSquatEnergyAccelFrame;  // 0x23c
    f32 mSquatJumpGravity;  // 0x240
    f32 mSquatJumpPow;  // 0x244
    f32 mSquatHighJumpPow;  // 0x248
    f32 mSquatJumpBackPow;  // 0x24c
    f32 mSquatJumpTramplePow;  // 0x250
    f32 mHipDropSpeed;  // 0x254
    s32 mHipDropLandCancelFrame;  // 0x258
    f32 mHipDropHeight;  // 0x25c
    s32 mHipDropMsgInterval;  // 0x260
    f32 mHipDropKnockDownRadiusMin;  // 0x264
    f32 mHipDropKnockDownRadiusMax;  // 0x268
    f32 mHipDropStartAnimRate;  // 0x26c
    s32 mHipDropKnockDownFrame;  // 0x270
    f32 mHipDropJumpPow;  // 0x274
    s32 mHipDropJumpPowCountMax;  // 0x278
    s32 mHipDropJumpPermitBeginFrame;  // 0x27c
    s32 mHipDropJumpPermitEndFrame;  // 0x280
    f32 mWallHeightLowLimit;  // 0x284
    f32 mWallGravity;  // 0x288
    f32 mWallMaxSpeed;  // 0x28c
    f32 mWallApartFrame;  // 0x290
    f32 mWallSnapDistance;  // 0x294
    f32 mWallSlideMaxSpeed;  // 0x298
    f32 mWallSlideAccel;  // 0x29c
    s32 mWallInhibitAfterPunch;  // 0x2a0
    f32 mWallJumpGravity;  // 0x2a4
    f32 mWallJumpHSpeed;  // 0x2a8
    f32 mWallJumpPow;  // 0x2ac
    s32 mWallJumpInvalidateInputFrame;  // 0x2b0
    s32 mWallJumpDirEffectiveFrame;  // 0x2b4
    f32 mWallJumpDirLimit;  // 0x2b8
    f32 mWallJumpLimitPlay;  // 0x2bc
    s32 mWallClimbReadyFrame;  // 0x2c0
    s32 mWallClimbReadyWaitFrame;  // 0x2c4
    s32 mWallClimbReadyFromSlopeFrame;  // 0x2c8
    f32 mWallClimbAccel;  // 0x2cc
    f32 mWallClimbMaxSpeed;  // 0x2d0
    f32 mWallClimbDashMaxSpeed;  // 0x2d4
    s32 mWallClimbFrame;  // 0x2d8
    s32 mWallClimbFrame1;  // 0x2dc
    s32 mWallClimbFrame2;  // 0x2e0
    s32 mWallClimbDashFrame;  // 0x2e4
    s32 mWallClimbDashFrame1;  // 0x2e8
    s32 mWallClimbDashFrame2;  // 0x2ec
    s32 mWallClimbStopFrame;  // 0x2f0
    f32 mWallClimbBrakeRate;  // 0x2f4
    s32 mWallClimbNoWallFrame;  // 0x2f8
    f32 mWallClimbMaxSideSpeed;  // 0x2fc
    f32 mWallClimbDashMaxSideSpeed;  // 0x300
    f32 mWallClimbSideAccel;  // 0x304
    f32 mWallClimbRetainThreshold;  // 0x308
    f32 mWallClimbInvalidSideMoveDegree;  // 0x30c
    f32 mWallClimbDashAnimRate;  // 0x310
    f32 mWallClimbNormalAnimRate;  // 0x314
    s32 mWallClimbJumpPowCountMax;  // 0x318
    f32 mWallClimbJumpDashAddition;  // 0x31c
    f32 mWallClimbJumpPowLow;  // 0x320
    f32 mWallClimbJumpPow;  // 0x324
    f32 mWallClimbSlideGravity;  // 0x328
    f32 mWallClimbSlideMaxSpeed;  // 0x32c
    f32 mWallClimbSlideSideAccel;  // 0x330
    f32 mWallClimbSlideSideMaxSpeed;  // 0x334
    f32 mWallClimbSlideStartBrakeRate;  // 0x338
    s32 mClimbAirStopFrame;  // 0x33c
    f32 mClimbAirStopRotateVelMax;  // 0x340
    f32 mLongJumpSuccessSpeed;  // 0x344
    f32 mLongJumpSpeedMin;  // 0x348
    f32 mLongJumpBrake;  // 0x34c
    f32 mLongJumpSideAccel;  // 0x350
    s32 mLongJumpCancelFrame;  // 0x354
    s32 mLongJumpFastSuccessFrame;  // 0x358
    f32 mLongJumpFastGravity;  // 0x35c
    f32 mLongJumpFastJumpPow;  // 0x360
    f32 mLongJumpFastSpeed;  // 0x364
    f32 mLongJumpSlowGravity;  // 0x368
    f32 mLongJumpSlowJumpPow;  // 0x36c
    f32 mLongJumpSlowSpeed;  // 0x370
    s32 mDashBrakeCommandFrame;  // 0x374
    s32 mDashBrakeActionFrame;  // 0x378
    f32 mDashBrakeSpeed;  // 0x37c
    f32 mTurnJumpGravity;  // 0x380
    f32 mTurnJumpPow;  // 0x384
    f32 mTurnJumpVelH;  // 0x388
    f32 mTurnJumpBrake;  // 0x38c
    f32 mTurnJumpAccel;  // 0x390
    f32 mTurnJumpSideAccel;  // 0x394
    s32 mTurnJumpToFlightDurationFrame;  // 0x398
    f32 mWaitRollingMinSpeed;  // 0x39c
    s32 mWaitRollingNoBrakeFrame;  // 0x3a0
    f32 mWaitRollingBrakeRate;  // 0x3a4
    f32 mWaitRollingSideBrakeRate;  // 0x3a8
    f32 mWaitRollingSideAccel;  // 0x3ac
    f32 mWaitRollingSideMaxSpeed;  // 0x3b0
    f32 mNormalRollingMinSpeed;  // 0x3b4
    s32 mNormalRollingNoBrakeFrame;  // 0x3b8
    f32 mNormalRollingBrakeRate;  // 0x3bc
    f32 mNormalRollingSideBrakeRate;  // 0x3c0
    f32 mNormalRollingSideAccel;  // 0x3c4
    f32 mNormalRollingSideMaxSpeed;  // 0x3c8
    f32 mAirRollingMinSpeed;  // 0x3cc
    s32 mAirRollingNoBrakeFrame;  // 0x3d0
    f32 mAirRollingBrakeRate;  // 0x3d4
    f32 mAirRollingSideBrakeRate;  // 0x3d8
    f32 mAirRollingSideAccel;  // 0x3dc
    f32 mAirRollingSideMaxSpeed;  // 0x3e0
    f32 mAirRollingJumpPow;  // 0x3e4
    f32 mAirRollingGravity;  // 0x3e8
    f32 mRollingMinSpeed;  // 0x3ec
    s32 mRollingNoBrakeFrame;  // 0x3f0
    f32 mRollingBrakeRate;  // 0x3f4
    f32 mRollingSideBrakeRate;  // 0x3f8
    f32 mRollingSideAccel;  // 0x3fc
    f32 mRollingSideMaxSpeed;  // 0x400
    f32 mRaccoonDogWaitRollingMinSpeed;  // 0x404
    s32 mRaccoonDogWaitRollingNoBrakeFrame;  // 0x408
    f32 mRaccoonDogWaitRollingBrakeRate;  // 0x40c
    f32 mRaccoonDogWaitRollingSideBrakeRate;  // 0x410
    f32 mRaccoonDogWaitRollingSideAccel;  // 0x414
    f32 mRaccoonDogWaitRollingSideMaxSpeed;  // 0x418
    f32 mRaccoonDogNormalRollingMinSpeed;  // 0x41c
    s32 mRaccoonDogNormalRollingNoBrakeFrame;  // 0x420
    f32 mRaccoonDogNormalRollingBrakeRate;  // 0x424
    f32 mRaccoonDogNormalRollingSideBrakeRate;  // 0x428
    f32 mRaccoonDogNormalRollingSideAccel;  // 0x42c
    f32 mRaccoonDogNormalRollingSideMaxSpeed;  // 0x430
    f32 mRaccoonDogDashRollingMinSpeed;  // 0x434
    s32 mRaccoonDogDashRollingNoBrakeFrame;  // 0x438
    f32 mRaccoonDogDashRollingBrakeRate;  // 0x43c
    f32 mRaccoonDogDashRollingSideBrakeRate;  // 0x440
    f32 mRaccoonDogDashRollingSideAccel;  // 0x444
    f32 mRaccoonDogDashRollingSideMaxSpeed;  // 0x448
    f32 mRollingTramplePow;  // 0x44c
    f32 mWaitRollingAttackJumpGravity;  // 0x450
    f32 mWaitRollingAttackJumpPow;  // 0x454
    f32 mWaitRollingAttackVelH;  // 0x458
    f32 mNormalRollingAttackJumpGravity;  // 0x45c
    f32 mNormalRollingAttackJumpPow;  // 0x460
    f32 mNormalRollingAttackVelH;  // 0x464
    f32 mRollingAttackJumpGravity;  // 0x468
    f32 mRollingAttackJumpPow;  // 0x46c
    f32 mRollingAttackVelH;  // 0x470
    f32 mRaccoonDogWaitRollingAttackJumpGravity;  // 0x474
    f32 mRaccoonDogWaitRollingAttackJumpPow;  // 0x478
    f32 mRaccoonDogWaitRollingAttackVelH;  // 0x47c
    f32 mRaccoonDogWaitRollingAttackHighJumpGravity;  // 0x480
    f32 mRaccoonDogWaitRollingAttackHighJumpPow;  // 0x484
    f32 mRaccoonDogWaitRollingAttackHighVelH;  // 0x488
    f32 mCommonRollingAttackSpeedMin;  // 0x48c
    f32 mCommonRollingAttackBrake;  // 0x490
    f32 mCommonRollingAttackSideAccel;  // 0x494
    f32 mRollingHitBound;  // 0x498
    s32 mWallHitLandCancelFrame;  // 0x49c
    s32 mDamageInvalidCount;  // 0x4a0
    s32 mDamageCancelFrame;  // 0x4a4
    s32 mInvincibleFrame;  // 0x4a8
    s32 mInvincibleDashFrame;  // 0x4ac
    f32 mInvincibleDashSpeed;  // 0x4b0
    f32 mInvincibleJumpPow;  // 0x4b4
    s32 mInvincibleJumpPowCountMax;  // 0x4b8
    s32 mTailAttackStart;  // 0x4bc
    s32 mTailAttackFrame;  // 0x4c0
    s32 mTailAttackInterval;  // 0x4c4
    f32 mStandSwimRisePower;  // 0x4c8
    f32 mStandSwimRiseSpeedMax;  // 0x4cc
    f32 mStandSwimGravity;  // 0x4d0
    f32 mStandSwimFallSpeedMax;  // 0x4d4
    f32 mStandSwimHorizontalFloorDashAccel;  // 0x4d8
    f32 mStandSwimHorizontalFloorDashSpeedMax;  // 0x4dc
    f32 mStandSwimHorizontalFloorAccel;  // 0x4e0
    f32 mStandSwimHorizontalFloorSpeedMax;  // 0x4e4
    f32 mNoSinkSwimHorizontalHighAccel;  // 0x4e8
    f32 mNoSinkSwimHorizontalHighInputMin;  // 0x4ec
    f32 mNoSinkSwimHorizontalHighSpeedMax;  // 0x4f0
    f32 mNoSinkSwimHorizontalHighSpeedMin;  // 0x4f4
    f32 mStandSwimHorizontalHighAccel;  // 0x4f8
    f32 mStandSwimHorizontalHighSpeedMax;  // 0x4fc
    f32 mStandSwimHorizontalLowAccel;  // 0x500
    f32 mStandSwimHorizontalLowSpeedMax;  // 0x504
    f32 mStandSwimHorizontalBrakeRate;  // 0x508
    s32 mStandSwimHighAccelPermitFrame;  // 0x50c
    f32 mStandSwimForwardBentDegree;  // 0x510
    f32 mStandSwimForwardBentBlend;  // 0x514
    f32 mStandSwimFlowFieldBlend;  // 0x518
    f32 mStandSwimRotSpeed;  // 0x51c
    f32 mStandSwimSurfaceRotSpeed;  // 0x520
    f32 mStandSwimSurfaceRotSpeedNoMovement;  // 0x524
    f32 mStandSwimWalkAnimMinRate;  // 0x528
    f32 mStandSwimWalkAnimMaxRate;  // 0x52c
    f32 mStandSwimWalkMaxSpeed;  // 0x530
    s32 mStandSwimPaddleAnimInterval;  // 0x534
    s32 mStandSwimPaddleAnimRateIntervalMax;  // 0x538
    s32 mStandSwimPaddleAnimRateIntervalMin;  // 0x53c
    f32 mStandSwimPaddleAnimMaxRate;  // 0x540
    f32 mSwimHRotSpeed;  // 0x544
    f32 mSwimVRotSpeed;  // 0x548
    f32 mSwimPaddleAccel;  // 0x54c
    f32 mSwimPaddleSpeedMax;  // 0x550
    s32 mSwimPaddleFrame;  // 0x554
    f32 mSwimKickAccel;  // 0x558
    f32 mSwimKickSpeedMax;  // 0x55c
    f32 mSwimKickBrake;  // 0x560
    f32 mSwimBrake;  // 0x564
    f32 mSwimSideBrake;  // 0x568
    s32 mStandSwimFromDiveTimer;  // 0x56c
    f32 mStandSwimFromDiveRisePower;  // 0x570
    f32 mStandSwimFromDiveRisePowerClimb;  // 0x574
    f32 mSwimDiveStartSpeed;  // 0x578
    f32 mSwimDiveBrake;  // 0x57c
    f32 mSwimDiveEndSpeed;  // 0x580
    s32 mSwimDiveLandCount;  // 0x584
    s32 mSwimDiveLandCancelFrame;  // 0x588
    s32 mSwimDiveButtonValidFrame;  // 0x58c
    f32 mDiveStartSpeed;  // 0x590
    f32 mDiveBrake;  // 0x594
    f32 mDiveBrakeSingleMode;  // 0x598
    f32 mDiveEndSpeed;  // 0x59c
    f32 mStandSwimTramplePower;  // 0x5a0
    f32 mDiveTramplePower;  // 0x5a4
    f32 mDiveTrampleCancelFrame;  // 0x5a8
    f32 mSwimSurfaceStartDist;  // 0x5ac
    f32 mSwimSurfaceEndDist;  // 0x5b0
    f32 mSwimSurfaceStartDistShort;  // 0x5b4
    f32 mSwimSurfaceEndDistShort;  // 0x5b8
    f32 mSwimSurfaceVelDamper;  // 0x5bc
    f32 mSwimSurfaceGravity;  // 0x5c0
    s32 mSwimSurfaceValidDamperFrame;  // 0x5c4
    s32 mSwimSurfaceDamperLerpFrame;  // 0x5c8
    f32 mSwimSurfaceBaseHeight;  // 0x5cc
    f32 mSwimSurfaceBaseHeightShort;  // 0x5d0
    f32 mSwimSurfaceSpring;  // 0x5d4
    f32 mSwimSurfaceVerticalOffset;  // 0x5d8
    f32 mSwimSurfacePivotRate;  // 0x5dc
    f32 mSwimSurfacePivotCancelAngle;  // 0x5e0
    f32 mSwimSurfaceSpeedThreshold;  // 0x5e4
    s32 mSwimSurfacePivotCounter;  // 0x5e8
    f32 mSwimSurfaceTiltDuringPivotMaxDegree;  // 0x5ec
    f32 mSwimSurfaceTiltMaxDegree;  // 0x5f0
    f32 mSwimSurfaceTiltMaxFrontAngle;  // 0x5f4
    f32 mSwimSurfaceClimbAnimationRate;  // 0x5f8
    f32 mSwimSurfaceSpringForSurfaceSwim;  // 0x5fc
    f32 mSwimSurfaceSpringForSurfaceSwimClimb;  // 0x600
    f32 mSwimJumpPow;  // 0x604
    s32 mSwimSquatInhibitFrame;  // 0x608
    f32 mPropellerRisePow;  // 0x60c
    s32 mPropellerPowSustain;  // 0x610
    s32 mPropellerPowSustainMin;  // 0x614
    s32 mPropellerPowRelease;  // 0x618
    f32 mPropellerBeforeDropGravity;  // 0x61c
    f32 mPropellerRiseGravity;  // 0x620
    f32 mPropellerAfterDropGravity;  // 0x624
    f32 mPropellerFallSpeedMax;  // 0x628
    f32 mPropellerButtonOffFallSpeedMax;  // 0x62c
    f32 mPropellerEngineBrakeVel;  // 0x630
    f32 mPropellerEngineBrakeRate;  // 0x634
    f32 mPropellerEngineBrakeEndVel;  // 0x638
    f32 mPropellerRotBlendRate;  // 0x63c
    f32 mPropellerSideDamper;  // 0x640
    f32 mPropellerStickOffBrakeRate;  // 0x644
    f32 mLongFallDistance;  // 0x648
    s32 mStatueFallStartFrame;  // 0x64c
    s32 mStatueLandFrame;  // 0x650
    s32 mStatueEndFrame;  // 0x654
    s32 mStatueEndAnimStep;  // 0x658
    f32 mStatueFallSpeedInWater;  // 0x65c
    f32 mSlideSlopeAngle;  // 0x660
    f32 mSlideSlopeEndAngle;  // 0x664
    f32 mSlideEndSpeed;  // 0x668
    f32 mSlideAccel;  // 0x66c
    f32 mSlideMaxSpeed;  // 0x670
    f32 mSlideSideBrake;  // 0x674
    f32 mSlideSideAccel;  // 0x678
    f32 mSlideSideMaxSpeed;  // 0x67c
    f32 mSlideSideAccelOnLevelLand;  // 0x680
    f32 mSlideSideMaxSpeedOnLevelLand;  // 0x684
    f32 mSlideBrake;  // 0x688
    f32 mForceSlideBrake;  // 0x68c
    f32 mSlidePostureBlendRate;  // 0x690
    f32 mForceSlideSpeed;  // 0x694
    f32 mForceSlideSpeedUpRate;  // 0x698
    f32 mSlideTiltBlendRate;  // 0x69c
    f32 mSlideTiltMaxDegree;  // 0x6a0
    s32 mSlideInvalidFrame;  // 0x6a4
    f32 mForceSlideMaxSpeed;  // 0x6a8
    s32 mSlideFallCancelFrame;  // 0x6ac
    f32 mSlideJumpHVelScale;  // 0x6b0
    s32 mHoldShakeInterval;  // 0x6b4
    s32 mHoldThrowFrontTiming;  // 0x6b8
    s32 mHoldThrowUpTiming;  // 0x6bc
    f32 mHoldJumpFrontVel;  // 0x6c0
    f32 mHoldJumpUpVel;  // 0x6c4
    s32 mClimbAttackInterval;  // 0x6c8
    s32 mClimbAttackWaitInterval;  // 0x6cc
    s32 mClimbAttackCancelFrame;  // 0x6d0
    s32 mClimbAttackSensorOnFrame;  // 0x6d4
    f32 mClimbBodyAttackFrontVel;  // 0x6d8
    f32 mClimbBodyAttackDownVel;  // 0x6dc
    s32 mClimbBodyAttackFrame;  // 0x6e0
    f32 mClimbBodyAttackGravity;  // 0x6e4
    f32 mClimbBodyAttackFallSpeedMax;  // 0x6e8
    f32 mClimbBodyAttackHBrakeRate;  // 0x6ec
    f32 mClimbBodyAttackSideAccel;  // 0x6f0
    f32 mClimbBodyAttackSideMoveDist;  // 0x6f4
    f32 mSinkSandMoveMaxSpeed;  // 0x6f8
    f32 mSinkSandMoveMaxDashSpeed;  // 0x6fc
    s32 mSinkSandInvalidFrameInJump;  // 0x700
    f32 mPushedBrakeRate;  // 0x704
    f32 mPushedBrakeMaxRate;  // 0x708
    f32 mPushedJumpCancelSpeed;  // 0x70c
    s32 mGroundSpinFrame;  // 0x710
    f32 mGroundSpinAccel;  // 0x714
    f32 mGroundSpinBrake;  // 0x718
    f32 mGroundSpinVelMax;  // 0x71c
    f32 mSpinJumpGravity;  // 0x720
    f32 mSpinJumpPow;  // 0x724
    s32 mSpinAttackInterval;  // 0x728
    s32 mSpinAttackCancelFrame;  // 0x72c
    s32 mSpinAttackSensorOnFrame;  // 0x730
    f32 mSpinAttackJumpPow;  // 0x734
    f32 mSpinAttackJumpGravity;  // 0x738
    f32 mSpinAttackGroundBrake;  // 0x73c
    f32 mSkateJumpGravity;  // 0x740
    f32 mSkateJumpPowLow;  // 0x744
    f32 mSkateJumpPow;  // 0x748
    s32 mSkateJumpPowCountMax;  // 0x74c
    f32 mSkateJumpThreshold;  // 0x750
    s32 mCoopHipDropFrame;  // 0x754
    f32 mCoopHipDropRadiusMin;  // 0x758
    f32 mCoopHipDropRadius;  // 0x75c
    s32 mGiantHipDropFrame;  // 0x760
    f32 mGiantHipDropRadiusMin;  // 0x764
    f32 mGiantHipDropRadiusMax;  // 0x768
    f32 mKnockDownVelH;  // 0x76c
    f32 mKnockDownVelV;  // 0x770
    s32 mKnockDownCancelFrame;  // 0x774
    f32 mReflectJumpGravity;  // 0x778
    f32 mReflectJump;  // 0x77c
    f32 mRisingReflectJumpHVelBrakeRate;  // 0x780
    s32 mTossCancelFrame;  // 0x784
    s32 mManekinekoFallStartFrame;  // 0x788
    s32 mManekinekoLandFrame;  // 0x78c
    s32 mManekinekoEndNoticeFrame;  // 0x790
    s32 mManekinekoEndFrame;  // 0x794
    s32 mManekinekoCancelFrame;  // 0x798
    f32 mManekinekoFallSpeedInWater;  // 0x79c
    s32 mGroomingMaxInterval;  // 0x7a0
    s32 mGroomingMinInterval;  // 0x7a4
    s32 mSePropellerBeginStep;  // 0x7a8
    f32 mSeFootNoteNormalVolMul;  // 0x7ac
    f32 mSeFootNoteNormalPitDec;  // 0x7b0
    f32 mSeFootNoteDashVolAdd;  // 0x7b4
    f32 mSeFootNoteDashPitAdd;  // 0x7b8
    f32 mGigaCommonAnimRate;  // 0x7bc
    f32 mGigaMiniRunAnimRateMax;  // 0x7c0
    f32 mGigaMiniDashAnimRateMax;  // 0x7c4
    f32 mGigaSuperRunAnimRateMax;  // 0x7c8
    f32 mGigaSuperDashAnimRateMax;  // 0x7cc
    f32 mGigaClimbRunAnimRateMax;  // 0x7d0
    f32 mGigaClimbDashAnimRateMax;  // 0x7d4
    f32 mGigaNormalMaxSpeed;  // 0x7d8
    f32 mGigaDashMaxSpeed;  // 0x7dc
    f32 mGigaSuperDashSpeed;  // 0x7e0
    f32 mGigaInvincibleDashSpeed;  // 0x7e4
    s32 mGigaAccelFrame;  // 0x7e8
    f32 mGigaSquatWalkSpeed;  // 0x7ec
    f32 mGigaGroundSpinAccel;  // 0x7f0
    f32 mGigaGroundSpinBrake;  // 0x7f4
    f32 mGigaKnockDownVelH;  // 0x7f8
    f32 mGigaKnockDownVelV;  // 0x7fc
    f32 mGigaLeftFootHrTime;  // 0x800
    f32 mGigaRightFootHrTime;  // 0x804
    f32 mGigaClimbLeftWalkHrTime;  // 0x808
    f32 mGigaClimbRightWalkHrTime;  // 0x80c
    f32 mGigaClimbLeftRunHrTime;  // 0x810
    f32 mGigaClimbRightRunHrTime;  // 0x814
    f32 mGigaRoundLimitDegreeMax;  // 0x818
    f32 mGigaRoundLimitDegreeMin;  // 0x81c
    f32 mGigaNormalRollingMinSpeed;  // 0x820
    f32 mGigaNormalGravityAddition;  // 0x824
    f32 mGigaSquatBrakeRate;  // 0x828
    f32 mGigaGravity;  // 0x82c
    f32 mGigaFallSpeedMax;  // 0x830
    f32 mGigaFloatFallSpeedMax;  // 0x834
    f32 mGigaJumpPow;  // 0x838
    f32 mGigaJumpPowLow;  // 0x83c
    s32 mGigaJumpPowCountMax;  // 0x840
    f32 mGigaJumpCancelBrakeRate;  // 0x844
    f32 mGigaJumpCancelMinSpeed;  // 0x848
    f32 mGigaHipDropSpeed;  // 0x84c
    f32 mGigaHipDropAnimRate;  // 0x850
    f32 mGigaHipDropJumpPow;  // 0x854
    f32 mGigaTrampleJump;  // 0x858
    f32 mGigaLongJumpSlowSpeed;  // 0x85c
    f32 mGigaLongJumpFastSpeed;  // 0x860
    f32 mGigaLongJumpSlowJumpPow;  // 0x864
    f32 mGigaLongJumpFastJumpPow;  // 0x868
    f32 mGigaLongJumpSlowGravity;  // 0x86c
    f32 mGigaLongJumpFastGravity;  // 0x870
    f32 mGigaSquatJumpGravity;  // 0x874
    f32 mGigaSquatJumpPow;  // 0x878
    f32 mGigaSquatHighJumpPow;  // 0x87c
    f32 mGigaSquatJumpBackPow;  // 0x880
    f32 mGigaSpinJumpGravity;  // 0x884
    f32 mGigaSpinJumpPow;  // 0x888
    f32 mGigaNormalRollingAttackJumpGravity;  // 0x88c
    f32 mGigaNormalRollingAttackJumpPow;  // 0x890
    f32 mGigaNormalRollingAttackVelH;  // 0x894
    f32 mGigaWallJumpHSpeed;  // 0x898
    f32 mGigaLongJumpBrake;  // 0x89c
    f32 mGigaLongJumpSpeedMin;  // 0x8a0
    f32 mGigaLongJumpSideAccel;  // 0x8a4
    s32 mGigaLandFrame;  // 0x8a8
    f32 mGigaWallClimbMaxSpeed;  // 0x8ac
    f32 mGigaWallClimbDashMaxSpeed;  // 0x8b0
    f32 mGigaWallClimbAccel;  // 0x8b4
    f32 mGigaWallClimbMaxSideSpeed;  // 0x8b8
    f32 mGigaWallClimbDashMaxSideSpeed;  // 0x8bc
    f32 mGigaWallClimbSideAccel;  // 0x8c0
    f32 mGigaWallSnapDistance;  // 0x8c4
    f32 mGigaWallClimbJumpPowLow;  // 0x8c8
    f32 mGigaWallClimbJumpPow;  // 0x8cc
    f32 mGigaWallClimbSlideGravity;  // 0x8d0
    f32 mGigaWallClimbSlideMaxSpeed;  // 0x8d4
    f32 mGigaWallClimbSlideSideAccel;  // 0x8d8
    f32 mGigaWallClimbSlideSideMaxSpeed;  // 0x8dc
    f32 mFlashRangeAttackLengthOffset;  // 0x8e0
    f32 mFlashRangeAttackDegreeScale;  // 0x8e4
    f32 mFlashRangeAttackHeightMax;  // 0x8e8
    f32 mFlashRangeAttackHeightMin;  // 0x8ec
    s32 mIsEnableHeadLightOfx;  // 0x8f0
    f32 mHeadLightOfxOffsetY;  // 0x8f4
    f32 mHeadLightOfxOffsetZ;  // 0x8f8
    f32 mHeadLightPrePassPointLightRadiusScale;  // 0x8fc
    f32 mHeadLightPrePassPointLightOffsetY;  // 0x900
    f32 mHeadLightPrePassPointLightOffsetZ;  // 0x904
    const PlayerConstParam* mOverrideParam;  // 0x908
    bool mIsOverride;                        // 0x910
};

#undef PLAYER_CONST_PARAM_DECLARE_GETTER
