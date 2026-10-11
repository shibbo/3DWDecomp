#include "MapObj/ChainModel.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(ChainModel, Wait);
    NERVE_DECL(ChainModel, Fall);
    NERVES_MAKE_NOSTRUCT(ChainModel, Wait, Fall)
}

ChainModel::ChainModel(const char* pName, const char* pArchiveName, const char* pSuffix)
    : al::LiveActor(pName), mArchiveName(pArchiveName), mSuffix(pSuffix) {}

ChainModel::~ChainModel() {}

void ChainModel::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, mArchiveName, mSuffix);
    al::initNerve(this, &NrvChainModelWait, 0);
    al::invalidateClipping(this);
    makeActorAppeared();
}

void ChainModel::exeWait() {
    if (al::isFirstStep(this))
        al::tryStartAction(this, "Wait");
}

void ChainModel::exeFall() {
    if (al::isFirstStep(this))
        mHasFallAction = al::tryStartAction(this, "BlowDown");
    al::addVelocityToGravity(this, 2.0f);
    al::scaleVelocity(this, 0.99f);
    if (isFallEnd()) {
        al::startHitReactionDeath(this);
        kill();
    }
}

bool ChainModel::isFallEnd() const {
    if (mHasFallAction)
        return al::isActionEnd(this);
    return al::isGreaterEqualStep(this, 60);
}

void ChainModel::startFall(const sead::Vector3f& rVelocity) {
    al::setVelocity(this, rVelocity);
    al::setNerve(this, &NrvChainModelFall);
    al::invalidateClipping(this);
}
