#include "MapObj/BlockStateHeadgear.hpp"
#include "MapObj/BoxPropeller.hpp"
#include "MapObj/BoxKiller.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(BlockStateHeadgear, Wait);
    NERVE_DECL(BlockStateHeadgear, AppearHeadgear);
    NERVES_MAKE_NOSTRUCT(BlockStateHeadgear, Wait, AppearHeadgear)
}
BlockStateHeadgear::BlockStateHeadgear(al::LiveActor* host, const al::ActorInitInfo& info, int item)
    : al::ActorStateBase("被り物ステート", host) {
    if (item == 19) {
        mPropeller = new BoxPropeller("プロペラボックス");
        al::initCreateActorNoPlacementInfo(mPropeller, info);
    } else if (item == 20) {
        mKiller = new BoxKiller("キラーボックス", 0, 0);
        al::initCreateActorNoPlacementInfo(mKiller, info);
    }
    mSingleMode = al::isSingleMode(info);
}
void BlockStateHeadgear::init() { initNerve(&NrvBlockStateHeadgearWait, 0); }
void BlockStateHeadgear::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvBlockStateHeadgearWait);
}
bool BlockStateHeadgear::reset() {
    if (mKiller) {
        if (mKiller->isCarrying()) return false;
        mKiller->respawn();
    }
    if (mPropeller) {
        if (mPropeller->isCarry()) return false;
        mPropeller->respawn();
    }
    return true;
}
bool BlockStateHeadgear::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender) {
    if (!al::isNerve(this, &NrvBlockStateHeadgearWait)) return false;
    bool hipDrop = msg && al::isMsgPlayerHipDropAll(msg);
    al::setNerve(this, &NrvBlockStateHeadgearAppearHeadgear);
    if (sender && alPlayerFunction::isPlayerActor(sender))
        rc::addScoreByFactor(mHostActor, sender, "アイテム出現", 0.0f, 0);
    if (mPropeller && hipDrop) mPropeller->setAppearFromHipDrop();
    else if (mKiller && hipDrop) mKiller->setAppearFromHipDrop();
    return true;
}
bool BlockStateHeadgear::isAppearHeadgear() { return al::isNerve(this, &NrvBlockStateHeadgearAppearHeadgear); }
void BlockStateHeadgear::exeWait() {
    if (al::isFirstStep(this)) al::startAction(mHostActor, "Wait");
}
void BlockStateHeadgear::exeAppearHeadgear() {
    if (al::isStep(this, 1)) {
        al::LiveActor* headgear = mPropeller ? static_cast<al::LiveActor*>(mPropeller) : mKiller;
        if (!mSingleMode || al::isDead(headgear)) {
            al::copyPose(headgear, mHostActor);
            headgear->appear();
        }
        kill();
    }
}
