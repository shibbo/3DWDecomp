#include "Layout/TimerClockNumber.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"

namespace {
NERVE_DECL(TimerClockNumber, Appear);
NERVES_MAKE_NOSTRUCT(TimerClockNumber, Appear)
}  // namespace

/**
 * @brief Creates the floating number shown when a clock adds time.
 * @param rInfo Layout initialization context and scene camera.
 * @param time Time increase displayed after the plus sign.
 */
TimerClockNumber::TimerClockNumber(const al::LayoutInitInfo& rInfo, s32 time)
    : al::LayoutActor("＋？時計レイアウト"), mSceneCameraInfo(rInfo.getSceneCameraInfo()) {
    al::initLayoutActor(this, rInfo, "PopScoreNumber", nullptr);
    initNerve(&NrvTimerClockNumberAppear, 0);
    al::setPaneString(this, "TxtScore",
                      sead::WFormatFixedSafeString<8>(u"+%d", time).cstr(), 0, -1);
    al::setLocalScale(this, 2.0f);
}

/**
 * @brief Places and shows the number relative to a world position.
 * @param rWorldPos World position, offset downward by 100 units before projection.
 */
void TimerClockNumber::appearWithWorldPos(const sead::Vector3f& rWorldPos) {
    mWorldPos.set(rWorldPos);
    mWorldPos.y -= 100.0f;
    updatePosition();
    appear();
}

/** @brief Projects the stored world position into the layout. */
void TimerClockNumber::updatePosition() {
    sead::Vector2f layoutPos;
    al::calcLayoutPosFromWorldPos(&layoutPos, this, mWorldPos, 0);
    al::setLocalTrans(this, layoutPos);
}

/** @brief Plays the time-up animation, follows the camera, and hides when finished. */
void TimerClockNumber::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        al::startAction(this, "TimeUp", "PlayerColor");
    }
    updatePosition();
    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}

/**
 * @brief Gets the camera supplied at initialization.
 * @return Scene camera used to project the floating number.
 */
al::SceneCameraInfo* TimerClockNumber::getSceneCameraInfo() const {
    return mSceneCameraInfo;
}
