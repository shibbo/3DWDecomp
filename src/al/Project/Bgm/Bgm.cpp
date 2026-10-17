#include "Project/Bgm/Bgm.hpp"

#include <audio/seadSoundHandle.h>
#include <math/seadMathCalcCommon.h>
#include <nn/atk/atk_SoundStartable.h>
#include <nn/atk/atk_SpecialSoundHandle.h>
#include <nn/atk/atk_WaveSoundHandle.h>

#include "Library/Audio/System/AudioSystemInfo.hpp"
#include "Library/Bgm/BgmDataBase.hpp"
#include "Library/Bgm/BgmFunction.hpp"
#include "Library/Bgm/LinearValueController.hpp"
#include "Project/Audio/System/AudioPlayer.hpp"
#include "Project/Audio/System/SoundHeapPtrWrapper.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"

namespace al {
/**
 * Constructs a controller resting at a value.
 * @param value Initial value.
 */
LinearValueController::LinearValueController(f32 value) : mValue(value) {}

/**
 * Moves the value one step toward the target.
 */
void LinearValueController::update() {
    if (mStep > 0.0f && mTarget > mValue) {
        mValue += mStep;

        if (mTarget < mValue) {
            mValue = mTarget;
        }
    } else if (mStep < 0.0f && mTarget < mValue) {
        mValue += mStep;

        if (mTarget > mValue) {
            mValue = mTarget;
        }
    }
}

/**
 * Sets a new target.
 * @param target Target value.
 * @param speed Change per update.
 */
void LinearValueController::changeTarget(f32 target, f32 speed) {
    mTarget = target;
    mStep = mValue < target ? speed : -speed;
}

/**
 * Constructs a low pass filter controller.
 * @param pHandle Sound handle.
 */
BgmLpfController::BgmLpfController(sead::SoundHandle* pHandle) : mHandle(pHandle) {
    mFreqController = new LinearValueController(0.0f);
}

/**
 * Updates the cut off frequency of the sound.
 */
void BgmLpfController::update() {
    mFreqController->update();

    if (mIsReached) {
        return;
    }

    mHandle->SetLpfFreq(mFreqController->getValue());

    if (mFreqController->isReachedTarget()) {
        mIsReached = true;
    }
}

/**
 * Changes the cut off frequency.
 * @param freq Target frequency.
 * @param speed Change per update.
 */
void BgmLpfController::changeCutOffFreq(f32 freq, f32 speed) {
    mFreqController->changeTarget(freq, speed);
    mHandle->SetLpfFreq(mFreqController->getValue());
    mIsReached = mFreqController->isReachedTarget();
}

/**
 * Constructs a pitch controller.
 * @param pHandle Sound handle.
 */
BgmPitchController::BgmPitchController(sead::SoundHandle* pHandle) : mHandle(pHandle) {
    mPitchController = new LinearValueController(1.0f);
    mModulationDepthController = new LinearValueController(0.0f);
}

inline void BgmPitchController::applyPitch() {
    f32 modulation = sinf(mModulationPhase) * mModulationDepthController->getValue();
    f32 pitch = mPitchController->getValue();
    mHandle->SetPitch(pitch * exp2f(modulation));
    mModulationPhase += mModulationSpeed * (sead::Mathf::pi2() / 60.0f);

    while (mModulationPhase >= sead::Mathf::pi2()) {
        mModulationPhase -= sead::Mathf::pi2();
    }
}

/**
 * Updates the pitch of the sound.
 */
void BgmPitchController::update() {
    mPitchController->update();
    mModulationDepthController->update();
    applyPitch();
}

/**
 * Changes the pitch.
 * @param pitch Target pitch.
 * @param speed Change per update.
 */
void BgmPitchController::changePitch(f32 pitch, f32 speed) {
    mPitchController->changeTarget(pitch, speed);
    applyPitch();
}

/**
 * Changes the pitch modulation.
 * @param depth Target modulation depth.
 * @param depthSpeed Change of the depth per update.
 * @param speed Modulation speed.
 */
void BgmPitchController::changeModulation(f32 depth, f32 depthSpeed, f32 speed) {
    mModulationDepthController->changeTarget(depth, depthSpeed);
    mModulationSpeed = speed;
    mModulationPhase = 0.0f;
}

/**
 * Constructs a volume controller.
 * @param pHandle Sound handle.
 */
BgmVolumeController::BgmVolumeController(sead::SoundHandle* pHandle) : mHandle(pHandle) {
    mVolumeController = new LinearValueController(1.0f);
}

/**
 * Updates the volume of the sound.
 */
void BgmVolumeController::update() {
    mVolumeController->update();

    if (mIsReached) {
        return;
    }

    mHandle->SetVolume(mVolumeController->getValue(), 0);

    if (mVolumeController->isReachedTarget()) {
        mIsReached = true;
    }
}

/**
 * Changes the volume.
 * @param volume Target volume.
 * @param speed Change per update.
 */
void BgmVolumeController::changeVolume(f32 volume, f32 speed) {
    mVolumeController->changeTarget(volume, speed);
    mHandle->SetVolume(mVolumeController->getValue(), 0);
    mIsReached = mVolumeController->isReachedTarget();
}

/**
 * Checks whether the volume is fading out.
 * @return True if the volume is fading out.
 */
bool BgmVolumeController::isFadeOutNow() const {
    f32 value = mVolumeController->getValue();

    if (value <= 0.0f) {
        return false;
    }

    return value > mVolumeController->getTarget();
}

/**
 * Checks whether the volume is fading in.
 * @return True if the volume is fading in.
 */
bool BgmVolumeController::isFadeInNow() const {
    f32 value = mVolumeController->getValue();

    if (value >= 1.0f) {
        return false;
    }

    return value < mVolumeController->getTarget();
}

/**
 * Checks whether the volume has faded out.
 * @return True if the volume is zero.
 */
bool BgmVolumeController::isFinishedFadeOut() const {
    return mVolumeController->getValue() <= 0.0f;
}

/**
 * Gets the current volume.
 * @return Current volume.
 */
f32 BgmVolumeController::getCurFadeVolume() const {
    return mVolumeController->getValue();
}
}  // namespace al

namespace {
using StartInfo = nn::atk::SoundStartable::StartInfo;

nn::atk::StreamRegionCallbackResult regionCallback(nn::atk::StreamRegionCallbackParam* pParam, void* pArg);

/**
 * Starts or prepares the sound of a BGM.
 * @param pPlayer Audio player.
 * @param pHandle Sound handle.
 * @param pParam Start parameters.
 * @param isPrepare Whether to only prepare the sound.
 */
void startSound(al::SeadAudioPlayer* pPlayer, sead::SoundHandle* pHandle, const al::BgmSoundStartParam* pParam,
                bool isPrepare) {
    pHandle->stop(pParam->mFadeOutFrames);

    StartInfo info;

    if (pParam->mStartSample > 0) {
        info.startOffsetType = StartInfo::StartOffsetType_Sample;
        info.startOffset = pParam->mStartSample;
        info.enableFlag |= StartInfo::EnableFlagBit_StartOffset;
    }

    if (pParam->mRegionCtrl != nullptr) {
        info.enableFlag |= StartInfo::EnableFlagBit_StreamSoundInfo;
        info.streamSoundInfo.regionCallback = regionCallback;
        info.streamSoundInfo.regionCallbackArg = pParam->mRegionCtrl;
    }

    if (pParam->mIsEnableNwRender) {
        info.enableFlag |= StartInfo::EnableFlagBit_VoiceRendererType;
        info.voiceRendererType = 3;
    }

    if (pParam->mName == nullptr) {
        return;
    }

    nn::atk::SoundStartable::StartResult result =
        isPrepare ? pPlayer->PrepareSound(pHandle, pParam->mName, &info) :
                    pPlayer->StartSound(pHandle, pParam->mName, &info);

    if (!result.IsSuccess()) {
        return;
    }

    if (pParam->mFadeInFrames > 0) {
        pHandle->FadeIn(pParam->mFadeInFrames);
    }

    pHandle->SetOutputLine(1);

    if (pParam->mIsDisableAudioEffect) {
        pHandle->SetOutputEffectSend(nn::atk::OutputDevice_Main, nn::atk::AuxBus_A, -1.0f);
    } else {
        pHandle->SetOutputEffectSend(nn::atk::OutputDevice_Main, nn::atk::AuxBus_A, 0.0f);
    }
}
}  // namespace

namespace al {
/**
 * Constructs a BGM with its parameter controllers.
 */
Bgm::Bgm() {
    mSoundHandle = new sead::SoundHandle();
    mStartParam = new BgmSoundStartParam;
    mVolumeController = new BgmVolumeController(mSoundHandle);
    mLpfController = new BgmLpfController(mSoundHandle);
    mPitchController = new BgmPitchController(mSoundHandle);
}

/**
 * Initializes the BGM.
 * @param pInfo Audio system info.
 */
void Bgm::init(AudioSystemInfo* pInfo) {
    mAudioPlayer = pInfo->getSeadAudioPlayerForBgm();
    mSuffixName = nullptr;
    mRegionCtrl = new BgmRegionCtrl();
    mRegionCtrl->mIsChangedRegion = true;
    mRegionCtrl->mBgm = this;
    mRegionCtrl->mRegionInfo = nullptr;
}

/**
 * Checks whether the BGM is playing or waiting for its start delay.
 * @return True if the BGM is playing.
 */
inline bool Bgm::isPlaying() const {
    return mSoundHandle->IsAttachedSound() || mStartDelayFrames > 0;
}

/**
 * Checks whether the BGM is paused.
 * @return True if the BGM is paused.
 */
inline bool Bgm::isPause() const {
    return mSoundHandle->IsPause() || mIsPausedInDelay;
}

/**
 * Gets the start sample of the current resource.
 * @return Start sample.
 */
inline s32 Bgm::getCurStartSample() const {
    return mResourceSuffixInfo != nullptr ? mResourceSuffixInfo->mStartSample : mResourceInfo->mStartSample;
}

/**
 * Calculates the current sample position converted to another tempo.
 * @param bpm Tempo to convert to.
 * @return Converted sample position.
 */
inline s32 Bgm::calcCurSamplePositionByBpm(f32 bpm) const {
    sead::SoundHandle* soundHandle = mSoundHandle;
    f32 curBpm = mBpm;
    nn::atk::StreamSoundHandle handle(soundHandle);
    f32 position = handle.GetPlaySamplePosition();

    return curBpm / bpm * position;
}

/**
 * Gets the region controller if the current resource allows region jumps.
 * @return Region controller, or nullptr.
 */
inline BgmRegionCtrl* Bgm::getEnableRegionCtrl() const {
    BgmRegionCtrl* regionCtrl = mRegionCtrl;

    if (!mResourceInfo->mIsEnableRegionJump) {
        regionCtrl = nullptr;
    }

    return regionCtrl;
}

/**
 * Sets the parameters to start a BGM resource.
 * @param pResourceInfo Resource info.
 * @param pName Sound name, or nullptr to use the resource name.
 * @param startSample Start sample.
 * @param fadeInFrames Fade in frames.
 * @param fadeOutFrames Fade out frames of the previous sound.
 * @param pRegionCtrl Region controller, or nullptr.
 */
inline void BgmSoundStartParam::set(const BgmResourceInfo* pResourceInfo, const char* pName, s32 startSample,
                                    s32 fadeInFrames, s32 fadeOutFrames, BgmRegionCtrl* pRegionCtrl) {
    mName = pName != nullptr ? pName : pResourceInfo->mName;
    mStartSample = startSample;
    mFadeInFrames = fadeInFrames;
    mFadeOutFrames = fadeOutFrames;
    mRegionCtrl = pRegionCtrl;
    mIsDisableAudioEffect = pResourceInfo->mIsDisableAudioEffect;
    mIsEnableNwRender = pResourceInfo->mIsEnableNwRender;
}

/**
 * Counts down the start delay and updates the parameter controllers.
 */
void Bgm::update() {
    if (mIsPrepared) {
        return;
    }

    if (!mIsPausedInDelay && mStartDelayFrames > 0) {
        mStartDelayFrames--;

        if (mStartDelayFrames == 0) {
            if (mSoundHandle->IsAttachedSound()) {
                mSoundHandle->StartPrepared();
            } else {
                startSound(mAudioPlayer, mSoundHandle, mStartParam, false);
            }
        }
    }

    if (!isPlaying() || isPause()) {
        return;
    }

    mLpfController->update();
    mPitchController->update();
    mVolumeController->update();
}

/**
 * Starts a BGM resource.
 * @param pResourceInfo Resource info.
 * @param rRequest Playing request.
 * @param isPrepare Whether to only prepare the sound.
 */
void Bgm::startBgm(const BgmResourceInfo* pResourceInfo, const BgmPlayingRequest& rRequest, bool isPrepare) {
    mResourceInfo = pResourceInfo;
    mIsPrepared = isPrepare;
    mStartDelayFrames = rRequest.startDelayFrames;
    mResourceSuffixInfo = nullptr;

    s32 startSample = rRequest.isRestart ? pResourceInfo->mStartSample : 0;

    if (rRequest._18 >= 0) {
        startSample = rRequest._18;
    }

    mStartParam->set(pResourceInfo, nullptr, startSample, rRequest.fadeInFrames, rRequest.fadeOutFrames,
                     getEnableRegionCtrl());

    if (mStartDelayFrames != 0) {
        mSoundHandle->stop(0);
    } else {
        startSound(mAudioPlayer, mSoundHandle, mStartParam, isPrepare);
    }

    mBpm = mResourceInfo->mBpm;
    mTrackChangeInfoList = nullptr;
    mSuffixName = "";
    mRegionCtrl->mRegionInfo = nullptr;
    mIsPausedInDelay = false;
}

/**
 * Starts a prepared BGM with a new request.
 * @param rRequest Playing request.
 */
void Bgm::startPreparedBgm(const BgmPlayingRequest& rRequest) {
    mStartDelayFrames = rRequest.startDelayFrames;
    mIsPrepared = false;

    if (mStartDelayFrames == 0) {
        mSoundHandle->StartPrepared();
    }
}

/**
 * Starts a prepared BGM with the request it was prepared with.
 */
void Bgm::startPreparedBgmExistingRequest() {
    mIsPrepared = false;

    if (mStartDelayFrames == 0) {
        mSoundHandle->StartPrepared();
    }
}

/**
 * Pauses the BGM.
 * @param fadeFrames Fade frames, or -1 to pause immediately.
 */
void Bgm::pauseBgm(s32 fadeFrames) {
    if (mIsPrepared || isPause()) {
        return;
    }

    if (fadeFrames == -1) {
        fadeFrames = 0;
    }

    if (mStartDelayFrames > 0) {
        mIsPausedInDelay = true;
        return;
    }

    mSoundHandle->Pause(true, fadeFrames);
}

/**
 * Resumes the BGM.
 * @param fadeFrames Fade frames, or 0 or less for the default.
 */
void Bgm::resumeBgm(s32 fadeFrames) {
    if (mIsPrepared || !isPause()) {
        return;
    }

    if (mStartDelayFrames > 0) {
        mIsPausedInDelay = false;
        return;
    }

    mSoundHandle->Pause(false, fadeFrames < 1 ? 20 : fadeFrames);
}

/**
 * Stops the BGM.
 * @param fadeFrames Fade frames, or -1 for the default.
 */
void Bgm::stopBgm(s32 fadeFrames) {
    mIsPausedInDelay = false;

    if (mStartDelayFrames > 0) {
        mStartDelayFrames = 0;
    }

    mSoundHandle->stop(fadeFrames == -1 ? 20 : fadeFrames);
    mTrackChangeInfoList = nullptr;
}

/**
 * Checks whether the BGM is fading out.
 * @return True if the BGM is fading out.
 */
bool Bgm::isFadeOutNow() const {
    return mVolumeController->isFadeOutNow();
}

/**
 * Checks whether the BGM is fading in.
 * @return True if the BGM is fading in.
 */
bool Bgm::isFadeInNow() const {
    return mVolumeController->isFadeInNow();
}

/**
 * Gets the tempo of the current resource.
 * @return Tempo, or 0 if there is no resource.
 */
f32 Bgm::getCurBpm() const {
    if (mResourceInfo == nullptr) {
        return 0.0f;
    }

    return mResourceSuffixInfo != nullptr ? mResourceSuffixInfo->mBpm : mResourceInfo->mBpm;
}

/**
 * Gets the current sample position.
 * @return Sample position, or -1.
 */
s32 Bgm::getCurSamplePosition() const {
    if (mResourceInfo == nullptr || !isPlaying()) {
        return -1;
    }

    if (alBgmFunction::isWaveSound(mResourceInfo->mName)) {
        nn::atk::WaveSoundHandle handle(mSoundHandle);
        return handle.GetPlaySamplePosition();
    }

    if (alBgmFunction::isStreamSound(mResourceInfo->mName)) {
        nn::atk::StreamSoundHandle handle(mSoundHandle);
        return handle.GetPlaySamplePosition();
    }

    return -1;
}

/**
 * Gets the current sample position if the sound is prepared.
 * @param pPosition Output sample position.
 * @return True if the position was written.
 */
bool Bgm::tryGetCurSamplePosition(s32* pPosition) {
    if (mResourceInfo == nullptr || !isPlaying()) {
        return false;
    }

    if (alBgmFunction::isWaveSound(mResourceInfo->mName)) {
        nn::atk::WaveSoundHandle handle(mSoundHandle);

        if (!handle.IsPrepared()) {
            return false;
        }

        *pPosition = handle.GetPlaySamplePosition();
        return true;
    }

    if (alBgmFunction::isStreamSound(mResourceInfo->mName)) {
        nn::atk::StreamSoundHandle handle(mSoundHandle);

        if (!handle.IsPrepared()) {
            return false;
        }

        *pPosition = handle.GetPlaySamplePosition();
        return true;
    }

    return false;
}

/**
 * Changes the volume.
 * @param pInfo Volume proc.
 */
void Bgm::changeVolume(const BgmVolumeProcInfo* pInfo) {
    mVolumeController->changeVolume(pInfo->mTargetVolume, pInfo->mVolumeDiff);
}

/**
 * Changes the track volumes that differ from the current track settings.
 * @param pInfo Track proc.
 */
void Bgm::changeTrack(const BgmTrackProcInfo* pInfo) {
    const BgmTrackChangeInfoList* changeInfoList = pInfo->mChangeTrackInfoList;
    nn::atk::StreamSoundHandle handle(mSoundHandle);

    if (changeInfoList != nullptr) {
        for (s32 i = 0; i < changeInfoList->getInfoNum(); i++) {
            const BgmTrackChangeInfo* changeInfo = changeInfoList->getInfo(i);

            if (!handle.IsAttachedSound()) {
                continue;
            }

            const BgmTrackChangeInfoList* curInfoList = mTrackChangeInfoList;

            if (curInfoList != nullptr) {
                const BgmTrackChangeInfo* curInfo = nullptr;
                s32 trackNo = changeInfo->mTrackNo;

                for (s32 j = 0; j < curInfoList->getInfoNum(); j++) {
                    const BgmTrackChangeInfo* info = curInfoList->getInfo(j);

                    if (info != nullptr && info->mTrackNo == trackNo) {
                        curInfo = info;
                        break;
                    }
                }

                if (curInfo == nullptr || curInfo->mVolume == changeInfo->mVolume) {
                    continue;
                }
            }

            handle.SetTrackVolume(1 << changeInfo->mTrackNo, changeInfo->mVolume,
                                  changeInfo->mFadeFrameNum != -1 ? changeInfo->mFadeFrameNum : 0);
        }
    }

    if (handle.IsAttachedSound()) {
        mTrackChangeInfoList = changeInfoList;
    }
}

/**
 * Requests a region jump if the region comes after the current one.
 * @param pInfo Region proc.
 */
void Bgm::changeRegion(const BgmRegionProcInfo* pInfo) {
    const BgmRegionProcInfo* curInfo = mRegionCtrl->mRegionInfo;

    if (curInfo != nullptr &&
        (curInfo->mHeadNo >= pInfo->mHeadNo || curInfo->mLoopEndNo >= pInfo->mHeadNo)) {
        return;
    }

    mRegionCtrl->mRegionInfo = pInfo;
    mRegionCtrl->mIsChangedRegion = true;
}

/**
 * Changes the pitch.
 * @param pInfo Pitch proc.
 */
void Bgm::changePitch(const BgmPitchProcInfo* pInfo) {
    mPitchController->changePitch(pInfo->mTargetPitch, pInfo->mPitchDiff);
}

/**
 * Changes the pitch.
 * @param pitch Target pitch.
 * @param speed Change per update.
 */
void Bgm::changePitch(f32 pitch, f32 speed) {
    mPitchController->changePitch(pitch, speed);
}

/**
 * Starts or stops the pitch modulation.
 * @param pInfo Pitch modulation proc.
 */
void Bgm::modulatePitch(const BgmPitchModulationProcInfo* pInfo) {
    if (pInfo->mIsEnable) {
        mPitchController->changeModulation(pInfo->mModDepth, pInfo->mModDepthDiff, pInfo->mModFreq);
    } else {
        mPitchController->changeModulation(0.0f, pInfo->mModDepthDiff, 0.0f);
    }
}

/**
 * Changes the low pass filter cut off frequency.
 * @param pInfo Low pass filter proc.
 */
void Bgm::lpf(const BgmLpfProcInfo* pInfo) {
    mLpfController->changeCutOffFreq(pInfo->mCutOffFreq, pInfo->mCutOffFreqDiff);
}

/**
 * Restarts the sound at a sample position.
 * @param sample Start sample.
 */
void Bgm::movePlayPosition(s32 sample) {
    mStartParam->set(mResourceInfo, nullptr, sample, 0, 0, getEnableRegionCtrl());
    startSound(mAudioPlayer, mSoundHandle, mStartParam, false);
}

/**
 * Switches to the resource with the given suffix.
 * @param pInfo Suffix proc.
 * @param isStartHead Whether to start at the head instead of the synced position.
 * @return Start sample, or -1 if the suffix can't be attached.
 */
s32 Bgm::attachSuffix(const BgmSuffixProcInfo* pInfo, bool isStartHead) {
    if (mResourceInfo == nullptr) {
        return -1;
    }

    mSuffixName = pInfo->mSuffixName;

    if (isEqualString(mSuffixName, "")) {
        return -1;
    }

    StringTmp<128> fileName;
    createFileNameBySuffix(&fileName, mResourceInfo->mName, mSuffixName);

    if (mResourceInfo->mResourceSuffixInfoList == nullptr) {
        return -1;
    }

    mResourceSuffixInfo = mResourceInfo->mResourceSuffixInfoList->tryFindInfo(mSuffixName);

    if (mResourceSuffixInfo == nullptr) {
        return -1;
    }

    if (!alSoundNameUtil::isExistItemName(fileName.cstr(), true)) {
        return -1;
    }

    s32 startSample = getCurStartSample();
    f32 bpm = getCurBpm();

    if (pInfo->mIsStartCurPosition) {
        startSample = calcCurSamplePositionByBpm(bpm);
    } else {
        mRegionCtrl->mIsChangedRegion = true;
    }

    mBpm = bpm;
    mTrackChangeInfoList = nullptr;

    if (isStartHead) {
        startSample = 0;
    }

    BgmSoundStartParam* startParam = mStartParam;
    const BgmResourceInfo* resourceInfo = mResourceInfo;
    const char* name = fileName.cstr();
    bool isEnableRegion = mResourceInfo->mIsEnableRegionJump;
    BgmRegionCtrl* regionCtrl = mRegionCtrl;
    startParam->set(resourceInfo, name, startSample, 0, 0,
                    isEnableRegion ? regionCtrl : nullptr);
    mIsPrepared = true;
    startSound(mAudioPlayer, mSoundHandle, mStartParam, true);
    return startSample;
}

/**
 * Switches back to the resource without suffix.
 * @param pInfo Suffix proc.
 * @return Start sample, or -1 if no suffix is attached.
 */
s32 Bgm::detachSuffix(const BgmSuffixProcInfo* pInfo) {
    if (mResourceInfo == nullptr || isEqualString(mSuffixName, "")) {
        return -1;
    }

    mSuffixName = "";

    BgmRegionCtrl* regionCtrl = getEnableRegionCtrl();
    f32 bpm = mResourceInfo->mBpm;
    s32 startSample;
    s32 fadeFrames;

    if (pInfo->mIsStartCurPosition) {
        startSample = calcCurSamplePositionByBpm(bpm);
        fadeFrames = 30;
    } else {
        startSample = mResourceInfo->mStartSample;
        fadeFrames = 0;
    }

    if (regionCtrl != nullptr) {
        startSample = 0;
        regionCtrl->mIsChangedRegion = true;
        regionCtrl->mIsRestoreRegionNo = true;
    }

    mBpm = bpm;
    mTrackChangeInfoList = nullptr;
    mStartParam->set(mResourceInfo, nullptr, startSample, fadeFrames, fadeFrames, regionCtrl);
    mIsPrepared = true;
    startSound(mAudioPlayer, mSoundHandle, mStartParam, true);
    return startSample;
}
}  // namespace al

namespace {
/**
 * Selects the next stream region of a BGM.
 * @param pRegionCtrl Region controller.
 * @param pParam Region callback parameters.
 * @return True if the sound continues.
 */
inline bool selectNextRegion(al::BgmRegionCtrl* pRegionCtrl, nn::atk::StreamRegionCallbackParam* pParam) {
    const al::BgmRegionProcInfo* info = pRegionCtrl->mRegionInfo;

    if (info == nullptr) {
        return true;
    }

    if (pRegionCtrl->mIsChangedRegion) {
        if (pRegionCtrl->isPlayedRegion(info)) {
            pParam->regionNo = info->mIsPlayHeadOneTime ? info->mLoopStartNo : info->mHeadNo;
        } else {
            if (!pRegionCtrl->mPlayedRegionInfos->pushBack(info)) {
                return false;
            }

            pParam->regionNo = info->mHeadNo;
        }

        if (pRegionCtrl->mIsRestoreRegionNo) {
            pRegionCtrl->mIsRestoreRegionNo = false;
            pParam->regionNo = pRegionCtrl->mRegionNo;
        }

        pRegionCtrl->mIsChangedRegion = false;
    } else {
        s32 regionNo = pParam->regionNo;

        if (regionNo < info->mLoopEndNo) {
            regionNo++;
        } else if (info->mIsLoop) {
            regionNo = info->mLoopStartNo;
        } else if (info->mNextSituationName == nullptr) {
            return false;
        }

        pParam->regionNo = regionNo;
    }

    pRegionCtrl->mRegionNo = pParam->regionNo;
    return true;
}

/**
 * Selects the next stream region of a BGM.
 * @param pParam Region callback parameters.
 * @param pArg Region controller.
 * @return Whether to continue playing.
 */
nn::atk::StreamRegionCallbackResult regionCallback(nn::atk::StreamRegionCallbackParam* pParam, void* pArg) {
    if (pArg == nullptr) {
        return nn::atk::StreamRegionCallbackResult_Finish;
    }

    if (!selectNextRegion(static_cast<al::BgmRegionCtrl*>(pArg), pParam)) {
        return nn::atk::StreamRegionCallbackResult_Finish;
    }

    return nn::atk::StreamRegionCallbackResult_Continue;
}
}  // namespace
