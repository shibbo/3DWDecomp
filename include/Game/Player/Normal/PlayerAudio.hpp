#pragma once

#include <basis/seadTypes.h>

#include "Library/Se/Project/ISeModifier.hpp"
#include "Player/IUsePlayerAudio.hpp"

namespace al {
class LiveActor;
class SePlayParamList;
}  // namespace al

class IUsePlayerDashChecker;
class IUsePlayerEquipment;
class IUsePlayerFlag;
class IUsePlayerInput;
class PlayerAliveWatcher;
class PlayerAnimator;
class PlayerFigureDirector;
class PlayerGiantDirector;
class PlayerGigaDirector;
class PlayerModelHolder;
class SinkSandControl;

/// Sounds of the player: voices, footsteps, landings, jumps and water in/out.
class PlayerAudio : public IUsePlayerAudio, public al::ISeModifier {
public:
    /// How the current walk animation sounds (picked from the playing animation).
    enum WalkProcType : u32 {
        cWalkProcType_None,
        cWalkProcType_Normal,
        cWalkProcType_Squat,
        cWalkProcType_Climb,
        cWalkProcType_ClimbSquat,
        cWalkProcType_Giant,
        cWalkProcType_ExKuribo,
        cWalkProcType_BoxKuribo,
        cWalkProcType_Giga,
        cWalkProcType_GigaCat,
        cWalkProcType_GigaMini,
    };

    /// How fast the player walks, as far as footsteps are concerned.
    enum WalkState : u32 {
        cWalkState_None,
        cWalkState_Walk,
        cWalkState_Run,
        cWalkState_Dash,
        cWalkState_SuperDash,
    };

    PlayerAudio(const char* pCharaName, bool isCheckDemoAndArea);

    void startSe(const sead::SafeString& rName) const override;
    bool startSeOld(const sead::SafeString& rName) const override;
    void holdSe(const sead::SafeString& rName) const override;
    void stopSeOld(const sead::SafeString& rName, s32 fadeFrames) const override;
    void stopSe(const sead::SafeString& rName) const override;
    void stopAllSe(s32 fadeFrames) const override;

    /** @brief Tests whether the player is in water. @return True if in water. */
    bool isInWater() override { return mIsInWater; }

    void tryUpdateMaterial(const char* pMaterialName) override;
    u32 modifyId(u32 id) override;
    void modifyParams(s32 id, al::SePlayParamList* pParamList) override;

    /**
     * @brief Changes nothing on held sounds.
     * @param id Sound id.
     * @param pParamList Play parameters.
     */
    void modifyHoldParams(s32 id, al::SePlayParamList* pParamList) override {}

    bool isEquipKuriboBox() const;
    bool isVoiceId(u32 id) const;
    void update(f32 animRate, bool isSkipWaitSe, bool isSkipWaterInOutSe);
    void updateFootNoteProc();
    bool isInDemo();
    void onCancelJump();
    void onChangeMainAction();
    void onLanding();
    bool isNormalLandForPrevAnim() const;
    void onJump();
    bool isNormalGigaJump() const;
    bool isNormalJump() const;
    void onHipDropLand();
    void onGiantStart();
    void onGigaStart();
    s32 getSeqVariableValueOnGiantStart() const;
    s32 getSeqVariableValueOnGiantEnd() const;
    void setAliveWatcher(PlayerAliveWatcher* pWatcher);
    void setSilentLand();
    void checkPlayLandFootNote();
    void checkPlayFootNote();
    u32 getWalkProcType() const;
    u32 decideActualWalkState(u32 type, f32 rate) const;
    void startLastFootNote(f32 rate);
    const char* decideFootNoteSeLabel(u32 type, u32 state) const;
    bool isLandChainNow() const;
    bool isNormalLandForNextAnim() const;
    bool isPassAnimFrame(s32 frame) const;

private:
    al::LiveActor* getModel() const;
    bool isAnim(const char* pName) const;
    bool isGiga() const;

    al::LiveActor* mActor;                          // 0x10
    PlayerModelHolder* mModelHolder;                // 0x18
    const IUsePlayerInput* mInput;                  // 0x20
    const IUsePlayerDashChecker* mDashChecker;      // 0x28
    const IUsePlayerFlag* mLoudFootFlag;            // 0x30
    const IUsePlayerDashChecker* mSuperDashChecker; // 0x38
    const PlayerGiantDirector* mGiantDirector;      // 0x40
    const PlayerGigaDirector* mGigaDirector;        // 0x48
    const PlayerFigureDirector* mFigureDirector;    // 0x50
    PlayerAnimator* mAnimator;                      // 0x58
    const IUsePlayerEquipment* mEquipment;          // 0x60
    PlayerAliveWatcher* mAliveWatcher;              // 0x68
    const SinkSandControl* mSinkSandControl;        // 0x70
    s32 mChara;                                     // 0x78
    u32 mVoiceIdMin;                                // 0x7c
    u32 mVoiceIdMax;                                // 0x80
    f32 mPrevAnimFrame;                             // 0x84
    bool mIsInWater;                                // 0x88
    bool mIsInWaterPrev;                            // 0x89
    s32 mSinkSandKeepFrame;                         // 0x8c
    bool mIsInSinkSand;                             // 0x90
    s32 mDashFrame;                                 // 0x94
    u32 mWalkState;                                 // 0x98
    s32 mJumpSeInterval;                            // 0x9c
    s32 mLandChainFrame;                            // 0xa0
    bool mIsFootNotePlayed;                         // 0xa4
    bool mIsLandPending;                            // 0xa5
    bool mIsGiantStarted;                           // 0xa6
    bool mIsCheckDemoAndArea;                       // 0xa7
    s32 mVoiceSuppressFrame;                        // 0xa8
    s32 mBreathVoiceFrame;                          // 0xac
    s32 mHipDropLandInterval;                       // 0xb0
    s32 mLandSeSuppressFrame;                       // 0xb4
};
