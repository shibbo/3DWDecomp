#include "CourseSelect/BgmBeatAnimeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
NERVE_DECL(BgmBeatAnimeController, Wait);
NERVE_DECL(BgmBeatAnimeController, WaitSyncBeatBySyncAnime);
NERVE_DECL(BgmBeatAnimeController, WaitSyncBeatLightStart);
NERVE_DECL(BgmBeatAnimeController, WaitSyncBeatByWaitAnime);
NERVE_DECL(BgmBeatAnimeController, SyncBeat);
NERVE_DECL(BgmBeatAnimeController, WaitSyncBeatLight);
NERVE_DECL(BgmBeatAnimeController, SyncBeatLight);
NERVES_MAKE_NOSTRUCT(BgmBeatAnimeController, Wait, WaitSyncBeatBySyncAnime, WaitSyncBeatLightStart,
                     WaitSyncBeatByWaitAnime, SyncBeat, WaitSyncBeatLight, SyncBeatLight)
}

/**
 * @brief Convert an animation beat index to a frame position.
 * @param beatIndex Animation's whole-beat index.
 * @return Position including the current fractional beat.
 */
inline float BgmBeatAnimeController::calcAnimeFrame(int beatIndex) const {
    return (mBeatRate + beatIndex) * 30.0f;
}

/** @brief Advance and wrap each animation's beat index. */
inline void BgmBeatAnimeController::advanceBeatIndices() {
    if (++mSklBeatIndex >= mSklBeatCount)
        mSklBeatIndex = 0;
    if (++mMtsBeatIndex >= mMtsBeatCount)
        mMtsBeatIndex = 0;
    if (++mMclBeatIndex >= mMclBeatCount)
        mMclBeatIndex = 0;
}

/** @brief Recover the current beat index from every available animation. */
inline void BgmBeatAnimeController::readBeatIndices() {
    mSklBeatIndex = al::isSklAnimExist(mpActor) ? static_cast<int>(al::getSklAnimFrame(mpActor, 0) / 30.0f) : 0;
    mMtsBeatIndex = al::isMtsAnimExist(mpActor) ? static_cast<int>(al::getMtsAnimFrame(mpActor) / 30.0f) : 0;
    mMclBeatIndex = al::isMclAnimExist(mpActor) ? static_cast<int>(al::getMclAnimFrame(mpActor) / 30.0f) : 0;
}

/** @brief Check music eligibility for animation synchronization. @return Whether beat synchronization is available. */
inline bool BgmBeatAnimeController::isSyncBgm() const {
    if (!al::isEnableRhythmAnim(mpActor, nullptr))
        return false;
    const char* pName = al::getCurPlayingBgmPlayName(mpActor);
    return pName && !al::isEqualString(pName, "HurryUp") && !al::isEqualString(pName, "Giant") &&
           !al::isEqualString(pName, "Invincible");
}

/**
 * @brief Check skeletal animation participation.
 * @param pActor Actor to inspect.
 * @param isIgnore Whether this channel is excluded.
 * @return Whether an enabled skeletal animation exists.
 */
inline bool BgmBeatAnimeController::isSklEnabled(const al::LiveActor* pActor, bool isIgnore) {
    return al::isSklAnimExist(pActor) && !isIgnore;
}

/**
 * @brief Check texture animation participation.
 * @param pActor Actor to inspect.
 * @param isIgnore Whether this channel is excluded.
 * @return Whether an enabled texture animation exists.
 */
inline bool BgmBeatAnimeController::isMtsEnabled(const al::LiveActor* pActor, bool isIgnore) {
    return al::isMtsAnimExist(pActor) && !isIgnore;
}

/**
 * @brief Check color animation participation.
 * @param pActor Actor to inspect.
 * @param isIgnore Whether this channel is excluded.
 * @return Whether an enabled color animation exists.
 */
inline bool BgmBeatAnimeController::isMclEnabled(const al::LiveActor* pActor, bool isIgnore) {
    return al::isMclAnimExist(pActor) && !isIgnore;
}

/** @brief Create an actor's music-synchronized animation controller. @param pActor Actor to animate. */
BgmBeatAnimeController::BgmBeatAnimeController(al::LiveActor* pActor)
    : al::NerveExecutor("BGM ビート同期アニメコントローラ"), mpActor(pActor) {
    initNerve(&NrvBgmBeatAnimeControllerWait, 0);
}

/**
 * @brief Set up the animation channels and choose the synchronization transition.
 * @param pActionName Action to start, or null to synchronize the existing action gently.
 * @param isIgnoreSkl Whether to leave skeletal animation unchanged.
 * @param isIgnoreMts Whether to leave texture animation unchanged.
 * @param isIgnoreMcl Whether to leave color animation unchanged.
 * @note Non-matching: initialization assignment scheduling differs from the original.
 */
void BgmBeatAnimeController::init(const char* pActionName, bool isIgnoreSkl, bool isIgnoreMts, bool isIgnoreMcl) {
    mIsIgnoreSkl = isIgnoreSkl;
    mIsIgnoreMts = isIgnoreMts;
    mIsIgnoreMcl = isIgnoreMcl;
    mIsBeatTriggered = false;
    mpActionName = pActionName;
    if (pActionName) {
        al::startAction(mpActor, mpActionName);
        initAnimeInfo();
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatBySyncAnime);
    } else {
        initAnimeInfo();
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLightStart);
    }
}

/** @brief Read animation lengths and frame positions as counts of 30-frame beats. */
void BgmBeatAnimeController::initAnimeInfo() {
    if (al::isSklAnimExist(mpActor)) {
        al::modf(al::getSklAnimFrameMax(mpActor, 0) + 1.0f, 30.0f);
        mSklBeatCount = static_cast<int>((al::getSklAnimFrameMax(mpActor, 0) + 1.0f) / 30.0f);
        mSklBeatIndex = static_cast<int>(al::getSklAnimFrame(mpActor, 0) / 30.0f);
    }
    if (al::isMtsAnimExist(mpActor)) {
        al::modf(al::getMtsAnimFrameMax(mpActor) + 1.0f, 30.0f);
        mMtsBeatCount = static_cast<int>((al::getMtsAnimFrameMax(mpActor) + 1.0f) / 30.0f);
        mMtsBeatIndex = static_cast<int>(al::getMtsAnimFrame(mpActor) / 30.0f);
    }
    if (al::isMclAnimExist(mpActor)) {
        al::modf(al::getMclAnimFrameMax(mpActor) + 1.0f, 30.0f);
        mMclBeatCount = static_cast<int>((al::getMclAnimFrameMax(mpActor) + 1.0f) / 30.0f);
        mMclBeatIndex = static_cast<int>(al::getMclAnimFrame(mpActor) / 30.0f);
    }
}

/** @brief Restart synchronization using the configured transition mode. */
void BgmBeatAnimeController::reset() {
    if (mpActionName)
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatBySyncAnime);
    else
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLightStart);
}

/** @brief Advance the synchronization state. */
void BgmBeatAnimeController::update() { updateNerve(); }

/** @brief Check whether the controller is aligning animation timing. @return Whether the junction state is active. */
bool BgmBeatAnimeController::isPlayingJunctionAnime() const {
    return al::isNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatByWaitAnime);
}

/** @brief Clear the beat event while idle. */
void BgmBeatAnimeController::exeWait() { mIsBeatTriggered = false; }

/** @brief Wait for a music beat, then align or begin catching up the animation. */
void BgmBeatAnimeController::exeWaitSyncBeatBySyncAnime() {
    mIsBeatTriggered = false;
    if (!isSyncBgm() || !al::isExistAction(mpActor) || !al::isTriggerBeat(mpActor, 1))
        return;
    float frame = al::getActionFrame(mpActor);
    float frameMax = al::getActionFrameMax(mpActor, al::getActionName(mpActor));
    if (!(frame <= 0.0f || frameMax - (frame + 1.0f) <= 0.0f)) {
        float remaining = frameMax - (frame + 1.0f);
        mJunctionStepCount = sead::Mathf::ceil(remaining / 6.0f);
        int remainder = static_cast<int>(remaining) % 6;
        float beat = frame / 30.0f;
        mBeatRate = beat - static_cast<int>(beat);
        float beatPerFrame = al::getBeatPerFrame(mpActor);
        mBeatAdvance = beatPerFrame * 1.2f;
        mFrameRateScale = 1.2f;
        mFinalFrameRateScale = (30.0f + remainder) / 30.0f;
        mFinalBeatAdvance = mFinalFrameRateScale * beatPerFrame;
        readBeatIndices();
        setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mpActor) * 1.2f);
        setAnimeFrame();
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatByWaitAnime);
        mIsBeatTriggered = true;
    } else {
        advanceBeatIndices();
        mBeatRate = al::getBeatRate(mpActor);
        setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mpActor));
        setAnimeFrame();
        al::setNerve(this, &NrvBgmBeatAnimeControllerSyncBeat);
    }
}

/** @brief Set action speed when skeletal animation participates. @param rate Desired animation frame rate. */
void BgmBeatAnimeController::setActionFrameRateWithSklAnimeCheck(float rate) {
    if (isSklEnabled(mpActor, mIsIgnoreSkl) && al::isSklAnimExist(mpActor))
        al::setActionFrameRate(mpActor, rate);
}

/**
 * @brief Set participating animation frames from their beat indices and fractional beat.
 * @note Non-matching: the final color-frame calculation uses different registers.
 */
void BgmBeatAnimeController::setAnimeFrame() {
    if (isSklEnabled(mpActor, mIsIgnoreSkl))
        al::setSklAnimFrame(mpActor, calcAnimeFrame(mSklBeatIndex), 0);
    if (isMtsEnabled(mpActor, mIsIgnoreMts))
        al::setMtsAnimFrame(mpActor, calcAnimeFrame(mMtsBeatIndex));
    if (isMclEnabled(mpActor, mIsIgnoreMcl))
        al::setMclAnimFrame(mpActor, calcAnimeFrame(mMclBeatIndex));
}

/** @brief Keep the animations on the current music beat. */
void BgmBeatAnimeController::exeSyncBeat() {
    mIsBeatTriggered = false;
    if (!isSyncBgm() || !al::isExistAction(mpActor)) {
        reset();
        return;
    }
    if (al::isTriggerBeat(mpActor, 1)) {
        advanceBeatIndices();
        mIsBeatTriggered = true;
    }
    mBeatRate = al::getBeatRate(mpActor);
    setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mpActor));
    setAnimeFrame();
}

/** @brief Gradually align the animation with the music before entering full synchronization. */
void BgmBeatAnimeController::exeWaitSyncBeatByWaitAnime() {
    mIsBeatTriggered = false;
    if (!isSyncBgm() || !al::isExistAction(mpActor)) {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatBySyncAnime);
        return;
    }
    mBeatRate = mBeatAdvance + mBeatRate;
    if (mBeatRate >= 1.0f) {
        advanceBeatIndices();
        mBeatRate -= 1.0f;
    }
    if (al::isTriggerBeat(mpActor, 1)) {
        mIsBeatTriggered = true;
        if (mJunctionStepCount != 0) {
            if (--mJunctionStepCount == 0) {
                mBeatAdvance = mFinalBeatAdvance;
                mFrameRateScale = mFinalFrameRateScale;
            }
        } else {
            readBeatIndices();
            float rate = al::getBeatRate(mpActor);
            float currentRate = mBeatRate;
            float endThreshold = 1.0f - al::getBeatPerFrame(mpActor) * 4.0f;
            float startThreshold = al::getBeatPerFrame(mpActor) * 4.0f;
            if (currentRate >= endThreshold && rate <= startThreshold)
                advanceBeatIndices();
            mBeatRate = al::getBeatRate(mpActor);
            al::setNerve(this, &NrvBgmBeatAnimeControllerSyncBeat);
        }
    }
    setActionFrameRateWithSklAnimeCheck(mFrameRateScale * al::getFrameRate(mpActor));
    setAnimeFrame();
}

/** @brief Wait for a beat before starting the gentle synchronization transition. */
void BgmBeatAnimeController::exeWaitSyncBeatLightStart() {
    mIsBeatTriggered = false;
    if (isSyncBgm() && al::isTriggerBeat(mpActor, 1)) {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLight);
        mIsBeatTriggered = true;
    }
}

/** @brief Slow the current animation to reach its next boundary on a music beat. */
void BgmBeatAnimeController::exeWaitSyncBeatLight() {
    mIsBeatTriggered = false;
    if (!isSyncBgm() || !al::isExistAction(mpActor)) {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLightStart);
        return;
    }
    if (al::isFirstStep(this)) {
        float beat = al::getActionFrame(mpActor) / 30.0f;
        mBeatRate = beat - static_cast<int>(beat);
        mFrameRateScale = 1.0f - mBeatRate;
        mBeatAdvance = al::getBeatPerFrame(mpActor) * mFrameRateScale;
        readBeatIndices();
    }
    mBeatRate = mBeatAdvance + mBeatRate;
    setActionFrameRateWithSklAnimeCheck(mFrameRateScale * al::getFrameRate(mpActor));
    setAnimeFrame();
    if (al::isTriggerBeat(mpActor, 1)) {
        advanceBeatIndices();
        al::setNerve(this, &NrvBgmBeatAnimeControllerSyncBeatLight);
        mIsBeatTriggered = true;
    }
}

/** @brief Keep the gently synchronized animations on the music beat. */
void BgmBeatAnimeController::exeSyncBeatLight() {
    mIsBeatTriggered = false;
    if (!isSyncBgm() || !al::isExistAction(mpActor)) {
        reset();
        return;
    }
    if (al::isTriggerBeat(mpActor, 1)) {
        advanceBeatIndices();
        mIsBeatTriggered = true;
    }
    mBeatRate = al::getBeatRate(mpActor);
    setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mpActor));
    setAnimeFrame();
}
