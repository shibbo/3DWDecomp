#include "CourseSelect/KinokoOneUpCourseSelect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/CourseSelectUtil.hpp"
#include "Util/ItemUtil.hpp"

namespace {
NERVE_DECL(KinokoOneUpCourseSelect, Wait);
NERVES_MAKE_NOSTRUCT(KinokoOneUpCourseSelect, Wait)
}

/**
 * @brief Create a collectible 1-Up Mushroom on the course-select map.
 * @param pName Actor name.
 */
KinokoOneUpCourseSelect::KinokoOneUpCourseSelect(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Load the mushroom and restore whether it has already been collected.
 * @param rInfo Actor placement and scene initialization data.
 */
void KinokoOneUpCourseSelect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("KinokoOneUp"), "CourseSelect");
    al::initNerve(this, &NrvKinokoOneUpCourseSelectWait, 0);
    mOneUpId = rc::getCourseSelectKinokoOneUpId(this);
    if (rc::isDisappearCourseSelectKinokoOneUp(GameDataHolderAccessor(this), mOneUpId)) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

/**
 * @brief Award a life and record acquisition when a player collects this mushroom.
 * @param pMsg Sensor message to inspect.
 * @param pSender Collecting player's sensor.
 * @param pReceiver Receiving mushroom sensor; unused.
 * @return Whether an item-collection request was handled.
 */
bool KinokoOneUpCourseSelect::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                       al::HitSensor* pReceiver) {
    if (al::isMsgPlayerItemGet(pMsg)) {
        rc::acquirerItemOneUp(this, pSender);
        rc::acquirerCourseSelectKinokoOneUp(GameDataHolderAccessor(this), mOneUpId);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    return false;
}

/** @brief Appear only when the mushroom has not already been collected. */
void KinokoOneUpCourseSelect::appear() {
    if (!rc::isDisappearCourseSelectKinokoOneUp(GameDataHolderAccessor(this), mOneUpId)) {
        al::LiveActor::appear();
    }
}

/** @brief Start the idle action on entry to the waiting state. */
void KinokoOneUpCourseSelect::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }
}

/** @brief Destroy the course-select mushroom actor. */
KinokoOneUpCourseSelect::~KinokoOneUpCourseSelect() = default;
