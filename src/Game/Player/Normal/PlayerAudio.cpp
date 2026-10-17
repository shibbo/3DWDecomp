#include "Player/Normal/PlayerAudio.hpp"

#include <cmath>
#include <math/seadMathCalcCommon.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Se/Project/SePlayParamList.hpp"
#include "Library/Sequence/DemoDirector.hpp"
#include "Project/Audio/System/SoundHeapPtrWrapper.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Player/IUsePlayerDashChecker.hpp"
#include "Player/IUsePlayerEquipment.hpp"
#include "Player/IUsePlayerFlag.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/Normal/PlayerAnimator.hpp"
#include "Player/Normal/PlayerFigureDirector.hpp"
#include "Player/Normal/PlayerGiantDirector.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/SinkSandControl.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
/// Footstep pitch per walk proc type and walk state.
const f32 sFootNotePitchTable[11][5] = {
    {1.0f, 1.0f, 1.0f, 1.0f, 1.0f},  {1.0f, 0.85f, 0.7f, 0.7f, 0.7f}, {1.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 0.7f, 0.7f, 1.0f, 1.0f},  {1.0f, 0.7f, 1.0f, 1.0f, 1.0f},  {1.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {1.0f, 0.85f, 0.7f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f, 1.0f},  {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
};

/// Footstep volume per walk proc type and walk state.
const f32 sFootNoteVolumeTable[11][5] = {
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.4f, 0.4f, 0.26f, 0.4f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.2f, 0.3f, 0.0f, 0.0f}, {0.0f, 0.2f, 0.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.4f, 0.4f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
};
}  // namespace

/**
 * @brief Gets the model of the current figure.
 * @return Current model.
 */
inline al::LiveActor* PlayerAudio::getModel() const {
    return mModelHolder->getCurrentModel();
}

/**
 * @brief Tests whether an animation is playing.
 * @param pName Animation name.
 * @return True if playing.
 */
inline bool PlayerAudio::isAnim(const char* pName) const {
    return mAnimator->isAnim(pName);
}

/**
 * @brief Tests whether the player is in giga form.
 * @return True if giga.
 */
inline bool PlayerAudio::isGiga() const {
    return mGigaDirector != nullptr && mGigaDirector->isGiga();
}

/**
 * @brief Constructs the player's audio.
 * @param pCharaName Name of the character (Mario, Luigi, ...).
 * @param isCheckDemoAndArea Whether demos and special areas mute some sounds.
 */
PlayerAudio::PlayerAudio(const char* pCharaName, bool isCheckDemoAndArea)
    : mActor(nullptr), mModelHolder(nullptr), mInput(nullptr), mDashChecker(nullptr),
      mLoudFootFlag(nullptr), mSuperDashChecker(nullptr), mGiantDirector(nullptr),
      mFigureDirector(nullptr), mAnimator(nullptr), mEquipment(nullptr), mAliveWatcher(nullptr),
      mSinkSandControl(nullptr), mChara(-1), mVoiceIdMin(al::AudioConst::SOUND_ID_INVALID),
      mVoiceIdMax(al::AudioConst::SOUND_ID_INVALID), mPrevAnimFrame(0.0f), mIsInWater(false),
      mIsInWaterPrev(false), mSinkSandKeepFrame(0), mIsInSinkSand(false), mDashFrame(-1),
      mWalkState(cWalkState_None), mJumpSeInterval(0), mLandChainFrame(0), mIsFootNotePlayed(false),
      mIsLandPending(false), mIsGiantStarted(false), mIsCheckDemoAndArea(isCheckDemoAndArea),
      mVoiceSuppressFrame(0), mBreathVoiceFrame(0), mHipDropLandInterval(0),
      mLandSeSuppressFrame(15) {
    if (al::isEqualString(pCharaName, "Mario")) {
        mChara = EPlayerChara::Mario;
    } else if (al::isEqualString(pCharaName, "Luigi")) {
        mChara = EPlayerChara::Luigi;
    } else if (al::isEqualString(pCharaName, "Peach")) {
        mChara = EPlayerChara::Peach;
    } else if (al::isEqualString(pCharaName, "Kinopio")) {
        mChara = EPlayerChara::Kinopio;
    } else if (al::isEqualString(pCharaName, "Rosetta")) {
        mChara = EPlayerChara::Rosetta;
    } else if (al::isEqualString(pCharaName, "KinopioBrigade") ||
               al::isEqualString(pCharaName, "KinopioBrigadeMember")) {
        mChara = EPlayerChara::KinopioBrigade;
    }

    mVoiceIdMin = alSoundNameUtil::getSoundId("SePvDummyForVoiceStop", false);
    mVoiceIdMax = alSoundNameUtil::getSoundId("SePvDummyForCheckVoiceId", false);
}

/**
 * @brief Starts a sound effect on the model through the old interface.
 * @param rName Sound effect name.
 * @return True if the sound effect was started.
 */
bool PlayerAudio::startSeOld(const sead::SafeString& rName) const {
    al::LiveActor* model = getModel();
    return al::startSeOld(model, sead::SafeString(rName.cstr()), nullptr) != nullptr;
}

/**
 * @brief Starts a sound effect on the model.
 * @param rName Sound effect name.
 */
void PlayerAudio::startSe(const sead::SafeString& rName) const {
    al::startSeByName(getModel(), rName);
}

/**
 * @brief Holds a sound effect on the model for this frame.
 * @param rName Sound effect name.
 */
void PlayerAudio::holdSe(const sead::SafeString& rName) const {
    al::holdSeByName(getModel(), rName);
}

/**
 * @brief Stops a sound effect on the model through the old interface.
 * @param rName Sound effect name.
 * @param fadeFrames Fade out length.
 */
void PlayerAudio::stopSeOld(const sead::SafeString& rName, s32 fadeFrames) const {
    al::stopSe(getModel(), rName, fadeFrames);
}

/**
 * @brief Stops a sound effect on the model.
 * @param rName Sound effect name.
 */
void PlayerAudio::stopSe(const sead::SafeString& rName) const {
    al::stopSeByName(getModel(), rName);
}

/**
 * @brief Stops all sound effects of the model.
 * @param fadeFrames Fade out length.
 */
void PlayerAudio::stopAllSe(s32 fadeFrames) const {
    al::stopAllSeFromUser(getModel(), fadeFrames);
}

/**
 * @brief Updates the material code of the actor's and the model's sound effects.
 * @param pMaterialName Material code.
 */
void PlayerAudio::tryUpdateMaterial(const char* pMaterialName) {
    al::tryUpdateSeMaterialCode(mActor, pMaterialName);
    al::tryUpdateSeMaterialCode(getModel(), pMaterialName);
}

/**
 * @brief Mutes voices while they are suppressed and starts the suppression after event voices.
 * @param id Sound id to play.
 * @return Sound id to play instead.
 */
u32 PlayerAudio::modifyId(u32 id) {
    if (mChara == EPlayerChara::KinopioBrigade) {
        return id;
    }

    if (isEquipKuriboBox() && isVoiceId(id)) {
        return al::AudioConst::SOUND_ID_INVALID;
    }

    if (mVoiceSuppressFrame != 0 && isVoiceId(id) &&
        id != alSoundNameUtil::getSoundId("SePvDamage", false) &&
        id != alSoundNameUtil::getSoundId("SePvDamageWater", false) &&
        id != alSoundNameUtil::getSoundId("SePvDamageLast", false) &&
        id != alSoundNameUtil::getSoundId("SePvDamageLastWater", false) &&
        id != alSoundNameUtil::getSoundId("SePvFallDown", false) &&
        id != alSoundNameUtil::getSoundId("SePvFallDownWater", false) &&
        id != alSoundNameUtil::getSoundId("SePvDamageLava", false) &&
        id != alSoundNameUtil::getSoundId("SePvDamagePoison", false) &&
        id != alSoundNameUtil::getSoundId("SePvForceDie", false)) {
        return al::AudioConst::SOUND_ID_INVALID;
    }

    if (id == alSoundNameUtil::getSoundId("SePvPassCheckPoint", false) ||
        id == alSoundNameUtil::getSoundId("SePvCollectItem", false) ||
        id == alSoundNameUtil::getSoundId("SePvCharaSwitchOn", false) ||
        id == alSoundNameUtil::getSoundId("SePvPowerUpToClimb", false) ||
        id == alSoundNameUtil::getSoundId("SePvDoublePlayerAppear", false) ||
        id == alSoundNameUtil::getSoundId("SePvGetStar", false) ||
        id == alSoundNameUtil::getSoundId("SePvGetGreenStar", false)) {
        mVoiceSuppressFrame = 75;
    }

    return id;
}

/**
 * @brief Tests whether the kuribo box is equipped.
 * @return True if equipped.
 */
bool PlayerAudio::isEquipKuriboBox() const {
    return mEquipment != nullptr &&
           mEquipment->isEquipmentAction(static_cast<EPlayerEquipmentAction>(2));
}

/**
 * @brief Tests whether a sound id is one of the character's voices.
 * @param id Sound id.
 * @return True if it is a voice.
 */
bool PlayerAudio::isVoiceId(u32 id) const {
    if (mVoiceIdMin < id && id < mVoiceIdMax) {
        return true;
    }

    return false;
}

/**
 * @brief Picks a random breath variation for voices.
 * @param id Sound id.
 * @param pParamList Play parameters to change.
 */
void PlayerAudio::modifyParams(s32 id, al::SePlayParamList* pParamList) {
    if (!isVoiceId(id)) {
        return;
    }

    s32 variation;
    if (mBreathVoiceFrame != 0) {
        variation = al::getRandomNonSync(2.0f) + 4.0f;
        mBreathVoiceFrame = 0;
    } else {
        variation = al::getRandomNonSync(6.0f);
        if (variation <= 1) {
            mBreathVoiceFrame = 120;
        }
    }

    pParamList->setLocalVariable(variation, 2);
}

/**
 * @brief Updates footsteps, water sounds and timers.
 * @param animRate Rate of the model's animation.
 * @param isSkipWaitSe Whether to skip the white tanooki wait sound.
 * @param isSkipWaterInOutSe Whether to skip entering and leaving water sounds.
 */
void PlayerAudio::update(f32 animRate, bool isSkipWaitSe, bool isSkipWaterInOutSe) {
    if (mLandSeSuppressFrame > 0) {
        mLandSeSuppressFrame--;
    }

    updateFootNoteProc();
    mIsInWater = rc::isInWaterArea(getModel());

    if (mIsCheckDemoAndArea &&
        (rc::isPlayerInRouteDokanSM(mActor) || rc::isPlayerInInkLimiter(mActor))) {
        mIsInWater = false;
    }

    if (mIsInWater && (!mIsCheckDemoAndArea ||
                       (rc::isInWaterArea(getModel(), 150.0f) && !rc::isPlayerGiga(mActor)))) {
        holdSe("InWaterAmbient");
    }

    if (!isSkipWaterInOutSe && !isGiga()) {
        bool isLight = GameDataFunction::isSingleMode(mActor) ? rc::isPlayerOnGround(mActor) : false;

        if (mIsInWaterPrev) {
            if (!mIsInWater) {
                startSe(isLight ? "GoOutOfWaterLight" : "GoOutOfWater");
            }
        } else if (mIsInWater) {
            startSe(isLight ? "GoIntoWaterLight" : "GoIntoWater");
        }
    }

    mIsInWaterPrev = mIsInWater;

    if (mHipDropLandInterval > 0) {
        mHipDropLandInterval--;
    }

    if (mSinkSandControl != nullptr) {
        if (mSinkSandControl->isInSinkSand()) {
            mIsInSinkSand = true;
            mSinkSandKeepFrame = 3;
        } else if (mSinkSandKeepFrame == 0) {
            mIsInSinkSand = false;
        } else {
            mSinkSandKeepFrame--;
        }
    }

    if (mFigureDirector != nullptr &&
        (mFigureDirector->getFigure() == EPlayerFigure::RaccoonDogWhite ||
         mFigureDirector->getFigure() == EPlayerFigure::ClimbWhite) &&
        !isSkipWaitSe && (!mIsCheckDemoAndArea || !isInDemo())) {
        al::holdSeWithParam(getModel(), "PgWaitTanookiWhite", animRate, nullptr);
    }

    if (mIsGiantStarted && mGiantDirector != nullptr && mGiantDirector->isRunningOut()) {
        startSe("PgGiantEnd");
        mIsGiantStarted = false;
    }

    if (mVoiceSuppressFrame != 0) {
        mVoiceSuppressFrame--;
    }

    if (mBreathVoiceFrame != 0) {
        mBreathVoiceFrame--;
    }
}

/**
 * @brief Updates the dash counter and jump interval, then plays landing and walking footsteps.
 */
void PlayerAudio::updateFootNoteProc() {
    if (mDashChecker->isDashingFast()) {
        mDashFrame++;
    } else {
        mDashFrame = -1;
    }

    if (mJumpSeInterval != 0) {
        mJumpSeInterval--;
    }

    checkPlayLandFootNote();
    checkPlayFootNote();
}

/**
 * @brief Tests whether a demo is running for the model.
 * @return True if a demo is active.
 */
bool PlayerAudio::isInDemo() {
    al::LiveActor* model = getModel();
    if (model == nullptr || model->mActorSceneInfo == nullptr) {
        return false;
    }

    al::DemoDirector* demoDirector = model->mActorSceneInfo->demoDirector;
    return demoDirector != nullptr && demoDirector->isActiveDemo();
}

/**
 * @brief Drops the pending landing sound after a cancelled jump.
 */
void PlayerAudio::onCancelJump() {
    mIsLandPending = false;
}

/**
 * @brief Starts the landing chain when the main action changes while a landing is pending.
 */
void PlayerAudio::onChangeMainAction() {
    if (mIsLandPending && mLandChainFrame == 0) {
        mLandChainFrame = 5;
    }
}

/**
 * @brief Plays landing sounds, or marks the landing as pending for the next animation.
 */
void PlayerAudio::onLanding() {
    if (!isNormalLandForPrevAnim()) {
        return;
    }

    if (mGiantDirector->isGiant()) {
        startSe("PgGiantLand");
        return;
    }

    if (isGiga()) {
        if (mLandSeSuppressFrame > 0) {
            return;
        }

        switch (mFigureDirector != nullptr ? mFigureDirector->getFigure() : EPlayerFigure::Super) {
        case EPlayerFigure::Climb:
            startSe("GigaCatLand");
            break;
        case EPlayerFigure::Mini:
            startSe("GigaLandMini");
            break;
        default:
            startSe("GigaLand");
            break;
        }
    }

    if (mAnimator->isAnim("SquatWalk") || mAnimator->isAnim("SquatWait") ||
        mAnimator->isAnim("JumpBackStart")) {
        startSe("FootLand");
        return;
    }

    mIsLandPending = true;
}

/**
 * @brief Tests whether the previous animation lands normally.
 * @return True unless it was a hip drop, knock down, wall hit or course select leave.
 */
bool PlayerAudio::isNormalLandForPrevAnim() const {
    if (isAnim("HipDrop") || isAnim("SwimHipDrop") || isAnim("KnockDown") || isAnim("WallHit")) {
        return false;
    }

    return !isAnim("CourseSelectLeave");
}

/**
 * @brief Plays the jump sounds.
 */
void PlayerAudio::onJump() {
    if (mJumpSeInterval != 0) {
        return;
    }

    mJumpSeInterval = 10;

    if (mGiantDirector->isGiant()) {
        startSe("PgGiantJump");
        return;
    }

    if (isNormalGigaJump()) {
        if (mFigureDirector != nullptr && mFigureDirector->getFigure() == EPlayerFigure::Mini) {
            startSe("GigaJumpMini");
        } else {
            startSe("GigaJump");
        }
        return;
    }

    if (isEquipKuriboBox()) {
        startSe("BoxKuriboJump");
        return;
    }

    if (mIsInSinkSand) {
        mJumpSeInterval = 15;
        startSe(mSinkSandControl->isInInk() ? "PgSinkInkJump" : "PgSinkSandJump");
        return;
    }

    if (!isAnim("WallJump")) {
        startSe("FootJump");
    }

    if (isNormalJump()) {
        startSe("JumpVoice");
    }
}

/**
 * @brief Tests whether a giga jump animation is playing.
 * @return True if playing.
 */
bool PlayerAudio::isNormalGigaJump() const {
    return isAnim("GigaJump") || isAnim("GigaJump2") || isAnim("GigaJump3");
}

/**
 * @brief Tests whether a normal jump animation is playing.
 * @return True if playing.
 */
bool PlayerAudio::isNormalJump() const {
    return isAnim("Jump") || isAnim("Jump2") || isAnim("Jump3");
}

/**
 * @brief Plays the hip drop landing sound, at most every ten frames.
 */
void PlayerAudio::onHipDropLand() {
    if (mHipDropLandInterval > 0) {
        return;
    }

    startSe("HipDropLand");
    mHipDropLandInterval = 10;
}

/**
 * @brief Plays the giant start sound once.
 */
void PlayerAudio::onGiantStart() {
    if (mIsGiantStarted) {
        return;
    }

    startSe("PgGiantStart");
    mIsGiantStarted = true;
}

/**
 * @brief Does nothing when the giga form starts.
 */
void PlayerAudio::onGigaStart() {}

/**
 * @brief Gets the sequence variable for the giant start.
 * @return Sequence variable value.
 */
s32 PlayerAudio::getSeqVariableValueOnGiantStart() const {
    return 10;
}

/**
 * @brief Gets the sequence variable for the giant end.
 * @return Sequence variable value.
 */
s32 PlayerAudio::getSeqVariableValueOnGiantEnd() const {
    return 0;
}

/**
 * @brief Sets the watcher of alive players.
 * @param pWatcher Alive watcher.
 */
void PlayerAudio::setAliveWatcher(PlayerAliveWatcher* pWatcher) {
    mAliveWatcher = pWatcher;
}

/**
 * @brief Lands without a landing sound.
 */
void PlayerAudio::setSilentLand() {
    mIsLandPending = false;
}

/**
 * @brief Plays the pending landing sound during the landing chain.
 */
void PlayerAudio::checkPlayLandFootNote() {
    if (mIsLandPending && mLandChainFrame >= 1 && mLandChainFrame <= 4) {
        if (mGiantDirector->isGiant()) {
            startSe("PgGiantLand");
        } else if (isNormalLandForNextAnim()) {
            startSe("LandNormal");
        }

        mIsLandPending = false;
    }

    if (mLandChainFrame != 0) {
        mLandChainFrame--;
    }
}

/**
 * @brief Plays the footsteps of the walk animation.
 */
void PlayerAudio::checkPlayFootNote() {
    al::LiveActor* model = getModel();
    al::getSklAnimFrame(model, 0);
    f32 rate = sead::Mathf::clampMax(al::getSklAnimFrameRate(model, 0), 3.0f);
    u32 type = getWalkProcType();
    u32 state = decideActualWalkState(type, rate);

    if (state == cWalkState_None) {
        if (mPrevAnimFrame != -1.0f) {
            startLastFootNote(rate);
        }

        mPrevAnimFrame = -1.0f;
        mIsFootNotePlayed = false;
    } else {
        const char* label = decideFootNoteSeLabel(type, state);

        if (label != nullptr) {
            if (mLandChainFrame == 0) {
                if (type == cWalkProcType_Normal || type == cWalkProcType_Climb ||
                    type == cWalkProcType_ClimbSquat || type == cWalkProcType_ExKuribo) {
                    f32 pitch = sFootNotePitchTable[type][state] + rate * 0.1f;
                    f32 volume = sFootNoteVolumeTable[type][state] + rate * 0.23f;

                    if (!mIsFootNotePlayed) {
                        volume *= 0.65f;
                    }

                    if (mLoudFootFlag->isOn()) {
                        volume *= 1.5f;
                    }

                    al::startSeSetPitchVolumeByName(model, label,
                                                    sead::Mathf::clamp(pitch, 0.3f, 2.0f),
                                                    sead::Mathf::clamp(volume, 0.0f, 2.0f));
                } else {
                    startSe(label);
                }

                if (state == cWalkState_Dash || state == cWalkState_SuperDash) {
                    if (al::isEqualString(label, "RunL")) {
                        startSe("AddDashL");
                    } else if (al::isEqualString(label, "RunR")) {
                        startSe("AddDashR");
                    }
                }
            }

            mIsFootNotePlayed = true;
        }

        mPrevAnimFrame = al::getSklAnimFrame(model, 0);
    }

    mWalkState = state;
}

/**
 * @brief Picks how footsteps sound from the playing animation.
 * @return Walk proc type.
 */
u32 PlayerAudio::getWalkProcType() const {
    if (isAnim("GigaMove")) {
        if (PlayerActionFunc::isClimb(mFigureDirector)) {
            return cWalkProcType_GigaCat;
        }

        return mFigureDirector->getFigure() == EPlayerFigure::Mini ? cWalkProcType_GigaMini :
                                                                     cWalkProcType_Giga;
    }

    if (isAnim("GiantMove")) {
        return cWalkProcType_Giant;
    }

    if (isEquipKuriboBox() && isAnim("SquatWalk")) {
        return cWalkProcType_BoxKuribo;
    }

    if (PlayerActionFunc::isClimb(mFigureDirector)) {
        if (isAnim("ClimbSquatWalk")) {
            return cWalkProcType_ClimbSquat;
        }

        if (isAnim("ClimbMove")) {
            return mAnimator->isClimbMoveAsWalk() ? cWalkProcType_Normal : cWalkProcType_Climb;
        }
    } else if (isAnim("SquatWalk")) {
        return cWalkProcType_Squat;
    }

    if (isAnim("Move")) {
        return cWalkProcType_Normal;
    }

    return cWalkProcType_None;
}

/**
 * @brief Decides how fast the player walks.
 * @param type Walk proc type.
 * @param rate Rate of the walk animation.
 * @return Walk state.
 */
u32 PlayerAudio::decideActualWalkState(u32 type, f32 rate) const {
    switch (type) {
    case cWalkProcType_Climb:
    case cWalkProcType_Giant:
    case cWalkProcType_ExKuribo:
    case cWalkProcType_Giga:
    case cWalkProcType_GigaCat:
    case cWalkProcType_GigaMini:
        return mInput->isDashButtonOn() ? cWalkState_Run : cWalkState_Walk;
    case cWalkProcType_Squat:
    case cWalkProcType_ClimbSquat:
    case cWalkProcType_BoxKuribo:
        return cWalkState_Walk;
    case cWalkProcType_Normal:
        if (rate <= 1.7f) {
            return cWalkState_Walk;
        }

        if (mSuperDashChecker->isDashingFast()) {
            return cWalkState_SuperDash;
        }

        if (mInput->isDashButtonOn()) {
            return mDashChecker->isDashingFast() ? cWalkState_Dash : cWalkState_Run;
        }

        return cWalkState_Walk;
    default:
        return cWalkState_None;
    }
}

/**
 * @brief Plays the last footstep when the player stops walking.
 * @param rate Rate of the walk animation.
 */
void PlayerAudio::startLastFootNote(f32 rate) {
    if (!isAnim("Wait") || mGiantDirector->isGiant() || isEquipKuriboBox() ||
        PlayerActionFunc::isClimb(mFigureDirector)) {
        return;
    }

    al::LiveActor* model = getModel();
    s32 frame = mPrevAnimFrame;

    switch (mWalkState) {
    case cWalkState_Walk:
        if (frame % 30 >= 15) {
            al::startSeSetPitchVolumeByName(
                model, "StepL", 1.0f, sead::Mathf::clamp((rate * 0.23f + 0.4f) * 0.75f, 0.0f, 2.0f));
        }
        break;
    case cWalkState_Run:
        if (frame % 30 >= 15) {
            al::startSeSetPitchVolumeByName(
                model, "RunL", 1.0f, sead::Mathf::clamp((rate * 0.23f + 0.4f) * 0.75f, 0.0f, 2.0f));
        }
        break;
    }
}

/**
 * @brief Decides which footstep to play at the current animation frame.
 * @param type Walk proc type.
 * @param state Walk state.
 * @return Footstep sound effect name, or nullptr if none is due.
 */
const char* PlayerAudio::decideFootNoteSeLabel(u32 type, u32 state) const {
    if (state == cWalkState_None) {
        return nullptr;
    }

    switch (type) {
    case cWalkProcType_Normal:
        if (mIsInWater && !mIsCheckDemoAndArea) {
            return nullptr;
        }

        switch (state) {
        case cWalkState_Walk:
            if (isPassAnimFrame(0)) {
                return "StepR";
            }

            if (isPassAnimFrame(15)) {
                return "SubStepR";
            }

            if (isPassAnimFrame(30)) {
                return "StepL";
            }

            if (isPassAnimFrame(45)) {
                return "SubStepL";
            }

            return nullptr;
        case cWalkState_Run:
            if (isPassAnimFrame(0)) {
                return "RunR";
            }

            if (isPassAnimFrame(10)) {
                return "SubRunR";
            }

            if (isPassAnimFrame(30)) {
                return "RunL";
            }

            if (isPassAnimFrame(40)) {
                return "SubRunL";
            }

            return nullptr;
        case cWalkState_Dash:
            switch (mDashFrame % 18) {
            case 0:
                return "RunR";
            case 6:
                return "SubRunR";
            case 9:
                return "RunL";
            case 15:
                return "SubRunL";
            default:
                return nullptr;
            }
        case cWalkState_SuperDash:
            if (isPassAnimFrame(0)) {
                return "RunR";
            }

            if (isPassAnimFrame(30)) {
                return "RunL";
            }

            return nullptr;
        default:
            return nullptr;
        }
    case cWalkProcType_Squat:
        if (isPassAnimFrame(17)) {
            return "SquatWalkL";
        }

        if (isPassAnimFrame(35)) {
            return "SquatWalkR";
        }

        return nullptr;
    case cWalkProcType_Climb:
        if (mIsInWater && !mIsCheckDemoAndArea) {
            return nullptr;
        }

        if (state == cWalkState_Walk) {
            if (isPassAnimFrame(30)) {
                return "ClimbMoveLFore";
            }

            if (isPassAnimFrame(0)) {
                return "ClimbMoveRFore";
            }

            if (isPassAnimFrame(50)) {
                return "StepL";
            }

            if (isPassAnimFrame(20)) {
                return "StepR";
            }
        } else if (state == cWalkState_Run) {
            if (isPassAnimFrame(8)) {
                return "ClimbMoveLFore";
            }

            if (isPassAnimFrame(0)) {
                return "ClimbMoveRFore";
            }

            if (isPassAnimFrame(31)) {
                return "RunL";
            }

            if (isPassAnimFrame(22)) {
                return "RunR";
            }
        }

        return nullptr;
    case cWalkProcType_ClimbSquat:
        if (mIsInWater && !mIsCheckDemoAndArea) {
            return nullptr;
        }

        if (isPassAnimFrame(0)) {
            return "ClimbMoveLFore";
        }

        if (isPassAnimFrame(18)) {
            return "ClimbMoveRFore";
        }

        if (isPassAnimFrame(12)) {
            return "StepL";
        }

        if (isPassAnimFrame(30)) {
            return "StepR";
        }

        return nullptr;
    case cWalkProcType_Giant:
        if (state == cWalkState_Run) {
            if (isPassAnimFrame(0)) {
                return "PgGiantRunL";
            }

            if (isPassAnimFrame(60)) {
                return "PgGiantRunR";
            }
        } else {
            if (isPassAnimFrame(0)) {
                return "PgGiantStepL";
            }

            if (isPassAnimFrame(60)) {
                return "PgGiantStepR";
            }
        }

        return nullptr;
    case cWalkProcType_ExKuribo:
        if (state == cWalkState_Walk || state == cWalkState_Run) {
            if (isPassAnimFrame(0)) {
                return "SePfExKuriboRunR";
            }

            if (isPassAnimFrame(30)) {
                return "SePfExKuriboRunL";
            }
        }

        return nullptr;
    case cWalkProcType_BoxKuribo:
        if (isPassAnimFrame(17)) {
            return "BoxKuriboStepL";
        }

        if (isPassAnimFrame(35)) {
            return "BoxKuriboStepR";
        }

        return nullptr;
    case cWalkProcType_Giga:
        if (state == cWalkState_Run) {
            if (isPassAnimFrame(0)) {
                return "GigaRunL";
            }

            if (isPassAnimFrame(60)) {
                return "GigaRunR";
            }
        } else {
            if (isPassAnimFrame(0)) {
                return "GigaStepL";
            }

            if (isPassAnimFrame(60)) {
                return "GigaStepR";
            }
        }

        return nullptr;
    case cWalkProcType_GigaCat:
        if (state == cWalkState_Walk) {
            if (isPassAnimFrame(50)) {
                return "GigaCatStepL";
            }

            if (isPassAnimFrame(100)) {
                return "GigaCatStepR";
            }

            if (isPassAnimFrame(50)) {
                return "GigaStepL";
            }

            if (isPassAnimFrame(100)) {
                return "GigaStepR";
            }
        } else if (state == cWalkState_Run) {
            if (isPassAnimFrame(50)) {
                return "GigaCatRunL";
            }

            if (isPassAnimFrame(100)) {
                return "GigaCatRunR";
            }

            if (isPassAnimFrame(100)) {
                return "GigaRunL";
            }

            if (isPassAnimFrame(50)) {
                return "GigaRunR";
            }
        }

        return nullptr;
    case cWalkProcType_GigaMini:
        if (state == cWalkState_Run) {
            if (isPassAnimFrame(0)) {
                return "GigaRunMiniL";
            }

            if (isPassAnimFrame(60)) {
                return "GigaRunMiniR";
            }
        } else {
            if (isPassAnimFrame(0)) {
                return "GigaStepMiniL";
            }

            if (isPassAnimFrame(60)) {
                return "GigaStepMiniR";
            }
        }

        return nullptr;
    default:
        return nullptr;
    }
}

/**
 * @brief Tests whether the landing chain is running.
 * @return True if running.
 */
bool PlayerAudio::isLandChainNow() const {
    return mLandChainFrame != 0;
}

/**
 * @brief Tests whether the next animation lands normally.
 * @return True unless it is a special landing, a trample jump or the course select wait.
 */
bool PlayerAudio::isNormalLandForNextAnim() const {
    if (isAnim("HipDropLand") || isAnim("SwimHipDropLand") || isAnim("KnockDownLand") ||
        isAnim("GigaKnockDownLand") || isAnim("WallHitLand") || isAnim("TrampleJump")) {
        return false;
    }

    return !isAnim("CourseSelectWait");
}

/**
 * @brief Tests whether the model's animation passed a frame since the last footstep check.
 * @param frame Animation frame.
 * @return True if the frame was passed, also when the animation looped.
 */
bool PlayerAudio::isPassAnimFrame(s32 frame) const {
    f32 current = al::getSklAnimFrame(getModel(), 0);
    f32 target = frame;
    f32 prev = mPrevAnimFrame;

    if (current >= target && prev < target) {
        return true;
    }

    if (current < target && prev >= target) {
        return false;
    }

    return prev > current;
}
