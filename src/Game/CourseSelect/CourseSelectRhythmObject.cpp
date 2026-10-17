#include "CourseSelect/CourseSelectRhythmObject.hpp"
#include "CourseSelect/BgmBeatAnimeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Thread/Functor.hpp"

/**
 * @brief Create a map object whose animation follows the music beat.
 * @param pName Actor name.
 */
CourseSelectRhythmObject::CourseSelectRhythmObject(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Configure beat synchronization, color animation, and appearance switches.
 * @param rInfo Actor placement, animation options, and stage-switch connections.
 */
void CourseSelectRhythmObject::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    if (getAudioKeeper() == nullptr) {
        al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    }
    bool isIgnoreSkl = false;
    bool isIgnoreMts = false;
    bool isIgnoreMcl = false;
    al::tryGetArg(&isIgnoreSkl, rInfo, "IsNoSyncBgmForSklAnime");
    al::tryGetArg(&isIgnoreMts, rInfo, "IsNoSyncBgmForMtsAnime");
    al::tryGetArg(&isIgnoreMcl, rInfo, "IsNoSyncBgmForMclAnime");
    mpBeatAnimeController = new BgmBeatAnimeController(this);
    mpBeatAnimeController->init(nullptr, isIgnoreSkl, isIgnoreMts, isIgnoreMcl);

    int colorFrame = 0;
    if (al::tryGetArg(&colorFrame, rInfo, "ColorAnimFrame") && colorFrame > 0) {
        al::startMclAnimAndSetFrameAndStop(this, "Color", static_cast<float>(colorFrame));
    }
    if (al::listenStageSwitchOnOffAppear(this, al::Functor(this, &CourseSelectRhythmObject::appearBySwitch),
                                       al::Functor(this, &CourseSelectRhythmObject::killBySwitch))) {
        mIsActive = false;
        al::hideModel(this);
        makeActorDead();
    } else {
        if (getModelKeeper() != nullptr && !al::isSklAnimExist(this)) {
            al::disableUpdateModelBounding(this);
        }
        makeActorAppeared();
    }
}

/** @brief Enable the actor and reveal its model when the appearance switch turns on. */
void CourseSelectRhythmObject::appearBySwitch() {
    mIsActive = true;
    appear();
    al::showModelIfHide(this);
}

/** @brief Disable the actor and hide its model when the appearance switch turns off. */
void CourseSelectRhythmObject::killBySwitch() {
    mIsActive = false;
    kill();
    al::hideModelIfShow(this);
}

/** @brief Appear and start the model's action only while enabled by the stage switch. */
void CourseSelectRhythmObject::makeActorAppeared() {
    if (mIsActive) {
        al::LiveActor::makeActorAppeared();
        al::startAction(this, al::getModelName(this));
    }
}

/** @brief Update music synchronization while the actor is active. */
void CourseSelectRhythmObject::control() {
    if (mIsActive) {
        mpBeatAnimeController->update();
    }
}

/** @brief Restart beat synchronization after the actor returns from clipping. */
void CourseSelectRhythmObject::endClipped() {
    al::LiveActor::endClipped();
    mpBeatAnimeController->reset();
}

/** @brief Destroy the course-select rhythm actor. */
CourseSelectRhythmObject::~CourseSelectRhythmObject() = default;
