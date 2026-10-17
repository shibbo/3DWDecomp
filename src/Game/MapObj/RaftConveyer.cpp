#include "MapObj/RaftConveyer.hpp"
#include "MapObj/RaftKeyKeeper.hpp"
#include "MapObj/RaftStep.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include <basis/seadRawPrint.h>
RaftConveyer::RaftConveyer(const char* pName) : al::LiveActor(pName) {}
RaftConveyer::~RaftConveyer() {}
void RaftConveyer::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorMapObjMovement(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    mKeyKeeper = new RaftKeyKeeper();
    mKeyKeeper->init(rInfo);
    al::tryGetArg(&mPartsInterval, rInfo, "PartsInterval");
    if (mPartsInterval < 10.0f)
        mPartsInterval = 10.0f;
    float length = mKeyKeeper->getTotalLength();
    bool validPath = mKeyKeeper->getKeyCount() <= 1 || !al::isNearZero(length, 0.001f);
    SEAD_ASSERT(validPath);
    float interval = mPartsInterval;
    int intervals = static_cast<int>(length / interval);
    int count = intervals + 3;
    auto* group = new al::DeriveActorGroup<RaftStep>("いかだ足場リスト", count);
    mRafts = group;
    for (int i = 0; i < group->mMaxActors; ++i) {
        auto* raft = new RaftStep("いかだ足場");
        al::initCreateActorWithPlacementInfo(raft, rInfo);
        group->registerActor(raft);
    }
    mAvailableRafts.allocBuffer(count, nullptr);
    float endCoord = interval * (intervals + 1);
    for (int i = 0; i < count; ++i) {
        auto* raft = mRafts->getDeriveActor(i);
        raft->setConveyer(this);
        raft->setKeyKeeper(mKeyKeeper);
        raft->setEndCoord(endCoord);
        mAvailableRafts.pushBack(raft);
    }
    for (int i = 0; i <= intervals; ++i) {
        if (mPartsInterval * i >= mKeyKeeper->getAppearEndCoord())
            break;
        auto* raft = mAvailableRafts.popFront();
        raft->appear();
        raft->forceSetCurrentCoord(mPartsInterval * i);
        if (i == 0)
            mLastRaft = raft;
    }
    float radius = 0.0f;
    mKeyKeeper->calcClippingSphere(&mClippingCenter, &radius, al::getClippingRadius(mRafts->getDeriveActor(0)));
    al::setClippingInfo(this, radius, &mClippingCenter);
    makeActorAppeared();
}
void RaftConveyer::control() {
    int count = mRafts->mNumActors;
    for (int i = 0; i < count; ++i) {
        auto* raft = mRafts->getDeriveActor(i);
        if (!al::isDead(raft)) {
            raft->update();
            if (al::isDead(raft))
                mAvailableRafts.pushBack(raft);
        }
    }
    if (mLastRaft->getCurrentCoord() >= mPartsInterval) {
        auto* raft = mAvailableRafts.popFront();
        raft->appear();
        raft->forceSetCurrentCoord(0.0f);
        mLastRaft = raft;
    }
}
void RaftConveyer::startClipped() {
    al::LiveActor::startClipped();
    for (int i = 0; i < mRafts->mNumActors; ++i)
        al::offDrawClipping(mRafts->getDeriveActor(i));
}
void RaftConveyer::endClipped() {
    al::LiveActor::endClipped();
    for (int i = 0; i < mRafts->mNumActors; ++i)
        al::onDrawClipping(mRafts->getDeriveActor(i));
}
