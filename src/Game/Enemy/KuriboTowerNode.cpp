#include "Enemy/KuriboTowerNode.hpp"
#include "Enemy/KuriboTower.hpp"
#include "Enemy/WalkerStateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include <cmath>

namespace {
WalkerStateParam sTowerWalkParam;
struct TowerWalkParamInitializer {
    /** @brief Sets the tower's gravity and friction parameters. */
    TowerWalkParamInitializer() {
        sTowerWalkParam.mGravity = 1.5f;
        sTowerWalkParam.mAirFriction = 0.99f;
        sTowerWalkParam.mGroundFriction = 0.9f;
    }
} sTowerWalkParamInitializer;
}

/** @brief Creates a node belonging to a Goomba tower. @param pName Actor name. */
KuriboTowerNode::KuriboTowerNode(const char* pName) : al::LiveActor(pName) {}

/** @brief Assigns the owning tower. @param pHost Tower controller. */
void KuriboTowerNode::setHost(KuriboTower* pHost) {
    mHost = pHost;
}

/** @brief Kills the node and informs its tower. */
void KuriboTowerNode::kill() {
    al::LiveActor::kill();
    mHost->killedChild(this);
}

/** @brief Gets the supporting node. @return The parent node, or nullptr without a tower. */
const KuriboTowerNode* KuriboTowerNode::getParent() const {
    return mHost ? mHost->getParent(this) : nullptr;
}

/** @brief Updates facing, velocity and stacking constraints.
 * @param isUpdateFront Whether to follow the supporting node's facing. */
void KuriboTowerNode::updatePosture(bool isUpdateFront) {
    const KuriboTowerNode* pParent = getParent();
    if (pParent && isUpdateFront) {
        sead::Vector3f front = al::getFront(pParent);
        f32 rate = std::pow(0.92f, static_cast<f32>(mHost->getTowerChildIndex(this) + 1)) * 7.5f;
        al::turnDirectionDegree(this, al::getFrontPtr(this), front, rate);
    }
    updateVelocity();
    restrictToTowerPosition();
}

/** @brief Applies gravity and friction according to contact with the tower. */
void KuriboTowerNode::updateVelocity() {
    if ((mHost->isBottom(this) && al::isOnGround(this, 2, 0.0f)) || mHost->isContact(this)) {
        al::addVelocityToGravity(this, 0.12f);
        al::scaleVelocity(this, sTowerWalkParam.mGroundFriction);
    } else {
        al::addVelocityToGravity(this, sTowerWalkParam.mGravity);
        al::scaleVelocity(this, sTowerWalkParam.mAirFriction);
    }
    mHost->revieseVelocityIfExistObstacleForward(this);
}

/** @brief Aligns the node with its support and stops fast downward movement on contact. */
void KuriboTowerNode::restrictToTowerPosition() {
    const KuriboTowerNode* pParent = getParent();
    if (!pParent)
        return;
    al::getTransPtr(this)->x = al::getTrans(pParent).x;
    al::getTransPtr(this)->z = al::getTrans(pParent).z;
    if (mHost->isContact(this)) {
        al::getTransPtr(this)->y = al::getTrans(pParent).y + pParent->getOffsetY();
        if (al::getVelocity(this).y < -8.0f)
            al::getVelocityPtr(this)->y = al::getVelocity(this).y * -0.0f;
    }
}
