#include "Enemy/Nokonoko.hpp"
#include "Enemy/NokonokoNormal.hpp"
#include "Enemy/NokonokoNaked.hpp"
#include "MapObj/Koura.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(Nokonoko, Wait)
NERVES_MAKE_NOSTRUCT(Nokonoko, Wait)
}

/** @brief Creates the Koopa body and shell coordinator. @param pName Actor name. */
Nokonoko::Nokonoko(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes both body forms and a detached shell. @param rInfo Placement information. */
void Nokonoko::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorEnemyMovement(this, rInfo);
    al::initNerve(this, &NrvNokonokoWait, 0);
    mNormal = new NokonokoNormal("ノコノコ 通常", this);
    mNormal->init(rInfo);
    mNaked = new NokonokoNaked("ノコノコ 裸", this);
    mNaked->init(rInfo);
    mNaked->kill();
    mKoura = new Koura("コウラ");
    al::initCreateActorNoPlacementInfo(mKoura, rInfo);
    mKoura->makeActorDead();
    al::listenStageSwitchOnKill(this, al::Functor(this, &Nokonoko::killBySwitch));
    makeActorAppeared();
}

/** @brief Removes both body forms and any shell not occupied by a player. */
void Nokonoko::killBySwitch() {
    if (al::isAlive(mNormal)) {
        al::startHitReactionDeath(mNormal);
        mNormal->kill();
    }
    if (al::isAlive(mNaked)) {
        al::startHitReactionDeath(mNaked);
        mNaked->kill();
    }
    if (!mKoura->isPlayerInside() && al::isAlive(mKoura))
        mKoura->startKillBySwitch();
}

/** @brief Leaves contact attacks to the active body. @param pSelf Own sensor. @param pOther Contact sensor. */
void Nokonoko::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {}

/** @brief Leaves sensor messages to the active body. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Always false. */
bool Nokonoko::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) { return false; }

/** @brief Leaves pointer messages to the active body. @param pMsg Message. @param pPointer Pointer. @param pTarget Target. @return Always false. */
bool Nokonoko::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) { return false; }

/** @brief Restores the shelled body at the naked body's pose. */
void Nokonoko::attachKoura() {
    mNormal->appear();
    al::copyPose(mNormal, mNaked);
    al::resetPosition(mNormal, false);
    mNormal->startAttachKoura();
    mNaked->kill();
}

/** @brief Ejects the naked body from the shell. */
void Nokonoko::startEject() {
    al::copyPose(mNaked, mNormal);
    al::resetPosition(mNaked, false);
    mNaked->startEject();
    mNormal->switchToNaked();
}

/** @brief Ejects the naked body upward. */
void Nokonoko::startEjectUpper() {
    al::copyPose(mNaked, mNormal);
    al::resetPosition(mNaked, false);
    mNaked->startEjectUpper();
    mNormal->switchToNaked();
}

/** @brief Restores a shell when the detached one is gone or clipped. @return Whether the shelled form was restored. */
bool Nokonoko::tryStartWear() {
    if (mActorSceneInfo->isSingleMode)
        return false;
    if (al::isDead(mKoura) || al::isClipped(mKoura)) {
        if (al::isClipped(mKoura))
            mKoura->startKill();
        mNormal->appear();
        mNormal->startMove();
        al::resetPosition(mNormal, al::getTrans(mNaked), false);
        mNaked->kill();
        return true;
    }
    return false;
}

/** @brief Removes the coordinator once both body forms are dead. */
void Nokonoko::exeWait() {
    if (al::isDead(mNormal) && al::isDead(mNaked))
        kill();
}
