#include "Boss/KoopaLastWall.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(KoopaLastWall, Sign);
NERVE_DECL(KoopaLastWall, Wait);
NERVE_DECL(KoopaLastWall, Break);
NERVES_MAKE_NOSTRUCT(KoopaLastWall, Sign, Wait, Break)
}

/**
 * @brief Creates the final-boss wall with no break collision assigned.
 * @param pName Actor name.
 */
KoopaLastWall::KoopaLastWall(const char* pName) : al::LiveActor(pName) {}

/** @brief Starts the wall's break-warning sequence. */
void KoopaLastWall::start() {
    al::setNerve(this, &NrvKoopaLastWallSign);
}

/**
 * @brief Initializes intact and broken collision and the starting stage switch.
 * @param rInfo Actor placement and scene initialization information.
 */
void KoopaLastWall::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvKoopaLastWallWait, 0);
    mBreakCollision = al::createCollisionObj(this, rInfo, "KoopaLastWallBreak",
        al::getHitSensor(this, "CollisionBase"), nullptr, nullptr);
    mBreakCollision->kill();
    al::listenStageSwitchOnStart(this, al::Functor(this, &KoopaLastWall::start));
    al::killPrePassLightAll(this, -1);
    makeActorAppeared();
}

/** @brief Maintains the wall's normal animation. */
void KoopaLastWall::exeWait() {
    al::startAction(this, "Normal");
}

/** @brief Enables warning lights and breaks the wall after its warning delay. */
void KoopaLastWall::exeSign() {
    if (al::isFirstStep(this)) {
        al::appearPrePassLightAll(this, -1);
    }
    if (al::isStep(this, 30)) {
        al::startAction(this, "BreakSign");
    }
    if (!al::isLessStep(this, 120)) {
        al::setNerve(this, &NrvKoopaLastWallBreak);
    }
}

/** @brief Switches to broken collision and signals that the wall has broken. */
void KoopaLastWall::exeBreak() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Break");
        al::tryOnStageSwitch(this, "SwitchBreakOn");
        mBreakCollision->appear();
        al::invalidateCollisionParts(this);
    }
}

/** @brief Destroys the wall's base actor resources. */
KoopaLastWall::~KoopaLastWall() = default;
