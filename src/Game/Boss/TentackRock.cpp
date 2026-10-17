#include "Boss/TentackRock.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(TentackRock, Fall);
NERVES_MAKE_NOSTRUCT(TentackRock, Fall)

/**
 * @brief Plays the landing sound and kills a rock through its actor interface.
 * @param pActor Rock actor to break.
 */
void breakRock(al::LiveActor* pActor) {
    al::startSe(pActor, "PgLand", nullptr);
    pActor->kill();
}
}

/**
 * @brief Creates a falling rock with no break model assigned.
 * @param pName Actor name.
 */
TentackRock::TentackRock(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the falling rock and its break model.
 * @param rInfo Actor placement and scene initialization information.
 */
void TentackRock::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, sead::SafeString("TentackBulletRock"), nullptr);
    al::initNerve(this, &NrvTentackRockFall, 0);
    mBreakModel = new al::BreakModel(this, "テンタックの岩[壊れモデル]", "TentackBulletRockBreak",
                                    nullptr, nullptr, "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);
    makeActorDead();
}

/** @brief Stops the warning sound, removes the rock, and appears its break model. */
void TentackRock::kill() {
    al::stopSeByName(this, "Sign");
    al::LiveActor::kill();
    al::appearBreakModelRandomRotateY(mBreakModel);
}

/**
 * @brief Pushes nearby sensors and breaks after successfully attacking a player.
 * @param pSelf Attacking sensor.
 * @param pOther Receiving sensor.
 */
void TentackRock::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyAttack(pSelf)) {
        al::sendMsgPush(pOther, pSelf);
        if (al::isSensorPlayer(pOther) && al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
            breakRock(this);
        }
    }
}

/**
 * @brief Breaks the rock when hit by Tentack's magma ball.
 * @param pMsg Incoming message.
 * @param pSelf Receiving sensor; unused.
 * @param pOther Sending sensor; unused.
 * @return Whether the message broke the rock.
 */
bool TentackRock::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (rc::isMsgTentackMagmaBallBreak(pMsg)) {
        breakRock(this);
        return true;
    }
    return false;
}

/**
 * @brief Breaks the rock in response to a touch-assist trigger.
 * @param pMsg Incoming message.
 * @param pPointer Screen pointer; unused.
 * @param pTarget Screen target; unused.
 * @return Whether the message broke the rock.
 */
bool TentackRock::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                       al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssistTrig(pMsg)) {
        breakRock(this);
        return true;
    }
    return false;
}

/**
 * @brief Places the rock at its starting position and begins falling.
 * @param rPosition Initial world position.
 */
void TentackRock::startFall(const sead::Vector3f& rPosition) {
    al::setTrans(this, rPosition);
    al::resetPosition(this, false);
    al::setVelocityZero(this);
    al::setNerve(this, &NrvTentackRockFall);
    appear();
}

/**
 * @brief Checks that both the rock and its break model have disappeared.
 * @return Whether the rock is ready for reuse.
 */
bool TentackRock::isDeadRock() const {
    return al::isDead(this) && al::isDead(mBreakModel);
}

/** @brief Applies falling motion and breaks on collision, death area, or timeout. */
void TentackRock::exeFall() {
    if (al::isFirstStep(this)) { al::startAction(this, "Fall"); }
    al::addVelocityToGravity(this, 2.0f);
    al::scaleVelocityY(this, 0.9f);
    if (al::isCollided(this) || rc::isInDeathArea(this) || al::isGreaterEqualStep(this, 600)) {
        breakRock(this);
    }
}

/** @brief Destroys the rock's base actor resources. */
TentackRock::~TentackRock() = default;
