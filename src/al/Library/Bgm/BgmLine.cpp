#include "Library/Bgm/BgmLine.hpp"

#include <attributes.h>
#include <cmath>
#include <audio/SoundHandle.h>
#include <math/seadMathCalcCommon.h>

#include "Library/Audio/System/AudioSystemInfo.hpp"
#include "Library/Bgm/BgmDataBase.hpp"
#include "Library/Bgm/BgmMusicalInfo.hpp"
#include "Library/Bgm/BgmRhythmDetector.hpp"
#include "Library/Bgm/LinearValueController.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Bgm/Bgm.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"

namespace al {
namespace {
/**
 * Checks whether a BGM player is playing or waiting for its delayed start.
 * @param pBgm BGM player.
 * @return True if the player is active.
 */
inline bool isActiveBgm(const Bgm* pBgm) {
    return pBgm->getSoundHandle()->IsAttachedSound() || pBgm->getStartDelayFrames() > 0;
}

/**
 * Checks whether a BGM player is paused.
 * @param pBgm BGM player.
 * @return True if the player is paused.
 */
inline bool isPauseBgm(const Bgm* pBgm) {
    return pBgm->getSoundHandle()->IsPause() || pBgm->isPausedInDelay();
}

/**
 * Reads the rhythm (animation) change points of a musical information.
 * @param pInfo Musical information to fill.
 * @param rIter Iterator of the rhythm information file.
 */
NOINLINE void initRhythmInfoList(BgmMusicalInfo* pInfo, const ByamlIter& rIter) {
    ByamlIter animListIter;
    rIter.tryGetIterByKey(&animListIter, "AnimList");
    s32 num = animListIter.getSize();
    pInfo->rhythmInfoNum = num;
    pInfo->rhythmInfoList = new BgmRhythmInfo*[num];

    for (s32 i = 0; i < pInfo->rhythmInfoNum; i++) {
        ByamlIter iter;
        BgmRhythmInfo* rhythmInfo = new BgmRhythmInfo;
        animListIter.tryGetIterByIndex(&iter, i);
        iter.tryGetFloatByKey(&rhythmInfo->beat, "Beat");
        iter.tryGetIntByKey(&rhythmInfo->animId, "AnimId");
        pInfo->rhythmInfoList[i] = rhythmInfo;
    }
}

/**
 * Reads the chord change points of a musical information.
 * @param pInfo Musical information to fill.
 * @param rIter Iterator of the rhythm information file.
 */
NOINLINE void initChordInfoList(BgmMusicalInfo* pInfo, const ByamlIter& rIter) {
    ByamlIter chordListIter;
    rIter.tryGetIterByKey(&chordListIter, "ChordList");
    s32 num = chordListIter.getSize();
    pInfo->chordInfoNum = num;
    pInfo->chordInfoList = new BgmChordInfo*[num];

    for (s32 i = 0; i < pInfo->chordInfoNum; i++) {
        ByamlIter iter;
        chordListIter.tryGetIterByIndex(&iter, i);
        ByamlIter chordIter;
        ByamlIter scaleIter;
        iter.tryGetIterByKey(&chordIter, "Chord");
        iter.tryGetIterByKey(&scaleIter, "Scale");
        s32 chordNum = chordIter.getSize();
        s32 scaleNum = scaleIter.getSize();

        BgmChordInfo* chordInfo = new BgmChordInfo;
        chordInfo->chordNum = chordNum;
        chordInfo->chord = new s32[chordNum];
        chordInfo->scaleNum = scaleNum;
        chordInfo->scale = new s32[scaleNum];

        for (s32 j = 0; j < chordNum; j++) {
            chordIter.tryGetIntByIndex(&chordInfo->chord[j], j);
        }

        for (s32 j = 0; j < scaleNum; j++) {
            scaleIter.tryGetIntByIndex(&chordInfo->scale[j], j);
        }

        iter.tryGetFloatByKey(&chordInfo->beat, "Beat");
        iter.tryGetIntByKey(&chordInfo->root, "Root");
        pInfo->chordInfoList[i] = chordInfo;
    }
}

/**
 * Creates a musical information from a rhythm information file.
 * @param pByml Rhythm information file.
 * @return The created musical information.
 */
inline BgmMusicalInfo* createMusicalInfo(const u8* pByml) {
    ByamlIter iter(pByml);
    BgmMusicalInfo* info = new BgmMusicalInfo;
    initRhythmInfoList(info, iter);
    initChordInfoList(info, iter);
    return info;
}

/**
 * Gets an info of a list that may not exist.
 * @param pList Info list.
 * @param index Index of the info.
 * @return The info, or nullptr if the index is out of range.
 */
template <typename T>
inline T* getInfo(const AudioInfoList<T>* pList, s32 index) {
    while (pList != nullptr) {
        s32 num = pList->mInfos->size();

        if (index < num) {
            return pList->mInfos->unsafeAt(index);
        }

        pList = pList->mNext;

        if (pList == nullptr) {
            break;
        }

        index -= num;

        if (index < 0) {
            break;
        }
    }

    return nullptr;
}

/**
 * Gets the number of infos in a list that may not exist.
 * @param pList Info list.
 * @return Number of infos, zero if the list does not exist.
 */
template <typename T>
inline s32 getInfoNumSafe(const AudioInfoList<T>* pList) {
    return pList != nullptr ? pList->getInfoNum() : 0;
}
}  // namespace

/**
 * Checks whether a situation is enabled for the current BGM.
 * @param pName Situation name.
 * @return True if the situation is enabled.
 */
inline bool BgmLine::isEnableSituation(const char* pName) const {
    const AudioInfoList<BgmEnableSituationInfo>* list =
        mCurPlayInfo->resourceInfo->mEnableSituationInfoList;
    return list != nullptr && list->tryFindInfo(pName) != nullptr;
}

/**
 * Changes to the situations that are triggered when the current BGM starts.
 */
inline void BgmLine::startTriggerSituation() {
    const AudioInfoList<BgmStartTriggerSituationInfo>* list =
        mCurPlayInfo->resourceInfo->mStartTriggerSituationInfoList;

    if (list == nullptr) {
        return;
    }

    for (s32 i = 0; i < list->getInfoNum(); i++) {
        changeSituation(list->getInfo(i)->mName, false);
    }
}

/**
 * Restores the hurry and water situations after a BGM started and starts playing.
 */
inline void BgmLine::startSituationAfterStart() {
    if (mIsHurry) {
        changeSituation("ChangeNormalToHurry", true);
    }

    if (mIsInWater) {
        changeSituation("InWaterFast", false);
    }

    if (mIsHurry && isEnableSituation("ChangeNormalToHurry")) {
        return;
    }

    mState = State_Play;
}

/**
 * Constructs the BGM line.
 * @param bpmRate Scale applied to the BPM given to the rhythm detector.
 */
BgmLine::BgmLine(f32 bpmRate) : mBpmRate(bpmRate) {}

/**
 * Creates the BGM players and collects the playable BGMs of this line.
 * @param pInfo Audio system information.
 * @param pLineInfo Information of this line.
 * @param pStageName Stage name used to find stage specific resources.
 */
void BgmLine::init(AudioSystemInfo* pInfo, const BgmLineInfo* pLineInfo, const char* pStageName) {
    mLineInfo = pLineInfo;
    mCurPlayerIndex = 0;
    mBgmPlayers = new Bgm*[2];
    mSituationInfoList = pInfo->mBgmDataBase->mSituationInfoList;
    mPlayInfoList = pInfo->mBgmDataBase->mPlayInfoList;

    const BgmStageInfo* stageInfo = nullptr;

    if (pStageName != nullptr && pInfo->mBgmDataBase->mStageInfoList != nullptr) {
        stageInfo = pInfo->mBgmDataBase->mStageInfoList->tryFindInfo(pStageName);
    }

    if (stageInfo != nullptr) {
        mStagePlayInfoList = stageInfo->mStagePlayInfoList;
    } else {
        mStagePlayInfoList = nullptr;
    }

    for (s32 i = 0; i < 2; i++) {
        Bgm* bgm = new Bgm;
        bgm->init(pInfo);
        mBgmPlayers[i] = bgm;
    }

    Resource* resource = findOrCreateResource("SoundData/BgmRhythmInfo", nullptr);
    BgmMusicalInfo* defaultMusicalInfo = createMusicalInfo(resource->getByml("DEFAULT"));

    mPlayInfos = new BgmLinePlayInfoArray;
    s32 playInfoNum = mPlayInfoList->getInfoNum();
    mPlayInfos->allocBuffer(playInfoNum, nullptr);

    for (s32 i = 0; i < playInfoNum; i++) {
        const BgmPlayInfo* playInfo = getInfo(mPlayInfoList, i);

        if (!isEqualString(mLineInfo->mName, playInfo->mLineName)) {
            continue;
        }

        const char* resourceName = playInfo->mDefaultResourceName;
        s32 startDelayFrames = 0;
        s32 fadeInFrames = 0;

        if (mStagePlayInfoList != nullptr) {
            for (s32 j = 0; j < getInfoNumSafe(mStagePlayInfoList); j++) {
                const BgmStagePlayInfo* stagePlayInfo = getInfo(mStagePlayInfoList, j);

                if (isEqualString(stagePlayInfo->mPlayInfoName, playInfo->mName)) {
                    resourceName = stagePlayInfo->mResourceName;
                    startDelayFrames = stagePlayInfo->mStartDelayFrameNum;
                    fadeInFrames = stagePlayInfo->mFadeInFrameNum;
                    break;
                }
            }
        }

        if (resourceName == nullptr || pInfo->mBgmDataBase->mResourceInfoList == nullptr) {
            continue;
        }

        const BgmResourceInfo* resourceInfo =
            pInfo->mBgmDataBase->mResourceInfoList->tryFindInfo(resourceName);

        if (resourceInfo == nullptr) {
            continue;
        }

        BgmLinePlayInfo* linePlayInfo = new BgmLinePlayInfo;
        linePlayInfo->name = playInfo->mName;
        linePlayInfo->resourceInfo = resourceInfo;

        BgmMusicalInfo* musicalInfo = nullptr;

        StringTmp<128> bymlName("%s.byml", resourceName);
        if (resource->isExistFile(bymlName)) {
            musicalInfo = createMusicalInfo(resource->getByml(resourceName));
        }

        linePlayInfo->musicalInfo = musicalInfo != nullptr ? musicalInfo : defaultMusicalInfo;
        linePlayInfo->startDelayFrames = startDelayFrames;
        linePlayInfo->fadeInFrames = fadeInFrames;
        mPlayInfos->pushBack(linePlayInfo);
    }

    mRhythmDetector = new BgmRhythmDetector;
    mCurPlayInfo = nullptr;
    mState = State_None;
    mIsHurry = false;
    mIsInWater = false;
}

/**
 * Updates the BGM players, the line state and the rhythm detection.
 */
void BgmLine::update() {
    Bgm* bgm = getCurBgm();
    s32 prevStartDelay = bgm->getStartDelayFrames();
    bgm->update();

    if (mState == State_FadeOut) {
        if (!isActiveBgm(bgm)) {
            clearBgmLine();
        }
    } else if (isRunning() && !isActiveBgm(bgm) && !mIsDisableAutoStop && !isPrepared()) {
        clearBgmLine();
    }

    if (isEnableRhythmDetection()) {
        s32 samplePos = -1;

        if (bgm->tryGetCurSamplePosition(&samplePos)) {
            mRhythmDetector->update(samplePos);
        }
    }

    if (prevStartDelay != 0 && !isPrepared() && bgm->getStartDelayFrames() == 0 &&
        mCurPlayInfo != nullptr && !mIsHurry && !mIsInWater) {
        startTriggerSituation();
    }
}

/**
 * Clears the current and prepared BGM.
 */
void BgmLine::clearBgmLine() {
    mState = State_None;
    mCurPlayInfo = nullptr;
    mPreparedPlayInfo = nullptr;
}

/**
 * Checks whether the line is running (waiting, playing or paused).
 * @return True if running.
 */
bool BgmLine::isRunning() const {
    return mState < State_PauseFadeOut;
}

/**
 * Checks whether a BGM is prepared.
 * @return True if prepared.
 */
bool BgmLine::isPrepared() const {
    return mPreparedPlayInfo != nullptr;
}

/**
 * @brief Checks whether rhythm detection is enabled.
 * @return True if enabled.
 */
bool BgmLine::isEnableRhythmDetection() const {
    if (mCurPlayInfo != nullptr && !isActiveBgm(getCurBgm()) && mIsDisableAutoStop) {
        return false;
    }

    return !isPause() && isRunning() && !isWaitStart();
}

/**
 * Starts a BGM on this line.
 * @param rRequest Playing request.
 */
void BgmLine::startBgm(const BgmPlayingRequest& rRequest) {
    if (!rRequest._15) {
        if (isRunningByPlayName(rRequest.name) && !isFadeOut()) {
            return;
        }

        if (isPreparedByPlayName(rRequest.name)) {
            startPreparedBgm(rRequest);
            return;
        }
    }

    if (isActiveBgm(getCurBgm())) {
        if (isRunning()) {
            stopBgm(rRequest.fadeOutFrames);
        }

        mCurPlayerIndex = (mCurPlayerIndex + 1) % 2;
    }

    mCurPlayInfo = findPlayInfo(rRequest.name);

    if (mCurPlayInfo == nullptr) {
        return;
    }

    mSituationName = nullptr;
    getCurBgm()->startBgm(mCurPlayInfo->resourceInfo, rRequest, false);

    if (getCurBgm()->getStartDelayFrames() == 0 && !mIsHurry && !mIsInWater) {
        startTriggerSituation();
    }

    const BgmResourceInfo* resourceInfo = mCurPlayInfo->resourceInfo;
    mRhythmDetector->init(mCurPlayInfo->musicalInfo, resourceInfo->mBpm * mBpmRate,
                          resourceInfo->mSampleRate, resourceInfo->mStartSample, -1);
    startSituationAfterStart();
}

/**
 * Checks whether a BGM is running on this line.
 * @param pName Play name.
 * @return True if the BGM is running.
 */
bool BgmLine::isRunningByPlayName(const char* pName) const {
    if (!isRunning() || mCurPlayInfo == nullptr) {
        return false;
    }

    return isEqualString(pName, mCurPlayInfo->name);
}

/**
 * Checks whether the line is fading out.
 * @return True if fading out.
 */
bool BgmLine::isFadeOut() const {
    return mState == State_PauseFadeOut || mState == State_FadeOut;
}

/**
 * Checks whether a BGM is prepared on this line.
 * @param pName Play name.
 * @return True if the BGM is prepared.
 */
bool BgmLine::isPreparedByPlayName(const char* pName) const {
    if (mPreparedPlayInfo == nullptr) {
        return false;
    }

    return isEqualString(pName, mPreparedPlayInfo->name);
}

/**
 * Starts the prepared BGM.
 * @param rRequest Playing request.
 */
void BgmLine::startPreparedBgm(const BgmPlayingRequest& rRequest) {
    if (mPreparedPlayInfo == nullptr) {
        return;
    }

    mCurPlayInfo = mPreparedPlayInfo;
    mPreparedPlayInfo = nullptr;

    if (isActiveBgm(getCurBgm())) {
        getCurBgm()->stopBgm(-1);
    }

    mCurPlayerIndex = (mCurPlayerIndex + 1) % 2;
    getCurBgm()->startPreparedBgm(rRequest);

    if (!mIsHurry && !mIsInWater) {
        startTriggerSituation();
    }

    startSituationAfterStart();
}

/**
 * Stops the current BGM.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLine::stopBgm(s32 fadeFrames) {
    if (mCurPlayInfo == nullptr) {
        return;
    }

    Bgm* bgm = getCurBgm();

    if (isActiveBgm(bgm)) {
        bgm->stopBgm(fadeFrames);
        mState = State_FadeOut;
        mCurPlayInfo = nullptr;
        mSituationName = nullptr;
    }
}

/**
 * Changes the situation of the current BGM and runs its processes.
 * @param pName Situation name.
 * @param isForce Whether suffixes are attached even if they are already attached.
 */
void BgmLine::changeSituation(const char* pName, bool isForce) {
    mSituationName = pName;

    if (isEqualString(pName, "ChangeNormalToHurry")) {
        mIsHurry = true;
    } else if (isEqualString(pName, "ChangeHurryToNormal")) {
        mIsHurry = false;
    }

    if (isEqualString(pName, "InWater")) {
        mIsInWater = true;
    } else if (isEqualString(pName, "OutWater")) {
        mIsInWater = false;
    }

    const BgmEnableSituationInfo* enableSituationInfo =
        mCurPlayInfo != nullptr ? tryGetBgmEnableSituationInfo(pName) : nullptr;

    if (enableSituationInfo == nullptr || mSituationInfoList == nullptr ||
        enableSituationInfo->mName == nullptr) {
        return;
    }

    const BgmSituationInfo* situationInfo =
        mSituationInfoList->tryFindInfo(enableSituationInfo->mName);

    if (situationInfo == nullptr || situationInfo->mSubSituationInfoList == nullptr ||
        enableSituationInfo->mSubSituationName == nullptr) {
        return;
    }

    const BgmSubSituationInfo* subSituationInfo =
        situationInfo->mSubSituationInfoList->tryFindInfo(enableSituationInfo->mSubSituationName);

    if (subSituationInfo == nullptr || subSituationInfo->mProcInfoList == nullptr) {
        return;
    }

    const AudioInfoList<BgmProcInfo>* procInfoList = subSituationInfo->mProcInfoList;

    for (s32 i = 0; i < procInfoList->getInfoNum(); i++) {
        const BgmProcInfo* procInfo = procInfoList->getInfo(i);

        if (isEqualString(procInfo->mProcInfoName, "ChangeTrack")) {
            getCurBgm()->changeTrack(static_cast<const BgmTrackProcInfo*>(procInfo));
        } else if (isEqualString(procInfo->mProcInfoName, "AttachSuffix")) {
            attachSuffix(procInfo, isForce);
        } else if (isEqualString(procInfo->mProcInfoName, "DetachSuffix")) {
            detachSuffix(procInfo);
        } else if (isEqualString(procInfo->mProcInfoName, "ChangeVolume")) {
            getCurBgm()->changeVolume(static_cast<const BgmVolumeProcInfo*>(procInfo));
        } else if (isEqualString(procInfo->mProcInfoName, "ChangeRegion")) {
            getCurBgm()->changeRegion(static_cast<const BgmRegionProcInfo*>(procInfo));
        } else if (isEqualString(procInfo->mProcInfoName, "ChangePitch")) {
            getCurBgm()->changePitch(static_cast<const BgmPitchProcInfo*>(procInfo));
        } else if (isEqualString(procInfo->mProcInfoName, "ModulatePitch")) {
            getCurBgm()->modulatePitch(static_cast<const BgmPitchModulationProcInfo*>(procInfo));
        } else if (isEqualString(procInfo->mProcInfoName, "Lpf")) {
            getCurBgm()->lpf(static_cast<const BgmLpfProcInfo*>(procInfo));
        } else if (isEqualString(procInfo->mProcInfoName, "MoveLoopStart")) {
            const BgmResourceInfo* resourceInfo = mCurPlayInfo->resourceInfo;
            s32 startSample = resourceInfo->mStartSample;

            if (mIsHurry && resourceInfo->mResourceSuffixInfoList != nullptr) {
                const BgmResourceSuffixInfo* suffixInfo =
                    resourceInfo->mResourceSuffixInfoList->tryFindInfo("_FAST");

                if (suffixInfo != nullptr && suffixInfo->mStartSample >= 0) {
                    startSample = suffixInfo->mStartSample;
                }
            }

            getCurBgm()->movePlayPosition(startSample);
        }
    }
}

/**
 * Finds the enable situation information of the current BGM.
 * @param pName Situation name.
 * @return The enable situation information, or nullptr if not found.
 */
const BgmEnableSituationInfo* BgmLine::tryGetBgmEnableSituationInfo(const char* pName) {
    if (pName == nullptr) {
        return nullptr;
    }

    const AudioInfoList<BgmEnableSituationInfo>* list =
        mCurPlayInfo->resourceInfo->mEnableSituationInfoList;

    if (list == nullptr) {
        return nullptr;
    }

    return list->tryFindInfo(pName);
}

/**
 * Prepares a BGM on the inactive BGM player.
 * @param rRequest Playing request.
 */
void BgmLine::prepareBgm(const BgmPlayingRequest& rRequest) {
    if (mCurPlayInfo != nullptr && isEqualString(rRequest.name, mCurPlayInfo->name)) {
        return;
    }

    u32 index = (mCurPlayerIndex + 1) % 2;
    Bgm* bgm = mBgmPlayers[index];

    if (isActiveBgm(bgm)) {
        bgm->stopBgm(-1);
    }

    mPreparedPlayInfo = findPlayInfo(rRequest.name);
    bgm->startBgm(mPreparedPlayInfo->resourceInfo, rRequest, true);

    const BgmResourceInfo* resourceInfo = mPreparedPlayInfo->resourceInfo;
    mRhythmDetector->init(mPreparedPlayInfo->musicalInfo, resourceInfo->mBpm * mBpmRate,
                          resourceInfo->mSampleRate, resourceInfo->mStartSample, -1);
    mState = State_Play;
}

/**
 * Checks whether preparing a BGM is unnecessary because it is already the current one.
 * @param pName Play name.
 * @return True if the BGM is the current one.
 */
bool BgmLine::isUnnecessaryPrepare(const char* pName) const {
    if (mCurPlayInfo == nullptr) {
        return false;
    }

    return isEqualString(pName, mCurPlayInfo->name);
}

/**
 * Starts the BGM that is waiting to be started.
 */
void BgmLine::startWaitingBgm() {
    if (mCurPlayInfo == nullptr) {
        return;
    }

    getCurBgm()->startPreparedBgmExistingRequest();

    if (!mIsHurry && !mIsInWater) {
        startTriggerSituation();
    }

    mState = State_Play;
}

/**
 * Starts an island BGM from a given sample position.
 * @param rRequest Playing request.
 * @param startSample Sample to start from, or -1 to start from the head.
 * @param startDelayFrames Start delay in frames, or -1 for the default.
 * @param fadeOutFrames Fade-out length in frames, or -1 for the default.
 * @return Fade length in frames for the previous BGM, or -1 for the default.
 */
s32 BgmLine::startIslandBgm(const BgmPlayingRequest& rRequest, s32 startSample,
                            s32 startDelayFrames, s32 fadeOutFrames) {
    BgmPlayingRequest request = rRequest;
    s32 fadeFrames = -1;

    if (startSample == -1) {
        request.fadeInFrames = 0;
        fadeFrames = 5;
    } else {
        request._18 = startSample;
    }

    if (startDelayFrames != -1) {
        request.fadeInFrames = startDelayFrames;
    }

    if (fadeOutFrames != -1) {
        fadeFrames = fadeOutFrames;
        request.fadeOutFrames = fadeOutFrames;
    }

    startBgm(request);
    return fadeFrames;
}

/**
 * Pauses the current BGM.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLine::pauseBgm(s32 fadeFrames) {
    if (mCurPlayInfo == nullptr || isWaitStart() || isPause()) {
        return;
    }

    Bgm* bgm = getCurBgm();

    if (!isActiveBgm(bgm) || isPauseBgm(bgm)) {
        return;
    }

    bgm->pauseBgm(fadeFrames);

    switch (mState) {
    case State_Play: {
        mState = State_Pause;
        Bgm* unactiveBgm = mBgmPlayers[getUnactiveBgmPlayerIndex()];

        if (isActiveBgm(unactiveBgm)) {
            unactiveBgm->pauseBgm(fadeFrames);
        }

        break;
    }
    case State_FadeOut:
        mState = State_PauseFadeOut;
        break;
    default:
        break;
    }
}

/**
 * Checks whether the line is paused.
 * @return True if paused.
 */
bool BgmLine::isPause() const {
    return mState == State_Pause || mState == State_PauseFadeOut;
}

/**
 * Resumes the current BGM.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmLine::resumeBgm(s32 fadeFrames) {
    if (mCurPlayInfo == nullptr || !isPause()) {
        return;
    }

    Bgm* bgm = getCurBgm();

    if (!isActiveBgm(bgm) || !isPauseBgm(bgm)) {
        return;
    }

    bgm->resumeBgm(fadeFrames);

    switch (mState) {
    case State_Pause: {
        mState = State_Play;
        Bgm* unactiveBgm = mBgmPlayers[getUnactiveBgmPlayerIndex()];

        if (isActiveBgm(unactiveBgm)) {
            unactiveBgm->resumeBgm(0);
        }

        break;
    }
    case State_PauseFadeOut:
        mState = State_FadeOut;
        break;
    default:
        break;
    }
}

/**
 * Stops both BGM players.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLine::stopAllBgmPlayer(s32 fadeFrames) {
    for (s32 i = 0; i < 2; i++) {
        if (isActiveBgm(mBgmPlayers[i])) {
            mBgmPlayers[i]->stopBgm(fadeFrames);
        }
    }

    mState = State_FadeOut;
}

/**
 * Attaches a suffix to the current BGM.
 * @param pInfo Suffix process information.
 * @param isForce Whether the suffix is attached even if it is already attached.
 */
void BgmLine::attachSuffix(const BgmProcInfo* pInfo, bool isForce) {
    if (mCurPlayInfo == nullptr) {
        return;
    }

    const AudioInfoList<BgmResourceSuffixInfo>* suffixInfoList =
        mCurPlayInfo->resourceInfo->mResourceSuffixInfoList;
    const BgmSuffixProcInfo* suffixProcInfo = static_cast<const BgmSuffixProcInfo*>(pInfo);

    if (suffixInfoList == nullptr || suffixProcInfo->mSuffixName == nullptr) {
        return;
    }

    const BgmResourceSuffixInfo* suffixInfo =
        suffixInfoList->tryFindInfo(suffixProcInfo->mSuffixName);

    if (suffixInfo == nullptr) {
        return;
    }

    getCurBgm()->attachSuffix(suffixProcInfo, isForce);
    mRhythmDetector->init(mCurPlayInfo->musicalInfo, suffixInfo->mBpm * mBpmRate,
                          suffixInfo->mSampleRate, suffixInfo->mStartSample, -1);
    if (mIsInWater) {
        changeSituation("InWaterFast", false);
    } else {
        changeSituation("InitWater", false);
    }

    mState = State_WaitStart;
}

/**
 * @brief Detaches the suffix from the current BGM.
 * @param pInfo Suffix process information.
 */
NOINLINE void BgmLine::detachSuffix(const BgmProcInfo* pInfo) {
    if (mCurPlayInfo == nullptr) {
        return;
    }

    const BgmSuffixProcInfo* suffixProcInfo = static_cast<const BgmSuffixProcInfo*>(pInfo);
    s32 curSample = getCurBgm()->detachSuffix(suffixProcInfo);
    BgmRhythmDetector* rhythmDetector = mRhythmDetector;
    const BgmResourceInfo* resourceInfo = mCurPlayInfo->resourceInfo;
    BgmMusicalInfo* musicalInfo = mCurPlayInfo->musicalInfo;
    s32 startSample = suffixProcInfo->mIsStartCurPosition ? curSample : -1;
    rhythmDetector->init(musicalInfo, resourceInfo->mBpm * mBpmRate,
                         resourceInfo->mSampleRate, resourceInfo->mStartSample, startSample);

    if (mIsInWater) {
        changeSituation("InWaterFast", false);
    } else {
        changeSituation("InitWater", false);
    }

    mState = State_WaitStart;
}

/**
 * Checks whether the line is waiting to start.
 * @return True if waiting.
 */
bool BgmLine::isWaitStart() const {
    return mState == State_WaitStart;
}

/**
 * Gets the play name of the current BGM.
 * @return The play name, or nullptr if no BGM is set.
 */
const char* BgmLine::getCurPlayName() const {
    if (mCurPlayInfo == nullptr) {
        return nullptr;
    }

    return mCurPlayInfo->name;
}

/**
 * Gets the name of this line.
 * @return The line name, or nullptr if the line is not initialized.
 */
const char* BgmLine::getLineName() const {
    if (mLineInfo == nullptr) {
        return nullptr;
    }

    return mLineInfo->mName;
}

/**
 * Gets the BPM of the current BGM.
 * @return The current BPM.
 */
f32 BgmLine::getCurBpm() const {
    return getCurBgm()->getCurBpm();
}

/**
 * Changes the pitch of the current BGM immediately.
 * @param pitch Target pitch.
 */
void BgmLine::changePitch(f32 pitch) {
    getCurBgm()->changePitch(pitch, 1.0f);
}

/**
 * Gets the current sample position of the current BGM.
 * @return The current sample position.
 */
s32 BgmLine::getCurPlayPos() const {
    return getCurBgm()->getCurSamplePosition();
}

/**
 * Changes the volume of the current BGM.
 * @param volume Target volume.
 * @param fadeFrames Fade length in frames.
 */
void BgmLine::changeBgmVolume(f32 volume, s32 fadeFrames) {
    s32 frames = fadeFrames > 1 ? fadeFrames : 1;
    BgmVolumeProcInfo procInfo;
    procInfo.mProcInfoName = "ChangeVolume";
    procInfo.mTargetVolume = volume;
    procInfo.mVolumeDiff =
        std::abs(getCurBgm()->getVolumeController()->getCurFadeVolume() - volume) / frames;
    getCurBgm()->changeVolume(&procInfo);
}

/**
 * Gets the index of the BGM player that is not in use.
 * @return The index of the inactive BGM player.
 */
u32 BgmLine::getUnactiveBgmPlayerIndex() const {
    return mCurPlayerIndex == 0;
}

/**
 * Finds a playable BGM of this line.
 * @param pName Play name.
 * @return The playable BGM, or nullptr if not found.
 */
inline BgmLinePlayInfo* BgmLine::findPlayInfo(const char* pName) const {
    const BgmLinePlayInfoArray* playInfos = mPlayInfos;

    for (s32 i = 0; i < playInfos->size(); i++) {
        if (isEqualString(playInfos->unsafeAt(i)->name, pName)) {
            return playInfos->unsafeAt(i);
        }
    }

    return nullptr;
}
}  // namespace al
