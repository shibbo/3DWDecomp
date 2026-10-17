#include "Enemy/BrosMoveStepKeeper.hpp"
#include "Enemy/KuriboTower.hpp"
#include "Enemy/KuriboTowerNode.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Joint/KeyPose.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

/** @brief Creates platform tracking data for each movement point.
 * @param pHost Bros enemy using these movement points.
 * @param pKeyPoseKeeper Movement point controller.
 * @param rInfo Initialization context for tower platforms.
 */
BrosMoveStepKeeper::BrosMoveStepKeeper(al::LiveActor* pHost, al::KeyPoseKeeper* pKeyPoseKeeper,
        const al::ActorInitInfo& rInfo) : mHost(pHost), mKeyPoseKeeper(pKeyPoseKeeper) {
    mSteps = new Step[al::getKeyPoseCount(mKeyPoseKeeper)];
    for (s32 i = 0; i < al::getKeyPoseCount(mKeyPoseKeeper); ++i) {
        s32 type = -1;
        mSteps[i].pTower = nullptr;
        mSteps[i].pConnector = nullptr;
        mSteps[i].isRouteDokan = false;
        mSteps[i].isPoseFixed = false;
        mSteps[i].isTowerGone = true;
        const al::KeyPose& rPose = mKeyPoseKeeper->getKeyPose(i);
        al::tryGetArg(&type, rPose.getPlacementInfo(), "ScaffoldType");
        mSteps[i].pConnector = new al::MtxConnector(rPose.getQuat(), rPose.getTrans());
        switch (type) {
        case 0:
            mSteps[i].pTower = new KuriboTower("クリボータワー(ブロス足場)");
            mSteps[i].isTowerGone = false;
            al::initCreateActorWithPlacementInfo(mSteps[i].pTower, rInfo, rPose.getPlacementInfo());
            mSteps[i].pTower->setBrosOnTower();
            break;
        case 1:
            mSteps[i].isRouteDokan = true;
            break;
        case 2:
            mSteps[i].isPoseFixed = true;
            break;
        }
    }
}

/** @brief Attaches movement points without towers to supporting collision. */
void BrosMoveStepKeeper::endInit() {
    for (s32 i = 0; i < al::getKeyPoseCount(mKeyPoseKeeper); ++i) {
        if (!mSteps[i].pTower) {
            al::attachMtxConnectorToCollision(mSteps[i].pConnector, mHost,
                mKeyPoseKeeper->getKeyPose(i).getTrans() + 100.0f * sead::Vector3f::ey,
                -1000.0f * sead::Vector3f::ey);
        }
    }
}

/** @brief Reappears the tower platforms and resets their disappearance flags. */
void BrosMoveStepKeeper::reset() {
    for (s32 i = 0; i < al::getKeyPoseCount(mKeyPoseKeeper); ++i) {
        if (mSteps[i].pTower) {
            mSteps[i].pTower->reappear();
            mSteps[i].isTowerGone = false;
        }
    }
}

/** @brief Removes all tower platforms.
 * @param isBlowDown Whether the towers are blown down.
 */
void BrosMoveStepKeeper::killKuriboTower(bool isBlowDown) {
    for (s32 i = 0; i < al::getKeyPoseCount(mKeyPoseKeeper); ++i) {
        if (mSteps[i].pTower) {
            mSteps[i].pTower->killComplete(isBlowDown);
            mSteps[i].isTowerGone = true;
        }
    }
}

/** @brief Updates platform poses and settles collapsed tower positions onto collision. */
void BrosMoveStepKeeper::update() {
    for (s32 i = 0; i < al::getKeyPoseCount(mKeyPoseKeeper); ++i) {
        const al::KeyPose& rPose = mKeyPoseKeeper->getKeyPose(i);
        Step& rStep = mSteps[i];
        if (rStep.pTower && !rStep.isTowerGone) {
            if (rStep.pTower->getTop()) {
                const_cast<sead::Vector3f&>(rPose.getTrans()).set(al::getTrans(rStep.pTower->getTop()) + sead::Vector3f(0.0f, 120.0f, 0.0f));
                al::makeQuatFrontUp(&const_cast<sead::Quatf&>(rPose.getQuat()), al::getFront(rStep.pTower->getTop()), sead::Vector3f::ey);
            } else if (alCollisionUtil::getFirstPolyOnArrow(mHost, &const_cast<sead::Vector3f&>(rPose.getTrans()), nullptr, rPose.getTrans(),
                    -10.0f * sead::Vector3f::ey, nullptr, nullptr)) {
                if (GameDataFunction::isSingleMode(mHost)) { rStep.pTower->kill(); }
                else { rStep.pTower = nullptr; }
                rStep.isTowerGone = true;
                rStep.pConnector->clear();
                rStep.pConnector->setBaseQuatTrans(rPose.getQuat(), rPose.getTrans());
                al::attachMtxConnectorToCollision(rStep.pConnector, mHost,
                    rPose.getTrans() + 100.0f * sead::Vector3f::ey, -10.0f * sead::Vector3f::ey);
            } else {
                const_cast<sead::Vector3f&>(rPose.getTrans()).y -= 7.5f;
            }
        } else if (rStep.pConnector) {
            rStep.pConnector->multTrans(&const_cast<sead::Vector3f&>(rPose.getTrans()), al::getConnectBaseTrans(rStep.pConnector));
        }
    }
}

/** @brief Tests whether the current point uses an active tower.
 * @return Whether the current tower exists and has not disappeared.
 */
bool BrosMoveStepKeeper::isCurrentStepKuribo() const {
    if (mSteps[mKeyPoseKeeper->getKeyPoseCurrentIdx()].pTower) {
        if (!mSteps[mKeyPoseKeeper->getKeyPoseCurrentIdx()].isTowerGone) { return true; }
    }
    return false;
}

/** @brief Tests whether the current point uses a route pipe.
 * @return Whether its route pipe flag is set.
 */
bool BrosMoveStepKeeper::isCurrentStepRouteDokan() const {
    return mSteps[mKeyPoseKeeper->getKeyPoseCurrentIdx()].isRouteDokan;
}

/** @brief Tests whether the current point has a fixed pose.
 * @return Whether its fixed pose flag is set.
 */
bool BrosMoveStepKeeper::isCurrentStepPoseFixed() const {
    return mSteps[mKeyPoseKeeper->getKeyPoseCurrentIdx()].isPoseFixed;
}

/** @brief Tests the next tower using the current point's disappearance flag.
 * @return Whether the next tower exists and the current tower is not marked gone.
 */
bool BrosMoveStepKeeper::isNextStepKuribo() const {
    if (mSteps[mKeyPoseKeeper->calcNextPoseIndex()].pTower) {
        if (!mSteps[mKeyPoseKeeper->getKeyPoseCurrentIdx()].isTowerGone) { return true; }
    }
    return false;
}

/** @brief Tests whether the next point uses a route pipe.
 * @return Whether its route pipe flag is set.
 */
bool BrosMoveStepKeeper::isNextStepRouteDokan() const {
    return mSteps[mKeyPoseKeeper->calcNextPoseIndex()].isRouteDokan;
}

/** @brief Tests whether the next point has a fixed pose.
 * @return Whether its fixed pose flag is set.
 */
bool BrosMoveStepKeeper::isNextStepPoseFixed() const {
    return mSteps[mKeyPoseKeeper->calcNextPoseIndex()].isPoseFixed;
}

/** @brief Gets the tower at a movement point.
 * @param index Movement point index.
 * @return Tower pointer, or nullptr for a point without a tower.
 */
KuriboTower* BrosMoveStepKeeper::tryGetStepKuribo(s32 index) const { return mSteps[index].pTower; }
