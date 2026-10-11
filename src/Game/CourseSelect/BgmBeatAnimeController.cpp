#include "CourseSelect/BgmBeatAnimeController.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"

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

/** Number of animation frames in one beat. */
constexpr f32 cFramesPerBeat = 30.0f;

/**
 * @brief Checks whether a skeletal animation can be driven.
 * @param pActor Animated actor.
 * @param isDisabled Whether driving the skeletal animation is disabled.
 * @return Whether the animation exists and is not disabled.
 */
inline bool isEnableSklAnime(const al::LiveActor* pActor, bool isDisabled) {
    return al::isSklAnimExist(pActor) && !isDisabled;
}

/**
 * @brief Checks whether a texture-pattern animation can be driven.
 * @param pActor Animated actor.
 * @param isDisabled Whether driving the texture-pattern animation is disabled.
 * @return Whether the animation exists and is not disabled.
 */
inline bool isEnableMtsAnime(const al::LiveActor* pActor, bool isDisabled) {
    return al::isMtsAnimExist(pActor) && !isDisabled;
}

/**
 * @brief Checks whether a color animation can be driven.
 * @param pActor Animated actor.
 * @param isDisabled Whether driving the color animation is disabled.
 * @return Whether the animation exists and is not disabled.
 */
inline bool isEnableMclAnime(const al::LiveActor* pActor, bool isDisabled) {
    return al::isMclAnimExist(pActor) && !isDisabled;
}
}  // namespace

/**
 * @brief Checks whether the current BGM can drive rhythm animations (special BGMs such as
 * hurry-up, giant and invincible are excluded).
 * @return Whether beat sync is possible.
 */
inline bool BgmBeatAnimeController::isEnableSyncBgm() const {
    if (!al::isEnableRhythmAnim(mActor, nullptr)) {
        return false;
    }

    const char* pBgmName = al::getCurPlayingBgmPlayName(mActor);
    if (pBgmName == nullptr) {
        return false;
    }

    return !al::isEqualString(pBgmName, "HurryUp") && !al::isEqualString(pBgmName, "Giant") &&
           !al::isEqualString(pBgmName, "Invincible");
}

/**
 * @brief Checks whether the BGM can drive rhythm animations and the actor plays an action.
 * @return Whether beat sync is possible.
 */
inline bool BgmBeatAnimeController::isEnableSyncAnime() const {
    return isEnableSyncBgm() && al::isExistAction(mActor);
}

/** @brief Advances every animation by one beat, wrapping around at its beat count. */
inline void BgmBeatAnimeController::advanceBeat() {
    mSklBeat = mSklBeat + 1 >= mSklBeatNum ? 0 : mSklBeat + 1;
    mMtsBeat = mMtsBeat + 1 >= mMtsBeatNum ? 0 : mMtsBeat + 1;
    mMclBeat = mMclBeat + 1 >= mMclBeatNum ? 0 : mMclBeat + 1;
}

/** @brief Recomputes the current beat of every animation from its current frame. */
inline void BgmBeatAnimeController::updateBeatFromAnime() {
    mSklBeat = al::isSklAnimExist(mActor) ?
                   static_cast<s32>(al::getSklAnimFrame(mActor, 0) / cFramesPerBeat) :
                   0;
    mMtsBeat = al::isMtsAnimExist(mActor) ?
                   static_cast<s32>(al::getMtsAnimFrame(mActor) / cFramesPerBeat) :
                   0;
    mMclBeat = al::isMclAnimExist(mActor) ?
                   static_cast<s32>(al::getMclAnimFrame(mActor) / cFramesPerBeat) :
                   0;
}

/**
 * @brief Creates the controller in the wait state.
 * @param pActor Actor whose animations are driven.
 */
BgmBeatAnimeController::BgmBeatAnimeController(al::LiveActor* pActor)
    : al::NerveExecutor("BGM ビート同期アニメコントローラ"), mActor(pActor) {
    initNerve(&NrvBgmBeatAnimeControllerWait, 0);
}

/**
 * @brief Starts the controller.
 * @param pActionName Action synchronized with the beat, or nullptr to only drive the
 * light (animation) beat.
 * @param isSklAnimeDisabled Whether the skeletal animation frame is left alone.
 * @param isMtsAnimeDisabled Whether the texture-pattern animation frame is left alone.
 * @param isMclAnimeDisabled Whether the color animation frame is left alone.
 */
void BgmBeatAnimeController::init(const char* pActionName, bool isSklAnimeDisabled,
                                  bool isMtsAnimeDisabled, bool isMclAnimeDisabled) {
    mIsSklAnimeDisabled = isSklAnimeDisabled;
    mIsTriggerBeat = false;
    mActionName = pActionName;
    mIsMtsAnimeDisabled = isMtsAnimeDisabled;
    mIsMclAnimeDisabled = isMclAnimeDisabled;

    if (pActionName != nullptr) {
        al::startAction(mActor, pActionName);
        initAnimeInfo();
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatBySyncAnime);
    } else {
        initAnimeInfo();
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLightStart);
    }
}

/** @brief Reads the beat count and current beat of each existing animation. */
void BgmBeatAnimeController::initAnimeInfo() {
    if (al::isSklAnimExist(mActor)) {
        al::modf(al::getSklAnimFrameMax(mActor, 0) + 1.0f, cFramesPerBeat);
        mSklBeatNum =
            static_cast<s32>((al::getSklAnimFrameMax(mActor, 0) + 1.0f) / cFramesPerBeat);
        mSklBeat = static_cast<s32>(al::getSklAnimFrame(mActor, 0) / cFramesPerBeat);
    }

    if (al::isMtsAnimExist(mActor)) {
        al::modf(al::getMtsAnimFrameMax(mActor) + 1.0f, cFramesPerBeat);
        mMtsBeatNum = static_cast<s32>((al::getMtsAnimFrameMax(mActor) + 1.0f) / cFramesPerBeat);
        mMtsBeat = static_cast<s32>(al::getMtsAnimFrame(mActor) / cFramesPerBeat);
    }

    if (al::isMclAnimExist(mActor)) {
        al::modf(al::getMclAnimFrameMax(mActor) + 1.0f, cFramesPerBeat);
        mMclBeatNum = static_cast<s32>((al::getMclAnimFrameMax(mActor) + 1.0f) / cFramesPerBeat);
        mMclBeat = static_cast<s32>(al::getMclAnimFrame(mActor) / cFramesPerBeat);
    }
}

/** @brief Restarts waiting for a beat. */
void BgmBeatAnimeController::reset() {
    if (mActionName != nullptr) {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatBySyncAnime);
    } else {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLightStart);
    }
}

/** @brief Runs the current state. */
void BgmBeatAnimeController::update() {
    updateNerve();
}

/**
 * @brief Checks whether the action is catching up with the beat.
 * @return Whether the junction animation is playing.
 */
bool BgmBeatAnimeController::isPlayingJunctionAnime() const {
    return al::isNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatByWaitAnime);
}

/** @brief Does nothing. */
void BgmBeatAnimeController::exeWait() {
    mIsTriggerBeat = false;
}

/**
 * @brief Waits for a beat, then either joins the beat directly or plays the rest of the
 * current action faster to catch up with it.
 */
void BgmBeatAnimeController::exeWaitSyncBeatBySyncAnime() {
    mIsTriggerBeat = false;

    if (!isEnableSyncAnime() || !al::isTriggerBeat(mActor, 1)) {
        return;
    }

    f32 frame = al::getActionFrame(mActor);
    f32 frameMax = al::getActionFrameMax(mActor, al::getActionName(mActor));
    f32 restFrame;
    if (frame > 0.0f && (restFrame = frameMax - (frame + 1.0f)) > 0.0f) {
        f32 restBeat = restFrame / 6.0f;
        s32 restFrameRem = static_cast<s32>(restFrame) % 6;
        s32 restBeatInt = static_cast<s32>(restBeat);
        f32 beat = frame / cFramesPerBeat;
        mJunctionBeatCount =
            restBeatInt + (restBeat != static_cast<f32>(restBeatInt) && restBeat >= 0.0f);
        mBeatRate = beat - static_cast<s32>(beat);

        f32 beatPerFrame = al::getBeatPerFrame(mActor);
        f32 nextScale = (restFrameRem + cFramesPerBeat) / cFramesPerBeat;
        mBeatStep = beatPerFrame * 1.2f;
        mFrameRateScale = 1.2f;
        mNextBeatStep = nextScale * beatPerFrame;
        mNextFrameRateScale = nextScale;
        updateBeatFromAnime();
        setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mActor) * 1.2f);
        setAnimeFrame();
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatByWaitAnime);
        mIsTriggerBeat = true;
    } else {
        advanceBeat();
        mBeatRate = al::getBeatRate(mActor);
        setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mActor));
        setAnimeFrame();
        al::setNerve(this, &NrvBgmBeatAnimeControllerSyncBeat);
    }
}

/**
 * @brief Sets the action frame rate unless the skeletal animation is disabled or missing.
 * @param frameRate New action frame rate.
 */
void BgmBeatAnimeController::setActionFrameRateWithSklAnimeCheck(f32 frameRate) {
    if (isEnableSklAnime(mActor, mIsSklAnimeDisabled) && al::isSklAnimExist(mActor)) {
        al::setActionFrameRate(mActor, frameRate);
    }
}

/** @brief Moves each enabled animation to the frame matching its beat and the beat rate. */
void BgmBeatAnimeController::setAnimeFrame() {
    if (isEnableSklAnime(mActor, mIsSklAnimeDisabled)) {
        al::setSklAnimFrame(mActor, (mBeatRate + mSklBeat) * cFramesPerBeat, 0);
    }

    if (isEnableMtsAnime(mActor, mIsMtsAnimeDisabled)) {
        al::setMtsAnimFrame(mActor, (mBeatRate + mMtsBeat) * cFramesPerBeat);
    }

    if (isEnableMclAnime(mActor, mIsMclAnimeDisabled)) {
        al::setMclAnimFrame(mActor, (mBeatRate + mMclBeat) * cFramesPerBeat);
    }
}

/** @brief Follows the BGM beat; restarts waiting when the BGM can no longer be followed. */
void BgmBeatAnimeController::exeSyncBeat() {
    mIsTriggerBeat = false;

    if (!isEnableSyncAnime()) {
        reset();
        return;
    }

    if (al::isTriggerBeat(mActor, 1)) {
        advanceBeat();
        mIsTriggerBeat = true;
    }

    mBeatRate = al::getBeatRate(mActor);
    setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mActor));
    setAnimeFrame();
}

/** @brief Plays the end of the current action at a modified speed until it meets the beat. */
void BgmBeatAnimeController::exeWaitSyncBeatByWaitAnime() {
    mIsTriggerBeat = false;

    if (!isEnableSyncAnime()) {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatBySyncAnime);
        return;
    }

    mBeatRate = mBeatStep + mBeatRate;
    if (mBeatRate >= 1.0f) {
        advanceBeat();
        mBeatRate -= 1.0f;
    }

    if (al::isTriggerBeat(mActor, 1)) {
        mIsTriggerBeat = true;
        if (mJunctionBeatCount != 0) {
            mJunctionBeatCount--;
            if (mJunctionBeatCount == 0) {
                mBeatStep = mNextBeatStep;
                mFrameRateScale = mNextFrameRateScale;
            }
        } else {
            updateBeatFromAnime();
            f32 bgmBeatRate = al::getBeatRate(mActor);
            f32 beatRate = mBeatRate;
            f32 minBeatRate = 1.0f - al::getBeatPerFrame(mActor) * 4.0f;
            f32 maxBgmBeatRate = al::getBeatPerFrame(mActor) * 4.0f;
            if (beatRate >= minBeatRate && bgmBeatRate <= maxBgmBeatRate) {
                advanceBeat();
            }

            mBeatRate = al::getBeatRate(mActor);
            al::setNerve(this, &NrvBgmBeatAnimeControllerSyncBeat);
        }
    }

    setActionFrameRateWithSklAnimeCheck(mFrameRateScale * al::getFrameRate(mActor));
    setAnimeFrame();
}

/** @brief Waits for a beat to start driving the animations without an action. */
void BgmBeatAnimeController::exeWaitSyncBeatLightStart() {
    mIsTriggerBeat = false;

    if (isEnableSyncBgm() && al::isTriggerBeat(mActor, 1)) {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLight);
        mIsTriggerBeat = true;
    }
}

/** @brief Plays the animations from their current frame until the next beat. */
void BgmBeatAnimeController::exeWaitSyncBeatLight() {
    mIsTriggerBeat = false;

    if (!isEnableSyncAnime()) {
        al::setNerve(this, &NrvBgmBeatAnimeControllerWaitSyncBeatLightStart);
        return;
    }

    if (al::isFirstStep(this)) {
        f32 beat = al::getActionFrame(mActor) / cFramesPerBeat;
        mBeatRate = beat - static_cast<s32>(beat);
        mFrameRateScale = 1.0f - mBeatRate;
        mBeatStep = al::getBeatPerFrame(mActor) * mFrameRateScale;
        updateBeatFromAnime();
    }

    mBeatRate = mBeatStep + mBeatRate;
    setActionFrameRateWithSklAnimeCheck(mFrameRateScale * al::getFrameRate(mActor));
    setAnimeFrame();

    if (al::isTriggerBeat(mActor, 1)) {
        advanceBeat();
        al::setNerve(this, &NrvBgmBeatAnimeControllerSyncBeatLight);
        mIsTriggerBeat = true;
    }
}

/** @brief Follows the BGM beat without an action; restarts waiting when it can't. */
void BgmBeatAnimeController::exeSyncBeatLight() {
    mIsTriggerBeat = false;

    if (!isEnableSyncAnime()) {
        reset();
        return;
    }

    if (al::isTriggerBeat(mActor, 1)) {
        advanceBeat();
        mIsTriggerBeat = true;
    }

    mBeatRate = al::getBeatRate(mActor);
    setActionFrameRateWithSklAnimeCheck(al::getFrameRate(mActor));
    setAnimeFrame();
}
