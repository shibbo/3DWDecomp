#include "Enemy/TuccondorGlass.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(TuccondorGlass, Wait)
NERVE_DECL(TuccondorGlass, End)
NERVES_MAKE_NOSTRUCT(TuccondorGlass, Wait, End)
}

/**
 * @brief Constructs the detachable glasses actor.
 * @param pName Actor name.
 */
TuccondorGlass::TuccondorGlass(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the glasses model and leaves it hidden until launched.
 * @param rInfo Actor initialization information.
 */
void TuccondorGlass::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TuccondorGlass", nullptr);
    al::initNerve(this, &NrvTuccondorGlassWait, 1);
    makeActorDead();
}

/**
 * @brief Launches the glasses away from the touch position.
 * @param rDirection Launch direction.
 */
void TuccondorGlass::appearBlow(const sead::Vector3f& rDirection) {
    al::LiveActor::appear();
    makeActorAppeared();
    al::setVelocitySeparateHV(this, rDirection, 3.0f, 20.0f);
    al::faceToDirection(this, -rDirection);
    al::setNerve(this, &NrvTuccondorGlassWait);
}

/** @brief Plays the launch animation and transitions to disappearance when it ends. */
void TuccondorGlass::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Blow");
    }
    updateVelocity();
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTuccondorGlassEnd);
    }
}

/** @brief Applies downward acceleration to the launched glasses. */
void TuccondorGlass::updateVelocity() {
    al::getVelocityPtr(this)->y += -1.0f;
}

/** @brief Emits the disappearance reaction and removes the glasses. */
void TuccondorGlass::exeEnd() {
    al::startHitReactionDisappear(this);
    kill();
}

/** @brief Checks whether the glasses have not yet been launched. @return Whether launch is available. */
bool TuccondorGlass::isReady() {
    return !al::isNerve(this, &NrvTuccondorGlassEnd) && al::isDead(this);
}

/** @brief Checks whether the glasses have finished their flight. @return Whether disappearance has begun. */
bool TuccondorGlass::isEnd() {
    return al::isNerve(this, &NrvTuccondorGlassEnd);
}

