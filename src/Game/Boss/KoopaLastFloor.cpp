#include "Boss/KoopaLastFloor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(KoopaLastFloor, Wait);
NERVE_DECL(KoopaLastFloor, Sign);
NERVES_MAKE_STRUCT(KoopaLastFloor, Wait, Sign)
}

/**
 * @brief Creates a final-boss floor with no break model assigned yet.
 * @param pName Actor name.
 */
KoopaLastFloor::KoopaLastFloor(const char* pName) : al::LiveActor(pName) {
}

/**
 * @brief Initializes the floor, optional break model, and warning stage switch.
 * @param rInfo Actor placement and scene initialization information.
 */
void KoopaLastFloor::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvKoopaLastFloor.Wait, 0);
    mBreakModel = al::tryGetSubActor(this, "壊れモデル");
    al::listenStageSwitchOnStart(this, al::Functor(this, &KoopaLastFloor::start));
    makeActorAppeared();
}

/** @brief Starts the warning when the floor is still waiting. */
void KoopaLastFloor::start() {
    if (al::isNerve(this, &NrvKoopaLastFloor.Wait)) {
        al::setNerve(this, &NrvKoopaLastFloor.Sign);
    }
}

/** @brief Plays the break reaction, appears the break model, and removes the floor. */
void KoopaLastFloor::kill() {
    al::startHitReactionBreak(this);
    if (mBreakModel) {
        mBreakModel->appear();
    }
    al::LiveActor::kill();
}

/**
 * @brief Breaks the floor in response to either explosion message.
 * @param pMsg Incoming sensor message.
 * @param pSelf Receiving sensor; unused.
 * @param pOther Sending sensor; unused.
 * @return True if an explosion message was handled.
 */
bool KoopaLastFloor::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                               al::HitSensor* pOther) {
    if (al::isMsgExplosion(pMsg) || al::isMsgExplosionCollide(pMsg)) {
        kill();
        return true;
    }
    return false;
}

/** @brief Waits for the warning switch or an explosion. */
void KoopaLastFloor::exeWait() {
}

/** @brief Starts the break-warning action on entry. */
void KoopaLastFloor::exeSign() {
    if (al::isFirstStep(this)) {
        al::tryStartAction(this, "BreakSign");
    }
}

/** @brief Destroys the floor's base actor resources. */
KoopaLastFloor::~KoopaLastFloor() = default;
