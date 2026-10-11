#include "Enemy/PackunTrace.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"

/** @brief Constructs the remains associated with a Piranha Plant.
 * @param pOwner Plant whose pose is copied when the remains appear.
 */
PackunTrace::PackunTrace(al::LiveActor* pOwner)
    : al::LiveActor("パックン残骸"), mOwner(pOwner) {
    getName();
}

/** @brief Initializes the model and collision connector, leaving the actor inactive.
 * @param rInfo Actor placement and scene information.
 */
void PackunTrace::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "PackunTrace", nullptr);
    mConnector = al::createMtxConnector(this);
    makeActorDead();
}

/** @brief Attaches the remains to the placed collision geometry. */
void PackunTrace::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mConnector, this, false);
}

/** @brief Follows the collision pose while preserving the current facing direction. */
void PackunTrace::control() {
    sead::Vector3f front;
    al::calcQuatFront(&front, al::getQuat(this));
    al::connectPoseQT(this, mConnector);
    sead::Quatf quat = al::getQuat(this);
    quat.setMul(mBaseQuat, quat);
    sead::Vector3f up;
    al::calcQuatUp(&up, quat);
    sead::Vector3f side = up.cross(front);
    if (!al::isNearZero(side, 0.001f)) {
        al::makeQuatUpFront(al::getQuatPtr(this), up, front);
    }
}

/** @brief Appears at the owner's pose with a random yaw and an appearance reaction. */
void PackunTrace::appear() {
    al::copyPose(this, mOwner);
    al::LiveActor::appear();
    al::rotateQuatYDirDegree(this, al::getRandomDegree());
    al::startHitReactionAppear(this);
}
