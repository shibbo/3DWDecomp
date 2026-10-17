#include "MapObj/BlockBrick.hpp"
#include "MapObj/BlockStateItem.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(BlockBrick, State);
    NERVE_DECL(BlockBrick, Empty);
    NERVES_MAKE_NOSTRUCT(BlockBrick, State, Empty)
}
BlockBrick::BlockBrick(const char* name) : al::LiveActor(name) {}
void BlockBrick::init(const al::ActorInitInfo& info) {
    mSingleMode = info.getActorSceneInfo().isSingleMode;
    const char* modelName = "BlockBrick";
    alPlacementFunction::tryGetModelName(&modelName, info);
    const char* suffix = rc::getBlockSuffixName(info, mSingleMode);
    if (!suffix) suffix = mSingleMode ? "SM" : nullptr;
    al::initActorWithArchiveName(this, info, modelName, suffix);
    al::initNerve(this, &NrvBlockBrickState, 1);
    mState = new BlockStateItem(this, info, false, false, true, false, false);
    al::initNerveState(this, mState, &NrvBlockBrickState, "BlockItem");
    al::offCollide(this);
    {
        al::StringTmp<128> breakName("%sBreak", modelName);
        mBreakModel = new al::BreakModel(this, "レンガブロック壊れモデル", breakName.cstr(), nullptr, nullptr, "Break", true);
        al::initCreateActorNoPlacementInfo(mBreakModel, info);
    }
    float shadowLength = -1.0f;
    al::tryGetArg(&shadowLength, info, "ShadowLength");
    if (shadowLength > 0.0f) al::setShadowDropLength(this, shadowLength, "シャドウマスク");
    bool expandClipping = false;
    al::tryGetArg(&expandClipping, info, "IsExpandClippingShadowLength");
    if (expandClipping) al::tryExpandClippingByShadowLength(this, &mClippingCenter);
    makeActorAppeared();
}
void BlockBrick::reappear() {
    if (mSingleMode) {
        al::validateHitSensors(this);
        al::setNerve(this, &NrvBlockBrickState);
        mState->reset();
    }
    al::LiveActor::appear();
}
void BlockBrick::respawn() {
    if (!al::isNerve(this, &NrvBlockBrickState) || al::isDead(this)) {
        al::validateHitSensors(this);
        al::setNerve(this, &NrvBlockBrickState);
        mState->reset();
        if (al::isDead(this)) al::LiveActor::appear();
    }
}
void BlockBrick::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) { mState->attackSensor(sender, receiver); }
bool BlockBrick::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isMsgPlayerGiantTouch(msg) || al::isMsgLaserAttack(msg)) && al::isNerve(this, &NrvBlockBrickState))
        tryAppearBreakModel();
    return mState->receiveMsg(msg, sender, receiver);
}
void BlockBrick::tryAppearBreakModel() { al::appearBreakModelRandomRotateY(mBreakModel); }
bool BlockBrick::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    return mState->receiveMsgScreenPoint(msg, pointer);
}
BlockStateItem* BlockBrick::getBlockStateItem() const { return mState; }
void BlockBrick::onConnectRailBlock() {
    mConnectedRail = true;
    al::setShadowFixed(this, false);
    mState->setConnectedRailBlock();
}
void BlockBrick::exeState() {
    if (al::updateNerveState(this)) {
        if (mConnectedRail) {
            al::hideModel(this);
            al::setNerve(this, &NrvBlockBrickEmpty);
        } else kill();
    }
}
void BlockBrick::exeEmpty() {
    if (al::isFirstStep(this)) al::invalidateHitSensors(this);
    al::copyPose(mState->getBlockEmpty(), this);
}
