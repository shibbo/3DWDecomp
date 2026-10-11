#include "MapObj/NeedleTrap.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
namespace {
    class NeedleTrapNrvStandby : public al::Nerve {
    public:
        void execute(al::NerveKeeper* keeper) const override { keeper->getParent<NeedleTrap>()->exeWait(); }
    };
    NERVE_DECL(NeedleTrap, Stop);
    NERVE_DECL(NeedleTrap, Sign);
    NERVE_DECL(NeedleTrap, Attack);
    NERVE_DECL(NeedleTrap, End);
    NERVE_DECL(NeedleTrap, Wait);
    NERVES_MAKE_NOSTRUCT(NeedleTrap, Standby, Stop, Sign, Attack, End, Wait)
}
NeedleTrap::NeedleTrap(const char* name) : al::LiveActor(name) {}
NeedleTrap::~NeedleTrap() {}
void NeedleTrap::init(const al::ActorInitInfo& info) {
    al::initMapPartsActor(this, info, nullptr, 0);
    al::initNerve(this, &NrvNeedleTrapStandby, 0);
    al::tryGetArg(&mWaitTime, info, "WaitTime");
    al::tryGetArg(&mAttackTime, info, "AttackTime");
    al::tryGetArg(&mStandbyTime, info, "StandbyTime");
    mConnector = al::createMtxConnector(this);
    al::initSubActorKeeperNoFile(this, info, 1);
    mNeedleCollision = al::createCollisionObj(this, info, "Needle", al::getHitSensor(this, "CollisionBase"), "ColPosition", nullptr);
    al::registerSubActorSyncClipping(this, mNeedleCollision, false);
    mHasStartSwitch = al::isValidStageSwitch(this, "SwitchStart");
    mHasKeepOnSwitch = al::isValidStageSwitch(this, "SwitchKeepOn");
    if (mHasKeepOnSwitch || mHasStartSwitch) al::setNerve(this, &NrvNeedleTrapStop);
    makeActorAppeared();
    mNeedleCollision->makeActorAppeared();
    al::invalidateCollisionParts(mNeedleCollision);
}
void NeedleTrap::initAfterPlacement() { al::attachMtxConnectorToCollision(mConnector, this, false); }
void NeedleTrap::control() {
    al::connectPoseQT(this, mConnector);
    if (al::isOnStageSwitch(this, "SwitchStop") && !al::isNerve(this, &NrvNeedleTrapStop)) {
        al::setNerve(this, &NrvNeedleTrapStop);
        return;
    }
    if ((mHasStartSwitch || mHasKeepOnSwitch) && (al::isOnStageSwitch(this, "SwitchStart") || al::isOnStageSwitch(this, "SwitchKeepOn")) && al::isNerve(this, &NrvNeedleTrapStop))
        al::setNerve(this, &NrvNeedleTrapStandby);
}
void NeedleTrap::exeStop() {
    if (al::isFirstStep(this)) {
        al::invalidateCollisionParts(mNeedleCollision);
        al::startAction(this, "Wait");
    }
}
void NeedleTrap::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (al::isNerve(this, &NrvNeedleTrapStandby)) {
        if (al::isGreaterEqualStep(this, mStandbyTime)) al::setNerve(this, &NrvNeedleTrapSign);
    } else if (al::isGreaterEqualStep(this, mWaitTime)) al::setNerve(this, &NrvNeedleTrapSign);
}
void NeedleTrap::exeSign() {
    if (al::isFirstStep(this)) al::startAction(this, "Sign");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvNeedleTrapAttack);
}
void NeedleTrap::exeAttack() {
    if (al::isFirstStep(this)) {
        al::validateCollisionParts(mNeedleCollision);
        al::startAction(this, "Attack");
    }
    if (mHasKeepOnSwitch) {
        if (al::isGreaterEqualStep(this, mAttackTime) && !al::isOnStageSwitch(this, "SwitchKeepOn")) al::setNerve(this, &NrvNeedleTrapEnd);
    } else if (al::isGreaterEqualStep(this, mAttackTime)) al::setNerve(this, &NrvNeedleTrapEnd);
}
void NeedleTrap::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End");
    if (al::isActionEnd(this)) {
        if (mHasKeepOnSwitch) al::setNerve(this, &NrvNeedleTrapStop);
        else {
            al::invalidateCollisionParts(mNeedleCollision);
            al::setNerve(this, &NrvNeedleTrapWait);
        }
    }
}
