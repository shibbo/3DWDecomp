#include "MapObj/FlowerRhythm.hpp"
#include "MapObj/BgmRhythmAnimeController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/DrcUtil.hpp"
namespace {
NERVE_DECL(FlowerRhythm, Wait);
NERVE_DECL(FlowerRhythm, Reaction);
NERVE_DECL(FlowerRhythm, ReactionAttack);
NERVES_MAKE_STRUCT(FlowerRhythm, Wait, Reaction, ReactionAttack)
const sead::Vector3f sFlowerItemOffset(0.0f, 80.0f, 0.0f);
}
FlowerRhythm::FlowerRhythm(const char* name) : al::LiveActor(name), mTrace(new al::LiveActor("リズム花[根元]")) {}
FlowerRhythm::~FlowerRhythm() {}
void FlowerRhythm::init(const al::ActorInitInfo& info) {
    bool moving = false;
    al::tryGetArg(&moving, info, "IsMoving");
    al::initMapPartsActor(this, info, moving ? "Moving" : nullptr, 0);
    al::initNerve(this, &NrvFlowerRhythm.Wait, 0);
    al::StringTmp<64> model;
    al::StringTmp<64> path;
    al::makeMapPartsModelName(&model, &path, info);
    if (al::isEqualString(model.cstr(), "FlowerPaw")) al::initActorWithArchiveName(mTrace, info, "FlowerPawTrace", nullptr);
    else al::initActorWithArchiveName(mTrace, info, "FlowerRhythmTrace", nullptr);
    mTrace->makeActorDead();
    const char* item = "None";
    if (al::tryGetStringArg(&item, info, "ItemType") && item && !al::isEqualString(item, "None")) {
        mItemType = item;
        al::setAppearItemOffset(this, sFlowerItemOffset);
    }
    if (al::isEqualString(model.cstr(), "FlowerRhythm")) {
        int color = 0;
        al::tryGetArg(&color, info, "FlowerRhythmColor");
        al::startMtpAnimAndSetFrameAndStop(this, "Color", float(color));
    } else if (al::isEqualString(model.cstr(), "FlowerPoinsettia")) {
        int color = 0;
        al::tryGetArg(&color, info, "FlowerPoinsettiaColor");
        al::startMtpAnimAndSetFrameAndStop(this, "Color", float(color));
    } else if (al::isEqualString(model.cstr(), "FlowerPaw")) {
        int color = 0;
        mIsPaw = true;
        al::tryGetArg(&color, info, "FlowerPawPhaseColor");
        al::startMtpAnimAndSetFrameAndStop(this, "FlowerPawColor", float(color));
        al::startMtpAnimAndSetFrameAndStop(mTrace, "FlowerPawColor", float(color));
    }
    if (al::isEqualString(model.cstr(), "FlowerDandelion")) {
        mCottonGone = false;
        al::emitEffect(al::getSubActor(this, 0), "Cotton", nullptr);
    }
    bool rotate = false;
    if (al::tryGetArg(&rotate, info, "IsRotateY") && rotate) {
        float rotation = al::getRandom(-25.0f, 25.0f);
        al::rotateQuatYDirDegree(this, rotation);
        al::rotateQuatYDirDegree(mTrace, rotation);
    }
    bool reverse = false;
    al::tryGetArg(&reverse, info, "IsRhythmAnimReverse");
    mRhythm = new BgmRhythmAnimeController(this, reverse);
    al::setShadowFixed(this, true);
    bool connect = false;
    if (al::tryGetArg(&connect, info, "IsConnectCollision") && connect) mConnector = al::createMtxConnector(this);
    makeActorAppeared();
}
void FlowerRhythm::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, false);
}
void FlowerRhythm::control() {
    if (mConnector) al::connectPoseQT(this, mConnector);
    if (!mCottonGone && !al::isNerve(this, &NrvFlowerRhythm.Reaction) && al::isMicInputOn(this)) {
        if (!((al::getTrans(this) - al::findNearestPlayerPos(this)).length() > 1500.0f)) {
            mReactionCooldown = 0;
            al::setNerve(this, &NrvFlowerRhythm.Reaction);
            return;
        }
    }
    if (mReactionCooldown - 1 >= 0) --mReactionCooldown;
    if (al::isNerve(this, &NrvFlowerRhythm.Wait) && mReactionCooldown >= 0 && al::isMicInputOn(this)) al::setNerve(this, &NrvFlowerRhythm.Reaction);
}
bool FlowerRhythm::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (!al::isNerve(this, &NrvFlowerRhythm.Wait)) return false;
    bool disasterAttack = false;
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        if (al::isSensorRide(sender)) disasterAttack = rc::isMsgBobsledBodyAttack(msg);
        else disasterAttack = al::isMsgLaserAttack(msg) || al::isMsgDisasterSpikeAttack(msg);
    }
    if (disasterAttack || al::isMsgEnemyAttackFire(msg) || al::isMsgPlayerFireBallAttack(msg)) { destroy(msg, sender); return false; }
    if (mIsPaw && ((EnemyStateUtil::isMsgBlowDown(msg) && !al::isMsgPlayerGiantAttack(msg)) || al::isMsgBallTrample(msg))) {
        tryAppearItem(msg, sender, true, false);
        al::setNerve(this, &NrvFlowerRhythm.ReactionAttack);
        return false;
    }
    if (!mCottonGone && al::isMsgPlayerCooperationHipDrop(msg)) {
        tryAppearItem(msg, sender, true, false);
        al::setNerve(this, &NrvFlowerRhythm.Reaction);
        return false;
    }
    if (((al::isSensorPlayer(sender) || al::isSensorKoopaJr(sender)) && al::isMsgPlayerItemGet(msg)) || al::isMsgBallTrample(msg) ||
        (EnemyStateUtil::isMsgBlowDown(msg) && !al::isMsgPlayerBodyLanding(msg)) || al::isMsgNpcTouch(msg) || al::isSensorEnemyAttack(sender)) {
        const sead::Vector3f& velocity = al::getActorVelocity(sender);
        if (al::isNearZero(sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z), 0.001f) && mReactionCooldown > 0) {
            mReactionCooldown = 30;
            return false;
        }
        if (!al::isSensorEnemyAttack(sender)) tryAppearItem(msg, sender, true, false);
        al::setNerve(this, &NrvFlowerRhythm.Reaction);
    }
    return false;
}
void FlowerRhythm::destroy(const al::SensorMsg* msg, const al::HitSensor* sender) {
    mTrace->appear();
    tryAppearItem(msg, sender, false, false);
    al::startHitReaction(this, "焼失");
    kill();
}
void FlowerRhythm::tryAppearItem(const al::SensorMsg* msg, const al::HitSensor* sender, bool direct, bool assist) {
    if (!mItemType) return;
    if (assist) {
        auto* sensor = DrcFunction::tryFindDrcPlayerSensor(this, static_cast<const al::HitSensor*>(nullptr));
        if (!sensor) return;
        al::setAppearItemFactor(this, "間接攻撃", sensor);
    } else if (direct) al::setAppearItemFactor(this, "直接攻撃", sender);
    else rc::setAppearItemFactorByMsg(this, msg, sender);
    al::appearItemTiming(this, mItemType);
    mItemType = nullptr;
}
bool FlowerRhythm::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (al::isMsgTouchAssistTrig(msg)) {
        tryAppearItemScreenPointer(msg, pointer);
        al::setNerve(this, &NrvFlowerRhythm.Reaction);
    } else if (!al::isNerve(this, &NrvFlowerRhythm.Reaction) && al::isMsgTouchAssist(msg)) {
        if (mReactionCooldown > 0) mReactionCooldown = 30;
        else { tryAppearItemScreenPointer(msg, pointer); al::setNerve(this, &NrvFlowerRhythm.Reaction); }
    }
    return false;
}
void FlowerRhythm::tryAppearItemScreenPointer(const al::SensorMsg* msg, const al::ScreenPointer* pointer) {
    if (mItemType) { rc::setAppearItemFactorByMsg(this, msg, pointer); al::appearItemTiming(this, mItemType); mItemType = nullptr; }
}
void FlowerRhythm::updateLinkedTrans(const sead::Vector3f& trans) { al::setTrans(this, trans); al::setTrans(mTrace, trans); }
void FlowerRhythm::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); mRhythm->update(); }
void FlowerRhythm::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
        if (!mCottonGone) {
            tryAppearItem(nullptr, nullptr, false, true);
            al::emitEffect(al::getSubActor(this, 0), "ReactionMic", nullptr);
            al::startSe(this, "PgBlowFtuff", nullptr);
            al::deleteEffect(al::getSubActor(this, 0), "Cotton");
            mCottonGone = true;
        }
    }
    if (al::isActionEnd(this)) { mReactionCooldown = 30; al::setNerve(this, &NrvFlowerRhythm.Wait); }
}
void FlowerRhythm::exeReactionAttack() {
    if (al::isFirstStep(this)) { al::startAction(this, "ReactionAttack"); tryAppearItem(nullptr, nullptr, false, true); }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvFlowerRhythm.Wait);
}
