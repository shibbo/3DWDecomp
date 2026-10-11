#include "Boss/KoopaLastFunction.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace KoopaLastFunction {

/**
 * @brief Initializes the final-boss archive and twelve hair spring controllers.
 * @param pActor Actor being initialized.
 * @param rInfo Actor placement and scene initialization information.
 * @param pName Archive suffix.
 * @param pControllers Optional array receiving the created controllers.
 */
void initActorKoopaLastCommon(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                              const char* pName,
                              sead::PtrArray<al::JointSpringController>* pControllers) {
    al::initActorWithArchiveName(pActor, rInfo, sead::SafeString("KoopaLast"), pName);
    al::initJointControllerKeeper(pActor, 16);
    auto initSpring = [pActor, pControllers](const char* pJointName) {
        auto* pController = al::initJointSpringController(pActor, pJointName);
        if (pControllers) { pControllers->pushBack(pController); }
    };
    initSpring("HairFront1");
    initSpring("HairFront2");
    initSpring("HairFront3");
    initSpring("HairL1");
    initSpring("HairL2");
    initSpring("HairL3");
    initSpring("HairR1");
    initSpring("HairR2");
    initSpring("HairR3");
    initSpring("HairMiddle1");
    initSpring("HairMiddle2");
    initSpring("HairMiddle3");
}

/**
 * @brief Sends the appropriate attack or object-breaking message for a sensor pair.
 * @param pSelf Attacking sensor.
 * @param pOther Receiving sensor.
 * @return Whether a message was handled.
 */
bool attackSensorCommon(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (al::isSensorEnemyBody(pSelf)) {
        if (!al::isSensorPlayer(pOther) &&
            (al::isSensorName(pSelf, "Body") || al::isSensorName(pSelf, "BodyUnder"))) {
            if (rc::sendMsgKoopaLastBreakObj(pOther, pSelf)) { return true; }
            return al::sendMsgExplosion(pOther, pSelf, nullptr);
        }
    } else if (al::isSensorEnemyAttack(pSelf) && al::isSensorPlayer(pOther)) {
        return al::sendMsgEnemyAttack(pOther, pSelf);
    }
    return false;
}

/**
 * @brief Sends explosion collision messages to triangles intersecting a sensor sphere.
 * @param pActor Actor owning the sensor and collision service.
 * @param pName Sensor name.
 */
void explosionCollision(al::LiveActor* pActor, const char* pName) {
    al::HitSensor* pSensor = al::getHitSensor(pActor, pName);
    if (al::isSensorValid(pSensor)) {
        u32 count = alCollisionUtil::checkStrikeSphere(pActor, al::getSensorPos(pSensor),
            al::getSensorRadius(pSensor), nullptr, nullptr);
        for (u32 i = 0; i < count; ++i) {
            const auto* pInfo = alCollisionUtil::getStrikeSphereInfo(pActor, i);
            al::sendMsgExplosionCollide(pInfo->mTriangle.getSensor(), pSensor, nullptr);
        }
    }
}

/**
 * @brief Accepts projectile attacks on body sensors other than the POW block.
 * @param pMsg Incoming sensor message.
 * @param pSender Sending sensor; unused.
 * @param pReceiver Receiving sensor.
 * @return Whether the projectile attack is accepted.
 */
bool receiveMsgCommon(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isMsgPlayerBoomerangBreak(pMsg) || al::isMsgPlayerFireBallAttack(pMsg)) {
        return al::isSensorEnemyBody(pReceiver) && !al::isSensorName(pReceiver, "PowBlock");
    }
    return false;
}

/**
 * @brief Checks for a general touch-assist message.
 * @param pMsg Incoming message.
 * @param pPointer Screen pointer; unused.
 * @param pTarget Screen target; unused.
 * @return Whether the message requests touch assistance.
 */
bool receiveMsgScreenPointCommon(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                  al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssistAll(pMsg);
}

/**
 * @brief Checks for an explosion reaching the POW block from a non-body sensor.
 * @param pMsg Incoming message.
 * @param pSender Sending sensor.
 * @param pReceiver Receiving sensor.
 * @return Whether this is a valid POW-block hit.
 */
bool isReceivePowBlockMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isMsgExplosion(pMsg) && !al::isSensorEnemyBody(pSender)) {
        return al::isSensorName(pReceiver, "PowBlock");
    }
    return false;
}

}  // namespace KoopaLastFunction
