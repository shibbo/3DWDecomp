#include "Library/Bgm/BgmLineKeeper.hpp"

#include "Library/Audio/System/AudioSystemInfo.hpp"
#include "Library/Bgm/BgmDataBase.hpp"
#include "Library/Bgm/BgmLine.hpp"
#include "Project/Base/StringUtil.hpp"

namespace al {

/**
 * Finds the information of the line that plays a BGM.
 * @param pPlayName Play name.
 * @return The line information, or nullptr if not found.
 */
inline const BgmLineInfo* BgmLineKeeper::findLineInfo(const char* pPlayName) const {
    if (pPlayName == nullptr) {
        return nullptr;
    }

    const AudioInfoList<BgmPlayInfo>* playInfoList = mDataBase->mPlayInfoList;

    if (playInfoList == nullptr) {
        return nullptr;
    }

    const AudioInfoList<BgmLineInfo>* lineInfoList = mLineInfoList;
    const BgmPlayInfo* playInfo = playInfoList->tryFindInfo(pPlayName);

    if (playInfo == nullptr) {
        return nullptr;
    }

    if ((lineInfoList == nullptr) | (playInfo->mLineName == nullptr)) {
        return nullptr;
    }

    return lineInfoList->tryFindInfo(playInfo->mLineName);
}

/**
 * Finds the information of a line by its index.
 * @param index Line index.
 * @return The line information.
 */
inline const BgmLineInfo* BgmLineKeeper::findLineInfoByIndex(u32 index) const {
    const AudioInfoList<BgmLineInfo>* lineInfoList = mLineInfoList;

    for (s32 i = 0; i < lineInfoList->getInfoNum(); i++) {
        const BgmLineInfo* info = lineInfoList->getInfo(i);

        if (info->mPriority == index) {
            return info;
        }
    }

    return nullptr;
}

/**
 * Clears the BGM request that waits for the island BGM to finish.
 */
inline void BgmLineKeeper::clearWaitingRequest() {
    mWaitingRequest.name = "Dummy";
    mWaitingTimer = 180;
    mIsStartingWaitingBgm = false;
}

/**
 * Constructs the line keeper.
 * @param bpmRate Scale applied to the BPM given to the rhythm detectors.
 */
BgmLineKeeper::BgmLineKeeper(f32 bpmRate) : mBpmRate(bpmRate) {
    _1c = false;
    _20 = nullptr;
}

/**
 * @brief Creates the lines of a combined line.
 * @param pInfo Audio system information.
 * @param pCombinedLineName Name of the combined line.
 * @param pStageName Stage name used to find stage specific resources.
 */
void BgmLineKeeper::init(AudioSystemInfo* pInfo, const char* pCombinedLineName,
                         const char* pStageName) {
    mLineInfoList = pInfo->mBgmDataBase->mCombinedLineInfoList->tryFindInfo(pCombinedLineName)
                        ->mLineInfoList;

    if (mLineInfoList == nullptr) {
        mLines = nullptr;
        return;
    }

    s32 lineNum = mLineInfoList->getInfoNum();
    mLines = new BgmLineArray;
    mLines->allocBuffer(lineNum, nullptr);

    for (s32 i = 0; i < lineNum; i++) {
        mLines->pushBack(nullptr);
    }

    for (s32 i = 0; i < lineNum; i++) {
        BgmLine* line = new BgmLine(mBpmRate);
        const BgmLineInfo* lineInfo =
            mLineInfoList != nullptr ? mLineInfoList->getInfo(i) : nullptr;
        line->init(pInfo, lineInfo, pStageName);

        BgmLineArray* lines = mLines;
        s32 lineIndex = lineInfo->mPriority;
        if (static_cast<u32>(lines->size()) > static_cast<u32>(lineIndex)) {
            lines->replace(lineIndex, line);
        }
    }

    mDataBase = pInfo->mBgmDataBase;
    mActiveLineIndex = 0;
    mIslandSamplePos.fill(-1);
    mWaitingTimer = 180;
}

/**
 * Updates the lines, starts the waiting BGM and switches back to a lower line when the active
 * line stopped.
 * @param isDisableStart Whether starting the waiting BGM is disabled.
 */
void BgmLineKeeper::update(bool isDisableStart) {
    if (mLines == nullptr) {
        return;
    }

    if (!isEqualString(mWaitingRequest.name, "Dummy") && !getCurLine()->isPause()) {
        if (mWaitingTimer == 0 && !isDisableStart) {
            mIsStartingWaitingBgm = true;
            startBgm(mWaitingRequest);
            clearWaitingRequest();
        } else if (mWaitingTimer != 0) {
            mWaitingTimer--;
        }
    }

    for (u32 i = 0; i < mLines->size(); i++) {
        BgmLine* line = mLines->unsafeAt(i);

        if (i != mActiveLineIndex && line->isRunning()) {
            if (!line->isPause() && !line->isWaitStart()) {
                line->pauseBgm(-1);
            }
        } else if (i == mActiveLineIndex && line->isWaitStart()) {
            line->startWaitingBgm();
        }

        line->update();
    }

    if (mIsDisableLineChange || getCurLine()->isRunning()) {
        return;
    }

    while (mActiveLineIndex != 0) {
        mActiveLineIndex--;

        if (getCurLine()->isRunning()) {
            break;
        }
    }

    BgmLine* line = getCurLine();

    if (line->isWaitStart()) {
        line->startWaitingBgm();
        return;
    }

    if (line->isPause()) {
        line->resumeBgm(mResumeFadeFrames);
        mResumeFadeFrames = -1;
    }
}

/**
 * @brief Starts a BGM, pausing or stopping other lines according to their priority.
 * @param rRequest Playing request.
 */
void BgmLineKeeper::startBgm(const BgmPlayingRequest& rRequest) {
    if (isEqualString(rRequest.name, "NoBGM") || mLines == nullptr) {
        return;
    }

    BgmPlayingRequest curRequest(getCurLine()->getCurPlayName());
    s32 curIslandIndex = checkIfIslandBgm(curRequest);
    s32 islandIndex = checkIfIslandBgm(rRequest);
    s32 phaseIndex = checkPhaseBgm(rRequest);

    bool isStartingWaiting = mIsStartingWaitingBgm;

    if (!mIsIslandMapBgmVolume && phaseIndex != -1) {
        if (curIslandIndex != -1 && !isStartingWaiting) {
            mWaitingRequest.name = rRequest.name;
            mWaitingRequest.fadeInFrames = rRequest.fadeInFrames;
            mWaitingRequest.startDelayFrames = rRequest.startDelayFrames;
            mWaitingRequest.fadeOutFrames = rRequest.fadeOutFrames;
            mWaitingRequest.isRestart = rRequest.isRestart;
            mWaitingRequest._15 = rRequest._15;
            mWaitingRequest._18 = rRequest._18;
            mWaitingRequest._1c = rRequest._1c;
            mWaitingRequest.fadeInFrames = mWaitingRequest._1c;
            return;
        }
    } else if (islandIndex != -1 && !isStartingWaiting) {
        mWaitingTimer = 180;
        mWaitingRequest.name = "Dummy";
        mIsStartingWaitingBgm = false;
    }

    const BgmLineInfo* lineInfo = findLineInfo(rRequest.name);

    if (lineInfo == nullptr) {
        return;
    }

    s32 lineIndex = lineInfo->mPriority;
    BgmLine* line = mLines->unsafeAt(lineIndex);

    if (lineIndex > mActiveLineIndex) {
        s32 fadeFrames = rRequest.fadeOutFrames;

        if (islandIndex != -1) {
            s32 islandFadeFrames = startIslandBgm(rRequest, line, islandIndex);

            if (islandFadeFrames >= 0) {
                fadeFrames = islandFadeFrames;
            }
        } else {
            line->startBgm(rRequest);

            if (curIslandIndex != -1) {
                mIslandSamplePos[curIslandIndex] = getCurLine()->getCurPlayPos();
            }
        }

        getCurLine()->pauseBgm(fadeFrames);
        mActiveLineIndex = lineIndex;
        return;
    }

    if (lineIndex < mActiveLineIndex) {
        s32 keepLineNum = 0;

        for (s32 i = mActiveLineIndex; lineIndex < i; i--) {
            const BgmLineInfo* info = findLineInfoByIndex(i);

            if (info->mIsDontChangeLowPriorityLineByAreaChange) {
                keepLineNum++;
                continue;
            }

            if (curIslandIndex != -1 && islandIndex == -1) {
                mIslandSamplePos[curIslandIndex] = mLines->unsafeAt(i)->getCurPlayPos();
            }

            s32 fadeFrames =
                mIsIslandMapBgmVolume ? mIslandMapBgmVolume[0] : rRequest.fadeOutFrames;
            mLines->unsafeAt(i)->stopBgm(fadeFrames);

            if (i == mActiveLineIndex) {
                u32 index = mActiveLineIndex;

                while (index != 0 && !mLines->unsafeAt(index)->isRunning()) {
                    index--;
                }

                mActiveLineIndex = index;
            }
        }

        if (line->isRunningByPlayName(rRequest.name)) {
            if (line->isPause()) {
                if (keepLineNum > 0) {
                    return;
                }

                line->resumeBgm(!mIsIslandMapBgmVolume ? rRequest.fadeInFrames :
                                                         mIslandMapBgmVolume[1]);
                return;
            }

            if (line->isWaitStart()) {
                if (keepLineNum > 0) {
                    return;
                }

                line->startWaitingBgm();
                return;
            }

            if (line->isRunning()) {
                return;
            }

            line->startBgm(rRequest);
        } else if (islandIndex != -1) {
            startIslandBgm(rRequest, line, islandIndex);
        } else {
            line->startBgm(rRequest);
        }

        if (keepLineNum > 0) {
            line->pauseBgm(-1);
        }
    } else {
        if (islandIndex != -1) {
            startIslandBgm(rRequest, line, islandIndex);
        } else {
            line->startBgm(rRequest);
        }
    }
}

/**
 * Gets the island index of a BGM.
 * @param rRequest Playing request.
 * @return The island index, or -1 if the BGM is not an island BGM.
 */
s32 BgmLineKeeper::checkIfIslandBgm(const BgmPlayingRequest& rRequest) {
    if (rRequest.name == nullptr) {
        return -1;
    }

    if (isEqualString(rRequest.name, "Island01")) {
        return 0;
    }

    if (isEqualString(rRequest.name, "Island02")) {
        return 1;
    }

    if (isEqualString(rRequest.name, "Island03")) {
        return 2;
    }

    if (isEqualString(rRequest.name, "Island04")) {
        return 3;
    }

    if (isEqualString(rRequest.name, "Island05")) {
        return 4;
    }

    if (isEqualString(rRequest.name, "Island06")) {
        return 5;
    }

    if (isEqualString(rRequest.name, "Island07")) {
        return 6;
    }

    if (isEqualString(rRequest.name, "Island08")) {
        return 7;
    }

    if (isEqualString(rRequest.name, "Island09")) {
        return 8;
    }

    if (isEqualString(rRequest.name, "Island10")) {
        return 9;
    }

    if (isEqualString(rRequest.name, "Island11")) {
        return 10;
    }

    if (isEqualString(rRequest.name, "Island12")) {
        return 11;
    }

    return -1;
}

/**
 * Gets the phase index of a BGM.
 * @param rRequest Playing request.
 * @return The phase index, or -1 if the BGM is not a phase BGM.
 */
s32 BgmLineKeeper::checkPhaseBgm(const BgmPlayingRequest& rRequest) {
    if (isEqualString(rRequest.name, "Phase1")) {
        return 0;
    }

    if (isEqualString(rRequest.name, "Phase2")) {
        return 1;
    }

    if (isEqualString(rRequest.name, "Phase3")) {
        return 2;
    }

    if (isEqualString(rRequest.name, "Phase4")) {
        return 3;
    }

    return -1;
}

/**
 * Starts an island BGM from the sample position it was left at.
 * @param rRequest Playing request.
 * @param pLine Line to start the BGM on.
 * @param islandIndex Island index of the BGM.
 * @return Fade length in frames for the previous BGM, or -1 for the default.
 */
s32 BgmLineKeeper::startIslandBgm(const BgmPlayingRequest& rRequest, BgmLine* pLine,
                                  s32 islandIndex) {
    if (pLine->isRunningByPlayName(rRequest.name)) {
        return -1;
    }

    s32 fadeFrames = pLine->startIslandBgm(rRequest, mIslandSamplePos[islandIndex],
                                           mIslandMapBgmVolume[1], mIslandMapBgmVolume[0]);
    clearIslandList(islandIndex);
    return fadeFrames;
}

/**
 * @brief Prepares a BGM on its line.
 * @param rRequest Playing request.
 */
void BgmLineKeeper::prepareBgm(const BgmPlayingRequest& rRequest) {
    if (isEqualString(rRequest.name, "NoBGM") || mLines == nullptr) {
        return;
    }

    const BgmLineInfo* lineInfo = findLineInfo(rRequest.name);

    if (lineInfo == nullptr) {
        return;
    }

    s32 lineIndex = lineInfo->mPriority;
    mLines->unsafeAt(lineIndex)->prepareBgm(rRequest);
}

/**
 * Starts a prepared BGM.
 * @param pName Play name.
 */
void BgmLineKeeper::startPreparedBgm(const char* pName) {
    if (isEqualString(pName, "NoBGM") | (mLines == nullptr)) {
        return;
    }

    const BgmLineInfo* lineInfo = findLineInfo(pName);

    if (lineInfo == nullptr) {
        return;
    }

    s32 lineIndex = lineInfo->mPriority;
    BgmPlayingRequest request(pName);
    mLines->unsafeAt(lineIndex)->startPreparedBgm(request);
}

/**
 * Stops a BGM.
 * @param pName Play name.
 * @param fadeFrames Fade-out length in frames.
 * @param resumeFadeFrames Fade-in length in frames of the line that is resumed afterwards.
 */
void BgmLineKeeper::stopBgm(const char* pName, s32 fadeFrames, s32 resumeFadeFrames) {
    if (isEqualString(pName, "NoBGM") || mLines == nullptr) {
        return;
    }

    const BgmLineInfo* lineInfo = findLineInfo(pName);

    if (lineInfo == nullptr) {
        return;
    }

    s32 lineIndex = lineInfo->mPriority;
    mResumeFadeFrames = resumeFadeFrames;
    const char* curPlayName = mLines->unsafeAt(lineIndex)->getCurPlayName();

    if (curPlayName != nullptr && isEqualString(curPlayName, pName)) {
        mLines->unsafeAt(lineIndex)->stopBgm(fadeFrames);
    }
}

/**
 * Pauses a BGM.
 * @param pName Play name.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLineKeeper::pauseBgm(const char* pName, s32 fadeFrames) {
    if (isEqualString(pName, "NoBGM") | (mLines == nullptr)) {
        return;
    }

    const BgmLineInfo* lineInfo = findLineInfo(pName);

    if (lineInfo == nullptr) {
        return;
    }

    s32 lineIndex = lineInfo->mPriority;
    mLines->unsafeAt(lineIndex)->pauseBgm(fadeFrames);
}

/**
 * Resumes a BGM, making its line the active one if it has a higher priority.
 * @param pName Play name.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmLineKeeper::resumeBgm(const char* pName, s32 fadeFrames) {
    if (isEqualString(pName, "NoBGM") || mLines == nullptr) {
        return;
    }

    const BgmLineInfo* lineInfo = findLineInfo(pName);

    if (lineInfo == nullptr) {
        return;
    }

    s32 lineIndex = lineInfo->mPriority;
    BgmLine* line = mLines->unsafeAt(lineIndex);

    if (lineIndex > mActiveLineIndex) {
        line->resumeBgm(fadeFrames);
        getCurLine()->pauseBgm(-1);
        mActiveLineIndex = lineIndex;
    } else if (lineIndex == mActiveLineIndex) {
        line->resumeBgm(fadeFrames);
    }
}

/**
 * Pauses the active line.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLineKeeper::pauseActiveBgmLine(s32 fadeFrames) {
    if (mLines == nullptr) {
        return;
    }

    getCurLine()->pauseBgm(fadeFrames);
}

/**
 * Resumes the active line.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmLineKeeper::resumeActiveBgmLine(s32 fadeFrames) {
    if (mLines == nullptr) {
        return;
    }

    getCurLine()->resumeBgm(fadeFrames);
}

/**
 * Pauses the island line.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLineKeeper::pauseIslandBgm(s32 fadeFrames) {
    BgmLine* line = getBgmLineByLineName("Island");

    if (line != nullptr) {
        line->pauseBgm(fadeFrames);
    }
}

/**
 * Finds a line by its name.
 * @param pName Line name.
 * @return The line, or nullptr if not found.
 */
BgmLine* BgmLineKeeper::getBgmLineByLineName(const char* pName) const {
    if (mLines == nullptr) {
        return nullptr;
    }

    for (s32 i = 0; i < mLines->size(); i++) {
        BgmLine* line = mLines->unsafeAt(i);

        if (isEqualString(line->getLineName(), pName)) {
            return line;
        }
    }

    return nullptr;
}

/**
 * Resumes the island line.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmLineKeeper::resumeIslandBgm(s32 fadeFrames) {
    BgmLine* line = getBgmLineByLineName("Island");

    if (line != nullptr) {
        line->resumeBgm(fadeFrames);
    }
}

/**
 * Pauses the ocean (main) line.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLineKeeper::pauseOceanBgm(s32 fadeFrames) {
    BgmLine* line = getBgmLineByLineName("Main");

    if (line != nullptr) {
        line->pauseBgm(fadeFrames);
    }
}

/**
 * Resumes the ocean (main) line.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmLineKeeper::resumeOceanBgm(s32 fadeFrames) {
    BgmLine* line = getBgmLineByLineName("Main");

    if (line != nullptr) {
        line->resumeBgm(fadeFrames);
    }
}

/**
 * Checks whether the active line is paused.
 * @return True if paused.
 */
bool BgmLineKeeper::isPauseActiveBgmLine() {
    if (mLines == nullptr) {
        return false;
    }

    return getCurLine()->isPause();
}

/**
 * Stops all lines.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLineKeeper::stopAllBgmLine(s32 fadeFrames) {
    if (mLines == nullptr) {
        return;
    }

    for (u32 i = 0; i < mLines->size(); i++) {
        mLines->unsafeAt(i)->stopAllBgmPlayer(fadeFrames);
    }
}

/**
 * @brief Stops all lines that may be stopped by a BGM stop area.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLineKeeper::tryStopAllBgmLine(s32 fadeFrames) {
    if (mLines == nullptr) {
        return;
    }

    for (u32 i = 0; i < mLines->size(); i++) {
        const BgmLineInfo* info = findLineInfoByIndex(i);

        if (!info->mIsDontStopByBgmStopArea) {
            mLines->unsafeAt(i)->stopAllBgmPlayer(fadeFrames);
        }
    }

    return;
}

/**
 * @brief Pauses the line of a BGM if another BGM than the given one is playing on a line that may be
 * stopped by a BGM change area.
 * @param pName Play name.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmLineKeeper::tryPauseBgmIfNotPlaying(const char* pName, s32 fadeFrames) {
    for (s32 i = mLines->size() - 1; i >= 0; i--) {
        const BgmLineInfo* info = findLineInfoByIndex(i);

        if (info->mIsDontStopByChangeBgmArea) {
            continue;
        }

        BgmLine* line = mLines->unsafeAt(i);
        const char* curPlayName = line->getCurPlayName();

        if (curPlayName == nullptr || isEqualString(pName, curPlayName)) {
            continue;
        }

        if (findLineInfo(pName) != nullptr) {
            line->pauseBgm(fadeFrames);
        }

        return;
    }

    return;
}

/**
 * Changes the situation of all lines.
 * @param pName Situation name.
 */
void BgmLineKeeper::changeSituation(const char* pName) {
    if (mLines == nullptr) {
        return;
    }

    for (u32 i = 0; i < mLines->size(); i++) {
        mLines->unsafeAt(i)->changeSituation(pName, false);
    }
}

/**
 * Gets the active line if it is running.
 * @return The active line, or nullptr if it is not running.
 */
BgmLine* BgmLineKeeper::getActiveBgmLine() const {
    if (mLines == nullptr) {
        return nullptr;
    }

    BgmLine* line = getCurLine();
    return line->isRunning() ? line : nullptr;
}

/**
 * @brief Changes whether a line stops automatically when its BGM ends.
 * @param pName Play name.
 * @param isDisableAutoStop Whether the automatic stop is disabled.
 */
void BgmLineKeeper::changeLineAutoStopMode(const char* pName, bool isDisableAutoStop) {
    const BgmLineInfo* lineInfo = findLineInfo(pName);

    if (lineInfo == nullptr) {
        return;
    }

    s32 lineIndex = lineInfo->mPriority;
    BgmLine* line = mLines->unsafeAt(lineIndex);

    if (line != nullptr) {
        line->setIsDisableAutoStop(isDisableAutoStop);
    }
}

/**
 * Changes the pitch of the active line.
 * @param pitch Target pitch.
 */
void BgmLineKeeper::setActiveBgmPitch(f32 pitch) {
    BgmLine* line = getActiveBgmLine();

    if (line != nullptr) {
        line->changePitch(pitch);
    }
}

/**
 * Forgets the sample positions of all islands except one.
 * @param islandIndex Island index to keep.
 */
void BgmLineKeeper::clearIslandList(u32 islandIndex) {
    for (u32 i = 0; i < cIslandNum; i++) {
        if (i != islandIndex) {
            mIslandSamplePos(i) = -1;
        }
    }
}

/**
 * Changes the volume of the active line.
 * @param volume Target volume.
 * @param fadeFrames Fade length in frames.
 */
void BgmLineKeeper::changeActiveBgmVolume(f32 volume, s32 fadeFrames) {
    BgmLine* line = getActiveBgmLine();

    if (line != nullptr) {
        line->changeBgmVolume(volume, fadeFrames);
    }
}

/**
 * Gets the sample position of a BGM.
 * @param pName Play name.
 * @return The sample position, or -1 if the BGM is not playing.
 */
s32 BgmLineKeeper::getBgmSamplePos(const char* pName) {
    if (isEqualString(pName, "NoBGM") || mLines == nullptr) {
        return -1;
    }

    const BgmLineInfo* lineInfo = findLineInfo(pName);

    if (lineInfo == nullptr) {
        return -1;
    }

    s32 lineIndex = lineInfo->mPriority;
    BgmLine* line = mLines->unsafeAt(lineIndex);
    const char* curPlayName = line->getCurPlayName();

    if (curPlayName != nullptr && isEqualString(curPlayName, pName)) {
        return line->getCurPlayPos();
    }

    return -1;
}
}  // namespace al
