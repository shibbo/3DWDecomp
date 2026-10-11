#include "Demo/DemoStartPosition.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"

/**
 * @brief Creates a demo start marker with an identity local transform.
 * @param pName Actor name.
 */
DemoStartPosition::DemoStartPosition(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the marker's pose, movement executor, and matrix connector.
 * @param rInfo Actor scene and placement initialization data.
 */
void DemoStartPosition::init(const al::ActorInitInfo& rInfo) {
    al::initExecutorUpdate(this, rInfo, "地形オブジェ[Movement]");
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRMSV(this);
    al::initActorSRT(this, rInfo);
    mConnector = new al::MtxConnector;
    makeActorAppeared();
}

/** @brief Attaches the marker to collision geometry after placement. */
void DemoStartPosition::initAfterPlacement() {
    al::attachMtxConnectorToCollisionRT(mConnector, this, false, false);
}

/** @brief Updates the marker pose from its connector and local transform. */
void DemoStartPosition::control() {
    al::connectPoseMtx(this, mConnector, mLocalMtx);
}
