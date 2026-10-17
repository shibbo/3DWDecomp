#include "MapObj/SuperStar.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/ItemStatePopUpFront.hpp"
#include "MapObj/Fury/CloudBonusWatcher.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Light/LightIntensityDirector.hpp"
#include "Library/Light/LightIntensityFunction.hpp"
#include "Library/Movement/RumbleCalculator.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/PostProcessing/OccludedEffectDirector.hpp"
#include "Library/PostProcessing/OccludedEffectRequestInfo.hpp"
#include "Library/PostProcessing/OfxFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/MapObj/SupportFreezeSyncGroupHolder.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(SuperStar, Wait);
NERVE_DECL(SuperStar, PopUpFront);
NERVE_DECL(SuperStar, AttachBubble);
NERVE_DECL(SuperStar, Runaway);
NERVES_MAKE_STRUCT(SuperStar, Wait, PopUpFront, AttachBubble)
NERVES_MAKE_NOSTRUCT(SuperStar, Runaway)
}
SuperStar::SuperStar(const char* name, ItemBubble* bubble) : al::LiveActor(name), mBubble(bubble) {}
SuperStar::~SuperStar() {}
void SuperStar::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "SuperStar", nullptr);
    al::initNerve(this, &NrvSuperStar.Wait, 1);
    mPopUpFront = new ItemStatePopUpFront(this);
    al::initNerveState(this, mPopUpFront, &NrvSuperStar.PopUpFront, "跳ね上げ(前方)");
    mRumble = new al::RumbleCalculatorCosMultLinear(2.5f, 1.5707963705062866f, 0.2f, 30);
    al::tryGetArg(&mIsInRouteDokan, info, "IsPlacementInRouteDokan");
    al::updateEffectMaterialRouteDokan(this, mIsInRouteDokan);
    if (mBubble) al::setNerve(this, &NrvSuperStar.AttachBubble);
    al::tryGetArg(&mUsingOccludedEffect, info, "UsingOccludedEffect");
    if (mUsingOccludedEffect) mOccludedEffect = OfxFunction::getOccludedEffectDirector(this)->createInfoByPresetName("ObjSmall");
    mIsSingleMode = info.getActorSceneInfo().isSingleMode;
    if (mIsSingleMode) mPlacementIndex = info.getPlacementInfo()._28;
    makeActorAppeared();
}
void SuperStar::initAfterPlacement() {
    al::updateMaterialCodeArea(this);
    if (mIsSingleMode) {
        auto* watcher = static_cast<CloudBonusWatcher*>(al::getSceneObj(this, 52));
        if (watcher) watcher->tryRegisterActor(this, mPlacementIndex);
    }
}
void SuperStar::appear() {
    al::LiveActor::appear();
    if (mBubble) al::setNerve(this, &NrvSuperStar.AttachBubble);
    else al::setNerve(this, &NrvSuperStar.Wait);
}
void SuperStar::reappear() { appear(); }
void SuperStar::control() {
    float exposure = LightIntensityFunction::getLightIntensityDirector(this)->getExposure();
    if (exposure < 2.0f) al::startVisAnimAndSetFrameAndStop(this, "SuperStar", 0.0f);
    else al::startVisAnimAndSetFrameAndStop(this, "SuperStar", 1.0f);
    if (!mRumble->isEnd()) { mRumble->calc(); al::setScaleY(this, mRumble->getValueY() + 1.0f); }
    else al::setScaleY(this, 1.0f);
    if (mUsingOccludedEffect && !al::isNerve(this, &NrvSuperStar.PopUpFront)) mOccludedEffect->requestByPos(al::getTrans(this));
    al::updateMaterialCodeWater(this);
}
bool SuperStar::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if ((al::isNerve(this, &NrvSuperStar.AttachBubble) || al::isNerve(this, &NrvSuperStar.Wait)) && rc::isMsgItemBubbleBreak(msg)) {
        al::setNerve(this, &NrvSuperStar.PopUpFront); return true;
    }
    if (al::isNerve(this, &NrvSuperStar.AttachBubble)) {
        if (!rc::isMsgItemBubbleBreakAndGetItem(msg)) return false;
        if (!mBubble->isEnableGetPlayerSensor()) return false;
        sender = mBubble->getHitPlayerSensor();
        rc::addScore(this, sender, 0.0f, 0);
        al::startHitReactionGet(this);
        rc::tryChangeToInvincibleMario(sender);
        kill(); return true;
    }
    if (al::isMsgPlayerFireBallAttack(msg) || al::isMsgPlayerBoomerangAttack(msg)) {
        if (mRumble->isEnd()) { al::startHitReactionHit(this); rc::requestHitReactionToAttacker(msg, receiver, sender); mRumble->start(0); }
        return al::isMsgPlayerFireBallAttack(msg);
    }
    if (!isEnableMsgItemGet(msg)) return false;
    if (!al::isSensorPlayer(sender) && !al::isSensorRide(sender)) return false;
    bool allPlayers = al::isMsgRideAllPlayerItemGet(msg);
    rc::addScore(this, sender, 0.0f, 0);
    al::startHitReactionGet(this);
    if (allPlayers) rc::tryChangeToInvincibleAllPlayer(this, false);
    else rc::tryChangeToInvincibleMario(sender);
    kill(); return true;
}
bool SuperStar::isEnableMsgItemGet(const al::SensorMsg* msg) const {
    if (mIsInRouteDokan) return rc::isMsgRouteDokanItemGet(msg);
    return al::isMsgItemGetDirectAll(msg);
}
bool SuperStar::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvSuperStar.AttachBubble)) return false;
    return al::isMsgTouchAssist(msg);
}
void SuperStar::appearPopUpFront() {
    al::invalidateHitSensors(this); al::invalidateClipping(this); al::hideModelIfShow(this); appear();
    mPopUpFront->setParamInvalidateClippingOnKill(false);
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvSuperStar.PopUpFront);
}
void SuperStar::appearItemTakeOut() {
    al::invalidateHitSensors(this); al::invalidateClipping(this); al::hideModelIfShow(this); appear();
    mPopUpFront->setParamInvalidateClippingOnKill(true);
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvSuperStar.PopUpFront);
}
void SuperStar::appearItemHoming(const al::HitSensor* target) {
    al::invalidateHitSensors(this); al::invalidateClipping(this); al::hideModelIfShow(this); appear();
    mPopUpFront->setParamInvalidateClippingOnKill(true);
    mPopUpFront->setParamHoming(target);
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && rc::isPlayerOnRaidon(target)) mPopUpFront->setParamInvalidateKillByArea(true);
    if (mPopUpOnCollide) mPopUpFront->setParamOnCollide();
    al::setNerve(this, &NrvSuperStar.PopUpFront);
}
void SuperStar::setPopUpFrontParam(const ItemStatePopUpFrontParam* param) { mPopUpFront->setParam(*param, nullptr); }
void SuperStar::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); }
void SuperStar::exeAttachBubble() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); al::offCollide(this); }
    if (al::isDead(mBubble)) al::setNerve(this, &NrvSuperStar.PopUpFront);
}
void SuperStar::exePopUpFront() {
    al::updateNerveStateAndNextNerve(this, &NrvSuperStarRunaway);
    rc::startHitReactionDeathIfThroughWater(this);
    if (al::isSklAnimPlaying(this, 0) && al::isSklAnimOneTime(this, 0) && al::isActionEnd(this)) al::startAction(this, "Wait");
}
void SuperStar::exeRunaway() {
    if (al::isFirstStep(this)) al::invalidateClipping(this);
    if (EnemyStateUtil::tryKillByAreaOrMaterialCodeWithHitReaction(this)) return;
    al::addVelocityToGravity(this, 0.5f);
    al::limitVelocitySeparateHV(this, al::getGravity(this), 15.0f, 3.0f);
    if (al::isOnGround(this, 0, 0.0f)) {
        sead::Vector3f direction(al::getVelocity(this));
        if (al::normalizeOrZero(&direction)) al::calcFrontDir(&direction, this);
        al::setVelocitySeparateHV(this, direction, 3.0f, 15.0f);
        al::startHitReaction(this, "着地");
    }
    al::reboundVelocityFromCollision(this, 1.0f, 0.0f, 1.0f);
    rc::startHitReactionDeathIfThroughWater(this);
}
namespace al {
bool registSupportFreezeSyncGroup(LiveActor* actor, const ActorInitInfo& info) {
    if (!actor->getHitSensorKeeper()) return false;
    if (!alPlacementFunction::isEnableLinkGroupId(info, "SupportFreezeSyncGroup")) return false;
    static_cast<SupportFreezeSyncGroupHolder*>(createSceneObj(actor, 22))->regist(actor, info);
    return true;
}
}
