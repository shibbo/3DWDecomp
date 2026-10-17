#include "MapObj/BlockEmpty.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    NERVE_DECL(BlockEmpty, Wait);
    NERVES_MAKE_NOSTRUCT(BlockEmpty, Wait)
}

BlockEmpty::BlockEmpty(const char* pName, const char* pArchiveName)
    : al::LiveActor(pName), mArchiveName(pArchiveName) {}
BlockEmpty::~BlockEmpty() {}

void BlockEmpty::init(const al::ActorInitInfo& rInfo) {
    const char* suffix = rc::getBlockSuffixName(rInfo, al::isSingleMode(rInfo));
    if (!suffix)
        suffix = al::isSingleMode(rInfo) ? "SM" : nullptr;
    al::initActorWithArchiveName(this, rInfo, mArchiveName, suffix);
    al::initNerve(this, &NrvBlockEmptyWait, 0);
    makeActorAppeared();
    bool expandClipping = false;
    al::tryGetArg(&expandClipping, rInfo, "IsExpandClippingShadowLength");
    if (expandClipping)
        al::tryExpandClippingByShadowLength(this, &mClippingOffset);
    float shadowLength = -1.0f;
    al::tryGetArg(&shadowLength, rInfo, "ShadowLength");
    if (shadowLength > 0.0f)
        al::setShadowDropLength(this, shadowLength, "シャドウマスク");
    al::updateMaterialCodeWater(this);
}

bool BlockEmpty::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgRaidonBreakLightReaction(pMsg))
        return true;
    if ((al::isMsgPlayerGiantTouch(pMsg) || al::isMsgLaserAttack(pMsg)) && !mPreventGiantBreak) {
        rc::addScore(this, pOther, 0.0f, 0);
        al::startHitReactionBreak(this);
        kill();
        return true;
    }
    if (rc::isMsgForBlockAll(pMsg, pOther, pSelf, 100.0f) && al::isSensorPlayer(pOther) &&
        (!GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) || !rc::isPlayerInWater(pOther)) &&
        mHitCooldown == 0) {
        al::startHitReactionHit(this);
        if (al::isMsgPlayerUpperPunch(pMsg)) {
            mHitCooldown = 5;
            return false;
        }
        mHitCooldown = 30;
    }
    return false;
}

void BlockEmpty::exeWait() {
    if (al::isStep(this, 3) && !mIsConnectedRailBlock)
        al::setNeedSetBaseMtxAndCalcAnimFlag(this, false);
    if (mHitCooldown - 1 >= 0)
        --mHitCooldown;
}

void BlockEmpty::updateLinkedTrans(const sead::Vector3f& trans) {
    al::setNeedSetBaseMtxAndCalcAnimFlag(this, true);
    al::LiveActor::updateLinkedTrans(trans);
    al::setNerve(this, &NrvBlockEmptyWait);
}

void BlockEmpty::onConnectRailBlock() {
    al::setShadowFixed(this, false);
    mIsConnectedRailBlock = true;
}
