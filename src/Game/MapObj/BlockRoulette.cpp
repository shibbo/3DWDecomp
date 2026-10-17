#include "MapObj/BlockRoulette.hpp"
#include "MapObj/BlockRouletteState.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include "Util/DrcUtil.hpp"
namespace {
    NERVE_DECL(BlockRoulette, Wait);
    NERVE_DECL(BlockRoulette, AppearItem);
    NERVES_MAKE_NOSTRUCT(BlockRoulette, Wait, AppearItem)
}
BlockRoulette::BlockRoulette(const char* name) : al::LiveActor(name) {}
BlockRoulette::~BlockRoulette() {}
void BlockRoulette::init(const al::ActorInitInfo& info) {
    const char* suffix = rc::getBlockSuffixName(info, false);
    if (!suffix) suffix = al::isSingleMode(info) ? "SM" : nullptr;
    al::initActorWithArchiveName(this, info, "BlockRouletteOuter", suffix);
    al::initNerve(this, &NrvBlockRouletteWait, 1);
    mState = new BlockRouletteState(this, info);
    al::initNerveState(this, mState, &NrvBlockRouletteWait, "BlockRoulette");
    bool expandShadow = false;
    al::tryGetArg(&expandShadow, info, "IsExpandClippingShadowLength");
    if (expandShadow) al::tryExpandClippingByShadowLength(this, &mClippingCenter);
    float shadowLength = -1.0f;
    al::tryGetArg(&shadowLength, info, "ShadowLength");
    if (shadowLength > 0.0f) al::setShadowDropLength(this, shadowLength, "ブロック影");
    mInnerModel = al::getSubActor(this, "内側モデル");
    al::tryListenStageSwitchAppear(this);
}
void BlockRoulette::respawn() {
    if (!al::isNerve(this, &NrvBlockRouletteWait) || al::isDead(this)) {
        al::setNerve(this, &NrvBlockRouletteWait);
        mState->reset();
        if (al::isDead(this)) makeActorAppeared();
    }
}
bool BlockRoulette::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgPlayerGiantTouch(msg)) {
        rc::addScoreByFactor(this, sender, "壊れ", 0.0f, 0);
        al::startHitReactionBreak(this);
        kill();
        return true;
    }
    if (mState->receiveMsg(msg, sender, receiver)) {
        rc::addScoreByFactor(this, sender, "アイテム出現", 0.0f, 0);
        if (al::isMsgPlayerHipDropAll(msg)) {
            al::startAction(this, "ReactionHipDrop");
            al::startAction(mInnerModel, "ReactionHipDrop");
        } else {
            al::startAction(this, "Reaction");
            al::startAction(mInnerModel, "Reaction");
        }
        return true;
    }
    return false;
}
bool BlockRoulette::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget* target) {
    if (al::isNerve(this, &NrvBlockRouletteWait)) {
        if (mState->receiveMsgScreenPoint(msg, pointer, target)) {
            rc::addScoreByFactor(this, DrcFunction::tryFindDrcPlayerSensor(this, pointer), "アイテム出現", 0.0f, 0);
            al::startAction(this, "Reaction");
            al::startAction(mInnerModel, "Reaction");
            return true;
        }
        return false;
    }
    return false;
}
void BlockRoulette::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::startAction(mInnerModel, "Wait");
    }
    if (al::updateNerveState(this)) al::setNerve(this, &NrvBlockRouletteAppearItem);
}
void BlockRoulette::exeAppearItem() {}
