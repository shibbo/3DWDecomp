#include "Boss/TentackHill.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(TentackHill, Appear);
NERVE_DECL(TentackHill, AppearWait);
NERVE_DECL(TentackHill, Wait);
NERVE_DECL(TentackHill, DisappearWait);
NERVE_DECL(TentackHill, Disappear);
NERVES_MAKE_NOSTRUCT(TentackHill, Appear, AppearWait, Wait, DisappearWait, Disappear)
}

/**
 * @brief Creates a Tentack hill actor.
 * @param pName Actor name.
 */
TentackHill::TentackHill(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the selected model and starts the hill hidden and dead.
 * @param rInfo Actor placement and scene initialization information.
 * @param pModelName Model archive name.
 * @param pSuffix Archive suffix.
 */
void TentackHill::initActorWithModelName(const al::ActorInitInfo& rInfo,
                                        const char* pModelName, const char* pSuffix) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString(pModelName), pSuffix);
    al::initNerve(this, &NrvTentackHillAppear, 0);
    al::hideModelIfShow(this);
    makeActorDead();
}

/** @brief Appears with a random rotation around the local Y axis. */
void TentackHill::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTentackHillAppear);
    al::rotateQuatYDirDegree(this, al::getRandomDegree());
}

/** @brief Plays the appearance animation, then holds the appeared pose. */
void TentackHill::exeAppear() {
    if (al::isFirstStep(this)) { al::startAction(this, "Appear"); }
    if (al::isActionEnd(this)) { al::setNerve(this, &NrvTentackHillAppearWait); }
}

/** @brief Starts the AppearWait animation on entry. */
void TentackHill::exeAppearWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "AppearWait"); }
}

/** @brief Starts the Wait animation on entry. */
void TentackHill::exeWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); }
}

/** @brief Starts the DisappearWait animation on entry. */
void TentackHill::exeDisappearWait() {
    if (al::isFirstStep(this)) { al::startAction(this, "DisappearWait"); }
}

/** @brief Plays the disappearance animation and kills the hill when it ends. */
void TentackHill::exeDisappear() {
    if (al::isFirstStep(this)) { al::startAction(this, "Disappear"); }
    if (al::isActionEnd(this)) { kill(); }
}

/**
 * @brief Starts waiting unless the hill is already in that state.
 * @return Whether the state changed.
 */
bool TentackHill::trySetWaitIfNotPlaying() {
    if (al::isNerve(this, &NrvTentackHillWait)) { return false; }
    al::setNerve(this, &NrvTentackHillWait);
    return true;
}

/** @brief Selects the disappearance waiting pose. */
void TentackHill::setReverse() {
    al::setNerve(this, &NrvTentackHillDisappearWait);
}

/** @brief Starts disappearing. */
void TentackHill::setDisappear() {
    al::setNerve(this, &NrvTentackHillDisappear);
}

/** @brief Destroys the hill's base actor resources. */
TentackHill::~TentackHill() = default;
