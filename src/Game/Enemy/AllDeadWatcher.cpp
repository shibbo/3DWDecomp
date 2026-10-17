#include "Enemy/AllDeadWatcher.hpp"

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMapUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(AllDeadWatcher, Wait)
NERVE_DECL(AllDeadWatcher, Watch)
NERVE_DECL(AllDeadWatcher, WatchPartial)
NERVES_MAKE_NOSTRUCT(AllDeadWatcher, Wait, Watch, WatchPartial)
}  // namespace

/**
 * @brief Constructs a watcher with no linked actors yet.
 * @param pName Actor name.
 */
AllDeadWatcher::AllDeadWatcher(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Creates the watched actors from the "WatchTargetEnemy" links and reads the placement
 * parameters.
 * @param rInfo Actor placement and scene information.
 */
void AllDeadWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvAllDeadWatcherWatch, 0);
    al::tryGetArg(&mIsEnableRespawn, rInfo, "EnableRespawn");
    al::tryGetArg(&mIsKillRemainingLinkedActors, rInfo, "KillRemainingLinkedActors");
    mIsValidSwitchPartialDeadOn = al::isValidStageSwitch(this, "SwitchPartialDeadOn");
    if (mIsValidSwitchPartialDeadOn) {
        al::setNerve(this, &NrvAllDeadWatcherWatchPartial);
    }

    mActorCount = al::calcLinkChildNum(rInfo, "WatchTargetEnemy");
    mActorTrans = new sead::Vector3f[mActorCount];
    ProjectActorFactory factory;
    mActors = new al::LiveActor*[mActorCount];
    for (int i = 0; i < mActorCount; i++) {
        al::LiveActor* actor =
            al::createLinksActorFromFactory(factory, rInfo, "WatchTargetEnemy", i);
        mActors[i] = actor;
        mActorTrans[i] = al::getTrans(actor);
    }

    al::tryGetArg(&mSwitchOnDelayStep, rInfo, "SwitchOnDelayStep");
    bool isComplete = false;
    if (al::isSingleMode(rInfo)) {
        al::tryGetArg(&mIsRequireOnGround, rInfo, "RequireOnGround");
        isComplete = SingleModeDataFunction::isIslandScenarioIDComplete(this, rInfo);
    }

    bool isSwitchSynced = al::trySyncStageSwitchAppear(this);
    if (isComplete || isSwitchSynced) {
        for (int i = 0; i < mActorCount; i++) {
            if (rInfo.getActorSceneInfo().isSingleMode) {
                mActors[i]->killComplete(true);
            } else {
                mActors[i]->makeActorDead();
            }
        }
    }
}

/**
 * @brief Restarts watching and brings the linked actors back.
 */
void AllDeadWatcher::appear() {
    if (mIsValidSwitchPartialDeadOn) {
        al::setNerve(this, &NrvAllDeadWatcherWatchPartial);
    } else {
        al::setNerve(this, &NrvAllDeadWatcherWatch);
    }

    mIsAllDead = false;
    _163 = false;
    al::tryOffSwitchDeadOn(this);
    al::tryOffStageSwitch(this, "SwitchPartialDeadOn");
    al::LiveActor::appear();
    for (int i = 0; i < mActorCount; i++) {
        al::LiveActor* actor = mActors[i];
        if (al::isSingleMode(actor) || mIsKilled) {
            al::resetPosition(actor, mActorTrans[i], false);
            actor->reappear();
        } else {
            actor->appear();
        }
    }
}

/**
 * @brief Kills the watcher, turning on the dead switch if every linked actor died, or killing
 * the remaining linked actors otherwise.
 */
void AllDeadWatcher::kill() {
    al::LiveActor::kill();
    mIsKilled = true;
    if (mIsAllDead) {
        al::tryOnSwitchDeadOn(this);
        auto* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
        if (koopaJr != nullptr) {
            koopaJr->tryPraiseReaction(60);
        }
    } else if (mIsKillRemainingLinkedActors) {
        for (int i = 0; i < mActorCount; i++) {
            mActors[i]->killComplete(false);
        }
    }

    mIsAllDead = false;
}

/**
 * @brief Waits until no linked actor is alive any more.
 */
void AllDeadWatcher::exeWatch() {
    for (int i = 0; i < mActorCount; i++) {
        if (al::isAlive(mActors[i]) || al::isDeadAlive(mActors[i])) {
            return;
        }
    }

    setWaitNerve();
}

/**
 * @brief Turns on the partial dead switch whenever a linked actor is dead.
 */
void AllDeadWatcher::exeWatchPartial() {
    for (int i = 0; i < mActorCount; i++) {
        if (al::isDead(mActors[i])) {
            al::tryOnStageSwitch(this, "SwitchPartialDeadOn");
            setWatchNerve();
        }
    }
}

/**
 * @brief Switches to the delay before the dead switch is turned on.
 */
void AllDeadWatcher::setWaitNerve() {
    al::setNerve(this, &NrvAllDeadWatcherWait);
}

/**
 * @brief Switches back to watching the linked actors.
 */
void AllDeadWatcher::setWatchNerve() {
    al::setNerve(this, &NrvAllDeadWatcherWatch);
}

/**
 * @brief Kills the watcher once the delay has passed (and, if required, every player is on the
 * ground or in water).
 */
void AllDeadWatcher::exeWait() {
    if (al::isGreaterEqualStep(this, mSwitchOnDelayStep) &&
        (!mIsRequireOnGround || rc::isAllPlayerOnGroundOrWater(this))) {
        mIsAllDead = true;
        kill();
    }
}
