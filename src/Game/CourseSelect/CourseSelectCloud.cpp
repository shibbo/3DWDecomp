#include "CourseSelect/CourseSelectCloud.hpp"
#include "CourseSelect/BgmBeatAnimeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(CourseSelectCloud, Wait);
NERVES_MAKE_NOSTRUCT(CourseSelectCloud, Wait)
}

/**
 * @brief Create a course-select cloud with music-synchronized animation.
 * @param pName Actor name.
 */
CourseSelectCloud::CourseSelectCloud(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the cloud model, audio, and beat animation controller.
 * @param rInfo Actor placement and scene initialization data.
 */
void CourseSelectCloud::init(const al::ActorInitInfo& rInfo) {
    al::initActorChangeModel(this, rInfo);
    al::initNerve(this, &NrvCourseSelectCloudWait, 0);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    mpBeatAnimeController = new BgmBeatAnimeController(this);
    makeActorAppeared();
}

/** @brief Start the idle action once and update its beat synchronization each frame. */
void CourseSelectCloud::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        mpBeatAnimeController->init(nullptr, false, false, false);
    }
    mpBeatAnimeController->update();
}

/** @brief Resynchronize animation when the cloud returns from clipping. */
void CourseSelectCloud::endClipped() {
    al::LiveActor::endClipped();
    mpBeatAnimeController->reset();
}

/** @brief Destroy the course-select cloud actor. */
CourseSelectCloud::~CourseSelectCloud() = default;
