#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class KuriboTower;

/** @brief Base class of the actors stacked into a Goomba Tower (Goombas and carried items). */
class KuriboTowerNode : public al::LiveActor {
public:
    explicit KuriboTowerNode(const char* pName);

    /** @brief Destroys the tower node. */
    ~KuriboTowerNode() override = default;

    void kill() override;

    /** @brief Called once the whole tower has been set up. */
    virtual void endInit() {}

    /**
     * @brief Checks whether the node is being knocked down by a hip drop.
     * @return Whether the node is in its hip drop down behavior.
     */
    virtual bool isNerveHipDropDown() const { return false; }

    /** @brief Requests the node to start behaving as the bottom (root) of the tower. */
    virtual void requestRootBehavior() {}

    /** @brief Requests the node to behave as a free actor after leaving the tower. */
    virtual void requestRelease() {}

    /**
     * @brief Requests the node to play its surprise reaction.
     * @return Whether the request was accepted.
     */
    virtual bool requestSurprise() { return false; }

    /**
     * @brief Requests the node to react to an attack performed by the tower.
     * @param pSensor Sensor that was attacked.
     */
    virtual void requestAttackReaction(al::HitSensor* pSensor) {}

    /**
     * @brief Checks whether the node is frozen by the support player.
     * @return Whether the node is in its support freeze behavior.
     */
    virtual bool isNerveSupportFreeze() const { return false; }

    /** @brief Requests the node to freeze together with another frozen node. */
    virtual void requestSupportFreezeSync() {}

    /** @brief Requests the node to end a synchronized freeze. */
    virtual void requestEndSupportFreezeSync() {}

    /**
     * @brief Gets the height that the node occupies in the tower.
     * @return The height offset to the next node.
     */
    virtual f32 getOffsetY() const { return 0.0f; }

    void setHost(KuriboTower* pHost);
    const KuriboTowerNode* getParent() const;
    void updatePosture(bool isUpdateFront);
    void updateVelocity();
    void restrictToTowerPosition();

    /**
     * @brief Checks whether the node keeps the tower waiting instead of walking.
     * @return Whether the tower plays its waiting actions.
     */
    bool isStopWalk() const { return mIsStopWalk; }

protected:
    KuriboTower* mHost = nullptr;
    bool mIsStopWalk = false;
    void* _158 = nullptr;
    void* _160 = nullptr;
};

static_assert(sizeof(KuriboTowerNode) == 0x168);
