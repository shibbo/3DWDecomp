#include "MapObj/ChikuwaBlock.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

ChikuwaBlock::ChikuwaBlock(const char* pName) : al::FallMapParts(pName) {}

ChikuwaBlock::~ChikuwaBlock() {}

void ChikuwaBlock::init(const al::ActorInitInfo& rInfo) {
    al::FallMapParts::init(rInfo, al::isSingleMode(rInfo) ? "SM" : nullptr);
}

void ChikuwaBlock::switchAppear() {
    al::FallMapParts::switchAppear();
    al::tryDeleteEmitterAndParticleAll(this);
    alLiveActorFunction::forceUpdateTrans(this, al::getTrans(this), false);
    mModelKeeper->update();
}

bool ChikuwaBlock::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                              al::HitSensor* pReceiver) {
    if (al::isMsgPlayerDisregard(pMsg))
        return true;
    if (rc::isMsgGoalKillRunaway(pMsg)) {
        kill();
        return true;
    }
    return al::FallMapParts::receiveMsg(pMsg, pSender, pReceiver);
}
