#include "NPC/RabbitStateReverse.hpp"
#include "NPC/Rabbit.hpp"
#include "NPC/RabbitRouteRider.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(RabbitStateReverse, ReverseStop)
NERVE_DECL(RabbitStateReverse, NoStop)
NERVE_DECL(RabbitStateReverse, Reverse)
NERVES_MAKE_NOSTRUCT(RabbitStateReverse, ReverseStop, NoStop, Reverse)
} // namespace
/** @brief Creates the reversal state. @param pRabbit Rabbit controlled by this state. */
RabbitStateReverse::RabbitStateReverse(Rabbit* pRabbit)
    : al::HostStateBase<Rabbit>("\u30a6\u30b5\u30ae\u53cd\u8ee2\u30b9\u30c6\u30fc\u30c8", pRabbit) {
}
/** @brief Releases the state's nerve executor. */
RabbitStateReverse::~RabbitStateReverse() = default;
/** @brief Initializes the stopping phase. */
void RabbitStateReverse::init() { initNerve(&NrvRabbitStateReverseReverseStop, 0); }
/** @brief Starts reversing, respecting the no-stop marker. */
void RabbitStateReverse::appear() {
    mIsDead = false;
    if (al::isNerve(this, &NrvRabbitStateReverseNoStop)) {
        al::setNerve(this, &NrvRabbitStateReverseReverse);
    } else {
        al::setNerve(this, &NrvRabbitStateReverseReverseStop);
    }
}
/** @brief Skips the stopping phase on the next appearance. */
void RabbitStateReverse::setNoStop() { al::setNerve(this, &NrvRabbitStateReverseNoStop); }
/** @brief Plays the stop animation before reversing. */
void RabbitStateReverse::exeReverseStop() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "Stop");
    }
    if (al::isStep(this, 15)) {
        al::setNerve(this, &NrvRabbitStateReverseReverse);
    }
}
/** @brief Turns toward the reversed route direction and ends after 24 frames. */
void RabbitStateReverse::exeReverse() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "Reverse");
    }
    al::turnDirectionDegree(getHost(), al::getFrontPtr(getHost()),
                            getHost()->getRouteRider()->getDirection(), 10.0f);
    if (al::isStep(this, 24)) {
        kill();
    }
}
