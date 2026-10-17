#include "MapObj/IllustItem.hpp"
#include "MapObj/IllustItemKeeper.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/ItemAssistRotateParam.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(IllustItem, Wait);
NERVE_DECL(IllustItem, SpinDrc);
NERVE_DECL(IllustItem, Got);
NERVE_DECL(IllustItem, Appear);
NERVES_MAKE_STRUCT(IllustItem, Wait, SpinDrc, Got, Appear)
const float sIllustShadowLengths[] = {160.0f, 500.0f, 1600.0f};
const ItemAssistRotateParam sIllustRotateParam(60, 12.0f, true, 1.7f, 120);
}
IllustItem::IllustItem(const char* name, bool disableAssist) : al::LiveActor(name), mDisableAssist(disableAssist) {}
IllustItem::~IllustItem() {}
void IllustItem::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    if (GameDataFunction::isAcquireIllustItem(GameDataHolderAccessor(this))) {
        al::initActorWithArchiveName(this, info, "IllustItemEmpty", nullptr);
        mAlreadyAcquired = true;
    } else {
        al::initActorWithArchiveName(this, info, "IllustItem", nullptr);
        mAlreadyAcquired = false;
    }
    al::initNerve(this, &NrvIllustItem.Wait, 1);
    rc::declareIllustItem(this);
    al::tryGetArg(&mInRouteDokan, info, "IsPlacementInRouteDokan");
    al::updateEffectMaterialRouteDokan(this, mInRouteDokan);
    makeActorAppeared();
    if (al::listenStageSwitchOnAppear(this, al::FunctorV0M(this, &IllustItem::startAppear))) makeActorDead();
    mAssistRotate = new ItemStateAssistRotate(this, &sIllustRotateParam);
    mAssistRotate->setRotateDegreePtr(&mRotate);
    al::initNerveState(this, mAssistRotate, &NrvIllustItem.SpinDrc, "DRCå›žè»¢");
    int shadowLength = 1;
    al::tryGetArg(&shadowLength, info, "ShadowLength");
    al::setShadowDropLength(this, sIllustShadowLengths[shadowLength], "Body");
    bool expandClipping = false;
    al::tryGetArg(&expandClipping, info, "IsExpandClippingShadowLength");
    if (expandClipping) al::tryExpandClippingByShadowLength(this, &mClippingCenter);
}
void IllustItem::startAppear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvIllustItem.Appear);
    al::invalidateHitSensors(this);
    al::setVelocityY(this, 8.0f);
    al::startAction(this, "Appear");
}
void IllustItem::initAfterPlacement() { al::updateMaterialCodeWater(this); }
void IllustItem::appear() { al::LiveActor::appear(); al::startAction(this, "Wait"); }
bool IllustItem::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvIllustItem.Got)) return false;
    if (!isEnableMsgItemGet(msg) || !al::isAlive(this)) return false;
    mGot = true;
    al::startHitReactionGet(this);
    rc::sendMsgRequestPlayerGetReaction(sender, receiver, "ã‚¤ãƒ©ã‚¹ãƒˆã‚¢ã‚¤ãƒ†ãƒ ã‚²ãƒƒãƒˆ");
    if (mAlreadyAcquired) {
        rc::addScore(this, sender, 0.0f, 0);
        doGet();
    } else {
        if (al::isSensorPlayer(sender)) {
            GameDataFunction::setStampPickupCharType(GameDataHolderWriter(this), rc::getPlayerCharaType(al::getSensorHost(sender)));
        }
        mCollector = sender;
        al::invalidateClipping(this);
        al::setNerve(this, &NrvIllustItem.Got);
    }
    return true;
}
bool IllustItem::isEnableMsgItemGet(const al::SensorMsg* msg) const {
    if (mInRouteDokan) return rc::isMsgRouteDokanItemGet(msg);
    return al::isMsgItemGetAll(msg);
}
void IllustItem::doGet() { rc::acquireIllustItem(this); al::killPrePassLight(this, "Body", -1); kill(); }
bool IllustItem::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer*, al::ScreenPointTarget*) {
    if (mDisableAssist) return false;
    if (al::isNerve(this, &NrvIllustItem.Got)) return false;
    if (al::isNerve(this, &NrvIllustItem.Appear)) return false;
    if (al::isMsgTouchAssistNoPat(msg)) { al::setNerve(this, &NrvIllustItem.SpinDrc); return true; }
    return false;
}
void IllustItem::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (mDisableAssist) mRotate += 1.7f;
    else mRotate = 1.7f;
    mRotate = al::modf(mRotate + 360.0f, 360.0f) + 0.0f;
    al::rotateQuatYDirDegree(this, al::getQuat(this), mRotate);
    if (!mDisableAssist && al::isMicInputOn(this)) al::setNerve(this, &NrvIllustItem.SpinDrc);
}
void IllustItem::exeAppear() {
    if (al::isStep(this, 60)) al::validateHitSensors(this);
    if (al::isGreaterEqualStep(this, 60)) al::setVelocityZero(this);
    else al::addVelocityY(this, -0.25f);
    if (al::isActionEnd(this)) al::setNerve(this, &NrvIllustItem.Wait);
}
void IllustItem::exeSpinDrc() {
    if (al::isFirstStep(this)) {
        if (!al::isActionPlaying(this, "Wait")) al::startAction(this, "Wait");
        al::tryDeleteEmitterAndParticleAll(this);
        al::setSklAnimFrameRate(this, 0.0f, 0);
        al::tryEmitEffect(this, "Touch", nullptr);
        mAssistRotate->setRotateDegree(mRotate);
        if (!mSpinSoundPlayed) { al::startSe(this, "PgSpin", nullptr); mSpinSoundPlayed = true; }
    }
    if (al::isStep(this, 2)) mSpinSoundPlayed = false;
    if (al::updateNerveState(this)) {
        al::tryDeleteEffect(this, "Touch");
        al::setSklAnimFrameRate(this, 1.0f, 0);
        al::setNerve(this, &NrvIllustItem.Wait);
    }
}
void IllustItem::exeGot() {
    if (al::isFirstStep(this)) { al::startAction(this, "Got"); rc::addScore(this, mCollector, 0.0f, 0); }
    if (al::isActionEnd(this)) doGet();
}
