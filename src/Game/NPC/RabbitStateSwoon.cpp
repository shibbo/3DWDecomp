#include "NPC/RabbitStateSwoon.hpp"
#include "NPC/Rabbit.hpp"
#include "NPC/RabbitRouteRider.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(RabbitStateSwoon, SwoonStart)
NERVE_DECL(RabbitStateSwoon, SwoonEnd)
NERVE_DECL(RabbitStateSwoon, Swoon)
NERVE_DECL(RabbitStateSwoon, Turn)
const struct {
    RabbitStateSwoonNrvSwoonStart Start;
    RabbitStateSwoonNrvSwoonEnd End;
} NrvRabbitStateSwoonEnds;
NERVES_MAKE_NOSTRUCT(RabbitStateSwoon, Swoon, Turn)
} // namespace
/** @brief Creates the swoon state. @param pRabbit Rabbit controlled by this state. */
RabbitStateSwoon::RabbitStateSwoon(Rabbit* pRabbit)
    : al::HostStateBase<Rabbit>("\u30a6\u30b5\u30ae\u6c17\u7d76\u30b9\u30c6\u30fc\u30c8", pRabbit),
      mStartAction("SwoonStart"), mSkipSwoon(false) {}
/** @brief Releases the state's nerve executor. */
RabbitStateSwoon::~RabbitStateSwoon() = default;
/** @brief Initializes the swoon entry phase. */
void RabbitStateSwoon::init() { initNerve(&NrvRabbitStateSwoonEnds.Start, 0); }
/** @brief Starts swooning or skips directly to its ending animation. */
void RabbitStateSwoon::appear() {
    mIsDead = false;
    al::setNerve(this, mSkipSwoon ? static_cast<const al::Nerve*>(&NrvRabbitStateSwoonEnds.End)
                                  : &NrvRabbitStateSwoonEnds.Start);
}
/** @brief Ends the state and resets the skip flag. */
void RabbitStateSwoon::kill() {
    mSkipSwoon = false;
    mIsDead = true;
}
/** @brief Plays the entry animation before the swoon loop. */
void RabbitStateSwoon::exeSwoonStart() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), mStartAction);
    }
    if (al::isActionEnd(getHost())) {
        al::setNerve(this, &NrvRabbitStateSwoonSwoon);
    }
}
/** @brief Loops the swoon animation for 120 frames. */
void RabbitStateSwoon::exeSwoon() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "SwoonLoop");
    }
    if (al::isStep(this, 120)) {
        al::setNerve(this, &NrvRabbitStateSwoonEnds.End);
    }
}
/** @brief Ends the swoon and optionally turns away from a nearby player. */
void RabbitStateSwoon::exeSwoonEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "SwoonEnd");
    }
    if (al::isActionEnd(getHost())) {
        const al::LiveActor* pPlayer =
            rc::tryFindNearestActivePlayerActorInSphere(getHost(), getHost()->getMoveEndDistance());
        if (pPlayer && getHost()->canStartReverse(pPlayer)) {
            al::setNerve(this, &NrvRabbitStateSwoonTurn);
        } else {
            kill();
        }
    }
}
/** @brief Reverses the route direction and turns until the animation ends. */
void RabbitStateSwoon::exeTurn() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "Reverse");
        getHost()->getRouteRider()->reverse();
    }
    al::turnToDirection(getHost(), getHost()->getRouteRider()->getDirection(), 8.0f);
    if (al::isActionEnd(getHost())) {
        kill();
    }
}
