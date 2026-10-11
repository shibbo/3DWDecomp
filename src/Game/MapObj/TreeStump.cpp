#include "MapObj/TreeStump.hpp"
#include "MapObj/TreeStumpWatcher.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(TreeStump, Wait);
    NERVE_DECL(TreeStump, ReactionStart);
    NERVE_DECL(TreeStump, Reaction);
    NERVE_DECL(TreeStump, ReactionEnd);
    NERVES_MAKE_NOSTRUCT(TreeStump, Wait, ReactionStart, Reaction, ReactionEnd)
}

TreeStump::TreeStump(const char* pName) : al::LiveActor(pName) {
}

void TreeStump::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTreeStumpWait, 0);
    al::trySyncStageSwitchAppear(this);
}

void TreeStump::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTreeStumpWait);
}

void TreeStump::kill() {
    al::LiveActor::kill();
}

void TreeStump::makeActorDead() {
    al::LiveActor::makeActorDead();
}

void TreeStump::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
}

bool TreeStump::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!al::isSensorCollision(pSelf)) {
        return false;
    }
    if (!al::isMsgPlayerHipDropAll(pMsg)) {
        return false;
    }
    if (isReaction()) {
        return false;
    }
    if (mIsStomped) {
        return false;
    }
    al::setNerve(this, &NrvTreeStumpReactionStart);
    return true;
}

bool TreeStump::isReaction() {
    return al::isNerve(this, &NrvTreeStumpReactionStart) ||
           al::isNerve(this, &NrvTreeStumpReaction) ||
           al::isNerve(this, &NrvTreeStumpReactionEnd);
}

bool TreeStump::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    return false;
}

void TreeStump::exeWait() {
}

void TreeStump::exeReactionStart() {
    al::setNerve(this, &NrvTreeStumpReaction);
}

void TreeStump::exeReaction() {
    sead::Vector3f trans = al::getTrans(this);
    trans.y += -130.0f;
    al::setTrans(this, trans);
    al::setNerve(this, &NrvTreeStumpReactionEnd);
}

void TreeStump::exeReactionEnd() {
    mIsStomped = true;
    if (mWatcher) {
        mWatcher->addStomped();
    }
    al::onStageSwitch(this, "SwitchStumpOn");
    al::setNerve(this, &NrvTreeStumpWait);
}

TreeStump::~TreeStump() {
}
