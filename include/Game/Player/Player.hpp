#pragma once

#include <basis/seadTypes.h>

namespace al {
class ActorInitInfo;
}

class IUseCollisionPartsMtx;
class IUsePlayerActionGraphBuilder;
class IUsePlayerActualMove;
class IUsePlayerAirTurnCheck;
class IUsePlayerAirTurnObserver;
class IUsePlayerAnimator;
class IUsePlayerAttack;
class IUsePlayerAudio;
class IUsePlayerBind;
class IUsePlayerBindEndParamGetter;
class IUsePlayerCeilingCheck;
class IUsePlayerCharaQuery;
class IUsePlayerCheckArea;
class IUsePlayerClimbJumpInfo;
class IUsePlayerCollision;
class IUsePlayerCollisionCheckArrow;
class IUsePlayerCollisionCheckedObserver;
class IUsePlayerCollisionCheckSphere;
class IUsePlayerCollisionCheckSphereMove;
class IUsePlayerCollisionPartsArray;
class IUsePlayerDamageInvalidCheck;
class IUsePlayerDamageObserver;
class IUsePlayerDashChecker;
class IUsePlayerDoubleMarioCheck;
class IUsePlayerDoubleMarioSpeed;
class IUsePlayerEffect;
class IUsePlayerEndBind;
class IUsePlayerEventReceiver;
class IUsePlayerFireBallLauncher;
class IUsePlayerFlag;
class IUsePlayerFlagControl;
class IUsePlayerForwardBent;
class IUsePlayerGetPos;
class IUsePlayerGlideInhibitor;
class IUsePlayerHeightChecker;
class IUsePlayerHipDropObserver;
class IUsePlayerHolded;
class IUsePlayerHorizontalSpeedAverage;
class IUsePlayerInput;
class IUsePlayerInputArranger;
class IUsePlayerInvincibleDash;
class IUsePlayerLifeControl;
class IUsePlayerLongFallCheck;
class IUsePlayerMoveSpeedScaler;
class IUsePlayerPropellerInhibitor;
class IUsePlayerPropellerJumpPhase;
class IUsePlayerPushed;
class IUsePlayerRaccoonDogFallTask;
class IUsePlayerReaction;
class IUsePlayerSensorControl;
class IUsePlayerSizeTrigger;
class IUsePlayerSnapWallInfo;
class IUsePlayerStoneStatue;
class IUsePlayerSubAction;
class IUsePlayerWallClimbInfo;
class IUsePlayerWallClimbInput;
class IUsePlayerWaterFlowField;
class IUsePlayerWaterSurfaceInfo;
class PlayerActionGraph;
class PlayerActionGraphRestarter;
class PlayerActionKnockDown;
class PlayerActionNode;
class PlayerActor;
class PlayerActualMove;
class PlayerAirTurnChecker;
class PlayerAmiiboDirector;
class PlayerAmiiboDirectorWatcher;
class PlayerBindControl;
class PlayerCeilingCheck;
class PlayerClimbAirAttackInhibitor;
class PlayerCollisionSize;
class PlayerConstParam;
class PlayerContinuousJump;
class PlayerCounterAfterPunch;
class PlayerDamageControl;
class PlayerDamageInvalidater;
class PlayerEquipmentDirector;
class PlayerFigureChangeObserver;
class PlayerFigureDirector;
class PlayerFlightDurationInhibitor;
class PlayerForwardBent;
class PlayerGiantDirector;
class PlayerGigaDirector;
class PlayerGlideInhibitor;
class PlayerHeightChecker;
class PlayerHorizontalSpeedAverage;
class PlayerInkChecker;
class PlayerInvincibleDash;
class PlayerInvincibleState;
class PlayerKiller;
class PlayerLandingChecker;
class PlayerLandingInformer;
class PlayerLongFallCheck;
class PlayerPropellerInhibitor;
class PlayerRaccoonDogFallTask;
class PlayerSimpleFlag;
class PlayerSizeTrigger;
class PlayerSpinJumpChecker;
class PlayerSubAction;
class PlayerSuperDashResetter;
class PlayerSwimSquatInhibitor;
class PlayerTrigger;
class PlayerWallJumpInfo;
class SinkSandControl;
struct PlayerProperty;

/// The player's action logic, shared by all the characters' actors.
class Player {
public:
    Player(IUsePlayerActionGraphBuilder* pBuilder, IUsePlayerInput* pInput,
           IUsePlayerInputArranger* pInputArranger, IUsePlayerAnimator* pAnimator,
           IUsePlayerEffect* pEffect, IUsePlayerAudio* pAudio, IUsePlayerBind* pBind,
           IUsePlayerCheckArea* pCheckArea, IUsePlayerCollision* pCollision,
           IUsePlayerCollisionPartsArray* pCollisionPartsArray, IUsePlayerSnapWallInfo* pSnapWallInfo,
           IUsePlayerCollisionCheckSphere* pCheckSphere,
           IUsePlayerCollisionCheckSphereMove* pCheckSphereMove,
           IUsePlayerCollisionCheckArrow* pCheckArrow, IUsePlayerFireBallLauncher* pFireBallLauncher,
           IUsePlayerFireBallLauncher* pBoomerangLauncher, IUsePlayerAttack* pAttack,
           IUsePlayerGetPos* pGetPos, IUsePlayerBindEndParamGetter* pBindEndParamGetter,
           IUseCollisionPartsMtx* pCollisionPartsMtx, IUsePlayerWaterFlowField* pWaterFlowField,
           IUsePlayerReaction* pReaction, const IUsePlayerWaterSurfaceInfo* pWaterSurfaceInfo,
           IUsePlayerStoneStatue* pStoneStatue, IUsePlayerHipDropObserver* pHipDropObserver,
           IUsePlayerEventReceiver* pEventReceiver, const IUsePlayerCharaQuery* pCharaQuery,
           IUsePlayerHolded* pHolded, const IUsePlayerWallClimbInfo* pWallClimbInfo,
           const IUsePlayerFlag* pFlag1, const IUsePlayerFlag* pFlag2,
           const IUsePlayerFlag* pFlag3, const IUsePlayerFlag* pFlag4,
           IUsePlayerWallClimbInput* pWallClimbInput, const IUsePlayerPushed* pPushed,
           const IUsePlayerDoubleMarioSpeed* pDoubleMarioSpeed,
           const IUsePlayerDoubleMarioCheck* pDoubleMarioCheck,
           IUsePlayerSensorControl* pSensorControl, const IUsePlayerFlag* pFlag5,
           const IUsePlayerFlag* pClimbFlag, IUsePlayerFlagControl* pFlagControl,
           const IUsePlayerMoveSpeedScaler* pMoveSpeedScaler, const IUsePlayerFlag* pFlag6,
           const IUsePlayerFlag* pFlag7, const IUsePlayerGetPos* pGetPosConst,
           const IUsePlayerClimbJumpInfo* pClimbJumpInfo, const PlayerConstParam* pConstParam,
           bool isGigaValid, PlayerActor* pActor);

    void init();
    void update();
    void updateHelpMario();
    void updateCollisionSize();
    void notifyCollisionCheckedObserver();
    void arrangeVelocity();
    void updateEffect();
    void updateSound();
    void setCollisionCheckSphere(IUsePlayerCollisionCheckSphere* pCheckSphere);
    void setCollisionCheckArrow(IUsePlayerCollisionCheckArrow* pCheckArrow);
    void setDamageObserver(IUsePlayerDamageObserver* pObserver);
    void invalidateCeilingCheck();
    IUsePlayerHipDropObserver* getHipDropObserver();
    IUsePlayerHeightChecker* getHeightChecker();
    IUsePlayerDamageInvalidCheck* getDamageInvalidater() const;
    IUsePlayerSubAction* getSubAction();
    IUsePlayerCeilingCheck* getCeilingCheck();
    const IUsePlayerAirTurnCheck* getAirTurnChecker() const;
    IUsePlayerAirTurnObserver* getAirTurnObserver();
    const IUsePlayerInvincibleDash* getInvincibleDash() const;
    const IUsePlayerWaterFlowField* getWaterFlowField() const;
    const IUsePlayerWaterSurfaceInfo* getWaterSurfaceInfo() const;
    IUsePlayerForwardBent* getForwardBent();
    IUsePlayerHorizontalSpeedAverage* getHorizontalSpeedAverage();
    const IUsePlayerSizeTrigger* getSizeTrigger() const;
    IUsePlayerRaccoonDogFallTask* getRaccoonDogFallTask();
    IUsePlayerLongFallCheck* getLongFallCheck();
    IUsePlayerPropellerInhibitor* getPropellerInhibitor();
    const IUsePlayerActualMove* getActualMove() const;
    IUsePlayerGlideInhibitor* getGlideInhibitor();
    void createAmiiboDirector(const PlayerActor* pActor, PlayerAmiiboDirectorWatcher* pWatcher,
                              const al::ActorInitInfo& rInfo);
    void setAmiiboDirector(PlayerAmiiboDirector* pDirector, const PlayerActor* pActor);
    bool isOnGround() const;
    void createBindControl(PlayerActionGraph* pActionGraph, IUsePlayerEndBind* pEndBind,
                           PlayerActionNode* pBindNode);
    void createPlayerKiller(PlayerActionGraph* pActionGraph, PlayerActionNode* pDieNode);
    void createRestarter(PlayerActionGraph* pActionGraph, PlayerActionNode* pRestartNode);
    void createPropellerJumpPhase(IUsePlayerPropellerJumpPhase* pPropellerJumpPhase);
    void createDashChecker(IUsePlayerDashChecker* pDashChecker);
    void createKnockDownSetter(PlayerActionKnockDown* pKnockDown);
    void createGiantAction(PlayerActionGraph* pActionGraph, PlayerActionNode* pStartNode,
                           PlayerActionNode* pEndNode);
    void createGigaAction(PlayerActionGraph* pActionGraph, PlayerActionNode* pStartNode,
                          PlayerActionNode* pEndNode, PlayerActionNode* pClimbStartNode);
    void reflectCeiling(f32 power);

    PlayerProperty* getProperty() const { return mProperty; }
    IUsePlayerCollision* getCollision() const { return mCollision; }
    PlayerFigureDirector* getFigureDirector() const { return mFigureDirector; }
    PlayerKiller* getKiller() const { return mKiller; }
    PlayerActionGraph* getActionGraph() const { return mActionGraph; }
    IUsePlayerLifeControl* getLifeControl() const { return mLifeControl; }
    PlayerInvincibleState* getInvincibleState() const { return mInvincibleState; }
    PlayerActionGraphRestarter* getRestarter() const { return mRestarter; }
    PlayerEquipmentDirector* getEquipmentDirector() const { return mEquipmentDirector; }
    IUsePlayerPropellerJumpPhase* getPropellerJumpPhase() const { return mPropellerJumpPhase; }
    const IUsePlayerCharaQuery* getCharaQuery() const { return mCharaQuery; }
    IUsePlayerDashChecker* getDashChecker() const { return mDashChecker; }
    PlayerGiantDirector* getGiantDirector() const { return mGiantDirector; }
    IUsePlayerAnimator* getAnimator() const { return mAnimator; }
    PlayerGigaDirector* getGigaDirector() const { return mGigaDirector; }

private:
    PlayerProperty* mProperty;  // 0x0
    IUsePlayerCollisionCheckSphere* mCheckSphere;  // 0x8
    IUsePlayerCollisionCheckArrow* mCheckArrow;  // 0x10
    IUsePlayerInput* mInput;  // 0x18
    IUsePlayerInputArranger* mInputArranger;  // 0x20
    IUsePlayerCollision* mCollision;  // 0x28
    IUsePlayerCollisionPartsArray* mCollisionPartsArray;  // 0x30
    IUsePlayerSnapWallInfo* mSnapWallInfo;  // 0x38
    PlayerCollisionSize* mCollisionSize;  // 0x40
    IUsePlayerAnimator* mAnimator;  // 0x48
    PlayerTrigger* mTrigger;  // 0x50
    IUsePlayerAudio* mAudio;  // 0x58
    PlayerBindControl* mBindControl;  // 0x60
    IUsePlayerCheckArea* mCheckArea;  // 0x68
    PlayerHeightChecker* mHeightChecker;  // 0x70
    PlayerDamageInvalidater* mDamageInvalidater;  // 0x78
    bool _80;  // makes cutVelocity() cut harder
    PlayerFigureDirector* mFigureDirector;  // 0x88
    PlayerFigureChangeObserver* mFigureChangeObserver;  // 0x90
    PlayerKiller* mKiller;  // 0x98
    PlayerActionGraph* mActionGraph;  // 0xa0
    IUsePlayerBind* mBind;  // 0xa8
    IUsePlayerLifeControl* mLifeControl;  // 0xb0
    PlayerInvincibleState* mInvincibleState;  // 0xb8
    PlayerSubAction* mSubAction;  // 0xc0
    IUsePlayerGetPos* mGetPos;  // 0xc8
    IUsePlayerEffect* mEffect;  // 0xd0
    bool mIsActionShifted;  // 0xd8
    PlayerCeilingCheck* mCeilingCheck;  // 0xe0
    IUsePlayerBindEndParamGetter* mBindEndParamGetter;  // 0xe8
    PlayerLandingChecker* mLandingChecker;  // 0xf0
    PlayerContinuousJump* mContinuousJump;  // 0xf8
    IUsePlayerReaction* mReaction;  // 0x100
    PlayerAirTurnChecker* mAirTurnChecker;  // 0x108
    PlayerDamageControl* mDamageControl;  // 0x110
    PlayerInvincibleDash* mInvincibleDash;  // 0x118
    PlayerActionGraphRestarter* mRestarter;  // 0x120
    IUsePlayerCollisionCheckedObserver* mCollisionCheckedObserver;  // 0x128
    IUsePlayerWaterFlowField* mWaterFlowField;  // 0x130
    const IUsePlayerWaterSurfaceInfo* mWaterSurfaceInfo;  // 0x138
    PlayerForwardBent* mForwardBent;  // 0x140
    PlayerWallJumpInfo* mWallJumpInfo;  // 0x148
    PlayerHorizontalSpeedAverage* mHorizontalSpeedAverage;  // 0x150
    PlayerSizeTrigger* mSizeTrigger;  // 0x158
    PlayerEquipmentDirector* mEquipmentDirector;  // 0x160
    IUsePlayerStoneStatue* mStoneStatue;  // 0x168
    IUsePlayerHipDropObserver* mHipDropObserver;  // 0x170
    PlayerRaccoonDogFallTask* mRaccoonDogFallTask;  // 0x178
    PlayerSimpleFlag* mFlag180;  // 0x180
    PlayerLongFallCheck* mLongFallCheck;  // 0x188
    PlayerPropellerInhibitor* mPropellerInhibitor;  // 0x190
    PlayerActualMove* mActualMove;  // 0x198
    PlayerLandingInformer* mLandingInformer;  // 0x1a0
    PlayerGlideInhibitor* mGlideInhibitor;  // 0x1a8
    PlayerCounterAfterPunch* mCounterAfterPunch;  // 0x1b0
    IUsePlayerPropellerJumpPhase* mPropellerJumpPhase;  // 0x1b8
    IUsePlayerAttack* mAttack;  // 0x1c0
    PlayerSwimSquatInhibitor* mSwimSquatInhibitor;  // 0x1c8
    IUsePlayerEventReceiver* mEventReceiver;  // 0x1d0
    const IUsePlayerCharaQuery* mCharaQuery;  // 0x1d8
    IUsePlayerHolded* mHolded;  // 0x1e0
    const IUsePlayerWallClimbInfo* mWallClimbInfo;  // 0x1e8
    PlayerSimpleFlag* mFlag1f0;  // 0x1f0
    PlayerSimpleFlag* mFlag1f8;  // 0x1f8
    PlayerSimpleFlag* mSinkSandFlag;  // 0x200
    IUsePlayerDashChecker* mDashChecker;  // 0x208
    const IUsePlayerFlag* mFlag1;  // 0x210
    const IUsePlayerFlag* mFlag2;  // 0x218
    const IUsePlayerFlag* mFlag3;  // 0x220
    const IUsePlayerFlag* mFlag4;  // 0x228
    IUsePlayerWallClimbInput* mWallClimbInput;  // 0x230
    SinkSandControl* mSinkSandControl;  // 0x238
    PlayerInkChecker* mInkChecker;  // 0x240
    const IUsePlayerPushed* mPushed;  // 0x248
    const IUsePlayerDoubleMarioSpeed* mDoubleMarioSpeed;  // 0x250
    const IUsePlayerDoubleMarioCheck* mDoubleMarioCheck;  // 0x258
    PlayerClimbAirAttackInhibitor* mClimbAirAttackInhibitor;  // 0x260
    PlayerFlightDurationInhibitor* mFlightDurationInhibitor;  // 0x268
    IUsePlayerSensorControl* mSensorControl;  // 0x270
    PlayerSpinJumpChecker* mSpinJumpChecker;  // 0x278
    PlayerGiantDirector* mGiantDirector;  // 0x280
    PlayerGigaDirector* mGigaDirector;  // 0x288
    PlayerAmiiboDirector* mAmiiboDirector;  // 0x290
    PlayerSuperDashResetter* mSuperDashResetter;  // 0x298
    const IUsePlayerFlag* mFlag5;  // 0x2a0
    PlayerActionKnockDown* mKnockDown;  // 0x2a8
    PlayerSimpleFlag* mFlag2b0;  // 0x2b0
    const IUsePlayerFlag* mClimbFlag;  // 0x2b8, keeps the climbing collision size when on
    IUsePlayerFlagControl* mFlagControl;  // 0x2c0
    PlayerSimpleFlag* mFlag2c8;  // 0x2c8
    const IUsePlayerMoveSpeedScaler* mMoveSpeedScaler;  // 0x2d0
    const IUsePlayerFlag* mFlag6;  // 0x2d8
    const IUsePlayerFlag* mFlag7;  // 0x2e0
    const IUsePlayerGetPos* mGetPosConst;  // 0x2e8
    const IUsePlayerClimbJumpInfo* mClimbJumpInfo;  // 0x2f0
    const PlayerConstParam* mConstParam;  // 0x2f8
    PlayerActor* mActor;  // 0x300
    bool mIsFirstUpdate;  // 0x308
    bool mIsValidCeilingCheck;  // 0x309
    bool mIsGigaValid;  // 0x30a, only in Bowser's Fury
    void* _310;
    s32 _318;
    s32 mUpdateCount;  // 0x31c
};
static_assert(sizeof(Player) == 0x320);
