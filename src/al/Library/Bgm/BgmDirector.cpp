#include "Library/Bgm/BgmDirector.hpp"

#include "Library/Audio/System/AudioSystemInfo.hpp"
#include "Library/Bgm/BgmLine.hpp"
#include "Library/Bgm/BgmLineKeeper.hpp"
#include "Library/Bgm/BgmRhythmCtrl.hpp"
#include "Project/Base/StringUtil.hpp"

namespace al {
/**
 * Constructs the director.
 */
BgmDirector::BgmDirector() = default;

/**
 * Creates the line keeper and the rhythm controller.
 * @param pInfo Audio system information.
 * @param pStageName Stage name.
 * @param pScenarioName Scenario name.
 * @param frameRate Frame rate scale.
 */
void BgmDirector::init(AudioSystemInfo* pInfo, const char* pStageName, const char* pScenarioName, f32 frameRate) {
    mVolume = frameRate;
    mAudioPlayer = pInfo->getSeadAudioPlayerForBgm();
    mBgmLineKeeper = new BgmLineKeeper(mVolume);
    mBgmLineKeeper->init(pInfo, pStageName, pScenarioName);
    mBgmRhythmCtrl = new BgmRhythmCtrl(mVolume);
    mBgmRhythmCtrl->init(this);
    mBgmDataBase = pInfo->mBgmDataBase;
    mPauseIdFlags = 0;
}

/**
 * Updates the lines and the rhythm controller.
 */
void BgmDirector::update() {
    mBgmLineKeeper->update(mIsDisableBgmStart);
    mBgmRhythmCtrl->update();
}

/**
 * Starts a BGM.
 * @param rRequest Play request.
 */
void BgmDirector::startBgm(const BgmPlayingRequest& rRequest) {
    if (mIsDisableBgmStart) {
        return;
    }

    mBgmLineKeeper->startBgm(rRequest);
}

/**
 * Prepares a BGM.
 * @param rRequest Play request.
 */
void BgmDirector::prepareBgm(const BgmPlayingRequest& rRequest) {
    if (mIsDisableBgmStart) {
        return;
    }

    mBgmLineKeeper->prepareBgm(rRequest);
}

/**
 * Starts a prepared BGM.
 * @param pName Play name.
 */
void BgmDirector::startPreparedBgm(const char* pName) {
    if (mIsDisableBgmStart) {
        return;
    }

    mBgmLineKeeper->startPreparedBgm(pName);
}

/**
 * Does nothing.
 * @param pName Play name.
 */
void BgmDirector::pauseBgm(const char* pName) {}

/**
 * Stops a BGM.
 * @param pName Play name.
 * @param fadeFrames Fade-out length in frames.
 * @param unk Unknown.
 */
void BgmDirector::stopBgm(const char* pName, s32 fadeFrames, s32 unk) {
    mBgmLineKeeper->stopBgm(pName, fadeFrames, unk);
}

/**
 * Pauses a BGM.
 * @param pName Play name.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmDirector::pauseBgm(const char* pName, s32 fadeFrames) {
    mBgmLineKeeper->pauseBgm(pName, fadeFrames);
}

/**
 * Resumes a BGM.
 * @param pName Play name.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmDirector::resumeBgm(const char* pName, s32 fadeFrames) {
    mBgmLineKeeper->resumeBgm(pName, fadeFrames);
}

/**
 * Pauses the active BGM line.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmDirector::pauseActiveBgm(s32 fadeFrames) {
    mBgmLineKeeper->pauseActiveBgmLine(fadeFrames);
}

/**
 * Resumes the active BGM line.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmDirector::resumeActiveBgm(s32 fadeFrames) {
    mBgmLineKeeper->resumeActiveBgmLine(fadeFrames);
}

/**
 * Pauses the island BGM.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmDirector::pauseIslandBgm(s32 fadeFrames) {
    mBgmLineKeeper->pauseIslandBgm(fadeFrames);
}

/**
 * Resumes the island BGM.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmDirector::resumeIslandBgm(s32 fadeFrames) {
    mBgmLineKeeper->resumeIslandBgm(fadeFrames);
}

/**
 * Pauses the ocean BGM.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmDirector::pauseOceanBgm(s32 fadeFrames) {
    mBgmLineKeeper->pauseOceanBgm(fadeFrames);
}

/**
 * Resumes the ocean BGM.
 * @param fadeFrames Fade-in length in frames.
 */
void BgmDirector::resumeOceanBgm(s32 fadeFrames) {
    mBgmLineKeeper->resumeOceanBgm(fadeFrames);
}

/**
 * Checks whether the active BGM line is paused.
 * @return True if paused.
 */
bool BgmDirector::isPauseActiveBgm() {
    return mBgmLineKeeper->isPauseActiveBgmLine();
}

/**
 * Checks whether a BGM line is paused.
 * @param pName Line name.
 * @return True if the line exists and is paused.
 */
bool BgmDirector::isPauseBgm(const char* pName) {
    BgmLine* line = mBgmLineKeeper->getBgmLineByLineName(pName);

    if (line == nullptr) {
        return false;
    }

    return line->isPause();
}

/**
 * Pauses the active BGM line for a pause id.
 * @param id Pause id flag.
 * @param fadeFrames Fade-out length in frames.
 * @return True if this is the first pause id.
 */
bool BgmDirector::pauseActiveBgmById(u32 id, s32 fadeFrames) {
    u32 prevFlags = mPauseIdFlags;
    mPauseIdFlags = prevFlags | id;

    if (prevFlags != 0 || mPauseIdFlags == 0) {
        return false;
    }

    mBgmLineKeeper->pauseActiveBgmLine(fadeFrames);
    return true;
}

/**
 * Resumes the active BGM line for a pause id.
 * @param id Pause id flag.
 * @param fadeFrames Fade-in length in frames.
 * @return True if this was the last pause id.
 */
bool BgmDirector::resumeActiveBgmById(u32 id, s32 fadeFrames) {
    u32 prevFlags = mPauseIdFlags;
    mPauseIdFlags = prevFlags & ~id;

    if (prevFlags == 0 || mPauseIdFlags != 0) {
        return false;
    }

    mBgmLineKeeper->resumeActiveBgmLine(fadeFrames);
    return true;
}

/**
 * Stops all BGM lines.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmDirector::stopAllBgm(s32 fadeFrames) {
    mBgmLineKeeper->stopAllBgmLine(fadeFrames);
    mPauseIdFlags = 0;
}

/**
 * @brief Stops all BGM lines if any is playing.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmDirector::tryStopAllBgm(s32 fadeFrames) {
    mBgmLineKeeper->tryStopAllBgmLine(fadeFrames);
}

/**
 * @brief Pauses a BGM line when it is playing a different track.
 * @param pName Play name.
 * @param fadeFrames Fade-out length in frames.
 */
void BgmDirector::tryPauseBgmIfNotPlaying(const char* pName, s32 fadeFrames) {
    mBgmLineKeeper->tryPauseBgmIfNotPlaying(pName, fadeFrames);
}

/**
 * Changes the BGM situation.
 * @param pName Situation name.
 */
void BgmDirector::changeSituation(const char* pName) {
    if (mIsDisableChangeSituation) {
        return;
    }

    mBgmLineKeeper->changeSituation(pName);
}

/**
 * Gets the situation name of a BGM line.
 * @param pLineName Line name.
 * @return Situation name, or nullptr.
 */
const char* BgmDirector::getBgmLineSituationName(const char* pLineName) const {
    BgmLine* line = mBgmLineKeeper->getBgmLineByLineName(pLineName);

    if (line == nullptr) {
        return nullptr;
    }

    return line->getSituationName();
}

/**
 * Changes the auto stop mode of a BGM line.
 * @param pName Line name.
 * @param isAutoStop Whether the line stops automatically.
 */
void BgmDirector::changeLineAutoStopMode(const char* pName, bool isAutoStop) {
    mBgmLineKeeper->changeLineAutoStopMode(pName, isAutoStop);
}

/**
 * Disables or enables line changes.
 * @param isDisable Whether to disable line changes.
 */
void BgmDirector::disableLineChange(bool isDisable) {
    mBgmLineKeeper->setIsDisableLineChange(isDisable);
}

/**
 * Changes the volume of the active BGM line.
 * @param volume Volume.
 * @param fadeFrames Length of the change in frames.
 */
void BgmDirector::changeBgmVolume(f32 volume, s32 fadeFrames) {
    if (mIsDisableVolumeChange) {
        return;
    }

    mBgmLineKeeper->changeActiveBgmVolume(volume, fadeFrames);
}

/**
 * Sets the island map BGM volume.
 * @param volume1 First volume.
 * @param volume2 Second volume.
 * @param isEnable Whether the island map volume is used.
 */
void BgmDirector::changeIslandMapBgmVolume(s32 volume1, s32 volume2, bool isEnable) {
    mBgmLineKeeper->setIslandMapBgmVolume(static_cast<f32>(volume1), static_cast<f32>(volume2), isEnable);
}

/**
 * Gets the sample position of a BGM.
 * @param pName Play name.
 * @return Sample position.
 */
s32 BgmDirector::getBgmSamplePos(const char* pName) {
    return mBgmLineKeeper->getBgmSamplePos(pName);
}

/**
 * Checks whether a BGM is playing on the active line.
 * @param pName Play name.
 * @return True if playing.
 */
bool BgmDirector::isBgmCurrentlyPlaying(const char* pName) {
    if (mBgmLineKeeper->getActiveBgmLine() == nullptr) {
        return false;
    }

    const char* playName = mBgmLineKeeper->getActiveBgmLine()->getCurPlayName();

    if (playName == nullptr) {
        return false;
    }

    return isEqualString(pName, playName);
}

/**
 * Gets the active BGM line.
 * @return Active line.
 */
BgmLine* BgmDirector::getActiveBgmLine() const {
    return mBgmLineKeeper->getActiveBgmLine();
}

/**
 * Sets the pitch of the active BGM line.
 * @param pitch Pitch.
 */
void BgmDirector::setActiveBgmPitch(f32 pitch) {
    mBgmLineKeeper->setActiveBgmPitch(pitch);
}
}  // namespace al
