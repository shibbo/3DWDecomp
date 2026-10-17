#include "Player/Normal/PlayerConstParam.hpp"
#include "Library/Yaml/ByamlIter.hpp"

/**
 * Sets every tuning value to its built-in default.
 */
PlayerConstParam::PlayerConstParam() : mOverrideParam(nullptr), mIsOverride(false) {
    mGravity = 1.2f;
    mCenterHeight = 80.0f;
    mBodyRadius = 40.0f;
    mCollectInfoRadiusAddition = 10.0f;
    mSnapGroundMaxLength = 30.0f;
    mSnapWallMaxLength = 30.0f;
    mStickRoundThreshold = 4.712389f;
    mHeightCheckLength = 1000.0f;
    mCutVelLimit = 0.0f;
    mCutVelRate = 0.5f;
    mThrowInvalidationFrames = 20;
    mTall = 160.0f;
    mChestRadius = 40.0f;
    mDashCheckRadius = 70.0f;
    mShadowCheckLength = 650.0f;
    mShadowLengthMax = 1250.0f;
    mPivotFrame = 6;
    mPivotDegree = 25.0f;
    mNormalMaxSpeed = 7.4f;
    mDashMaxSpeed = 11.0f;
    mSuperDashSpeed = 14.0f;
    mSuperDashTimer = 120;
    mSuperDashTimerMini = 120;
    mSuperDashTimerFire = 120;
    mSuperDashTimerClimb = 60;
    mSuperDashTimerRaccoonDog = 120;
    mSuperDashTimerBoomerang = 120;
    mSuperDashTimerRaccoonDogWhite = 120;
    mSuperDashStartAnimRate = 1.7f;
    mSuperDashStartAnimFrame = 50;
    mBrakeFrame = 2;
    mDashBrakeFrame = 10;
    mStickOnBrakeFrame = 120;
    mBrakeFrameOnIce = 90;
    mAccelFrame = 2;
    mDashAccelFrame = 120;
    mRoundLimitDegreeMax = 11.0f;
    mRoundLimitDegreeMin = 11.0f;
    mRunAnimRateMax = 2.6f;
    mGiantRunAnimRateMax = 2.6f;
    mShortAnimRateEff = 1.1f;
    mDashStartFrame = 30;
    mModifiedDashStartFrame = 30;
    mDashStartBlendFrame = 6;
    mDashInputSuccessFrame = 10;
    mDownHillAccelStartDegree = 10.0f;
    mDownHillAccelEndDegree = 60.0f;
    mDownHillAccelAddRate = 2.0f;
    mDashPanelSpeed = 20.0f;
    mModifiedDashPanelSpeed = 23.0f;
    mDashPanelOverRate = 2.0f;
    mDashPanelTimer = 90;
    mFlingPoleSpeed = 20.0f;
    mGroundOffFrame = 5;
    mClimbToGroundMoveFrame = 15;
    mTiltMaxDegree = 30.0f;
    mTiltBlendRate = 0.05f;
    mTiltMaxFrontAngle = 5.0f;
    mHoldingTiltMaxFrontAngle = 0.0f;
    mTiltStartSpeed = 9.6f;
    mTiltEndSpeed = 15.0f;
    mClimbRunAnimRateEff = 1.3f;
    mPanelDashAnimRate = 4.0f;
    mModifiedPanelDashAnimRate = 4.0f;
    mSlopeMaxSpeedScale = 0.55f;
    mMaxSpeedScale = 0.85f;
    mDashSignAnimRate = 4.5f;
    mDashSignMaxLoop = 2.0f;
    mDashSignMaxSpeed = 4.5f;
    mDashSignAnimFrameMax = 60;
    mJumpDirRotLimit = 11.25f;
    mJumpPowLow = 16.4f;
    mJumpPow = 18.4f;
    mJumpPowCountMax = 11;
    mJumpExtensionGravityRate = 0.0f;
    mJumpSideVelRate = 0.35f;
    mJumpFrontBrakeRate = 0.0253f;
    mJumpRotBlendRate = 0.106f;
    mJumpAccelFrame = 8;
    mReleaseAccelFrame = 2;
    mJumpAccelAddFrame = 60;
    mJumpCancelBrakeRate = 0.654f;
    mJumpCancelMinSpeed = 20.0f;
    mContinuousJumpTimer = 30;
    mContinuousJumpCount = 2;
    mFallSpeedMax = 22.5f;
    mDashJumpAddition = 0.1f;
    mPunchReflectPower = 17.0f;
    mGlideInhibitFrameAfterPunch = 30;
    mWalkMinSpeedRate = 0.5f;
    mJumpHVelSamplingNum = 20;
    mFollowFrontDamper = 0.98f;
    mFollowBackDamper = 0.98f;
    mFlightDurationCount = 35;
    mFlightDurationRotBlendRate = 0.05f;
    mFlightDurationJumpStartInhibitFrame = 10;
    mFlightDurationSideDamper = 0.841f;
    mRaccoonDogFallSpeedMax = 10.0f;
    mRaccoonDogFallGravityRate = 0.2f;
    mRaccoonDogFallGravityAdd = 0.0025f;
    mRaccoonDogFirstFallDamper = 0.25f;
    mRaccoonDogRotBlendRate = 0.293f;
    mRaccoonDogFallSideDamper = 0.707f;
    mTrampleJumpGravity = 0.95f;
    mTrampleJump = 12.0f;
    mTrampleJumpSideVelRate = 0.35f;
    mTrampleHighJumpGravity = 0.65f;
    mTrampleHighJump = 19.5f;
    mTrampleHipDropGravity = 0.95f;
    mTrampleHipDropJump = 20.0f;
    mRisingTrampleJumpGravity = 1.25f;
    mRisingTrampleJump = 22.5f;
    mRisingTrampleHighJumpGravity = 0.95f;
    mRisingTrampleHighJump = 24.5f;
    mRisingTrampleHVelBrakeRate = 0.316f;
    mPunchedJumpGravity = 0.8f;
    mPunchedJump = 20.0f;
    mRisingPunchedHVelBrakeRate = 0.316f;
    mTossedJumpGravity = 0.8f;
    mTossedJump = 31.0f;
    mTossedHighJumpGravity = 0.8f;
    mTossedHighJump = 31.0f;
    mTossedHipDropGravity = 0.8f;
    mTossedHipDropJump = 31.0f;
    mRisingTossedJumpGravity = 0.8f;
    mRisingTossedJump = 31.0f;
    mRisingTossedHighJumpGravity = 0.8f;
    mRisingTossedHighJump = 31.0f;
    mRisingTossedHVelBrakeRate = 0.316f;
    mSquatBrakeEndSpeed = 3.5f;
    mSquatShiftSpeedRate = 0.98f;
    mSquatAccelRate = 1.2f;
    mSquatNoBrakeFrame = 0;
    mSquatBrakeRate = 0.964f;
    mSquatBrakeRateOnSkate = 0.985f;
    mSquatBrakeSideAccel = 0.25f;
    mSquatBrakeSideBrakeRate = 0.93f;
    mSquatBrakeSideBrakeRateOnSkate = 0.985f;
    mSquatBrakeSideMaxSpeedRate = 0.5f;
    mSquatWalkSpeed = 3.5f;
    mSquatWalkFrontVecBlend = 0.894f;
    mSquatEnergyAccelFrame = 20;
    mSquatJumpGravity = 0.95f;
    mSquatJumpPow = 18.5f;
    mSquatHighJumpPow = 28.98f;
    mSquatJumpBackPow = 0.0f;
    mSquatJumpTramplePow = 15.0f;
    mHipDropSpeed = 37.5f;
    mHipDropLandCancelFrame = 24;
    mHipDropHeight = 40.0f;
    mHipDropMsgInterval = 10;
    mHipDropKnockDownRadiusMin = 150.0f;
    mHipDropKnockDownRadiusMax = 150.0f;
    mHipDropStartAnimRate = 1.3f;
    mHipDropKnockDownFrame = 1;
    mHipDropJumpPow = 28.0f;
    mHipDropJumpPowCountMax = 11;
    mHipDropJumpPermitBeginFrame = 10;
    mHipDropJumpPermitEndFrame = 30;
    mWallHeightLowLimit = 100.0f;
    mWallGravity = 0.6f;
    mWallMaxSpeed = 16.0f;
    mWallApartFrame = 20.0f;
    mWallSnapDistance = 100.0f;
    mWallSlideMaxSpeed = 5.0f;
    mWallSlideAccel = 0.125f;
    mWallInhibitAfterPunch = 10;
    mWallJumpGravity = 0.95f;
    mWallJumpHSpeed = 8.6f;
    mWallJumpPow = 23.0f;
    mWallJumpInvalidateInputFrame = 20;
    mWallJumpDirEffectiveFrame = 20;
    mWallJumpDirLimit = 60.0f;
    mWallJumpLimitPlay = 20.0f;
    mWallClimbReadyFrame = 120;
    mWallClimbReadyWaitFrame = 3;
    mWallClimbReadyFromSlopeFrame = 10;
    mWallClimbAccel = 2.0f;
    mWallClimbMaxSpeed = 5.0f;
    mWallClimbDashMaxSpeed = 7.0f;
    mWallClimbFrame = 150;
    mWallClimbFrame1 = 68;
    mWallClimbFrame2 = 0;
    mWallClimbDashFrame = 117;
    mWallClimbDashFrame1 = 59;
    mWallClimbDashFrame2 = 0;
    mWallClimbStopFrame = 110;
    mWallClimbBrakeRate = 0.8f;
    mWallClimbNoWallFrame = 4;
    mWallClimbMaxSideSpeed = 4.0f;
    mWallClimbDashMaxSideSpeed = 5.3f;
    mWallClimbSideAccel = 2.0f;
    mWallClimbRetainThreshold = 60.0f;
    mWallClimbInvalidSideMoveDegree = 5.0f;
    mWallClimbDashAnimRate = 1.2f;
    mWallClimbNormalAnimRate = 0.9f;
    mWallClimbJumpPowCountMax = 0;
    mWallClimbJumpDashAddition = 0.0f;
    mWallClimbJumpPowLow = 18.0f;
    mWallClimbJumpPow = 18.0f;
    mWallClimbSlideGravity = 0.3f;
    mWallClimbSlideMaxSpeed = 16.0f;
    mWallClimbSlideSideAccel = 0.1f;
    mWallClimbSlideSideMaxSpeed = 2.0f;
    mWallClimbSlideStartBrakeRate = 0.9f;
    mClimbAirStopFrame = 20;
    mClimbAirStopRotateVelMax = 15.0f;
    mLongJumpSuccessSpeed = 6.0f;
    mLongJumpSpeedMin = 2.5f;
    mLongJumpBrake = 0.5f;
    mLongJumpSideAccel = 0.125f;
    mLongJumpCancelFrame = 40;
    mLongJumpFastSuccessFrame = 20;
    mLongJumpFastGravity = 0.5f;
    mLongJumpFastJumpPow = 13.0f;
    mLongJumpFastSpeed = 13.5f;
    mLongJumpSlowGravity = 0.75f;
    mLongJumpSlowJumpPow = 15.5f;
    mLongJumpSlowSpeed = 10.5f;
    mDashBrakeCommandFrame = 30;
    mDashBrakeActionFrame = 8;
    mDashBrakeSpeed = 7.5f;
    mTurnJumpGravity = 0.8f;
    mTurnJumpPow = 25.53f;
    mTurnJumpVelH = 4.5f;
    mTurnJumpBrake = 0.5f;
    mTurnJumpAccel = 0.25f;
    mTurnJumpSideAccel = 0.075f;
    mTurnJumpToFlightDurationFrame = 50;
    mWaitRollingMinSpeed = 10.0f;
    mWaitRollingNoBrakeFrame = 10;
    mWaitRollingBrakeRate = 0.949f;
    mWaitRollingSideBrakeRate = 0.949f;
    mWaitRollingSideAccel = 0.125f;
    mWaitRollingSideMaxSpeed = 7.5f;
    mNormalRollingMinSpeed = 12.5f;
    mNormalRollingNoBrakeFrame = 30;
    mNormalRollingBrakeRate = 0.949f;
    mNormalRollingSideBrakeRate = 0.949f;
    mNormalRollingSideAccel = 0.125f;
    mNormalRollingSideMaxSpeed = 7.5f;
    mAirRollingMinSpeed = 12.5f;
    mAirRollingNoBrakeFrame = 30;
    mAirRollingBrakeRate = 0.949f;
    mAirRollingSideBrakeRate = 0.949f;
    mAirRollingSideAccel = 0.125f;
    mAirRollingSideMaxSpeed = 7.5f;
    mAirRollingJumpPow = 15.0f;
    mAirRollingGravity = 1.0f;
    mRollingMinSpeed = 14.0f;
    mRollingNoBrakeFrame = 40;
    mRollingBrakeRate = 0.949f;
    mRollingSideBrakeRate = 0.949f;
    mRollingSideAccel = 0.25f;
    mRollingSideMaxSpeed = 2.5f;
    mRaccoonDogWaitRollingMinSpeed = 0.0f;
    mRaccoonDogWaitRollingNoBrakeFrame = 0;
    mRaccoonDogWaitRollingBrakeRate = 1.0f;
    mRaccoonDogWaitRollingSideBrakeRate = 1.0f;
    mRaccoonDogWaitRollingSideAccel = 0.0f;
    mRaccoonDogWaitRollingSideMaxSpeed = 0.0f;
    mRaccoonDogNormalRollingMinSpeed = 12.5f;
    mRaccoonDogNormalRollingNoBrakeFrame = 0;
    mRaccoonDogNormalRollingBrakeRate = 0.949f;
    mRaccoonDogNormalRollingSideBrakeRate = 0.949f;
    mRaccoonDogNormalRollingSideAccel = 1.25f;
    mRaccoonDogNormalRollingSideMaxSpeed = 2.5f;
    mRaccoonDogDashRollingMinSpeed = 14.0f;
    mRaccoonDogDashRollingNoBrakeFrame = 24;
    mRaccoonDogDashRollingBrakeRate = 0.949f;
    mRaccoonDogDashRollingSideBrakeRate = 0.949f;
    mRaccoonDogDashRollingSideAccel = 0.25f;
    mRaccoonDogDashRollingSideMaxSpeed = 2.5f;
    mRollingTramplePow = 15.0f;
    mWaitRollingAttackJumpGravity = 0.5f;
    mWaitRollingAttackJumpPow = 13.0f;
    mWaitRollingAttackVelH = 9.5f;
    mNormalRollingAttackJumpGravity = 0.5f;
    mNormalRollingAttackJumpPow = 12.5f;
    mNormalRollingAttackVelH = 13.5f;
    mRollingAttackJumpGravity = 0.5f;
    mRollingAttackJumpPow = 13.0f;
    mRollingAttackVelH = 15.0f;
    mRaccoonDogWaitRollingAttackJumpGravity = 0.5f;
    mRaccoonDogWaitRollingAttackJumpPow = 13.0f;
    mRaccoonDogWaitRollingAttackVelH = 0.0f;
    mRaccoonDogWaitRollingAttackHighJumpGravity = 0.5f;
    mRaccoonDogWaitRollingAttackHighJumpPow = 17.5f;
    mRaccoonDogWaitRollingAttackHighVelH = 0.0f;
    mCommonRollingAttackSpeedMin = 2.5f;
    mCommonRollingAttackBrake = 0.25f;
    mCommonRollingAttackSideAccel = 0.125f;
    mRollingHitBound = 1.25f;
    mWallHitLandCancelFrame = 30;
    mDamageInvalidCount = 180;
    mDamageCancelFrame = 50;
    mInvincibleFrame = 700;
    mInvincibleDashFrame = 60;
    mInvincibleDashSpeed = 13.0f;
    mInvincibleJumpPow = 15.0f;
    mInvincibleJumpPowCountMax = 10;
    mTailAttackStart = 0;
    mTailAttackFrame = 14;
    mTailAttackInterval = 14;
    mStandSwimRisePower = 1.0f;
    mStandSwimRiseSpeedMax = 5.5f;
    mStandSwimGravity = 0.125f;
    mStandSwimFallSpeedMax = 5.0f;
    mStandSwimHorizontalFloorDashAccel = 0.12f;
    mStandSwimHorizontalFloorDashSpeedMax = 5.0f;
    mStandSwimHorizontalFloorAccel = 0.125f;
    mStandSwimHorizontalFloorSpeedMax = 3.5f;
    mNoSinkSwimHorizontalHighAccel = 0.25f;
    mNoSinkSwimHorizontalHighInputMin = 0.3f;
    mNoSinkSwimHorizontalHighSpeedMax = 8.0f;
    mNoSinkSwimHorizontalHighSpeedMin = 4.0f;
    mStandSwimHorizontalHighAccel = 0.125f;
    mStandSwimHorizontalHighSpeedMax = 6.0f;
    mStandSwimHorizontalLowAccel = 0.125f;
    mStandSwimHorizontalLowSpeedMax = 5.0f;
    mStandSwimHorizontalBrakeRate = 0.975f;
    mStandSwimHighAccelPermitFrame = 30;
    mStandSwimForwardBentDegree = 30.0f;
    mStandSwimForwardBentBlend = 0.0513f;
    mStandSwimFlowFieldBlend = 0.776f;
    mStandSwimRotSpeed = 7.5f;
    mStandSwimSurfaceRotSpeed = 4.0f;
    mStandSwimSurfaceRotSpeedNoMovement = 25.0f;
    mStandSwimWalkAnimMinRate = 0.2f;
    mStandSwimWalkAnimMaxRate = 1.9f;
    mStandSwimWalkMaxSpeed = 5.0f;
    mStandSwimPaddleAnimInterval = 32;
    mStandSwimPaddleAnimRateIntervalMax = 22;
    mStandSwimPaddleAnimRateIntervalMin = 5;
    mStandSwimPaddleAnimMaxRate = 3.0f;
    mSwimHRotSpeed = 2.0f;
    mSwimVRotSpeed = 2.0f;
    mSwimPaddleAccel = 0.75f;
    mSwimPaddleSpeedMax = 12.0f;
    mSwimPaddleFrame = 20;
    mSwimKickAccel = 0.25f;
    mSwimKickSpeedMax = 6.0f;
    mSwimKickBrake = 0.99f;
    mSwimBrake = 0.975f;
    mSwimSideBrake = 0.949f;
    mStandSwimFromDiveTimer = 30;
    mStandSwimFromDiveRisePower = 4.0f;
    mStandSwimFromDiveRisePowerClimb = 2.0f;
    mSwimDiveStartSpeed = 26.5f;
    mSwimDiveBrake = 0.875f;
    mSwimDiveEndSpeed = 2.5f;
    mSwimDiveLandCount = 6;
    mSwimDiveLandCancelFrame = 10;
    mSwimDiveButtonValidFrame = 15;
    mDiveStartSpeed = 26.5f;
    mDiveBrake = 0.875f;
    mDiveBrakeSingleMode = 1.5f;
    mDiveEndSpeed = 2.5f;
    mStandSwimTramplePower = 8.0f;
    mDiveTramplePower = 11.0f;
    mDiveTrampleCancelFrame = 20.0f;
    mSwimSurfaceStartDist = 160.0f;
    mSwimSurfaceEndDist = 250.0f;
    mSwimSurfaceStartDistShort = 150.0f;
    mSwimSurfaceEndDistShort = 200.0f;
    mSwimSurfaceVelDamper = 0.949f;
    mSwimSurfaceGravity = 0.125f;
    mSwimSurfaceValidDamperFrame = 24;
    mSwimSurfaceDamperLerpFrame = 26;
    mSwimSurfaceBaseHeight = 80.0f;
    mSwimSurfaceBaseHeightShort = 50.0f;
    mSwimSurfaceSpring = 0.05f;
    mSwimSurfaceVerticalOffset = 16.0f;
    mSwimSurfacePivotRate = 0.01f;
    mSwimSurfacePivotCancelAngle = 0.8f;
    mSwimSurfaceSpeedThreshold = 5.0f;
    mSwimSurfacePivotCounter = 3;
    mSwimSurfaceTiltDuringPivotMaxDegree = 60.0f;
    mSwimSurfaceTiltMaxDegree = 60.0f;
    mSwimSurfaceTiltMaxFrontAngle = 30.0f;
    mSwimSurfaceClimbAnimationRate = 2.2f;
    mSwimSurfaceSpringForSurfaceSwim = 0.012f;
    mSwimSurfaceSpringForSurfaceSwimClimb = 0.009f;
    mSwimJumpPow = 25.34f;
    mSwimSquatInhibitFrame = 30;
    mPropellerRisePow = 2.475f;
    mPropellerPowSustain = 24;
    mPropellerPowSustainMin = 16;
    mPropellerPowRelease = 2;
    mPropellerBeforeDropGravity = 0.95f;
    mPropellerRiseGravity = 0.95f;
    mPropellerAfterDropGravity = 0.0375f;
    mPropellerFallSpeedMax = 12.5f;
    mPropellerButtonOffFallSpeedMax = 20.0f;
    mPropellerEngineBrakeVel = 0.0f;
    mPropellerEngineBrakeRate = 0.0f;
    mPropellerEngineBrakeEndVel = 1.0f;
    mPropellerRotBlendRate = 0.194f;
    mPropellerSideDamper = 0.163f;
    mPropellerStickOffBrakeRate = 0.949f;
    mLongFallDistance = 1500.0f;
    mStatueFallStartFrame = 28;
    mStatueLandFrame = 30;
    mStatueEndFrame = 480;
    mStatueEndAnimStep = 330;
    mStatueFallSpeedInWater = 12.5f;
    mSlideSlopeAngle = 26.0f;
    mSlideSlopeEndAngle = 10.0f;
    mSlideEndSpeed = 3.0f;
    mSlideAccel = 0.3f;
    mSlideMaxSpeed = 30.0f;
    mSlideSideBrake = 0.99f;
    mSlideSideAccel = 0.8f;
    mSlideSideMaxSpeed = 10.0f;
    mSlideSideAccelOnLevelLand = 0.5f;
    mSlideSideMaxSpeedOnLevelLand = 5.0f;
    mSlideBrake = 0.97f;
    mForceSlideBrake = 0.98f;
    mSlidePostureBlendRate = 0.1f;
    mForceSlideSpeed = 5.0f;
    mForceSlideSpeedUpRate = 1.5f;
    mSlideTiltBlendRate = 0.05f;
    mSlideTiltMaxDegree = 30.0f;
    mSlideInvalidFrame = 15;
    mForceSlideMaxSpeed = 15.0f;
    mSlideFallCancelFrame = 30;
    mSlideJumpHVelScale = 0.6f;
    mHoldShakeInterval = 15;
    mHoldThrowFrontTiming = 3;
    mHoldThrowUpTiming = 3;
    mHoldJumpFrontVel = 11.0f;
    mHoldJumpUpVel = 18.0f;
    mClimbAttackInterval = 14;
    mClimbAttackWaitInterval = 30;
    mClimbAttackCancelFrame = 18;
    mClimbAttackSensorOnFrame = 10;
    mClimbBodyAttackFrontVel = 20.0f;
    mClimbBodyAttackDownVel = 17.0f;
    mClimbBodyAttackFrame = 90;
    mClimbBodyAttackGravity = 1.2f;
    mClimbBodyAttackFallSpeedMax = 30.0f;
    mClimbBodyAttackHBrakeRate = 0.5f;
    mClimbBodyAttackSideAccel = 0.0f;
    mClimbBodyAttackSideMoveDist = 300.0f;
    mSinkSandMoveMaxSpeed = 4.0f;
    mSinkSandMoveMaxDashSpeed = 6.0f;
    mSinkSandInvalidFrameInJump = 6;
    mPushedBrakeRate = 0.96f;
    mPushedBrakeMaxRate = 0.95f;
    mPushedJumpCancelSpeed = 3.0f;
    mGroundSpinFrame = 60;
    mGroundSpinAccel = 0.5f;
    mGroundSpinBrake = 0.95f;
    mGroundSpinVelMax = 10.0f;
    mSpinJumpGravity = 0.4f;
    mSpinJumpPow = 20.0f;
    mSpinAttackInterval = 14;
    mSpinAttackCancelFrame = 30;
    mSpinAttackSensorOnFrame = 20;
    mSpinAttackJumpPow = 15.0f;
    mSpinAttackJumpGravity = 0.8f;
    mSpinAttackGroundBrake = 0.0f;
    mSkateJumpGravity = 0.8f;
    mSkateJumpPowLow = 16.4f;
    mSkateJumpPow = 18.4f;
    mSkateJumpPowCountMax = 11;
    mSkateJumpThreshold = 5.0f;
    mCoopHipDropFrame = 10;
    mCoopHipDropRadiusMin = 100.0f;
    mCoopHipDropRadius = 1000.0f;
    mGiantHipDropFrame = 10;
    mGiantHipDropRadiusMin = 100.0f;
    mGiantHipDropRadiusMax = 1000.0f;
    mKnockDownVelH = 5.0f;
    mKnockDownVelV = 10.0f;
    mKnockDownCancelFrame = 15;
    mReflectJumpGravity = 1.2f;
    mReflectJump = 30.0f;
    mRisingReflectJumpHVelBrakeRate = 0.316f;
    mTossCancelFrame = 25;
    mManekinekoFallStartFrame = 28;
    mManekinekoLandFrame = 30;
    mManekinekoEndNoticeFrame = 330;
    mManekinekoEndFrame = 480;
    mManekinekoCancelFrame = 30;
    mManekinekoFallSpeedInWater = 12.5f;
    mGroomingMaxInterval = 1200;
    mGroomingMinInterval = 300;
    mSePropellerBeginStep = 22;
    mSeFootNoteNormalVolMul = 0.6f;
    mSeFootNoteNormalPitDec = 0.12f;
    mSeFootNoteDashVolAdd = 0.35f;
    mSeFootNoteDashPitAdd = 0.01f;
    mGigaCommonAnimRate = 0.65f;
    mGigaMiniRunAnimRateMax = 2.5f;
    mGigaMiniDashAnimRateMax = 2.5f;
    mGigaSuperRunAnimRateMax = 2.2f;
    mGigaSuperDashAnimRateMax = 2.2f;
    mGigaClimbRunAnimRateMax = 1.8f;
    mGigaClimbDashAnimRateMax = 2.2f;
    mGigaNormalMaxSpeed = 145.0f;
    mGigaDashMaxSpeed = 175.0f;
    mGigaSuperDashSpeed = 175.0f;
    mGigaInvincibleDashSpeed = 36.0f;
    mGigaAccelFrame = 15;
    mGigaSquatWalkSpeed = 90.0f;
    mGigaGroundSpinAccel = 7.0f;
    mGigaGroundSpinBrake = 0.9f;
    mGigaKnockDownVelH = 50.0f;
    mGigaKnockDownVelV = 50.0f;
    mGigaLeftFootHrTime = 70.0f;
    mGigaRightFootHrTime = 10.0f;
    mGigaClimbLeftWalkHrTime = 50.0f;
    mGigaClimbRightWalkHrTime = 100.0f;
    mGigaClimbLeftRunHrTime = 50.0f;
    mGigaClimbRightRunHrTime = 100.0f;
    mGigaRoundLimitDegreeMax = 6.0f;
    mGigaRoundLimitDegreeMin = 6.0f;
    mGigaNormalRollingMinSpeed = 230.0f;
    mGigaNormalGravityAddition = 40.0f;
    mGigaSquatBrakeRate = 0.94f;
    mGigaGravity = 12.0f;
    mGigaFallSpeedMax = 1200.0f;
    mGigaFloatFallSpeedMax = 320.0f;
    mGigaJumpPow = 250.0f;
    mGigaJumpPowLow = 250.0f;
    mGigaJumpPowCountMax = 20;
    mGigaJumpCancelBrakeRate = 0.9f;
    mGigaJumpCancelMinSpeed = 400.0f;
    mGigaHipDropSpeed = 480.0f;
    mGigaHipDropAnimRate = 0.9f;
    mGigaHipDropJumpPow = 450.0f;
    mGigaTrampleJump = 160.0f;
    mGigaLongJumpSlowSpeed = 170.0f;
    mGigaLongJumpFastSpeed = 170.0f;
    mGigaLongJumpSlowJumpPow = 170.0f;
    mGigaLongJumpFastJumpPow = 170.0f;
    mGigaLongJumpSlowGravity = 5.0f;
    mGigaLongJumpFastGravity = 5.0f;
    mGigaSquatJumpGravity = 8.5f;
    mGigaSquatJumpPow = 290.0f;
    mGigaSquatHighJumpPow = 380.0f;
    mGigaSquatJumpBackPow = 0.0f;
    mGigaSpinJumpGravity = 6.0f;
    mGigaSpinJumpPow = 300.0f;
    mGigaNormalRollingAttackJumpGravity = 7.0f;
    mGigaNormalRollingAttackJumpPow = 190.0f;
    mGigaNormalRollingAttackVelH = 260.0f;
    mGigaWallJumpHSpeed = 125.0f;
    mGigaLongJumpBrake = 5.0f;
    mGigaLongJumpSpeedMin = 25.0f;
    mGigaLongJumpSideAccel = 1.25f;
    mGigaLandFrame = 12;
    mGigaWallClimbMaxSpeed = 150.0f;
    mGigaWallClimbDashMaxSpeed = 200.0f;
    mGigaWallClimbAccel = 30.0f;
    mGigaWallClimbMaxSideSpeed = 120.0f;
    mGigaWallClimbDashMaxSideSpeed = 160.0f;
    mGigaWallClimbSideAccel = 30.0f;
    mGigaWallSnapDistance = 1400.0f;
    mGigaWallClimbJumpPowLow = 300.0f;
    mGigaWallClimbJumpPow = 300.0f;
    mGigaWallClimbSlideGravity = 1.3f;
    mGigaWallClimbSlideMaxSpeed = 150.0f;
    mGigaWallClimbSlideSideAccel = 1.5f;
    mGigaWallClimbSlideSideMaxSpeed = 30.0f;
    mFlashRangeAttackLengthOffset = 100.0f;
    mFlashRangeAttackDegreeScale = 0.7f;
    mFlashRangeAttackHeightMax = 0.0f;
    mFlashRangeAttackHeightMin = -200.0f;
    mIsEnableHeadLightOfx = 0;
    mHeadLightOfxOffsetY = 50.0f;
    mHeadLightOfxOffsetZ = 50.0f;
    mHeadLightPrePassPointLightRadiusScale = 0.7f;
    mHeadLightPrePassPointLightOffsetY = -35.0f;
    mHeadLightPrePassPointLightOffsetZ = -200.0f;
}

/**
 * Sets every tuning value to its default, then overrides it with the entry of the same name
 * in the given BYAML dictionary when there is one.
 * @param rIter the PlayerConst dictionary
 */
PlayerConstParam::PlayerConstParam(const al::ByamlIter& rIter)
    : mOverrideParam(nullptr), mIsOverride(false) {
    mGravity = 1.2f;
    rIter.tryGetFloatByKey(&mGravity, "Gravity");

    mCenterHeight = 80.0f;
    rIter.tryGetFloatByKey(&mCenterHeight, "CenterHeight");

    mBodyRadius = 40.0f;
    rIter.tryGetFloatByKey(&mBodyRadius, "BodyRadius");

    mCollectInfoRadiusAddition = 10.0f;
    rIter.tryGetFloatByKey(&mCollectInfoRadiusAddition, "CollectInfoRadiusAddition");

    mSnapGroundMaxLength = 30.0f;
    rIter.tryGetFloatByKey(&mSnapGroundMaxLength, "SnapGroundMaxLength");

    mSnapWallMaxLength = 30.0f;
    rIter.tryGetFloatByKey(&mSnapWallMaxLength, "SnapWallMaxLength");

    mStickRoundThreshold = 4.712389f;
    rIter.tryGetFloatByKey(&mStickRoundThreshold, "StickRoundThreshold");

    mHeightCheckLength = 1000.0f;
    rIter.tryGetFloatByKey(&mHeightCheckLength, "HeightCheckLength");

    mCutVelLimit = 0.0f;
    rIter.tryGetFloatByKey(&mCutVelLimit, "CutVelLimit");

    mCutVelRate = 0.5f;
    rIter.tryGetFloatByKey(&mCutVelRate, "CutVelRate");

    mThrowInvalidationFrames = 20;
    rIter.tryGetIntByKey(&mThrowInvalidationFrames, "ThrowInvalidationFrames");

    mTall = 160.0f;
    rIter.tryGetFloatByKey(&mTall, "Tall");

    mChestRadius = 40.0f;
    rIter.tryGetFloatByKey(&mChestRadius, "ChestRadius");

    mDashCheckRadius = 70.0f;
    rIter.tryGetFloatByKey(&mDashCheckRadius, "DashCheckRadius");

    mShadowCheckLength = 650.0f;
    rIter.tryGetFloatByKey(&mShadowCheckLength, "ShadowCheckLength");

    mShadowLengthMax = 1250.0f;
    rIter.tryGetFloatByKey(&mShadowLengthMax, "ShadowLengthMax");

    mPivotFrame = 6;
    rIter.tryGetIntByKey(&mPivotFrame, "PivotFrame");

    mPivotDegree = 25.0f;
    rIter.tryGetFloatByKey(&mPivotDegree, "PivotDegree");

    mNormalMaxSpeed = 7.4f;
    rIter.tryGetFloatByKey(&mNormalMaxSpeed, "NormalMaxSpeed");

    mDashMaxSpeed = 11.0f;
    rIter.tryGetFloatByKey(&mDashMaxSpeed, "DashMaxSpeed");

    mSuperDashSpeed = 14.0f;
    rIter.tryGetFloatByKey(&mSuperDashSpeed, "SuperDashSpeed");

    mSuperDashTimer = 120;
    rIter.tryGetIntByKey(&mSuperDashTimer, "SuperDashTimer");

    mSuperDashTimerMini = 120;
    rIter.tryGetIntByKey(&mSuperDashTimerMini, "SuperDashTimerMini");

    mSuperDashTimerFire = 120;
    rIter.tryGetIntByKey(&mSuperDashTimerFire, "SuperDashTimerFire");

    mSuperDashTimerClimb = 60;
    rIter.tryGetIntByKey(&mSuperDashTimerClimb, "SuperDashTimerClimb");

    mSuperDashTimerRaccoonDog = 120;
    rIter.tryGetIntByKey(&mSuperDashTimerRaccoonDog, "SuperDashTimerRaccoonDog");

    mSuperDashTimerBoomerang = 120;
    rIter.tryGetIntByKey(&mSuperDashTimerBoomerang, "SuperDashTimerBoomerang");

    mSuperDashTimerRaccoonDogWhite = 120;
    rIter.tryGetIntByKey(&mSuperDashTimerRaccoonDogWhite, "SuperDashTimerRaccoonDogWhite");

    mSuperDashStartAnimRate = 1.7f;
    rIter.tryGetFloatByKey(&mSuperDashStartAnimRate, "SuperDashStartAnimRate");

    mSuperDashStartAnimFrame = 50;
    rIter.tryGetIntByKey(&mSuperDashStartAnimFrame, "SuperDashStartAnimFrame");

    mBrakeFrame = 2;
    rIter.tryGetIntByKey(&mBrakeFrame, "BrakeFrame");

    mDashBrakeFrame = 10;
    rIter.tryGetIntByKey(&mDashBrakeFrame, "DashBrakeFrame");

    mStickOnBrakeFrame = 120;
    rIter.tryGetIntByKey(&mStickOnBrakeFrame, "StickOnBrakeFrame");

    mBrakeFrameOnIce = 90;
    rIter.tryGetIntByKey(&mBrakeFrameOnIce, "BrakeFrameOnIce");

    mAccelFrame = 2;
    rIter.tryGetIntByKey(&mAccelFrame, "AccelFrame");

    mDashAccelFrame = 120;
    rIter.tryGetIntByKey(&mDashAccelFrame, "DashAccelFrame");

    mRoundLimitDegreeMax = 11.0f;
    rIter.tryGetFloatByKey(&mRoundLimitDegreeMax, "RoundLimitDegreeMax");

    mRoundLimitDegreeMin = 11.0f;
    rIter.tryGetFloatByKey(&mRoundLimitDegreeMin, "RoundLimitDegreeMin");

    mRunAnimRateMax = 2.6f;
    rIter.tryGetFloatByKey(&mRunAnimRateMax, "RunAnimRateMax");

    mGiantRunAnimRateMax = 2.6f;
    rIter.tryGetFloatByKey(&mGiantRunAnimRateMax, "GiantRunAnimRateMax");

    mShortAnimRateEff = 1.1f;
    rIter.tryGetFloatByKey(&mShortAnimRateEff, "ShortAnimRateEff");

    mDashStartFrame = 30;
    rIter.tryGetIntByKey(&mDashStartFrame, "DashStartFrame");

    mModifiedDashStartFrame = 30;
    rIter.tryGetIntByKey(&mModifiedDashStartFrame, "ModifiedDashStartFrame");

    mDashStartBlendFrame = 6;
    rIter.tryGetIntByKey(&mDashStartBlendFrame, "DashStartBlendFrame");

    mDashInputSuccessFrame = 10;
    rIter.tryGetIntByKey(&mDashInputSuccessFrame, "DashInputSuccessFrame");

    mDownHillAccelStartDegree = 10.0f;
    rIter.tryGetFloatByKey(&mDownHillAccelStartDegree, "DownHillAccelStartDegree");

    mDownHillAccelEndDegree = 60.0f;
    rIter.tryGetFloatByKey(&mDownHillAccelEndDegree, "DownHillAccelEndDegree");

    mDownHillAccelAddRate = 2.0f;
    rIter.tryGetFloatByKey(&mDownHillAccelAddRate, "DownHillAccelAddRate");

    mDashPanelSpeed = 20.0f;
    rIter.tryGetFloatByKey(&mDashPanelSpeed, "DashPanelSpeed");

    mModifiedDashPanelSpeed = 23.0f;
    rIter.tryGetFloatByKey(&mModifiedDashPanelSpeed, "ModifiedDashPanelSpeed");

    mDashPanelOverRate = 2.0f;
    rIter.tryGetFloatByKey(&mDashPanelOverRate, "DashPanelOverRate");

    mDashPanelTimer = 90;
    rIter.tryGetIntByKey(&mDashPanelTimer, "DashPanelTimer");

    mFlingPoleSpeed = 20.0f;
    rIter.tryGetFloatByKey(&mFlingPoleSpeed, "FlingPoleSpeed");

    mGroundOffFrame = 5;
    rIter.tryGetIntByKey(&mGroundOffFrame, "GroundOffFrame");

    mClimbToGroundMoveFrame = 15;
    rIter.tryGetIntByKey(&mClimbToGroundMoveFrame, "ClimbToGroundMoveFrame");

    mTiltMaxDegree = 30.0f;
    rIter.tryGetFloatByKey(&mTiltMaxDegree, "TiltMaxDegree");

    mTiltBlendRate = 0.05f;
    rIter.tryGetFloatByKey(&mTiltBlendRate, "TiltBlendRate");

    mTiltMaxFrontAngle = 5.0f;
    rIter.tryGetFloatByKey(&mTiltMaxFrontAngle, "TiltMaxFrontAngle");

    mHoldingTiltMaxFrontAngle = 0.0f;
    rIter.tryGetFloatByKey(&mHoldingTiltMaxFrontAngle, "HoldingTiltMaxFrontAngle");

    mTiltStartSpeed = 9.6f;
    rIter.tryGetFloatByKey(&mTiltStartSpeed, "TiltStartSpeed");

    mTiltEndSpeed = 15.0f;
    rIter.tryGetFloatByKey(&mTiltEndSpeed, "TiltEndSpeed");

    mClimbRunAnimRateEff = 1.3f;
    rIter.tryGetFloatByKey(&mClimbRunAnimRateEff, "ClimbRunAnimRateEff");

    mPanelDashAnimRate = 4.0f;
    rIter.tryGetFloatByKey(&mPanelDashAnimRate, "PanelDashAnimRate");

    mModifiedPanelDashAnimRate = 4.0f;
    rIter.tryGetFloatByKey(&mModifiedPanelDashAnimRate, "ModifiedPanelDashAnimRate");

    mSlopeMaxSpeedScale = 0.55f;
    rIter.tryGetFloatByKey(&mSlopeMaxSpeedScale, "SlopeMaxSpeedScale");

    mMaxSpeedScale = 0.85f;
    rIter.tryGetFloatByKey(&mMaxSpeedScale, "MaxSpeedScale");

    mDashSignAnimRate = 4.5f;
    rIter.tryGetFloatByKey(&mDashSignAnimRate, "DashSignAnimRate");

    mDashSignMaxLoop = 2.0f;
    rIter.tryGetFloatByKey(&mDashSignMaxLoop, "DashSignMaxLoop");

    mDashSignMaxSpeed = 4.5f;
    rIter.tryGetFloatByKey(&mDashSignMaxSpeed, "DashSignMaxSpeed");

    mDashSignAnimFrameMax = 60;
    rIter.tryGetIntByKey(&mDashSignAnimFrameMax, "DashSignAnimFrameMax");

    mJumpDirRotLimit = 11.25f;
    rIter.tryGetFloatByKey(&mJumpDirRotLimit, "JumpDirRotLimit");

    mJumpPowLow = 16.4f;
    rIter.tryGetFloatByKey(&mJumpPowLow, "JumpPowLow");

    mJumpPow = 18.4f;
    rIter.tryGetFloatByKey(&mJumpPow, "JumpPow");

    mJumpPowCountMax = 11;
    rIter.tryGetIntByKey(&mJumpPowCountMax, "JumpPowCountMax");

    mJumpExtensionGravityRate = 0.0f;
    rIter.tryGetFloatByKey(&mJumpExtensionGravityRate, "JumpExtensionGravityRate");

    mJumpSideVelRate = 0.35f;
    rIter.tryGetFloatByKey(&mJumpSideVelRate, "JumpSideVelRate");

    mJumpFrontBrakeRate = 0.0253f;
    rIter.tryGetFloatByKey(&mJumpFrontBrakeRate, "JumpFrontBrakeRate");

    mJumpRotBlendRate = 0.106f;
    rIter.tryGetFloatByKey(&mJumpRotBlendRate, "JumpRotBlendRate");

    mJumpAccelFrame = 8;
    rIter.tryGetIntByKey(&mJumpAccelFrame, "JumpAccelFrame");

    mReleaseAccelFrame = 2;
    rIter.tryGetIntByKey(&mReleaseAccelFrame, "ReleaseAccelFrame");

    mJumpAccelAddFrame = 60;
    rIter.tryGetIntByKey(&mJumpAccelAddFrame, "JumpAccelAddFrame");

    mJumpCancelBrakeRate = 0.654f;
    rIter.tryGetFloatByKey(&mJumpCancelBrakeRate, "JumpCancelBrakeRate");

    mJumpCancelMinSpeed = 20.0f;
    rIter.tryGetFloatByKey(&mJumpCancelMinSpeed, "JumpCancelMinSpeed");

    mContinuousJumpTimer = 30;
    rIter.tryGetIntByKey(&mContinuousJumpTimer, "ContinuousJumpTimer");

    mContinuousJumpCount = 2;
    rIter.tryGetIntByKey(&mContinuousJumpCount, "ContinuousJumpCount");

    mFallSpeedMax = 22.5f;
    rIter.tryGetFloatByKey(&mFallSpeedMax, "FallSpeedMax");

    mDashJumpAddition = 0.1f;
    rIter.tryGetFloatByKey(&mDashJumpAddition, "DashJumpAddition");

    mPunchReflectPower = 17.0f;
    rIter.tryGetFloatByKey(&mPunchReflectPower, "PunchReflectPower");

    mGlideInhibitFrameAfterPunch = 30;
    rIter.tryGetIntByKey(&mGlideInhibitFrameAfterPunch, "GlideInhibitFrameAfterPunch");

    mWalkMinSpeedRate = 0.5f;
    rIter.tryGetFloatByKey(&mWalkMinSpeedRate, "WalkMinSpeedRate");

    mJumpHVelSamplingNum = 20;
    rIter.tryGetIntByKey(&mJumpHVelSamplingNum, "JumpHVelSamplingNum");

    mFollowFrontDamper = 0.98f;
    rIter.tryGetFloatByKey(&mFollowFrontDamper, "FollowFrontDamper");

    mFollowBackDamper = 0.98f;
    rIter.tryGetFloatByKey(&mFollowBackDamper, "FollowBackDamper");

    mFlightDurationCount = 35;
    rIter.tryGetIntByKey(&mFlightDurationCount, "FlightDurationCount");

    mFlightDurationRotBlendRate = 0.05f;
    rIter.tryGetFloatByKey(&mFlightDurationRotBlendRate, "FlightDurationRotBlendRate");

    mFlightDurationJumpStartInhibitFrame = 10;
    rIter.tryGetIntByKey(&mFlightDurationJumpStartInhibitFrame,
                         "FlightDurationJumpStartInhibitFrame");

    mFlightDurationSideDamper = 0.841f;
    rIter.tryGetFloatByKey(&mFlightDurationSideDamper, "FlightDurationSideDamper");

    mRaccoonDogFallSpeedMax = 10.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogFallSpeedMax, "RaccoonDogFallSpeedMax");

    mRaccoonDogFallGravityRate = 0.2f;
    rIter.tryGetFloatByKey(&mRaccoonDogFallGravityRate, "RaccoonDogFallGravityRate");

    mRaccoonDogFallGravityAdd = 0.0025f;
    rIter.tryGetFloatByKey(&mRaccoonDogFallGravityAdd, "RaccoonDogFallGravityAdd");

    mRaccoonDogFirstFallDamper = 0.25f;
    rIter.tryGetFloatByKey(&mRaccoonDogFirstFallDamper, "RaccoonDogFirstFallDamper");

    mRaccoonDogRotBlendRate = 0.293f;
    rIter.tryGetFloatByKey(&mRaccoonDogRotBlendRate, "RaccoonDogRotBlendRate");

    mRaccoonDogFallSideDamper = 0.707f;
    rIter.tryGetFloatByKey(&mRaccoonDogFallSideDamper, "RaccoonDogFallSideDamper");

    mTrampleJumpGravity = 0.95f;
    rIter.tryGetFloatByKey(&mTrampleJumpGravity, "TrampleJumpGravity");

    mTrampleJump = 12.0f;
    rIter.tryGetFloatByKey(&mTrampleJump, "TrampleJump");

    mTrampleJumpSideVelRate = 0.35f;
    rIter.tryGetFloatByKey(&mTrampleJumpSideVelRate, "TrampleJumpSideVelRate");

    mTrampleHighJumpGravity = 0.65f;
    rIter.tryGetFloatByKey(&mTrampleHighJumpGravity, "TrampleHighJumpGravity");

    mTrampleHighJump = 19.5f;
    rIter.tryGetFloatByKey(&mTrampleHighJump, "TrampleHighJump");

    mTrampleHipDropGravity = 0.95f;
    rIter.tryGetFloatByKey(&mTrampleHipDropGravity, "TrampleHipDropGravity");

    mTrampleHipDropJump = 20.0f;
    rIter.tryGetFloatByKey(&mTrampleHipDropJump, "TrampleHipDropJump");

    mRisingTrampleJumpGravity = 1.25f;
    rIter.tryGetFloatByKey(&mRisingTrampleJumpGravity, "RisingTrampleJumpGravity");

    mRisingTrampleJump = 22.5f;
    rIter.tryGetFloatByKey(&mRisingTrampleJump, "RisingTrampleJump");

    mRisingTrampleHighJumpGravity = 0.95f;
    rIter.tryGetFloatByKey(&mRisingTrampleHighJumpGravity, "RisingTrampleHighJumpGravity");

    mRisingTrampleHighJump = 24.5f;
    rIter.tryGetFloatByKey(&mRisingTrampleHighJump, "RisingTrampleHighJump");

    mRisingTrampleHVelBrakeRate = 0.316f;
    rIter.tryGetFloatByKey(&mRisingTrampleHVelBrakeRate, "RisingTrampleHVelBrakeRate");

    mPunchedJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mPunchedJumpGravity, "PunchedJumpGravity");

    mPunchedJump = 20.0f;
    rIter.tryGetFloatByKey(&mPunchedJump, "PunchedJump");

    mRisingPunchedHVelBrakeRate = 0.316f;
    rIter.tryGetFloatByKey(&mRisingPunchedHVelBrakeRate, "RisingPunchedHVelBrakeRate");

    mTossedJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mTossedJumpGravity, "TossedJumpGravity");

    mTossedJump = 31.0f;
    rIter.tryGetFloatByKey(&mTossedJump, "TossedJump");

    mTossedHighJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mTossedHighJumpGravity, "TossedHighJumpGravity");

    mTossedHighJump = 31.0f;
    rIter.tryGetFloatByKey(&mTossedHighJump, "TossedHighJump");

    mTossedHipDropGravity = 0.8f;
    rIter.tryGetFloatByKey(&mTossedHipDropGravity, "TossedHipDropGravity");

    mTossedHipDropJump = 31.0f;
    rIter.tryGetFloatByKey(&mTossedHipDropJump, "TossedHipDropJump");

    mRisingTossedJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mRisingTossedJumpGravity, "RisingTossedJumpGravity");

    mRisingTossedJump = 31.0f;
    rIter.tryGetFloatByKey(&mRisingTossedJump, "RisingTossedJump");

    mRisingTossedHighJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mRisingTossedHighJumpGravity, "RisingTossedHighJumpGravity");

    mRisingTossedHighJump = 31.0f;
    rIter.tryGetFloatByKey(&mRisingTossedHighJump, "RisingTossedHighJump");

    mRisingTossedHVelBrakeRate = 0.316f;
    rIter.tryGetFloatByKey(&mRisingTossedHVelBrakeRate, "RisingTossedHVelBrakeRate");

    mSquatBrakeEndSpeed = 3.5f;
    rIter.tryGetFloatByKey(&mSquatBrakeEndSpeed, "SquatBrakeEndSpeed");

    mSquatShiftSpeedRate = 0.98f;
    rIter.tryGetFloatByKey(&mSquatShiftSpeedRate, "SquatShiftSpeedRate");

    mSquatAccelRate = 1.2f;
    rIter.tryGetFloatByKey(&mSquatAccelRate, "SquatAccelRate");

    mSquatNoBrakeFrame = 0;
    rIter.tryGetIntByKey(&mSquatNoBrakeFrame, "SquatNoBrakeFrame");

    mSquatBrakeRate = 0.964f;
    rIter.tryGetFloatByKey(&mSquatBrakeRate, "SquatBrakeRate");

    mSquatBrakeRateOnSkate = 0.985f;
    rIter.tryGetFloatByKey(&mSquatBrakeRateOnSkate, "SquatBrakeRateOnSkate");

    mSquatBrakeSideAccel = 0.25f;
    rIter.tryGetFloatByKey(&mSquatBrakeSideAccel, "SquatBrakeSideAccel");

    mSquatBrakeSideBrakeRate = 0.93f;
    rIter.tryGetFloatByKey(&mSquatBrakeSideBrakeRate, "SquatBrakeSideBrakeRate");

    mSquatBrakeSideBrakeRateOnSkate = 0.985f;
    rIter.tryGetFloatByKey(&mSquatBrakeSideBrakeRateOnSkate, "SquatBrakeSideBrakeRateOnSkate");

    mSquatBrakeSideMaxSpeedRate = 0.5f;
    rIter.tryGetFloatByKey(&mSquatBrakeSideMaxSpeedRate, "SquatBrakeSideMaxSpeedRate");

    mSquatWalkSpeed = 3.5f;
    rIter.tryGetFloatByKey(&mSquatWalkSpeed, "SquatWalkSpeed");

    mSquatWalkFrontVecBlend = 0.894f;
    rIter.tryGetFloatByKey(&mSquatWalkFrontVecBlend, "SquatWalkFrontVecBlend");

    mSquatEnergyAccelFrame = 20;
    rIter.tryGetIntByKey(&mSquatEnergyAccelFrame, "SquatEnergyAccelFrame");

    mSquatJumpGravity = 0.95f;
    rIter.tryGetFloatByKey(&mSquatJumpGravity, "SquatJumpGravity");

    mSquatJumpPow = 18.5f;
    rIter.tryGetFloatByKey(&mSquatJumpPow, "SquatJumpPow");

    mSquatHighJumpPow = 28.98f;
    rIter.tryGetFloatByKey(&mSquatHighJumpPow, "SquatHighJumpPow");

    mSquatJumpBackPow = 0.0f;
    rIter.tryGetFloatByKey(&mSquatJumpBackPow, "SquatJumpBackPow");

    mSquatJumpTramplePow = 15.0f;
    rIter.tryGetFloatByKey(&mSquatJumpTramplePow, "SquatJumpTramplePow");

    mHipDropSpeed = 37.5f;
    rIter.tryGetFloatByKey(&mHipDropSpeed, "HipDropSpeed");

    mHipDropLandCancelFrame = 24;
    rIter.tryGetIntByKey(&mHipDropLandCancelFrame, "HipDropLandCancelFrame");

    mHipDropHeight = 40.0f;
    rIter.tryGetFloatByKey(&mHipDropHeight, "HipDropHeight");

    mHipDropMsgInterval = 10;
    rIter.tryGetIntByKey(&mHipDropMsgInterval, "HipDropMsgInterval");

    mHipDropKnockDownRadiusMin = 150.0f;
    rIter.tryGetFloatByKey(&mHipDropKnockDownRadiusMin, "HipDropKnockDownRadiusMin");

    mHipDropKnockDownRadiusMax = 150.0f;
    rIter.tryGetFloatByKey(&mHipDropKnockDownRadiusMax, "HipDropKnockDownRadiusMax");

    mHipDropStartAnimRate = 1.3f;
    rIter.tryGetFloatByKey(&mHipDropStartAnimRate, "HipDropStartAnimRate");

    mHipDropKnockDownFrame = 1;
    rIter.tryGetIntByKey(&mHipDropKnockDownFrame, "HipDropKnockDownFrame");

    mHipDropJumpPow = 28.0f;
    rIter.tryGetFloatByKey(&mHipDropJumpPow, "HipDropJumpPow");

    mHipDropJumpPowCountMax = 11;
    rIter.tryGetIntByKey(&mHipDropJumpPowCountMax, "HipDropJumpPowCountMax");

    mHipDropJumpPermitBeginFrame = 10;
    rIter.tryGetIntByKey(&mHipDropJumpPermitBeginFrame, "HipDropJumpPermitBeginFrame");

    mHipDropJumpPermitEndFrame = 30;
    rIter.tryGetIntByKey(&mHipDropJumpPermitEndFrame, "HipDropJumpPermitEndFrame");

    mWallHeightLowLimit = 100.0f;
    rIter.tryGetFloatByKey(&mWallHeightLowLimit, "WallHeightLowLimit");

    mWallGravity = 0.6f;
    rIter.tryGetFloatByKey(&mWallGravity, "WallGravity");

    mWallMaxSpeed = 16.0f;
    rIter.tryGetFloatByKey(&mWallMaxSpeed, "WallMaxSpeed");

    mWallApartFrame = 20.0f;
    rIter.tryGetFloatByKey(&mWallApartFrame, "WallApartFrame");

    mWallSnapDistance = 100.0f;
    rIter.tryGetFloatByKey(&mWallSnapDistance, "WallSnapDistance");

    mWallSlideMaxSpeed = 5.0f;
    rIter.tryGetFloatByKey(&mWallSlideMaxSpeed, "WallSlideMaxSpeed");

    mWallSlideAccel = 0.125f;
    rIter.tryGetFloatByKey(&mWallSlideAccel, "WallSlideAccel");

    mWallInhibitAfterPunch = 10;
    rIter.tryGetIntByKey(&mWallInhibitAfterPunch, "WallInhibitAfterPunch");

    mWallJumpGravity = 0.95f;
    rIter.tryGetFloatByKey(&mWallJumpGravity, "WallJumpGravity");

    mWallJumpHSpeed = 8.6f;
    rIter.tryGetFloatByKey(&mWallJumpHSpeed, "WallJumpHSpeed");

    mWallJumpPow = 23.0f;
    rIter.tryGetFloatByKey(&mWallJumpPow, "WallJumpPow");

    mWallJumpInvalidateInputFrame = 20;
    rIter.tryGetIntByKey(&mWallJumpInvalidateInputFrame, "WallJumpInvalidateInputFrame");

    mWallJumpDirEffectiveFrame = 20;
    rIter.tryGetIntByKey(&mWallJumpDirEffectiveFrame, "WallJumpDirEffectiveFrame");

    mWallJumpDirLimit = 60.0f;
    rIter.tryGetFloatByKey(&mWallJumpDirLimit, "WallJumpDirLimit");

    mWallJumpLimitPlay = 20.0f;
    rIter.tryGetFloatByKey(&mWallJumpLimitPlay, "WallJumpLimitPlay");

    mWallClimbReadyFrame = 120;
    rIter.tryGetIntByKey(&mWallClimbReadyFrame, "WallClimbReadyFrame");

    mWallClimbReadyWaitFrame = 3;
    rIter.tryGetIntByKey(&mWallClimbReadyWaitFrame, "WallClimbReadyWaitFrame");

    mWallClimbReadyFromSlopeFrame = 10;
    rIter.tryGetIntByKey(&mWallClimbReadyFromSlopeFrame, "WallClimbReadyFromSlopeFrame");

    mWallClimbAccel = 2.0f;
    rIter.tryGetFloatByKey(&mWallClimbAccel, "WallClimbAccel");

    mWallClimbMaxSpeed = 5.0f;
    rIter.tryGetFloatByKey(&mWallClimbMaxSpeed, "WallClimbMaxSpeed");

    mWallClimbDashMaxSpeed = 7.0f;
    rIter.tryGetFloatByKey(&mWallClimbDashMaxSpeed, "WallClimbDashMaxSpeed");

    mWallClimbFrame = 150;
    rIter.tryGetIntByKey(&mWallClimbFrame, "WallClimbFrame");

    mWallClimbFrame1 = 68;
    rIter.tryGetIntByKey(&mWallClimbFrame1, "WallClimbFrame1");

    mWallClimbFrame2 = 0;
    rIter.tryGetIntByKey(&mWallClimbFrame2, "WallClimbFrame2");

    mWallClimbDashFrame = 117;
    rIter.tryGetIntByKey(&mWallClimbDashFrame, "WallClimbDashFrame");

    mWallClimbDashFrame1 = 59;
    rIter.tryGetIntByKey(&mWallClimbDashFrame1, "WallClimbDashFrame1");

    mWallClimbDashFrame2 = 0;
    rIter.tryGetIntByKey(&mWallClimbDashFrame2, "WallClimbDashFrame2");

    mWallClimbStopFrame = 110;
    rIter.tryGetIntByKey(&mWallClimbStopFrame, "WallClimbStopFrame");

    mWallClimbBrakeRate = 0.8f;
    rIter.tryGetFloatByKey(&mWallClimbBrakeRate, "WallClimbBrakeRate");

    mWallClimbNoWallFrame = 4;
    rIter.tryGetIntByKey(&mWallClimbNoWallFrame, "WallClimbNoWallFrame");

    mWallClimbMaxSideSpeed = 4.0f;
    rIter.tryGetFloatByKey(&mWallClimbMaxSideSpeed, "WallClimbMaxSideSpeed");

    mWallClimbDashMaxSideSpeed = 5.3f;
    rIter.tryGetFloatByKey(&mWallClimbDashMaxSideSpeed, "WallClimbDashMaxSideSpeed");

    mWallClimbSideAccel = 2.0f;
    rIter.tryGetFloatByKey(&mWallClimbSideAccel, "WallClimbSideAccel");

    mWallClimbRetainThreshold = 60.0f;
    rIter.tryGetFloatByKey(&mWallClimbRetainThreshold, "WallClimbRetainThreshold");

    mWallClimbInvalidSideMoveDegree = 5.0f;
    rIter.tryGetFloatByKey(&mWallClimbInvalidSideMoveDegree, "WallClimbInvalidSideMoveDegree");

    mWallClimbDashAnimRate = 1.2f;
    rIter.tryGetFloatByKey(&mWallClimbDashAnimRate, "WallClimbDashAnimRate");

    mWallClimbNormalAnimRate = 0.9f;
    rIter.tryGetFloatByKey(&mWallClimbNormalAnimRate, "WallClimbNormalAnimRate");

    mWallClimbJumpPowCountMax = 0;
    rIter.tryGetIntByKey(&mWallClimbJumpPowCountMax, "WallClimbJumpPowCountMax");

    mWallClimbJumpDashAddition = 0.0f;
    rIter.tryGetFloatByKey(&mWallClimbJumpDashAddition, "WallClimbJumpDashAddition");

    mWallClimbJumpPowLow = 18.0f;
    rIter.tryGetFloatByKey(&mWallClimbJumpPowLow, "WallClimbJumpPowLow");

    mWallClimbJumpPow = 18.0f;
    rIter.tryGetFloatByKey(&mWallClimbJumpPow, "WallClimbJumpPow");

    mWallClimbSlideGravity = 0.3f;
    rIter.tryGetFloatByKey(&mWallClimbSlideGravity, "WallClimbSlideGravity");

    mWallClimbSlideMaxSpeed = 16.0f;
    rIter.tryGetFloatByKey(&mWallClimbSlideMaxSpeed, "WallClimbSlideMaxSpeed");

    mWallClimbSlideSideAccel = 0.1f;
    rIter.tryGetFloatByKey(&mWallClimbSlideSideAccel, "WallClimbSlideSideAccel");

    mWallClimbSlideSideMaxSpeed = 2.0f;
    rIter.tryGetFloatByKey(&mWallClimbSlideSideMaxSpeed, "WallClimbSlideSideMaxSpeed");

    mWallClimbSlideStartBrakeRate = 0.9f;
    rIter.tryGetFloatByKey(&mWallClimbSlideStartBrakeRate, "WallClimbSlideStartBrakeRate");

    mClimbAirStopFrame = 20;
    rIter.tryGetIntByKey(&mClimbAirStopFrame, "ClimbAirStopFrame");

    mClimbAirStopRotateVelMax = 15.0f;
    rIter.tryGetFloatByKey(&mClimbAirStopRotateVelMax, "ClimbAirStopRotateVelMax");

    mLongJumpSuccessSpeed = 6.0f;
    rIter.tryGetFloatByKey(&mLongJumpSuccessSpeed, "LongJumpSuccessSpeed");

    mLongJumpSpeedMin = 2.5f;
    rIter.tryGetFloatByKey(&mLongJumpSpeedMin, "LongJumpSpeedMin");

    mLongJumpBrake = 0.5f;
    rIter.tryGetFloatByKey(&mLongJumpBrake, "LongJumpBrake");

    mLongJumpSideAccel = 0.125f;
    rIter.tryGetFloatByKey(&mLongJumpSideAccel, "LongJumpSideAccel");

    mLongJumpCancelFrame = 40;
    rIter.tryGetIntByKey(&mLongJumpCancelFrame, "LongJumpCancelFrame");

    mLongJumpFastSuccessFrame = 20;
    rIter.tryGetIntByKey(&mLongJumpFastSuccessFrame, "LongJumpFastSuccessFrame");

    mLongJumpFastGravity = 0.5f;
    rIter.tryGetFloatByKey(&mLongJumpFastGravity, "LongJumpFastGravity");

    mLongJumpFastJumpPow = 13.0f;
    rIter.tryGetFloatByKey(&mLongJumpFastJumpPow, "LongJumpFastJumpPow");

    mLongJumpFastSpeed = 13.5f;
    rIter.tryGetFloatByKey(&mLongJumpFastSpeed, "LongJumpFastSpeed");

    mLongJumpSlowGravity = 0.75f;
    rIter.tryGetFloatByKey(&mLongJumpSlowGravity, "LongJumpSlowGravity");

    mLongJumpSlowJumpPow = 15.5f;
    rIter.tryGetFloatByKey(&mLongJumpSlowJumpPow, "LongJumpSlowJumpPow");

    mLongJumpSlowSpeed = 10.5f;
    rIter.tryGetFloatByKey(&mLongJumpSlowSpeed, "LongJumpSlowSpeed");

    mDashBrakeCommandFrame = 30;
    rIter.tryGetIntByKey(&mDashBrakeCommandFrame, "DashBrakeCommandFrame");

    mDashBrakeActionFrame = 8;
    rIter.tryGetIntByKey(&mDashBrakeActionFrame, "DashBrakeActionFrame");

    mDashBrakeSpeed = 7.5f;
    rIter.tryGetFloatByKey(&mDashBrakeSpeed, "DashBrakeSpeed");

    mTurnJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mTurnJumpGravity, "TurnJumpGravity");

    mTurnJumpPow = 25.53f;
    rIter.tryGetFloatByKey(&mTurnJumpPow, "TurnJumpPow");

    mTurnJumpVelH = 4.5f;
    rIter.tryGetFloatByKey(&mTurnJumpVelH, "TurnJumpVelH");

    mTurnJumpBrake = 0.5f;
    rIter.tryGetFloatByKey(&mTurnJumpBrake, "TurnJumpBrake");

    mTurnJumpAccel = 0.25f;
    rIter.tryGetFloatByKey(&mTurnJumpAccel, "TurnJumpAccel");

    mTurnJumpSideAccel = 0.075f;
    rIter.tryGetFloatByKey(&mTurnJumpSideAccel, "TurnJumpSideAccel");

    mTurnJumpToFlightDurationFrame = 50;
    rIter.tryGetIntByKey(&mTurnJumpToFlightDurationFrame, "TurnJumpToFlightDurationFrame");

    mWaitRollingMinSpeed = 10.0f;
    rIter.tryGetFloatByKey(&mWaitRollingMinSpeed, "WaitRollingMinSpeed");

    mWaitRollingNoBrakeFrame = 10;
    rIter.tryGetIntByKey(&mWaitRollingNoBrakeFrame, "WaitRollingNoBrakeFrame");

    mWaitRollingBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mWaitRollingBrakeRate, "WaitRollingBrakeRate");

    mWaitRollingSideBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mWaitRollingSideBrakeRate, "WaitRollingSideBrakeRate");

    mWaitRollingSideAccel = 0.125f;
    rIter.tryGetFloatByKey(&mWaitRollingSideAccel, "WaitRollingSideAccel");

    mWaitRollingSideMaxSpeed = 7.5f;
    rIter.tryGetFloatByKey(&mWaitRollingSideMaxSpeed, "WaitRollingSideMaxSpeed");

    mNormalRollingMinSpeed = 12.5f;
    rIter.tryGetFloatByKey(&mNormalRollingMinSpeed, "NormalRollingMinSpeed");

    mNormalRollingNoBrakeFrame = 30;
    rIter.tryGetIntByKey(&mNormalRollingNoBrakeFrame, "NormalRollingNoBrakeFrame");

    mNormalRollingBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mNormalRollingBrakeRate, "NormalRollingBrakeRate");

    mNormalRollingSideBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mNormalRollingSideBrakeRate, "NormalRollingSideBrakeRate");

    mNormalRollingSideAccel = 0.125f;
    rIter.tryGetFloatByKey(&mNormalRollingSideAccel, "NormalRollingSideAccel");

    mNormalRollingSideMaxSpeed = 7.5f;
    rIter.tryGetFloatByKey(&mNormalRollingSideMaxSpeed, "NormalRollingSideMaxSpeed");

    mAirRollingMinSpeed = 12.5f;
    rIter.tryGetFloatByKey(&mAirRollingMinSpeed, "AirRollingMinSpeed");

    mAirRollingNoBrakeFrame = 30;
    rIter.tryGetIntByKey(&mAirRollingNoBrakeFrame, "AirRollingNoBrakeFrame");

    mAirRollingBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mAirRollingBrakeRate, "AirRollingBrakeRate");

    mAirRollingSideBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mAirRollingSideBrakeRate, "AirRollingSideBrakeRate");

    mAirRollingSideAccel = 0.125f;
    rIter.tryGetFloatByKey(&mAirRollingSideAccel, "AirRollingSideAccel");

    mAirRollingSideMaxSpeed = 7.5f;
    rIter.tryGetFloatByKey(&mAirRollingSideMaxSpeed, "AirRollingSideMaxSpeed");

    mAirRollingJumpPow = 15.0f;
    rIter.tryGetFloatByKey(&mAirRollingJumpPow, "AirRollingJumpPow");

    mAirRollingGravity = 1.0f;
    rIter.tryGetFloatByKey(&mAirRollingGravity, "AirRollingGravity");

    mRollingMinSpeed = 14.0f;
    rIter.tryGetFloatByKey(&mRollingMinSpeed, "RollingMinSpeed");

    mRollingNoBrakeFrame = 40;
    rIter.tryGetIntByKey(&mRollingNoBrakeFrame, "RollingNoBrakeFrame");

    mRollingBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mRollingBrakeRate, "RollingBrakeRate");

    mRollingSideBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mRollingSideBrakeRate, "RollingSideBrakeRate");

    mRollingSideAccel = 0.25f;
    rIter.tryGetFloatByKey(&mRollingSideAccel, "RollingSideAccel");

    mRollingSideMaxSpeed = 2.5f;
    rIter.tryGetFloatByKey(&mRollingSideMaxSpeed, "RollingSideMaxSpeed");

    mRaccoonDogWaitRollingMinSpeed = 0.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingMinSpeed, "RaccoonDogWaitRollingMinSpeed");

    mRaccoonDogWaitRollingNoBrakeFrame = 0;
    rIter.tryGetIntByKey(&mRaccoonDogWaitRollingNoBrakeFrame, "RaccoonDogWaitRollingNoBrakeFrame");

    mRaccoonDogWaitRollingBrakeRate = 1.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingBrakeRate, "RaccoonDogWaitRollingBrakeRate");

    mRaccoonDogWaitRollingSideBrakeRate = 1.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingSideBrakeRate,
                           "RaccoonDogWaitRollingSideBrakeRate");

    mRaccoonDogWaitRollingSideAccel = 0.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingSideAccel, "RaccoonDogWaitRollingSideAccel");

    mRaccoonDogWaitRollingSideMaxSpeed = 0.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingSideMaxSpeed,
                           "RaccoonDogWaitRollingSideMaxSpeed");

    mRaccoonDogNormalRollingMinSpeed = 12.5f;
    rIter.tryGetFloatByKey(&mRaccoonDogNormalRollingMinSpeed, "RaccoonDogNormalRollingMinSpeed");

    mRaccoonDogNormalRollingNoBrakeFrame = 0;
    rIter.tryGetIntByKey(&mRaccoonDogNormalRollingNoBrakeFrame,
                         "RaccoonDogNormalRollingNoBrakeFrame");

    mRaccoonDogNormalRollingBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mRaccoonDogNormalRollingBrakeRate, "RaccoonDogNormalRollingBrakeRate");

    mRaccoonDogNormalRollingSideBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mRaccoonDogNormalRollingSideBrakeRate,
                           "RaccoonDogNormalRollingSideBrakeRate");

    mRaccoonDogNormalRollingSideAccel = 1.25f;
    rIter.tryGetFloatByKey(&mRaccoonDogNormalRollingSideAccel, "RaccoonDogNormalRollingSideAccel");

    mRaccoonDogNormalRollingSideMaxSpeed = 2.5f;
    rIter.tryGetFloatByKey(&mRaccoonDogNormalRollingSideMaxSpeed,
                           "RaccoonDogNormalRollingSideMaxSpeed");

    mRaccoonDogDashRollingMinSpeed = 14.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogDashRollingMinSpeed, "RaccoonDogDashRollingMinSpeed");

    mRaccoonDogDashRollingNoBrakeFrame = 24;
    rIter.tryGetIntByKey(&mRaccoonDogDashRollingNoBrakeFrame, "RaccoonDogDashRollingNoBrakeFrame");

    mRaccoonDogDashRollingBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mRaccoonDogDashRollingBrakeRate, "RaccoonDogDashRollingBrakeRate");

    mRaccoonDogDashRollingSideBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mRaccoonDogDashRollingSideBrakeRate,
                           "RaccoonDogDashRollingSideBrakeRate");

    mRaccoonDogDashRollingSideAccel = 0.25f;
    rIter.tryGetFloatByKey(&mRaccoonDogDashRollingSideAccel, "RaccoonDogDashRollingSideAccel");

    mRaccoonDogDashRollingSideMaxSpeed = 2.5f;
    rIter.tryGetFloatByKey(&mRaccoonDogDashRollingSideMaxSpeed,
                           "RaccoonDogDashRollingSideMaxSpeed");

    mRollingTramplePow = 15.0f;
    rIter.tryGetFloatByKey(&mRollingTramplePow, "RollingTramplePow");

    mWaitRollingAttackJumpGravity = 0.5f;
    rIter.tryGetFloatByKey(&mWaitRollingAttackJumpGravity, "WaitRollingAttackJumpGravity");

    mWaitRollingAttackJumpPow = 13.0f;
    rIter.tryGetFloatByKey(&mWaitRollingAttackJumpPow, "WaitRollingAttackJumpPow");

    mWaitRollingAttackVelH = 9.5f;
    rIter.tryGetFloatByKey(&mWaitRollingAttackVelH, "WaitRollingAttackVelH");

    mNormalRollingAttackJumpGravity = 0.5f;
    rIter.tryGetFloatByKey(&mNormalRollingAttackJumpGravity, "NormalRollingAttackJumpGravity");

    mNormalRollingAttackJumpPow = 12.5f;
    rIter.tryGetFloatByKey(&mNormalRollingAttackJumpPow, "NormalRollingAttackJumpPow");

    mNormalRollingAttackVelH = 13.5f;
    rIter.tryGetFloatByKey(&mNormalRollingAttackVelH, "NormalRollingAttackVelH");

    mRollingAttackJumpGravity = 0.5f;
    rIter.tryGetFloatByKey(&mRollingAttackJumpGravity, "RollingAttackJumpGravity");

    mRollingAttackJumpPow = 13.0f;
    rIter.tryGetFloatByKey(&mRollingAttackJumpPow, "RollingAttackJumpPow");

    mRollingAttackVelH = 15.0f;
    rIter.tryGetFloatByKey(&mRollingAttackVelH, "RollingAttackVelH");

    mRaccoonDogWaitRollingAttackJumpGravity = 0.5f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingAttackJumpGravity,
                           "RaccoonDogWaitRollingAttackJumpGravity");

    mRaccoonDogWaitRollingAttackJumpPow = 13.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingAttackJumpPow,
                           "RaccoonDogWaitRollingAttackJumpPow");

    mRaccoonDogWaitRollingAttackVelH = 0.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingAttackVelH, "RaccoonDogWaitRollingAttackVelH");

    mRaccoonDogWaitRollingAttackHighJumpGravity = 0.5f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingAttackHighJumpGravity,
                           "RaccoonDogWaitRollingAttackHighJumpGravity");

    mRaccoonDogWaitRollingAttackHighJumpPow = 17.5f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingAttackHighJumpPow,
                           "RaccoonDogWaitRollingAttackHighJumpPow");

    mRaccoonDogWaitRollingAttackHighVelH = 0.0f;
    rIter.tryGetFloatByKey(&mRaccoonDogWaitRollingAttackHighVelH,
                           "RaccoonDogWaitRollingAttackHighVelH");

    mCommonRollingAttackSpeedMin = 2.5f;
    rIter.tryGetFloatByKey(&mCommonRollingAttackSpeedMin, "CommonRollingAttackSpeedMin");

    mCommonRollingAttackBrake = 0.25f;
    rIter.tryGetFloatByKey(&mCommonRollingAttackBrake, "CommonRollingAttackBrake");

    mCommonRollingAttackSideAccel = 0.125f;
    rIter.tryGetFloatByKey(&mCommonRollingAttackSideAccel, "CommonRollingAttackSideAccel");

    mRollingHitBound = 1.25f;
    rIter.tryGetFloatByKey(&mRollingHitBound, "RollingHitBound");

    mWallHitLandCancelFrame = 30;
    rIter.tryGetIntByKey(&mWallHitLandCancelFrame, "WallHitLandCancelFrame");

    mDamageInvalidCount = 180;
    rIter.tryGetIntByKey(&mDamageInvalidCount, "DamageInvalidCount");

    mDamageCancelFrame = 50;
    rIter.tryGetIntByKey(&mDamageCancelFrame, "DamageCancelFrame");

    mInvincibleFrame = 700;
    rIter.tryGetIntByKey(&mInvincibleFrame, "InvincibleFrame");

    mInvincibleDashFrame = 60;
    rIter.tryGetIntByKey(&mInvincibleDashFrame, "InvincibleDashFrame");

    mInvincibleDashSpeed = 13.0f;
    rIter.tryGetFloatByKey(&mInvincibleDashSpeed, "InvincibleDashSpeed");

    mInvincibleJumpPow = 15.0f;
    rIter.tryGetFloatByKey(&mInvincibleJumpPow, "InvincibleJumpPow");

    mInvincibleJumpPowCountMax = 10;
    rIter.tryGetIntByKey(&mInvincibleJumpPowCountMax, "InvincibleJumpPowCountMax");

    mTailAttackStart = 0;
    rIter.tryGetIntByKey(&mTailAttackStart, "TailAttackStart");

    mTailAttackFrame = 14;
    rIter.tryGetIntByKey(&mTailAttackFrame, "TailAttackFrame");

    mTailAttackInterval = 14;
    rIter.tryGetIntByKey(&mTailAttackInterval, "TailAttackInterval");

    mStandSwimRisePower = 1.0f;
    rIter.tryGetFloatByKey(&mStandSwimRisePower, "StandSwimRisePower");

    mStandSwimRiseSpeedMax = 5.5f;
    rIter.tryGetFloatByKey(&mStandSwimRiseSpeedMax, "StandSwimRiseSpeedMax");

    mStandSwimGravity = 0.125f;
    rIter.tryGetFloatByKey(&mStandSwimGravity, "StandSwimGravity");

    mStandSwimFallSpeedMax = 5.0f;
    rIter.tryGetFloatByKey(&mStandSwimFallSpeedMax, "StandSwimFallSpeedMax");

    mStandSwimHorizontalFloorDashAccel = 0.12f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalFloorDashAccel,
                           "StandSwimHorizontalFloorDashAccel");

    mStandSwimHorizontalFloorDashSpeedMax = 5.0f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalFloorDashSpeedMax,
                           "StandSwimHorizontalFloorDashSpeedMax");

    mStandSwimHorizontalFloorAccel = 0.125f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalFloorAccel, "StandSwimHorizontalFloorAccel");

    mStandSwimHorizontalFloorSpeedMax = 3.5f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalFloorSpeedMax, "StandSwimHorizontalFloorSpeedMax");

    mNoSinkSwimHorizontalHighAccel = 0.25f;
    rIter.tryGetFloatByKey(&mNoSinkSwimHorizontalHighAccel, "NoSinkSwimHorizontalHighAccel");

    mNoSinkSwimHorizontalHighInputMin = 0.3f;
    rIter.tryGetFloatByKey(&mNoSinkSwimHorizontalHighInputMin, "NoSinkSwimHorizontalHighInputMin");

    mNoSinkSwimHorizontalHighSpeedMax = 8.0f;
    rIter.tryGetFloatByKey(&mNoSinkSwimHorizontalHighSpeedMax, "NoSinkSwimHorizontalHighSpeedMax");

    mNoSinkSwimHorizontalHighSpeedMin = 4.0f;
    rIter.tryGetFloatByKey(&mNoSinkSwimHorizontalHighSpeedMin, "NoSinkSwimHorizontalHighSpeedMin");

    mStandSwimHorizontalHighAccel = 0.125f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalHighAccel, "StandSwimHorizontalHighAccel");

    mStandSwimHorizontalHighSpeedMax = 6.0f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalHighSpeedMax, "StandSwimHorizontalHighSpeedMax");

    mStandSwimHorizontalLowAccel = 0.125f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalLowAccel, "StandSwimHorizontalLowAccel");

    mStandSwimHorizontalLowSpeedMax = 5.0f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalLowSpeedMax, "StandSwimHorizontalLowSpeedMax");

    mStandSwimHorizontalBrakeRate = 0.975f;
    rIter.tryGetFloatByKey(&mStandSwimHorizontalBrakeRate, "StandSwimHorizontalBrakeRate");

    mStandSwimHighAccelPermitFrame = 30;
    rIter.tryGetIntByKey(&mStandSwimHighAccelPermitFrame, "StandSwimHighAccelPermitFrame");

    mStandSwimForwardBentDegree = 30.0f;
    rIter.tryGetFloatByKey(&mStandSwimForwardBentDegree, "StandSwimForwardBentDegree");

    mStandSwimForwardBentBlend = 0.0513f;
    rIter.tryGetFloatByKey(&mStandSwimForwardBentBlend, "StandSwimForwardBentBlend");

    mStandSwimFlowFieldBlend = 0.776f;
    rIter.tryGetFloatByKey(&mStandSwimFlowFieldBlend, "StandSwimFlowFieldBlend");

    mStandSwimRotSpeed = 7.5f;
    rIter.tryGetFloatByKey(&mStandSwimRotSpeed, "StandSwimRotSpeed");

    mStandSwimSurfaceRotSpeed = 4.0f;
    rIter.tryGetFloatByKey(&mStandSwimSurfaceRotSpeed, "StandSwimSurfaceRotSpeed");

    mStandSwimSurfaceRotSpeedNoMovement = 25.0f;
    rIter.tryGetFloatByKey(&mStandSwimSurfaceRotSpeedNoMovement,
                           "StandSwimSurfaceRotSpeedNoMovement");

    mStandSwimWalkAnimMinRate = 0.2f;
    rIter.tryGetFloatByKey(&mStandSwimWalkAnimMinRate, "StandSwimWalkAnimMinRate");

    mStandSwimWalkAnimMaxRate = 1.9f;
    rIter.tryGetFloatByKey(&mStandSwimWalkAnimMaxRate, "StandSwimWalkAnimMaxRate");

    mStandSwimWalkMaxSpeed = 5.0f;
    rIter.tryGetFloatByKey(&mStandSwimWalkMaxSpeed, "StandSwimWalkMaxSpeed");

    mStandSwimPaddleAnimInterval = 32;
    rIter.tryGetIntByKey(&mStandSwimPaddleAnimInterval, "StandSwimPaddleAnimInterval");

    mStandSwimPaddleAnimRateIntervalMax = 22;
    rIter.tryGetIntByKey(&mStandSwimPaddleAnimRateIntervalMax,
                         "StandSwimPaddleAnimRateIntervalMax");

    mStandSwimPaddleAnimRateIntervalMin = 5;
    rIter.tryGetIntByKey(&mStandSwimPaddleAnimRateIntervalMin,
                         "StandSwimPaddleAnimRateIntervalMin");

    mStandSwimPaddleAnimMaxRate = 3.0f;
    rIter.tryGetFloatByKey(&mStandSwimPaddleAnimMaxRate, "StandSwimPaddleAnimMaxRate");

    mSwimHRotSpeed = 2.0f;
    rIter.tryGetFloatByKey(&mSwimHRotSpeed, "SwimHRotSpeed");

    mSwimVRotSpeed = 2.0f;
    rIter.tryGetFloatByKey(&mSwimVRotSpeed, "SwimVRotSpeed");

    mSwimPaddleAccel = 0.75f;
    rIter.tryGetFloatByKey(&mSwimPaddleAccel, "SwimPaddleAccel");

    mSwimPaddleSpeedMax = 12.0f;
    rIter.tryGetFloatByKey(&mSwimPaddleSpeedMax, "SwimPaddleSpeedMax");

    mSwimPaddleFrame = 20;
    rIter.tryGetIntByKey(&mSwimPaddleFrame, "SwimPaddleFrame");

    mSwimKickAccel = 0.25f;
    rIter.tryGetFloatByKey(&mSwimKickAccel, "SwimKickAccel");

    mSwimKickSpeedMax = 6.0f;
    rIter.tryGetFloatByKey(&mSwimKickSpeedMax, "SwimKickSpeedMax");

    mSwimKickBrake = 0.99f;
    rIter.tryGetFloatByKey(&mSwimKickBrake, "SwimKickBrake");

    mSwimBrake = 0.975f;
    rIter.tryGetFloatByKey(&mSwimBrake, "SwimBrake");

    mSwimSideBrake = 0.949f;
    rIter.tryGetFloatByKey(&mSwimSideBrake, "SwimSideBrake");

    mStandSwimFromDiveTimer = 30;
    rIter.tryGetIntByKey(&mStandSwimFromDiveTimer, "StandSwimFromDiveTimer");

    mStandSwimFromDiveRisePower = 4.0f;
    rIter.tryGetFloatByKey(&mStandSwimFromDiveRisePower, "StandSwimFromDiveRisePower");

    mStandSwimFromDiveRisePowerClimb = 2.0f;
    rIter.tryGetFloatByKey(&mStandSwimFromDiveRisePowerClimb, "StandSwimFromDiveRisePowerClimb");

    mSwimDiveStartSpeed = 26.5f;
    rIter.tryGetFloatByKey(&mSwimDiveStartSpeed, "SwimDiveStartSpeed");

    mSwimDiveBrake = 0.875f;
    rIter.tryGetFloatByKey(&mSwimDiveBrake, "SwimDiveBrake");

    mSwimDiveEndSpeed = 2.5f;
    rIter.tryGetFloatByKey(&mSwimDiveEndSpeed, "SwimDiveEndSpeed");

    mSwimDiveLandCount = 6;
    rIter.tryGetIntByKey(&mSwimDiveLandCount, "SwimDiveLandCount");

    mSwimDiveLandCancelFrame = 10;
    rIter.tryGetIntByKey(&mSwimDiveLandCancelFrame, "SwimDiveLandCancelFrame");

    mSwimDiveButtonValidFrame = 15;
    rIter.tryGetIntByKey(&mSwimDiveButtonValidFrame, "SwimDiveButtonValidFrame");

    mDiveStartSpeed = 26.5f;
    rIter.tryGetFloatByKey(&mDiveStartSpeed, "DiveStartSpeed");

    mDiveBrake = 0.875f;
    rIter.tryGetFloatByKey(&mDiveBrake, "DiveBrake");

    mDiveBrakeSingleMode = 1.5f;
    rIter.tryGetFloatByKey(&mDiveBrakeSingleMode, "DiveBrakeSingleMode");

    mDiveEndSpeed = 2.5f;
    rIter.tryGetFloatByKey(&mDiveEndSpeed, "DiveEndSpeed");

    mStandSwimTramplePower = 8.0f;
    rIter.tryGetFloatByKey(&mStandSwimTramplePower, "StandSwimTramplePower");

    mDiveTramplePower = 11.0f;
    rIter.tryGetFloatByKey(&mDiveTramplePower, "DiveTramplePower");

    mDiveTrampleCancelFrame = 20.0f;
    rIter.tryGetFloatByKey(&mDiveTrampleCancelFrame, "DiveTrampleCancelFrame");

    mSwimSurfaceStartDist = 160.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceStartDist, "SwimSurfaceStartDist");

    mSwimSurfaceEndDist = 250.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceEndDist, "SwimSurfaceEndDist");

    mSwimSurfaceStartDistShort = 150.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceStartDistShort, "SwimSurfaceStartDistShort");

    mSwimSurfaceEndDistShort = 200.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceEndDistShort, "SwimSurfaceEndDistShort");

    mSwimSurfaceVelDamper = 0.949f;
    rIter.tryGetFloatByKey(&mSwimSurfaceVelDamper, "SwimSurfaceVelDamper");

    mSwimSurfaceGravity = 0.125f;
    rIter.tryGetFloatByKey(&mSwimSurfaceGravity, "SwimSurfaceGravity");

    mSwimSurfaceValidDamperFrame = 24;
    rIter.tryGetIntByKey(&mSwimSurfaceValidDamperFrame, "SwimSurfaceValidDamperFrame");

    mSwimSurfaceDamperLerpFrame = 26;
    rIter.tryGetIntByKey(&mSwimSurfaceDamperLerpFrame, "SwimSurfaceDamperLerpFrame");

    mSwimSurfaceBaseHeight = 80.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceBaseHeight, "SwimSurfaceBaseHeight");

    mSwimSurfaceBaseHeightShort = 50.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceBaseHeightShort, "SwimSurfaceBaseHeightShort");

    mSwimSurfaceSpring = 0.05f;
    rIter.tryGetFloatByKey(&mSwimSurfaceSpring, "SwimSurfaceSpring");

    mSwimSurfaceVerticalOffset = 16.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceVerticalOffset, "SwimSurfaceVerticalOffset");

    mSwimSurfacePivotRate = 0.01f;
    rIter.tryGetFloatByKey(&mSwimSurfacePivotRate, "SwimSurfacePivotRate");

    mSwimSurfacePivotCancelAngle = 0.8f;
    rIter.tryGetFloatByKey(&mSwimSurfacePivotCancelAngle, "SwimSurfacePivotCancelAngle");

    mSwimSurfaceSpeedThreshold = 5.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceSpeedThreshold, "SwimSurfaceSpeedThreshold");

    mSwimSurfacePivotCounter = 3;
    rIter.tryGetIntByKey(&mSwimSurfacePivotCounter, "SwimSurfacePivotCounter");

    mSwimSurfaceTiltDuringPivotMaxDegree = 60.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceTiltDuringPivotMaxDegree,
                           "SwimSurfaceTiltDuringPivotMaxDegree");

    mSwimSurfaceTiltMaxDegree = 60.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceTiltMaxDegree, "SwimSurfaceTiltMaxDegree");

    mSwimSurfaceTiltMaxFrontAngle = 30.0f;
    rIter.tryGetFloatByKey(&mSwimSurfaceTiltMaxFrontAngle, "SwimSurfaceTiltMaxFrontAngle");

    mSwimSurfaceClimbAnimationRate = 2.2f;
    rIter.tryGetFloatByKey(&mSwimSurfaceClimbAnimationRate, "SwimSurfaceClimbAnimationRate");

    mSwimSurfaceSpringForSurfaceSwim = 0.012f;
    rIter.tryGetFloatByKey(&mSwimSurfaceSpringForSurfaceSwim, "SwimSurfaceSpringForSurfaceSwim");

    mSwimSurfaceSpringForSurfaceSwimClimb = 0.009f;
    rIter.tryGetFloatByKey(&mSwimSurfaceSpringForSurfaceSwimClimb,
                           "SwimSurfaceSpringForSurfaceSwimClimb");

    mSwimJumpPow = 25.34f;
    rIter.tryGetFloatByKey(&mSwimJumpPow, "SwimJumpPow");

    mSwimSquatInhibitFrame = 30;
    rIter.tryGetIntByKey(&mSwimSquatInhibitFrame, "SwimSquatInhibitFrame");

    mPropellerRisePow = 2.475f;
    rIter.tryGetFloatByKey(&mPropellerRisePow, "PropellerRisePow");

    mPropellerPowSustain = 24;
    rIter.tryGetIntByKey(&mPropellerPowSustain, "PropellerPowSustain");

    mPropellerPowSustainMin = 16;
    rIter.tryGetIntByKey(&mPropellerPowSustainMin, "PropellerPowSustainMin");

    mPropellerPowRelease = 2;
    rIter.tryGetIntByKey(&mPropellerPowRelease, "PropellerPowRelease");

    mPropellerBeforeDropGravity = 0.95f;
    rIter.tryGetFloatByKey(&mPropellerBeforeDropGravity, "PropellerBeforeDropGravity");

    mPropellerRiseGravity = 0.95f;
    rIter.tryGetFloatByKey(&mPropellerRiseGravity, "PropellerRiseGravity");

    mPropellerAfterDropGravity = 0.0375f;
    rIter.tryGetFloatByKey(&mPropellerAfterDropGravity, "PropellerAfterDropGravity");

    mPropellerFallSpeedMax = 12.5f;
    rIter.tryGetFloatByKey(&mPropellerFallSpeedMax, "PropellerFallSpeedMax");

    mPropellerButtonOffFallSpeedMax = 20.0f;
    rIter.tryGetFloatByKey(&mPropellerButtonOffFallSpeedMax, "PropellerButtonOffFallSpeedMax");

    mPropellerEngineBrakeVel = 0.0f;
    rIter.tryGetFloatByKey(&mPropellerEngineBrakeVel, "PropellerEngineBrakeVel");

    mPropellerEngineBrakeRate = 0.0f;
    rIter.tryGetFloatByKey(&mPropellerEngineBrakeRate, "PropellerEngineBrakeRate");

    mPropellerEngineBrakeEndVel = 1.0f;
    rIter.tryGetFloatByKey(&mPropellerEngineBrakeEndVel, "PropellerEngineBrakeEndVel");

    mPropellerRotBlendRate = 0.194f;
    rIter.tryGetFloatByKey(&mPropellerRotBlendRate, "PropellerRotBlendRate");

    mPropellerSideDamper = 0.163f;
    rIter.tryGetFloatByKey(&mPropellerSideDamper, "PropellerSideDamper");

    mPropellerStickOffBrakeRate = 0.949f;
    rIter.tryGetFloatByKey(&mPropellerStickOffBrakeRate, "PropellerStickOffBrakeRate");

    mLongFallDistance = 1500.0f;
    rIter.tryGetFloatByKey(&mLongFallDistance, "LongFallDistance");

    mStatueFallStartFrame = 28;
    rIter.tryGetIntByKey(&mStatueFallStartFrame, "StatueFallStartFrame");

    mStatueLandFrame = 30;
    rIter.tryGetIntByKey(&mStatueLandFrame, "StatueLandFrame");

    mStatueEndFrame = 480;
    rIter.tryGetIntByKey(&mStatueEndFrame, "StatueEndFrame");

    mStatueEndAnimStep = 330;
    rIter.tryGetIntByKey(&mStatueEndAnimStep, "StatueEndAnimStep");

    mStatueFallSpeedInWater = 12.5f;
    rIter.tryGetFloatByKey(&mStatueFallSpeedInWater, "StatueFallSpeedInWater");

    mSlideSlopeAngle = 26.0f;
    rIter.tryGetFloatByKey(&mSlideSlopeAngle, "SlideSlopeAngle");

    mSlideSlopeEndAngle = 10.0f;
    rIter.tryGetFloatByKey(&mSlideSlopeEndAngle, "SlideSlopeEndAngle");

    mSlideEndSpeed = 3.0f;
    rIter.tryGetFloatByKey(&mSlideEndSpeed, "SlideEndSpeed");

    mSlideAccel = 0.3f;
    rIter.tryGetFloatByKey(&mSlideAccel, "SlideAccel");

    mSlideMaxSpeed = 30.0f;
    rIter.tryGetFloatByKey(&mSlideMaxSpeed, "SlideMaxSpeed");

    mSlideSideBrake = 0.99f;
    rIter.tryGetFloatByKey(&mSlideSideBrake, "SlideSideBrake");

    mSlideSideAccel = 0.8f;
    rIter.tryGetFloatByKey(&mSlideSideAccel, "SlideSideAccel");

    mSlideSideMaxSpeed = 10.0f;
    rIter.tryGetFloatByKey(&mSlideSideMaxSpeed, "SlideSideMaxSpeed");

    mSlideSideAccelOnLevelLand = 0.5f;
    rIter.tryGetFloatByKey(&mSlideSideAccelOnLevelLand, "SlideSideAccelOnLevelLand");

    mSlideSideMaxSpeedOnLevelLand = 5.0f;
    rIter.tryGetFloatByKey(&mSlideSideMaxSpeedOnLevelLand, "SlideSideMaxSpeedOnLevelLand");

    mSlideBrake = 0.97f;
    rIter.tryGetFloatByKey(&mSlideBrake, "SlideBrake");

    mForceSlideBrake = 0.98f;
    rIter.tryGetFloatByKey(&mForceSlideBrake, "ForceSlideBrake");

    mSlidePostureBlendRate = 0.1f;
    rIter.tryGetFloatByKey(&mSlidePostureBlendRate, "SlidePostureBlendRate");

    mForceSlideSpeed = 5.0f;
    rIter.tryGetFloatByKey(&mForceSlideSpeed, "ForceSlideSpeed");

    mForceSlideSpeedUpRate = 1.5f;
    rIter.tryGetFloatByKey(&mForceSlideSpeedUpRate, "ForceSlideSpeedUpRate");

    mSlideTiltBlendRate = 0.05f;
    rIter.tryGetFloatByKey(&mSlideTiltBlendRate, "SlideTiltBlendRate");

    mSlideTiltMaxDegree = 30.0f;
    rIter.tryGetFloatByKey(&mSlideTiltMaxDegree, "SlideTiltMaxDegree");

    mSlideInvalidFrame = 15;
    rIter.tryGetIntByKey(&mSlideInvalidFrame, "SlideInvalidFrame");

    mForceSlideMaxSpeed = 15.0f;
    rIter.tryGetFloatByKey(&mForceSlideMaxSpeed, "ForceSlideMaxSpeed");

    mSlideFallCancelFrame = 30;
    rIter.tryGetIntByKey(&mSlideFallCancelFrame, "SlideFallCancelFrame");

    mSlideJumpHVelScale = 0.6f;
    rIter.tryGetFloatByKey(&mSlideJumpHVelScale, "SlideJumpHVelScale");

    mHoldShakeInterval = 15;
    rIter.tryGetIntByKey(&mHoldShakeInterval, "HoldShakeInterval");

    mHoldThrowFrontTiming = 3;
    rIter.tryGetIntByKey(&mHoldThrowFrontTiming, "HoldThrowFrontTiming");

    mHoldThrowUpTiming = 3;
    rIter.tryGetIntByKey(&mHoldThrowUpTiming, "HoldThrowUpTiming");

    mHoldJumpFrontVel = 11.0f;
    rIter.tryGetFloatByKey(&mHoldJumpFrontVel, "HoldJumpFrontVel");

    mHoldJumpUpVel = 18.0f;
    rIter.tryGetFloatByKey(&mHoldJumpUpVel, "HoldJumpUpVel");

    mClimbAttackInterval = 14;
    rIter.tryGetIntByKey(&mClimbAttackInterval, "ClimbAttackInterval");

    mClimbAttackWaitInterval = 30;
    rIter.tryGetIntByKey(&mClimbAttackWaitInterval, "ClimbAttackWaitInterval");

    mClimbAttackCancelFrame = 18;
    rIter.tryGetIntByKey(&mClimbAttackCancelFrame, "ClimbAttackCancelFrame");

    mClimbAttackSensorOnFrame = 10;
    rIter.tryGetIntByKey(&mClimbAttackSensorOnFrame, "ClimbAttackSensorOnFrame");

    mClimbBodyAttackFrontVel = 20.0f;
    rIter.tryGetFloatByKey(&mClimbBodyAttackFrontVel, "ClimbBodyAttackFrontVel");

    mClimbBodyAttackDownVel = 17.0f;
    rIter.tryGetFloatByKey(&mClimbBodyAttackDownVel, "ClimbBodyAttackDownVel");

    mClimbBodyAttackFrame = 90;
    rIter.tryGetIntByKey(&mClimbBodyAttackFrame, "ClimbBodyAttackFrame");

    mClimbBodyAttackGravity = 1.2f;
    rIter.tryGetFloatByKey(&mClimbBodyAttackGravity, "ClimbBodyAttackGravity");

    mClimbBodyAttackFallSpeedMax = 30.0f;
    rIter.tryGetFloatByKey(&mClimbBodyAttackFallSpeedMax, "ClimbBodyAttackFallSpeedMax");

    mClimbBodyAttackHBrakeRate = 0.5f;
    rIter.tryGetFloatByKey(&mClimbBodyAttackHBrakeRate, "ClimbBodyAttackHBrakeRate");

    mClimbBodyAttackSideAccel = 0.0f;
    rIter.tryGetFloatByKey(&mClimbBodyAttackSideAccel, "ClimbBodyAttackSideAccel");

    mClimbBodyAttackSideMoveDist = 300.0f;
    rIter.tryGetFloatByKey(&mClimbBodyAttackSideMoveDist, "ClimbBodyAttackSideMoveDist");

    mSinkSandMoveMaxSpeed = 4.0f;
    rIter.tryGetFloatByKey(&mSinkSandMoveMaxSpeed, "SinkSandMoveMaxSpeed");

    mSinkSandMoveMaxDashSpeed = 6.0f;
    rIter.tryGetFloatByKey(&mSinkSandMoveMaxDashSpeed, "SinkSandMoveMaxDashSpeed");

    mSinkSandInvalidFrameInJump = 6;
    rIter.tryGetIntByKey(&mSinkSandInvalidFrameInJump, "SinkSandInvalidFrameInJump");

    mPushedBrakeRate = 0.96f;
    rIter.tryGetFloatByKey(&mPushedBrakeRate, "PushedBrakeRate");

    mPushedBrakeMaxRate = 0.95f;
    rIter.tryGetFloatByKey(&mPushedBrakeMaxRate, "PushedBrakeMaxRate");

    mPushedJumpCancelSpeed = 3.0f;
    rIter.tryGetFloatByKey(&mPushedJumpCancelSpeed, "PushedJumpCancelSpeed");

    mGroundSpinFrame = 60;
    rIter.tryGetIntByKey(&mGroundSpinFrame, "GroundSpinFrame");

    mGroundSpinAccel = 0.5f;
    rIter.tryGetFloatByKey(&mGroundSpinAccel, "GroundSpinAccel");

    mGroundSpinBrake = 0.95f;
    rIter.tryGetFloatByKey(&mGroundSpinBrake, "GroundSpinBrake");

    mGroundSpinVelMax = 10.0f;
    rIter.tryGetFloatByKey(&mGroundSpinVelMax, "GroundSpinVelMax");

    mSpinJumpGravity = 0.4f;
    rIter.tryGetFloatByKey(&mSpinJumpGravity, "SpinJumpGravity");

    mSpinJumpPow = 20.0f;
    rIter.tryGetFloatByKey(&mSpinJumpPow, "SpinJumpPow");

    mSpinAttackInterval = 14;
    rIter.tryGetIntByKey(&mSpinAttackInterval, "SpinAttackInterval");

    mSpinAttackCancelFrame = 30;
    rIter.tryGetIntByKey(&mSpinAttackCancelFrame, "SpinAttackCancelFrame");

    mSpinAttackSensorOnFrame = 20;
    rIter.tryGetIntByKey(&mSpinAttackSensorOnFrame, "SpinAttackSensorOnFrame");

    mSpinAttackJumpPow = 15.0f;
    rIter.tryGetFloatByKey(&mSpinAttackJumpPow, "SpinAttackJumpPow");

    mSpinAttackJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mSpinAttackJumpGravity, "SpinAttackJumpGravity");

    mSpinAttackGroundBrake = 0.0f;
    rIter.tryGetFloatByKey(&mSpinAttackGroundBrake, "SpinAttackGroundBrake");

    mSkateJumpGravity = 0.8f;
    rIter.tryGetFloatByKey(&mSkateJumpGravity, "SkateJumpGravity");

    mSkateJumpPowLow = 16.4f;
    rIter.tryGetFloatByKey(&mSkateJumpPowLow, "SkateJumpPowLow");

    mSkateJumpPow = 18.4f;
    rIter.tryGetFloatByKey(&mSkateJumpPow, "SkateJumpPow");

    mSkateJumpPowCountMax = 11;
    rIter.tryGetIntByKey(&mSkateJumpPowCountMax, "SkateJumpPowCountMax");

    mSkateJumpThreshold = 5.0f;
    rIter.tryGetFloatByKey(&mSkateJumpThreshold, "SkateJumpThreshold");

    mCoopHipDropFrame = 10;
    rIter.tryGetIntByKey(&mCoopHipDropFrame, "CoopHipDropFrame");

    mCoopHipDropRadiusMin = 100.0f;
    rIter.tryGetFloatByKey(&mCoopHipDropRadiusMin, "CoopHipDropRadiusMin");

    mCoopHipDropRadius = 1000.0f;
    rIter.tryGetFloatByKey(&mCoopHipDropRadius, "CoopHipDropRadius");

    mGiantHipDropFrame = 10;
    rIter.tryGetIntByKey(&mGiantHipDropFrame, "GiantHipDropFrame");

    mGiantHipDropRadiusMin = 100.0f;
    rIter.tryGetFloatByKey(&mGiantHipDropRadiusMin, "GiantHipDropRadiusMin");

    mGiantHipDropRadiusMax = 1000.0f;
    rIter.tryGetFloatByKey(&mGiantHipDropRadiusMax, "GiantHipDropRadiusMax");

    mKnockDownVelH = 5.0f;
    rIter.tryGetFloatByKey(&mKnockDownVelH, "KnockDownVelH");

    mKnockDownVelV = 10.0f;
    rIter.tryGetFloatByKey(&mKnockDownVelV, "KnockDownVelV");

    mKnockDownCancelFrame = 15;
    rIter.tryGetIntByKey(&mKnockDownCancelFrame, "KnockDownCancelFrame");

    mReflectJumpGravity = 1.2f;
    rIter.tryGetFloatByKey(&mReflectJumpGravity, "ReflectJumpGravity");

    mReflectJump = 30.0f;
    rIter.tryGetFloatByKey(&mReflectJump, "ReflectJump");

    mRisingReflectJumpHVelBrakeRate = 0.316f;
    rIter.tryGetFloatByKey(&mRisingReflectJumpHVelBrakeRate, "RisingReflectJumpHVelBrakeRate");

    mTossCancelFrame = 25;
    rIter.tryGetIntByKey(&mTossCancelFrame, "TossCancelFrame");

    mManekinekoFallStartFrame = 28;
    rIter.tryGetIntByKey(&mManekinekoFallStartFrame, "ManekinekoFallStartFrame");

    mManekinekoLandFrame = 30;
    rIter.tryGetIntByKey(&mManekinekoLandFrame, "ManekinekoLandFrame");

    mManekinekoEndNoticeFrame = 330;
    rIter.tryGetIntByKey(&mManekinekoEndNoticeFrame, "ManekinekoEndNoticeFrame");

    mManekinekoEndFrame = 480;
    rIter.tryGetIntByKey(&mManekinekoEndFrame, "ManekinekoEndFrame");

    mManekinekoCancelFrame = 30;
    rIter.tryGetIntByKey(&mManekinekoCancelFrame, "ManekinekoCancelFrame");

    mManekinekoFallSpeedInWater = 12.5f;
    rIter.tryGetFloatByKey(&mManekinekoFallSpeedInWater, "ManekinekoFallSpeedInWater");

    mGroomingMaxInterval = 1200;
    rIter.tryGetIntByKey(&mGroomingMaxInterval, "GroomingMaxInterval");

    mGroomingMinInterval = 300;
    rIter.tryGetIntByKey(&mGroomingMinInterval, "GroomingMinInterval");

    mSePropellerBeginStep = 22;
    rIter.tryGetIntByKey(&mSePropellerBeginStep, "SePropellerBeginStep");

    mSeFootNoteNormalVolMul = 0.6f;
    rIter.tryGetFloatByKey(&mSeFootNoteNormalVolMul, "SeFootNoteNormalVolMul");

    mSeFootNoteNormalPitDec = 0.12f;
    rIter.tryGetFloatByKey(&mSeFootNoteNormalPitDec, "SeFootNoteNormalPitDec");

    mSeFootNoteDashVolAdd = 0.35f;
    rIter.tryGetFloatByKey(&mSeFootNoteDashVolAdd, "SeFootNoteDashVolAdd");

    mSeFootNoteDashPitAdd = 0.01f;
    rIter.tryGetFloatByKey(&mSeFootNoteDashPitAdd, "SeFootNoteDashPitAdd");

    mGigaCommonAnimRate = 0.65f;
    rIter.tryGetFloatByKey(&mGigaCommonAnimRate, "GigaCommonAnimRate");

    mGigaMiniRunAnimRateMax = 2.5f;
    rIter.tryGetFloatByKey(&mGigaMiniRunAnimRateMax, "GigaMiniRunAnimRateMax");

    mGigaMiniDashAnimRateMax = 2.5f;
    rIter.tryGetFloatByKey(&mGigaMiniDashAnimRateMax, "GigaMiniDashAnimRateMax");

    mGigaSuperRunAnimRateMax = 2.2f;
    rIter.tryGetFloatByKey(&mGigaSuperRunAnimRateMax, "GigaSuperRunAnimRateMax");

    mGigaSuperDashAnimRateMax = 2.2f;
    rIter.tryGetFloatByKey(&mGigaSuperDashAnimRateMax, "GigaSuperDashAnimRateMax");

    mGigaClimbRunAnimRateMax = 1.8f;
    rIter.tryGetFloatByKey(&mGigaClimbRunAnimRateMax, "GigaClimbRunAnimRateMax");

    mGigaClimbDashAnimRateMax = 2.2f;
    rIter.tryGetFloatByKey(&mGigaClimbDashAnimRateMax, "GigaClimbDashAnimRateMax");

    mGigaNormalMaxSpeed = 145.0f;
    rIter.tryGetFloatByKey(&mGigaNormalMaxSpeed, "GigaNormalMaxSpeed");

    mGigaDashMaxSpeed = 175.0f;
    rIter.tryGetFloatByKey(&mGigaDashMaxSpeed, "GigaDashMaxSpeed");

    mGigaSuperDashSpeed = 175.0f;
    rIter.tryGetFloatByKey(&mGigaSuperDashSpeed, "GigaSuperDashSpeed");

    mGigaInvincibleDashSpeed = 36.0f;
    rIter.tryGetFloatByKey(&mGigaInvincibleDashSpeed, "GigaInvincibleDashSpeed");

    mGigaAccelFrame = 15;
    rIter.tryGetIntByKey(&mGigaAccelFrame, "GigaAccelFrame");

    mGigaSquatWalkSpeed = 90.0f;
    rIter.tryGetFloatByKey(&mGigaSquatWalkSpeed, "GigaSquatWalkSpeed");

    mGigaGroundSpinAccel = 7.0f;
    rIter.tryGetFloatByKey(&mGigaGroundSpinAccel, "GigaGroundSpinAccel");

    mGigaGroundSpinBrake = 0.9f;
    rIter.tryGetFloatByKey(&mGigaGroundSpinBrake, "GigaGroundSpinBrake");

    mGigaKnockDownVelH = 50.0f;
    rIter.tryGetFloatByKey(&mGigaKnockDownVelH, "GigaKnockDownVelH");

    mGigaKnockDownVelV = 50.0f;
    rIter.tryGetFloatByKey(&mGigaKnockDownVelV, "GigaKnockDownVelV");

    mGigaLeftFootHrTime = 70.0f;
    rIter.tryGetFloatByKey(&mGigaLeftFootHrTime, "GigaLeftFootHrTime");

    mGigaRightFootHrTime = 10.0f;
    rIter.tryGetFloatByKey(&mGigaRightFootHrTime, "GigaRightFootHrTime");

    mGigaClimbLeftWalkHrTime = 50.0f;
    rIter.tryGetFloatByKey(&mGigaClimbLeftWalkHrTime, "GigaClimbLeftWalkHrTime");

    mGigaClimbRightWalkHrTime = 100.0f;
    rIter.tryGetFloatByKey(&mGigaClimbRightWalkHrTime, "GigaClimbRightWalkHrTime");

    mGigaClimbLeftRunHrTime = 50.0f;
    rIter.tryGetFloatByKey(&mGigaClimbLeftRunHrTime, "GigaClimbLeftRunHrTime");

    mGigaClimbRightRunHrTime = 100.0f;
    rIter.tryGetFloatByKey(&mGigaClimbRightRunHrTime, "GigaClimbRightRunHrTime");

    mGigaRoundLimitDegreeMax = 6.0f;
    rIter.tryGetFloatByKey(&mGigaRoundLimitDegreeMax, "GigaRoundLimitDegreeMax");

    mGigaRoundLimitDegreeMin = 6.0f;
    rIter.tryGetFloatByKey(&mGigaRoundLimitDegreeMin, "GigaRoundLimitDegreeMin");

    mGigaNormalRollingMinSpeed = 230.0f;
    rIter.tryGetFloatByKey(&mGigaNormalRollingMinSpeed, "GigaNormalRollingMinSpeed");

    mGigaNormalGravityAddition = 40.0f;
    rIter.tryGetFloatByKey(&mGigaNormalGravityAddition, "GigaNormalGravityAddition");

    mGigaSquatBrakeRate = 0.94f;
    rIter.tryGetFloatByKey(&mGigaSquatBrakeRate, "GigaSquatBrakeRate");

    mGigaGravity = 12.0f;
    rIter.tryGetFloatByKey(&mGigaGravity, "GigaGravity");

    mGigaFallSpeedMax = 1200.0f;
    rIter.tryGetFloatByKey(&mGigaFallSpeedMax, "GigaFallSpeedMax");

    mGigaFloatFallSpeedMax = 320.0f;
    rIter.tryGetFloatByKey(&mGigaFloatFallSpeedMax, "GigaFloatFallSpeedMax");

    mGigaJumpPow = 250.0f;
    rIter.tryGetFloatByKey(&mGigaJumpPow, "GigaJumpPow");

    mGigaJumpPowLow = 250.0f;
    rIter.tryGetFloatByKey(&mGigaJumpPowLow, "GigaJumpPowLow");

    mGigaJumpPowCountMax = 20;
    rIter.tryGetIntByKey(&mGigaJumpPowCountMax, "GigaJumpPowCountMax");

    mGigaJumpCancelBrakeRate = 0.9f;
    rIter.tryGetFloatByKey(&mGigaJumpCancelBrakeRate, "GigaJumpCancelBrakeRate");

    mGigaJumpCancelMinSpeed = 400.0f;
    rIter.tryGetFloatByKey(&mGigaJumpCancelMinSpeed, "GigaJumpCancelMinSpeed");

    mGigaHipDropSpeed = 480.0f;
    rIter.tryGetFloatByKey(&mGigaHipDropSpeed, "GigaHipDropSpeed");

    mGigaHipDropAnimRate = 0.9f;
    rIter.tryGetFloatByKey(&mGigaHipDropAnimRate, "GigaHipDropAnimRate");

    mGigaHipDropJumpPow = 450.0f;
    rIter.tryGetFloatByKey(&mGigaHipDropJumpPow, "GigaHipDropJumpPow");

    mGigaTrampleJump = 160.0f;
    rIter.tryGetFloatByKey(&mGigaTrampleJump, "GigaTrampleJump");

    mGigaLongJumpSlowSpeed = 170.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpSlowSpeed, "GigaLongJumpSlowSpeed");

    mGigaLongJumpFastSpeed = 170.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpFastSpeed, "GigaLongJumpFastSpeed");

    mGigaLongJumpSlowJumpPow = 170.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpSlowJumpPow, "GigaLongJumpSlowJumpPow");

    mGigaLongJumpFastJumpPow = 170.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpFastJumpPow, "GigaLongJumpFastJumpPow");

    mGigaLongJumpSlowGravity = 5.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpSlowGravity, "GigaLongJumpSlowGravity");

    mGigaLongJumpFastGravity = 5.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpFastGravity, "GigaLongJumpFastGravity");

    mGigaSquatJumpGravity = 8.5f;
    rIter.tryGetFloatByKey(&mGigaSquatJumpGravity, "GigaSquatJumpGravity");

    mGigaSquatJumpPow = 290.0f;
    rIter.tryGetFloatByKey(&mGigaSquatJumpPow, "GigaSquatJumpPow");

    mGigaSquatHighJumpPow = 380.0f;
    rIter.tryGetFloatByKey(&mGigaSquatHighJumpPow, "GigaSquatHighJumpPow");

    mGigaSquatJumpBackPow = 0.0f;
    rIter.tryGetFloatByKey(&mGigaSquatJumpBackPow, "GigaSquatJumpBackPow");

    mGigaSpinJumpGravity = 6.0f;
    rIter.tryGetFloatByKey(&mGigaSpinJumpGravity, "GigaSpinJumpGravity");

    mGigaSpinJumpPow = 300.0f;
    rIter.tryGetFloatByKey(&mGigaSpinJumpPow, "GigaSpinJumpPow");

    mGigaNormalRollingAttackJumpGravity = 7.0f;
    rIter.tryGetFloatByKey(&mGigaNormalRollingAttackJumpGravity,
                           "GigaNormalRollingAttackJumpGravity");

    mGigaNormalRollingAttackJumpPow = 190.0f;
    rIter.tryGetFloatByKey(&mGigaNormalRollingAttackJumpPow, "GigaNormalRollingAttackJumpPow");

    mGigaNormalRollingAttackVelH = 260.0f;
    rIter.tryGetFloatByKey(&mGigaNormalRollingAttackVelH, "GigaNormalRollingAttackVelH");

    mGigaWallJumpHSpeed = 125.0f;
    rIter.tryGetFloatByKey(&mGigaWallJumpHSpeed, "GigaWallJumpHSpeed");

    mGigaLongJumpBrake = 5.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpBrake, "GigaLongJumpBrake");

    mGigaLongJumpSpeedMin = 25.0f;
    rIter.tryGetFloatByKey(&mGigaLongJumpSpeedMin, "GigaLongJumpSpeedMin");

    mGigaLongJumpSideAccel = 1.25f;
    rIter.tryGetFloatByKey(&mGigaLongJumpSideAccel, "GigaLongJumpSideAccel");

    mGigaLandFrame = 12;
    rIter.tryGetIntByKey(&mGigaLandFrame, "GigaLandFrame");

    mGigaWallClimbMaxSpeed = 150.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbMaxSpeed, "GigaWallClimbMaxSpeed");

    mGigaWallClimbDashMaxSpeed = 200.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbDashMaxSpeed, "GigaWallClimbDashMaxSpeed");

    mGigaWallClimbAccel = 30.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbAccel, "GigaWallClimbAccel");

    mGigaWallClimbMaxSideSpeed = 120.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbMaxSideSpeed, "GigaWallClimbMaxSideSpeed");

    mGigaWallClimbDashMaxSideSpeed = 160.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbDashMaxSideSpeed, "GigaWallClimbDashMaxSideSpeed");

    mGigaWallClimbSideAccel = 30.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbSideAccel, "GigaWallClimbSideAccel");

    mGigaWallSnapDistance = 1400.0f;
    rIter.tryGetFloatByKey(&mGigaWallSnapDistance, "GigaWallSnapDistance");

    mGigaWallClimbJumpPowLow = 300.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbJumpPowLow, "GigaWallClimbJumpPowLow");

    mGigaWallClimbJumpPow = 300.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbJumpPow, "GigaWallClimbJumpPow");

    mGigaWallClimbSlideGravity = 1.3f;
    rIter.tryGetFloatByKey(&mGigaWallClimbSlideGravity, "GigaWallClimbSlideGravity");

    mGigaWallClimbSlideMaxSpeed = 150.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbSlideMaxSpeed, "GigaWallClimbSlideMaxSpeed");

    mGigaWallClimbSlideSideAccel = 1.5f;
    rIter.tryGetFloatByKey(&mGigaWallClimbSlideSideAccel, "GigaWallClimbSlideSideAccel");

    mGigaWallClimbSlideSideMaxSpeed = 30.0f;
    rIter.tryGetFloatByKey(&mGigaWallClimbSlideSideMaxSpeed, "GigaWallClimbSlideSideMaxSpeed");

    mFlashRangeAttackLengthOffset = 100.0f;
    rIter.tryGetFloatByKey(&mFlashRangeAttackLengthOffset, "FlashRangeAttackLengthOffset");

    mFlashRangeAttackDegreeScale = 0.7f;
    rIter.tryGetFloatByKey(&mFlashRangeAttackDegreeScale, "FlashRangeAttackDegreeScale");

    mFlashRangeAttackHeightMax = 0.0f;
    rIter.tryGetFloatByKey(&mFlashRangeAttackHeightMax, "FlashRangeAttackHeightMax");

    mFlashRangeAttackHeightMin = -200.0f;
    rIter.tryGetFloatByKey(&mFlashRangeAttackHeightMin, "FlashRangeAttackHeightMin");

    mIsEnableHeadLightOfx = 0;
    rIter.tryGetIntByKey(&mIsEnableHeadLightOfx, "IsEnableHeadLightOfx");

    mHeadLightOfxOffsetY = 50.0f;
    rIter.tryGetFloatByKey(&mHeadLightOfxOffsetY, "HeadLightOfxOffsetY");

    mHeadLightOfxOffsetZ = 50.0f;
    rIter.tryGetFloatByKey(&mHeadLightOfxOffsetZ, "HeadLightOfxOffsetZ");

    mHeadLightPrePassPointLightRadiusScale = 0.7f;
    rIter.tryGetFloatByKey(&mHeadLightPrePassPointLightRadiusScale,
                           "HeadLightPrePassPointLightRadiusScale");

    mHeadLightPrePassPointLightOffsetY = -35.0f;
    rIter.tryGetFloatByKey(&mHeadLightPrePassPointLightOffsetY,
                           "HeadLightPrePassPointLightOffsetY");

    mHeadLightPrePassPointLightOffsetZ = -200.0f;
    rIter.tryGetFloatByKey(&mHeadLightPrePassPointLightOffsetZ,
                           "HeadLightPrePassPointLightOffsetZ");
}
