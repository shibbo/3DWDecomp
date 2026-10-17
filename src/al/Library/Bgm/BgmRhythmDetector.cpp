#include "Library/Bgm/BgmRhythmDetector.hpp"

#include "Library/Bgm/BgmMusicalInfo.hpp"
#include "Library/Yaml/ByamlIter.hpp"

namespace {
/** Number of samples the animation beat runs ahead of the actual beat. */
constexpr s32 cAnimPreSampleNum = 4400;

/** Least common multiple of 1 to 8, used to wrap the beat counters. */
constexpr s32 cBeatCountLoop = 840;

/**
 * Rounds a beat up to the next integer when it is just below it.
 * @param beat Beat.
 * @return Rounded beat.
 */
inline f32 roundBeat(f32 beat) {
    s32 intBeat = static_cast<s32>(beat);

    if (beat - static_cast<f32>(intBeat) > 0.99f) {
        return static_cast<f32>(intBeat + 1);
    }

    return beat;
}

/**
 * Gets the fractional part of a beat.
 * @param beat Beat.
 * @return Fractional part.
 */
inline f32 calcBeatRate(f32 beat) {
    return beat - static_cast<f32>(static_cast<s32>(beat));
}

/**
 * Checks whether a beat boundary was crossed between two beats.
 * @param prevBeat Beat of the previous update.
 * @param curBeat Beat of the current update.
 * @return True if a beat boundary was crossed.
 */
inline bool isPassBeat(f32 prevBeat, f32 curBeat) {
    if (curBeat >= prevBeat) {
        if (curBeat - prevBeat > 1.0f) {
            return false;
        }

        if (static_cast<f32>(static_cast<s32>(curBeat)) - prevBeat > 0.0f) {
            return true;
        }

        return false;
    }

    if (calcBeatRate(prevBeat) <= calcBeatRate(curBeat)) {
        return false;
    }

    return true;
}

/**
 * Advances a beat counter and resets the progress rate in the beat.
 * @param pCount Beat counter.
 * @param pRate Progress rate in the beat.
 * @param beat Current beat.
 */
inline void countBeat(s32* pCount, f32* pRate, f32 beat) {
    *pCount = (*pCount + 1) % cBeatCountLoop;
    *pRate = calcBeatRate(beat);
}
}  // namespace

namespace al {
/**
 * Constructs a rhythm detector.
 */
BgmRhythmDetector::BgmRhythmDetector() {
    _20.allocBuffer(6, nullptr);

    for (s32 i = 0; i < 6; i++) {
        _20.pushBack(new s32(1000));
    }
}

/**
 * @brief Initializes the detector for a BGM.
 * @param pMusicalInfo Musical information of the BGM.
 * @param bpm Beats per minute.
 * @param sampleRate Sample rate.
 * @param beginSample Sample of the first beat.
 * @param loopStartSample Sample the playback starts from, or -1 to start from the first beat.
 */
void BgmRhythmDetector::init(BgmMusicalInfo* pMusicalInfo, f32 bpm, s32 sampleRate,
                             s32 beginSample, s32 loopStartSample) {
    mBpm = bpm;
    mBeatOffset = static_cast<f32>(beginSample);
    mSampleRate = sampleRate;
    mMusicalInfo = pMusicalInfo;
    mSamplePerBeat = static_cast<f32>(sampleRate * 60) / bpm;
    mBeatPerFrame = bpm / 60.0f / 60.0f;
    mFrameRate = bpm / 120.0f;
    mAnimFrame = 0.0f;
    mAnimType = -1;
    mIsFirstUpdate = true;
    mIsStartBeat = false;
    mIsStartBeatForAnime = false;
    mIsTriggerRestartBgm = true;
    mIsTriggerBeat = false;
    mIsTriggerBeatForAnime = false;
    mIsTriggerRhythm = false;
    mIsTriggerAnimChange = false;
    mCurRhythmBeat = 0.0f;
    mBeatRate = 0.0f;
    mBeatRateForAnime = 0.0f;
    mBeatCount = 0;
    mBeatCountForAnime = 0;

    s32 startSample = loopStartSample == -1 ? beginSample : loopStartSample;
    f32 startBeat = static_cast<f32>(startSample) / mSamplePerBeat;
    f32 startBeatForAnime = static_cast<f32>(startSample + cAnimPreSampleNum) / mSamplePerBeat;
    mCurBeat = roundBeat(startBeat);
    mCurBeatForAnime = roundBeat(startBeatForAnime);

    while (mBeatOffset >= mSamplePerBeat) {
        mBeatOffset -= mSamplePerBeat;
    }

    f32 offsetRate = mBeatOffset / mSamplePerBeat;
    // The unordered comparisons preserve a NaN offset, as in the original detector.
    mBeatOffset = (!(offsetRate >= 0.9f) & !(offsetRate <= 0.1f)) ? offsetRate : 0.0f;
    mBeatRate = calcBeatRate(mCurBeat - mBeatOffset);
    mBeatRateForAnime = calcBeatRate(mCurBeatForAnime - mBeatOffset);
}

/**
 * Gets the sample from which triggers start being detected.
 * @return Trigger start sample.
 */
s32 BgmRhythmDetector::getTrgStartSample() const {
    return -cAnimPreSampleNum;
}

/**
 * Updates the beat, trigger, animation and chord states.
 * @param curSample Current playback sample.
 */
void BgmRhythmDetector::update(s32 curSample) {
    s32 animSample = curSample + cAnimPreSampleNum;
    f32 prevAnimBeat = mCurBeatForAnime;
    f32 animBeat = roundBeat(static_cast<f32>(animSample) / mSamplePerBeat);
    mCurBeatForAnime = animBeat;

    if (animSample < 0) {
        mAnimType = 0;
        return;
    }

    mIsTriggerRhythm = false;
    mIsTriggerAnimChange = false;

    if (mIsFirstUpdate) {
        const BgmRhythmInfo* curInfo = tryFindCurRhythmInfo(animBeat);
        mNextRhythmInfo = tryFindNextRhythmInfo(animBeat);
        mAnimType = curInfo->animId;
        mCurRhythmBeat = curInfo->beat;
        mIsTriggerRhythm = true;
        mIsTriggerAnimChange = true;
        mIsFirstUpdate = false;
    } else {
        mIsTriggerRestartBgm = false;
    }

    if (animBeat >= mBeatOffset) {
        f32 beat = animBeat - mBeatOffset;
        mBeatRateForAnime += mBeatPerFrame;

        bool& rIsTrigger = mIsTriggerBeatForAnime;
        if (!mIsStartBeatForAnime) {
            rIsTrigger = true;
            mIsStartBeatForAnime = true;
        } else if (isPassBeat(prevAnimBeat - mBeatOffset, beat)) {
            rIsTrigger = true;
        } else {
            rIsTrigger = false;
        }

        if (rIsTrigger) {
            countBeat(&mBeatCountForAnime, &mBeatRateForAnime, beat);
        }
    }

    f32 curBeat = static_cast<f32>(curSample) / mSamplePerBeat;
    if (curBeat >= mBeatOffset) {
        f32 beat = roundBeat(curBeat - mBeatOffset);
        f32 prevBeat = mCurBeat;
        mCurBeat = beat;
        mBeatRate += mBeatPerFrame;

        if (!mIsStartBeat) {
            mIsTriggerBeat = true;
            mIsStartBeat = true;
        } else if (isPassBeat(prevBeat, beat)) {
            mIsTriggerBeat = true;
        } else {
            mIsTriggerBeat = false;
        }

        if (mIsTriggerBeat) {
            countBeat(&mBeatCount, &mBeatRate, beat);
        }
    }

    if (mCurRhythmBeat >= animBeat) {
        const BgmRhythmInfo* curInfo = tryFindCurRhythmInfo(animBeat);
        mAnimType = curInfo->animId;
        mCurRhythmBeat = curInfo->beat;
        mNextRhythmInfo = tryFindNextRhythmInfo(animBeat);
    }

    if (animBeat >= mNextRhythmInfo->beat) {
        mIsTriggerRhythm = true;

        if (mNextRhythmInfo->animId != mAnimType) {
            mIsTriggerAnimChange = true;
            mAnimType = mNextRhythmInfo->animId;
        }

        mCurRhythmBeat = mNextRhythmInfo->beat;
        mNextRhythmInfo = tryFindNextRhythmInfo(animBeat);

        if (isOverBeatInRhythmInfoList(animBeat)) {
            mIsOverRhythmInfoList = true;
        }
    }

    mAnimFrame = (animBeat - mCurRhythmBeat) * 30.0f;

    const BgmMusicalInfo* musicalInfo = mMusicalInfo;
    if (musicalInfo->chordInfoNum < 1) {
        return;
    }

    mChordInfoCurrent = nullptr;
    for (s32 i = 0; i < musicalInfo->chordInfoNum; i++) {
        if (musicalInfo->chordInfoList[i]->beat > animBeat && i != 0) {
            mChordInfoCurrent = musicalInfo->chordInfoList[i - 1];
            break;
        }
    }

    if (mChordInfoCurrent == nullptr) {
        mChordInfoCurrent = musicalInfo->chordInfoList[musicalInfo->chordInfoNum - 1];
    }
}

/**
 * Finds the animation change point a beat lies in.
 * @param beat Beat.
 * @return Rhythm information at the beat.
 */
const BgmRhythmInfo* BgmRhythmDetector::tryFindCurRhythmInfo(f32 beat) const {
    for (s32 i = 0; i < mMusicalInfo->rhythmInfoNum; i++) {
        if (mMusicalInfo->rhythmInfoList[i]->beat > beat) {
            if (i == 0) {
                return mMusicalInfo->rhythmInfoList[0];
            }

            return mMusicalInfo->rhythmInfoList[i - 1];
        }
    }

    return mMusicalInfo->rhythmInfoList[0];
}

/**
 * Finds the next animation change point after a beat.
 * @param beat Beat.
 * @return Next rhythm information.
 */
const BgmRhythmInfo* BgmRhythmDetector::tryFindNextRhythmInfo(f32 beat) const {
    for (s32 i = 0; i < mMusicalInfo->rhythmInfoNum; i++) {
        if (mMusicalInfo->rhythmInfoList[i]->beat > beat) {
            return mMusicalInfo->rhythmInfoList[i];
        }
    }

    return mMusicalInfo->rhythmInfoList[0];
}

/**
 * Checks whether a beat is past the last animation change point.
 * @param beat Beat.
 * @return True if the beat is past the last change point.
 */
bool BgmRhythmDetector::isOverBeatInRhythmInfoList(f32 beat) const {
    return mMusicalInfo->rhythmInfoList[mMusicalInfo->rhythmInfoNum - 1]->beat <= beat;
}

/**
 * Checks whether a beat with the given interval is triggered this update.
 * @param beat Beat interval (1 to 8).
 * @return True if the beat is triggered.
 */
bool BgmRhythmDetector::isTriggerBeat(s32 beat) const {
    if (beat >= 1 && beat <= 8) {
        if (!mIsTriggerBeat) {
            return false;
        }

        if (mBeatCount == 0) {
            return false;
        }

        return (mBeatCount - 1) % beat == 0;
    }

    return false;
}

/**
 * Checks whether a beat for animation with the given interval is triggered this update.
 * @param beat Beat interval (1 to 8).
 * @return True if the beat is triggered.
 */
bool BgmRhythmDetector::isTriggerBeatForAnime(s32 beat) const {
    if (beat >= 1 && beat <= 8) {
        if (!mIsTriggerBeatForAnime) {
            return false;
        }

        if (mBeatCountForAnime == 0) {
            return false;
        }

        return (mBeatCountForAnime - 1) % beat == 0;
    }

    return false;
}

/**
 * Calculates the animation frame at a beat.
 * @param beat Beat.
 * @return Animation frame since the last change point.
 */
s32 BgmRhythmDetector::calcAnimFrame(f32 beat) const {
    s32 frame = 0;
    for (s32 i = 0; i < mMusicalInfo->rhythmInfoNum; i++) {
        if (mMusicalInfo->rhythmInfoList[i]->beat > beat && i != 0) {
            frame = (beat - mMusicalInfo->rhythmInfoList[i - 1]->beat) * 30.0f;
            break;
        }
    }

    return frame;
}


/**
 * Finds the animation type at a beat.
 * @param beat Beat.
 * @return Animation type.
 */
s32 BgmRhythmDetector::findAnimType(f32 beat) const {
    for (s32 i = 0; i < mMusicalInfo->rhythmInfoNum; i++) {
        if (mMusicalInfo->rhythmInfoList[i]->beat > beat && i != 0) {
            return mMusicalInfo->rhythmInfoList[i - 1]->animId;
        }
    }

    return 0;
}

/**
 * Reads the animation change points.
 * @param iter Iterator of the beat list.
 */
void BgmRhythmDetector::initBeatList(ByamlIter iter) {
    ByamlIter beatIter;
    mMusicalInfo->rhythmInfoNum = iter.getSize();
    mMusicalInfo->rhythmInfoList = new BgmRhythmInfo*[mMusicalInfo->rhythmInfoNum];

    for (s32 i = 0; i < mMusicalInfo->rhythmInfoNum; i++) {
        mMusicalInfo->rhythmInfoList[i] = new BgmRhythmInfo();
        iter.tryGetIterByIndex(&beatIter, i);
        beatIter.tryGetFloatByKey(&mMusicalInfo->rhythmInfoList[i]->beat, "Beat");
        beatIter.tryGetIntByKey(&mMusicalInfo->rhythmInfoList[i]->animId, "AnimId");
    }
}

/**
 * Reads the chord change points.
 * @param iter Iterator of the chord list.
 */
void BgmRhythmDetector::initChordList(ByamlIter iter) {
    ByamlIter chordIter;
    mMusicalInfo->chordInfoNum = iter.getSize();
    mMusicalInfo->chordInfoList = new BgmChordInfo*[mMusicalInfo->chordInfoNum];

    for (s32 i = 0; i < mMusicalInfo->chordInfoNum; i++) {
        iter.tryGetIterByIndex(&chordIter, i);
        ByamlIter chordListIter;
        ByamlIter scaleListIter;
        chordIter.tryGetIterByKey(&chordListIter, "Chord");
        chordIter.tryGetIterByKey(&scaleListIter, "Scale");
        s32 chordNum = chordListIter.getSize();
        s32 scaleNum = scaleListIter.getSize();

        BgmChordInfo* info = new BgmChordInfo;
        info->chordNum = chordNum;
        info->chord = new s32[chordNum];
        info->scaleNum = scaleNum;
        info->scale = new s32[scaleNum];
        mMusicalInfo->chordInfoList[i] = info;

        for (s32 j = 0; j < chordNum; j++) {
            chordListIter.tryGetIntByIndex(&mMusicalInfo->chordInfoList[i]->chord[j], j);
        }

        for (s32 j = 0; j < scaleNum; j++) {
            scaleListIter.tryGetIntByIndex(&mMusicalInfo->chordInfoList[i]->scale[j], j);
        }

        chordIter.tryGetFloatByKey(&mMusicalInfo->chordInfoList[i]->beat, "Beat");
        chordIter.tryGetIntByKey(&mMusicalInfo->chordInfoList[i]->root, "Root");
    }

    if (mMusicalInfo->chordInfoNum > 0) {
        mChordInfoCurrent = mMusicalInfo->chordInfoList[0];
    }
}
}  // namespace al
