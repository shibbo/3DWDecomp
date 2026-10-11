#include "Boss/KoopaChaseWarpDummy.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(KoopaChaseWarpDummy, Wait);
NERVE_DECL(KoopaChaseWarpDummy, Provocation);
NERVE_DECL(KoopaChaseWarpDummy, Move);
NERVE_DECL(KoopaChaseWarpDummy, Land);
NERVES_MAKE_NOSTRUCT(KoopaChaseWarpDummy, Wait, Provocation, Move, Land)
}

/**
 * @brief Creates a warp dummy with no key poses and zero damage.
 * @param pName Actor name.
 */
KoopaChaseWarpDummy::KoopaChaseWarpDummy(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the selected chase model and its key-pose movement.
 * @param rInfo Actor placement and scene initialization information.
 */
void KoopaChaseWarpDummy::init(const al::ActorInitInfo& rInfo) {
    bool isLv2 = false;
    al::tryGetArg(&isLv2, rInfo, "IsLv2");
    al::initActorWithArchiveName(this, rInfo, sead::SafeString(isLv2 ? "KoopaChaseLv2" : "KoopaChase"), "WarpDummy");
    al::initNerve(this, &NrvKoopaChaseWarpDummyMove, 0);
    mKeyPoseKeeper = al::createKeyPoseKeeper(rInfo);
    al::setTrans(this, al::getCurrentKeyTrans(mKeyPoseKeeper));
    al::setQuat(this, al::getCurrentKeyQuat(mKeyPoseKeeper));
    mMoveTime = al::calcKeyMoveMoveTime(mKeyPoseKeeper);
    makeActorDead();
}

/** @brief Resets the key poses and starts moving. */
void KoopaChaseWarpDummy::appear() {
    al::LiveActor::appear();
    al::resetKeyPose(mKeyPoseKeeper);
    al::setNerve(this, &NrvKoopaChaseWarpDummyMove);
}

/**
 * @brief Returns the Koopa dummy subactor.
 * @return Koopa subactor.
 */
al::LiveActor* KoopaChaseWarpDummy::getKoopa() const {
    return al::getSubActor(this, "クッパダミー");
}

/**
 * @brief Appears using animations for the specified damage level.
 * @param damageCount Current chase damage count.
 */
void KoopaChaseWarpDummy::appear(int damageCount) {
    mDamageCount = damageCount;
    appear();
}

/** @brief Interpolates key poses until the dummy reaches its landing point. */
void KoopaChaseWarpDummy::exeMove() {
    if (al::isFirstStep(this)) {
        al::startAction(this, KoopaChaseFunction::getAnimNameWarpJumpLoop(mDamageCount));
        al::startAction(getKoopa(), "WarpJumpLoop");
    }
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoseKeeper);
        if (al::isStop(mKeyPoseKeeper)) {
            al::setNerve(this, &NrvKoopaChaseWarpDummyLand);
        } else {
            al::setNerve(this, &NrvKoopaChaseWarpDummyMove);
        }
    }
}

/** @brief Plays the landing animations before entering the waiting loop. */
void KoopaChaseWarpDummy::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, KoopaChaseFunction::getAnimNameWarpJumpEnd(mDamageCount));
        al::startAction(getKoopa(), "WarpJumpEnd");
    }
    if (al::isActionEnd(this)) {
        al::startAction(this, KoopaChaseFunction::getAnimNameRun(mDamageCount));
        al::startAction(getKoopa(), "Wait");
        al::setNerve(this, &NrvKoopaChaseWarpDummyWait);
    }
}

/** @brief Starts a provocation after waiting two seconds. */
void KoopaChaseWarpDummy::exeWait() {
    if (al::isGreaterEqualStep(this, 120)) {
        al::startAction(getKoopa(), "Provocation");
        al::setNerve(this, &NrvKoopaChaseWarpDummyProvocation);
    }
}

/** @brief Restores the waiting animation when the provocation finishes. */
void KoopaChaseWarpDummy::exeProvocation() {
    if (al::isActionEnd(getKoopa())) {
        al::startAction(getKoopa(), "Wait");
        al::setNerve(this, &NrvKoopaChaseWarpDummyWait);
    }
}

/** @brief Destroys the dummy's base actor resources. */
KoopaChaseWarpDummy::~KoopaChaseWarpDummy() = default;
