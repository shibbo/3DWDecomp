#include "Enemy/PukupukuFly.hpp"
#include "Enemy/EnemyStateBlowDown.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(PukupukuFly, FlyWait)
NERVE_DECL(PukupukuFly, BlowDown)
NERVE_DECL(PukupukuFly, PressDown)
NERVES_MAKE_STRUCT(PukupukuFly, FlyWait, BlowDown, PressDown)
}

/** @brief Creates a flying Cheep Cheep. @param pName Actor name. */
PukupukuFly::PukupukuFly(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes rail movement and defeat states. @param rInfo Placement information. */
void PukupukuFly::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Pukupuku", nullptr);
    al::tryGetArg(&mSpeed, rInfo, "Speed");
    if (al::isExistRail(this)) {
        al::setSyncRailToNearestPos(this);
        al::setRailClippingInfo(&mClippingCenter, this, 100.0f, 100.0f);
    }
    al::initNerve(this, &NrvPukupukuFly.FlyWait, 1);
    al::startAction(this, "FlyWait");
    mBlowDownParam = new EnemyStateBlowDownParam(false);
    mBlowDown = new EnemyStateBlowDown(this, mBlowDownParam);
    al::initNerveState(this, mBlowDown, &NrvPukupukuFly.BlowDown, "[state]吹き飛ばし");
    al::offCollide(this);
    makeActorAppeared();
}

/** @brief Follows the looping rail until defeated. */
void PukupukuFly::control() {
    if (isEnableDown()) {
        al::moveSyncRailLoop(this, mSpeed);
        sead::Vector3f direction = al::getRailDir(this);
        al::turnQuatFrontToDirDegreeH(this, direction, 180.0f);
    }
}

/** @brief Checks whether the fish can receive a defeating attack. @return Whether it is not already defeated. */
bool PukupukuFly::isEnableDown() {
    return !al::isNerve(this, &NrvPukupukuFly.BlowDown) && !al::isNerve(this, &NrvPukupukuFly.PressDown);
}

/** @brief Plays the flying animation. */
void PukupukuFly::exeFlyWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "FlyWait");
}

/** @brief Plays the flattened defeat animation and releases an item. */
void PukupukuFly::exePressDown() {
    if (al::isFirstStep(this)) {
        al::setVelocityZero(this);
        al::startAction(this, "SwimPressDown");
    }
    if (al::isActionEnd(this)) {
        al::appearItem(this);
        al::startHitReactionDeath(this);
        kill();
    }
}

/** @brief Completes knockback and releases an item. */
void PukupukuFly::exeBlowDown() {
    if (al::updateNerveState(this)) {
        al::appearItem(this);
        kill();
    }
}

/** @brief Pushes and attacks actors touching the attack sensor. @param pSelf Own sensor. @param pOther Contact sensor. */
void PukupukuFly::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isEnableDown() && al::isSensorName(pSelf, "Attack")) {
        al::sendMsgPush(pOther, pSelf);
        al::sendMsgEnemyAttack(pOther, pSelf);
    }
}

/** @brief Handles flattening and knockback attacks. @param pMsg Message. @param pOther Sender. @param pSelf Own sensor. @return Whether handled. */
bool PukupukuFly::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!isEnableDown())
        return false;
    if (al::isSensorName(pSelf, "Body")) {
        if (EnemyStateUtil::tryRequestPressDownAndNextNerve(pMsg, pOther, pSelf, this, &NrvPukupukuFly.PressDown, false) ||
            EnemyStateUtil::tryRequestBlowDownAndNextNerve(pMsg, pOther, pSelf, mBlowDown, &NrvPukupukuFly.BlowDown, false)) {
            rc::setAppearItemFactorByMsg(this, pMsg, pOther);
            rc::addScoreCombo(this, pOther, pMsg, 100.0f);
            return true;
        }
    }
    return false;
}
