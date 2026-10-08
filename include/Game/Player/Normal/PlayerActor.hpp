#pragma once

#include <basis/seadTypes.h>
#include <gfx/seadColor.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Player/PlayerDef.hpp"

namespace al {
class ComboCounter;
class LayoutInitInfo;
class PadRumbleKeeper;
}  // namespace al

class IUsePlayerKeyConfig;
class IUsePlayerRetargettingInfoCreator;
class IUsePlayerRetargettingSelector;
class IUsePlayerActionGraphBuilder;
class PlayerBigBgmController;
class PlayerInvincibleBgmController;
class IUsePlayerModelVisibility;
class IUsePlayerFlagSwitch;
class IUsePlayerPuppet;
class Player;
class PlayerActionObserver;
class PlayerAmiiboDirector;
class PlayerAmiiboDirectorWatcher;
class PlayerAudio;
class PlayerGigaDirector;
class PlayerConstParam;
class PlayerInput;
class PlayerModelHolder;
struct PlayerProperty;

/// The actor of a playable character (Mario, Luigi, ...).
class PlayerActor : public al::LiveActor {
public:
    explicit PlayerActor(const sead::Matrix34f* pViewMtx);
    void initSpecial(const al::ActorInitInfo& rInfo, s32 port, const char* pCharacterName,
                     IUsePlayerRetargettingInfoCreator* pCreator,
                     IUsePlayerRetargettingSelector* pSelector,
                     IUsePlayerActionGraphBuilder* pBuilder, const char* pName,
                     u32 playerType, s32 index, const char* pSuffix);
    void initNameplate(const al::LayoutInitInfo& rInfo);
    void setInvincibleBgmController(PlayerInvincibleBgmController* pController) {
        mInvincibleBgmController = pController;
    }
    al::PadRumbleKeeper* getPadRumbleKeeper() const { return mPadRumbleKeeper; }
    void setBigBgmController(PlayerBigBgmController* pController);
    virtual void permitBind();
    virtual void clearBindable();
    virtual void cancelBind();
    virtual void sendMsgBindDamage();
    virtual void attackByTail();
    virtual void startAttackLoopByTail();
    virtual void endAttackLoopByTail();
    virtual void notifyReaction(const char* pName);
    virtual void notifyDamage();
    virtual void notifyDie();
    virtual void notifyOnFloorTrig();
    virtual void onAbyss();
    virtual void onDying();
    virtual void onVanishDying();
    virtual void onForceDying();
    virtual void onDamage();
    virtual void onRevive();
    virtual void onWarpStart();
    virtual void onWarpEnd();
    virtual void onInvincibleStart();
    virtual void onInvincibleBgmEnd(u32 frame);
    virtual void onInvincibleEnd();
    virtual void onInvincibleRestart();
    virtual void onInvincibleGetStar();
    virtual void onInvincibleCancel(bool isForce);
    virtual void onGiantStart();
    virtual void onGiantEnd();
    virtual void onGiantRestart();
    virtual void onGiantCancel();
    virtual void onGigaStart();
    virtual void onGigaStartByBell();
    virtual void onGigaEnd();
    virtual void onGigaRestart();
    virtual void onGigaCancel();
    virtual void onLanding();
    virtual void onWallJump();
    virtual void onWallFall();
    virtual void onBodyAttackStart();
    virtual void onBodyAttackLanding();
    virtual void onBodyAttackStickWall();
    virtual void onHipDropStart();
    virtual void onHipDropLand();
    virtual void onHipDropLandLoop();
    virtual void onClimbAttackStart();
    virtual void onClimbAttackEnd();
    virtual void onSuperDash();
    virtual void onSuperDashLand();
    virtual void onSetDashTime(s32 time);
    virtual void onSetModifiedDashTime(s32 time);
    virtual void onSetFlingPoleDashTime(s32 time);
    virtual void onChangeAction();
    virtual void onRaccoonDogFallStart();
    virtual void onPunchHit();
    virtual void onWallClimbReady();
    virtual void onWallHit();
    virtual void onSpinAttackStart();
    virtual void onSpinAttackEnd();
    virtual void onKickGroundInWater();
    virtual void onKnockDown();
    virtual void onManekinekoDropStart();
    virtual void onManekinekoDropFallStart();
    virtual void onManekinekoDropLand();
    virtual void onManekinekoDropEndNotice();
    virtual void onManekinekoDropEnd();
    virtual void onHoldedEnd();
    virtual void onSwimDiveLand();
    virtual void onForceKill();
    virtual void requestFlingPoleFlagClear();
    virtual void requestDashFlagClear();
    virtual void onCancelJumpAudio();
    virtual EPlayerChara getChara() const;
    virtual bool isChara(EPlayerChara chara) const;
    virtual void validatePrimeSensors();
    virtual void invalidatePrimeSensors();
    virtual void queryHoldedPosture(sead::Vector3f* pTrans, sead::Vector3f* pFront,
                                    sead::Vector3f* pUp) const;
    virtual void queryHoldingHostVelocity(sead::Vector3f* pVelocity) const;
    virtual void pushHoldingHost(const sead::Vector3f& rPush);
    virtual void queryHoldedPostureOffset(sead::Vector3f* pOffset) const;
    virtual void queryHoldedHostMoveDir(sead::Vector3f& rDir) const;
    virtual void requestClearHoldedHost();
    virtual al::ComboCounter* tryGetTrampleCounter();
    virtual al::ComboCounter* tryGetSlidingCounter();
    virtual al::ComboCounter* tryGetInvincibleComboCounter();
    virtual f32 getSpeedScale() const;
    virtual void updateDoubleMario();
    virtual void updateHold();

    const char* getFigureTypeName();
    const char* getRealFigureTypeName();
    bool isManekinekoAlive() const;
    void requestRelease();
    PlayerProperty* getProperty();
    const PlayerProperty* getProperty() const;
    void updatePosture();
    IUsePlayerFlagSwitch* createInterfaceSilhouetteHiddenFlag();

    /**
     * @brief Sets whether the title scene plays a change demo with this player.
     * @param isChange True while the change demo plays.
     */
    void setTitleDemoChange(bool isChange) { mIsTitleDemoChange = isChange; }
    void copyNameplate(al::LiveActor* pActor);
    void setNameplateVisible(bool isVisible, bool isForce);
    void setEnableSingleJoyCamera(bool isEnable);
    s32 getInputPort() const;
    void replaceInputPort(s32 port);
    void appearSingleMode(bool isDemo);
    void setSingleModeInput(bool isEnable);
    const IUsePlayerKeyConfig* getKeyConfig() const;
    IUsePlayerPuppet* getPlayerPuppet();
    void requestBind(al::HitSensor* pSensor, f32 priority, s32 type);
    void cancelRequestBind(al::HitSensor* pSensor);
    void cancelBindForDemo();
    void cancelForDemo();
    void cancelDeathAnim();
    void forceKill();
    void pauseInvincible(bool isPauseBgm);
    void resumeInvincible(bool isResumeBgm);
    void setInvicibleBgmState(bool isOn);
    bool isInRouteDokanOrDokan() const;
    bool isInDokanNotRouteDokan() const;
    bool isInInkLimiter() const;
    bool isInKoura() const;
    void logGetItem(const al::HitSensor* pItemSensor);
    bool isTreeClimbing() const;
    bool isOnFloor(const al::LiveActor* pFloorActor) const;
    bool isHoldingAnotherPlayer() const;
    void hideHoldingItem(bool isHide, bool isHideShadow);
    void cancelSinkSe();
    void clearPanelDash();
    void setSilentLand();
    void resetAirLimitedAction();
    void calcHeadColliderPos(sead::Vector3f* pPos) const;
    void calcBodyColliderPos(sead::Vector3f* pPos) const;
    void validateDynamics();
    void invalidateDynamics();
    void validateEffect();
    void invalidateEffect();
    bool isValidEffect() const;
    void validateWaterEffect();
    void invalidateWaterEffect();
    bool isValidWaterEffect() const;
    PlayerAmiiboDirector* getAmiiboDirector();
    void pausePlayerAmiiboDirector(bool isPause);
    void endPausePlayerAmiiboDirector();
    void setAmiiboDirector(PlayerAmiiboDirector* pDirector);
    void createPlayerAmiiboDirector(PlayerAmiiboDirectorWatcher* pWatcher,
                                    const al::ActorInitInfo& rInfo);
    bool isValidCircleShadow() const;
    void validateCircleShadow();
    void invalidateCircleShadow();

    PlayerConstParam* getConstParam() const { return mConstParam; }
    Player* getPlayer() const { return mPlayer; }
    PlayerGigaDirector* getPlayerGigaDirector() const;
    PlayerAudio* getAudio() const { return mAudio; }
    void setViewMtx(const sead::Matrix34f* pViewMtx) { mViewMtx = pViewMtx; }
    void setUseInputForHold(bool isUseInput) { mIsUseInputForHold = isUseInput; }
    PlayerInput* getInput() const { return mInput; }
    const sead::Matrix34f* getViewMtx() const { return mViewMtx; }
    al::HitSensor* getBindSensor() const { return mBindSensor; }
    PlayerActionObserver* getActionObserver() const { return mActionObserver; }
    PlayerModelHolder* getModelHolder() const { return mModelHolder; }
    IUsePlayerModelVisibility* getModelVisibility() const { return mModelVisibility; }
    bool isDamageTrigOn() const { return mIsDamageTrigOn; }
    al::HitSensor* getHoldingSensor() const { return mHoldingSensor; }
    al::HitSensor* getHoldedSensor() const { return mHoldedSensor; }
    al::LiveActor* getKoopaJr() const { return mKoopaJr; }
    void setKoopaJr(al::LiveActor* pKoopaJr) { mKoopaJr = pKoopaJr; }
    bool isInvincibleModelAppear() const { return mIsInvincibleModelAppear; }
    const sead::Color4f& getInvincibleColor() const { return mInvincibleColor; }
    void setValidGetItem(bool isValid) { mIsValidGetItem = isValid; }
    bool isGiantLanding() const { return mIsGiantLanding; }
    bool isSingleMode() const { return mIsSingleMode; }
    bool isRaidonExist() const { return mIsRaidonExist; }

private:
    u8 _144[0x1b8 - 0x144];
    PlayerConstParam* mConstParam;  // 0x1b8
    u8 _1c0[0x1c8 - 0x1c0];
    Player* mPlayer;  // 0x1c8
    u8 _1d0[0x1d8 - 0x1d0];
    PlayerInput* mInput;  // 0x1d8
    u8 _1e0[0x1e8 - 0x1e0];
    PlayerAudio* mAudio;  // 0x1e8
    const sead::Matrix34f* mViewMtx;  // 0x1f0
    u8 _1f8[0x210 - 0x1f8];
    al::HitSensor* mBindSensor;  // 0x210
    u8 _218[0x228 - 0x218];
    PlayerActionObserver* mActionObserver;  // 0x228
    u8 _230[0x240 - 0x230];
    PlayerModelHolder* mModelHolder;  // 0x240
    u8 _248[0x330 - 0x248];
    IUsePlayerModelVisibility* mModelVisibility;  // 0x330
    u8 _338[0x380 - 0x338];
    bool mIsTitleDemoChange;  // 0x380, set by the title scene while a change demo plays
    u8 _381[0x382 - 0x381];
    bool mIsDamageTrigOn;  // 0x382
    bool mIsValidGetItem;  // 0x383
    u8 _384[0x398 - 0x384];
    al::PadRumbleKeeper* mPadRumbleKeeper; // 0x398
    u8 _3a0[0x3e8 - 0x3a0];
    al::HitSensor* mHoldingSensor;  // 0x3e8
    u8 _3f0[0x418 - 0x3f0];
    al::HitSensor* mHoldedSensor;  // 0x418
    u8 _420[0x4c9 - 0x420];
    bool mIsInvincibleModelAppear;  // 0x4c9
    u8 _4ca[0x4d4 - 0x4ca];
    sead::Color4f mInvincibleColor;  // 0x4d4
    bool mIsGiantLanding;  // 0x4e4
    u8 _4e5[0x530 - 0x4e5];
    PlayerInvincibleBgmController* mInvincibleBgmController; // 0x530
    u8 _538[0x5d8 - 0x538];
    al::LiveActor* mKoopaJr;  // 0x5d8
    u8 _5e0[0x610 - 0x5e0];
    bool mIsSingleMode;  // 0x610
    bool mIsRaidonExist;  // 0x611
    u8 _612[0x63c - 0x612];
    bool mIsUseInputForHold;  // 0x63c
    u8 _63d[0x648 - 0x63d];
};

static_assert(sizeof(PlayerActor) == 0x648);
