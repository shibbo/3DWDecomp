#include "Enemy/BrosStateWait.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"

namespace {
NERVE_DECL(BrosStateWait, Wait)
NERVES_MAKE_NOSTRUCT(BrosStateWait, Wait)
}

/** @brief Initializes the Bros waiting state.
 * @param pHost Bros enemy waiting to activate.
 */
BrosStateWait::BrosStateWait(al::LiveActor* pHost) : al::ActorStateBase("ブロス状態[待機]", pHost) {
    initNerve(&NrvBrosStateWaitWait, 0);
}

/** @brief Waits for the start switch or a nearby player. */
void BrosStateWait::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(mHostActor, "Wait"); }
    if (al::isValidStageSwitch(mHostActor, "SwitchStart")) {
        if (al::isOnStageSwitch(mHostActor, "SwitchStart")) { kill(); }
    } else if (al::isNearPlayer(mHostActor, 1800.0f)) { kill(); }
}
