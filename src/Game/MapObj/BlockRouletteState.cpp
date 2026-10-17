#include "MapObj/BlockRouletteState.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Screen/ScreenPointKeeper.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(BlockRouletteState, Wait);
    NERVE_DECL(BlockRouletteState, AppearItem);
    NERVES_MAKE_STRUCT(BlockRouletteState, Wait, AppearItem)
}
BlockRouletteState::BlockRouletteState(al::LiveActor* actor, const al::ActorInitInfo& info) : al::ActorStateBase("ルーレットステート", actor) {
    initNerve(&NrvBlockRouletteState.Wait, 0);
    mDummyModels = new al::LiveActor*[6];
    for (int i = 0; i < 6; ++i) mDummyModels[i] = nullptr;
    mScales = new float[6];
    mScales[0] = 0.70f;
    mScales[1] = 0.73f;
    mScales[2] = 0.68f;
    mScales[3] = 0.70f;
    mScales[4] = 0.70f;
    mScales[5] = 0.58f;
    int pattern = 0;
    al::tryGetArg(&pattern, info, "ItemPattern");
    if (pattern < 0 || pattern > 2) pattern = 0;
    bool enableStar = true;
    al::tryGetArg(&enableStar, info, "IsEnableSuperStar");
    bool disableMushroom = false;
    al::tryGetArg(&disableMushroom, info, "IsDisableSuperMushroom");
    mDummyModels[1] = al::getSubActor(mHostActor, "ダミーモデル[スーパーキノコ]");
    mDummyModels[2] = al::getSubActor(mHostActor, "ダミーモデル[スーパーベル]");
    mDummyModels[3] = al::getSubActor(mHostActor, "ダミーモデル[ファイアフラワー]");
    auto* leaf = al::getSubActor(mHostActor, "ダミーモデル[スーパーこのは]");
    auto* boomerang = al::getSubActor(mHostActor, "ダミーモデル[ブーメランフラワー]");
    auto* star = al::getSubActor(mHostActor, "ダミーモデル[スーパースター]");
    if (pattern >= 1) mDummyModels[4] = leaf;
    else leaf->makeActorDead();
    if (pattern >= 2) mDummyModels[5] = boomerang;
    else boomerang->makeActorDead();
    if (enableStar) {
        mDummyModels[0] = star;
        al::killPrePassLight(star, "SuperStar", -1);
    } else star->makeActorDead();
    if (disableMushroom) {
        mDummyModels[1]->makeActorDead();
        mDummyModels[1] = nullptr;
        mCurrentType = getNextType();
    }
    al::killPrePassLightAll(mDummyModels[2], -1);
    for (int i = 0; i < 6; ++i) {
        if (!mDummyModels[i]) continue;
        al::setScaleAll(mDummyModels[i], mScales[i]);
        al::hideModel(mDummyModels[i]);
        al::offCollide(mDummyModels[i]);
        al::invalidateHitSensors(mDummyModels[i]);
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(mHostActor))) {
            if (auto* keeper = mDummyModels[i]->mScreenPointKeeper) keeper->invalidate();
        }
    }
    al::showModel(mDummyModels[getNextType()]);
}
int BlockRouletteState::getNextType() const {
    int type = mCurrentType;
    for (int i = 0; i < 6; ++i) {
        type = al::modi(type + 1 + 6, 6);
        if (mDummyModels[type]) return type;
    }
    return 0;
}
bool BlockRouletteState::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (!al::isNerve(this, &NrvBlockRouletteState.Wait)) return false;
    if (rc::isMsgAskControlUserId(msg, mUserId)) return true;
    if (!rc::isMsgForBlockAll(msg, sender, receiver, 100.0f)) return false;
    if (al::isMsgLaserAttack(msg)) return false;
    appearItem();
    al::setNerve(this, &NrvBlockRouletteState.AppearItem);
    return true;
}
void BlockRouletteState::appearItem() {
    killDummyModel();
    sead::Vector3f trans = al::getTrans(mHostActor);
    sead::Vector3f front(sead::Vector3f::ez);
    al::calcQuatFront(&front, al::getQuat(mHostActor));
    switch (mCurrentType) {
    case 0: al::appearItemTiming(mHostActor, "スーパースター", trans, front); break;
    case 1: al::appearItemTiming(mHostActor, "スーパーキノコ", trans, front); break;
    case 2: al::appearItemTiming(mHostActor, "スーパーベル", trans, front); break;
    case 3: al::appearItemTiming(mHostActor, "ファイアフラワー", trans, front); break;
    case 4: al::appearItemTiming(mHostActor, "スーパーこのは", trans, front); break;
    case 5: al::appearItemTiming(mHostActor, "ブーメランフラワー", trans, front); break;
    }
    al::startHitReaction(mHostActor, "アイテム出現");
}
bool BlockRouletteState::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (!al::isNerve(this, &NrvBlockRouletteState.Wait)) return false;
    if (!al::isMsgTouchAssistTrig(msg)) return false;
    appearItem();
    al::setNerve(this, &NrvBlockRouletteState.AppearItem);
    return true;
}
void BlockRouletteState::exeWait() {
    auto* current = mDummyModels[mCurrentType];
    --mSoundFadeFrames;
    float volume = 1.0f;
    if (mSoundFadeFrames >= 0) volume = sead::Mathf::clamp(1.0f + float(mSoundFadeFrames) / -80.0f, 0.0f, 1.0f);
    else mSoundFadeFrames = 0;
    if (al::getNerveStep(this) % 8 == 0) {
        auto* next = mDummyModels[getNextType()];
        if (al::isHideModel(next)) { al::showModel(next); al::hideShadow(next); }
        if (GameDataFunction::isSingleMode(GameDataHolderAccessor(mHostActor))) al::startSeSetVolumeByName(mHostActor, "Shuffle", volume);
        else al::startSe(mHostActor, "Shuffle", nullptr);
    }
    if (al::isIntervalStep(this, 8, 0)) {
        if (!al::isHideModel(current)) al::hideModel(current);
        mCurrentType = getNextType();
    }
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(mHostActor))) al::holdSeSetPitchVolumeByName(mHostActor, "LvShuffle", 1.0f, volume);
    else al::holdSe(mHostActor, "LvShuffle", nullptr);
}
void BlockRouletteState::exeAppearItem() { kill(); }
void BlockRouletteState::reset() {
    if (al::isNerve(this, &NrvBlockRouletteState.Wait)) return;
    al::setNerve(this, &NrvBlockRouletteState.Wait);
    for (int i = 0; i < 6; ++i) {
        if (!mDummyModels[i]) continue;
        if (al::isDead(mDummyModels[i])) mDummyModels[i]->makeActorAppeared();
        al::hideModelIfShow(mDummyModels[i]);
    }
}
void BlockRouletteState::killDummyModel() {
    for (int i = 0; i < 6; ++i) if (mDummyModels[i]) mDummyModels[i]->kill();
}
