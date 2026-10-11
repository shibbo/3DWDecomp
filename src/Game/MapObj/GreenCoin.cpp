#include "MapObj/GreenCoin.hpp"
#include "MapObj/GreenRing.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/CoinUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(GreenCoin, Appear);
NERVE_DECL(GreenCoin, Spin);
NERVE_DECL(GreenCoin, CountDown);
NERVES_MAKE_STRUCT(GreenCoin, Appear, Spin, CountDown)
}
GreenCoin::GreenCoin(const char* name) : al::LiveActor(name) {}
GreenCoin::~GreenCoin() {}
void GreenCoin::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    al::initNerve(this, &NrvGreenCoin.Appear, 1);
    mFlashing = new al::FlashingCtrl(this, true, false);
    al::calcFrontDir(&mItemDirection, this);
    mConnector = al::tryCreateMtxConnector(this, info);
    mBaseQuat.set(al::getQuat(this));
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, mBaseQuat);
    if (!al::isNearZero(up.cross(sead::Vector3f(0.0f, 0.0f, 1.0f)), 0.001f))
        al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f(0.0f, 0.0f, 1.0f));
    mAssistRotate = new ItemStateAssistRotate(this, CoinUtil::getCoinAssistRotateParam());
    if (mConnector) mAssistRotate->setConnector(mConnector, mBaseQuat);
    al::initNerveState(this, mAssistRotate, &NrvGreenCoin.Spin, "DRC回転ステート");
    makeActorDead();
}
void GreenCoin::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 300.0f);
}
void GreenCoin::appear() {
    al::LiveActor::appear();
    mFlashing->start(600);
}
void GreenCoin::control() {
    ++mCollectedFrame;
    if (al::isNerve(this, &NrvGreenCoin.CountDown))
        CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvGreenCoin.Spin);
}
void GreenCoin::acquirerCoinGreen() {
    al::startHitReactionGet(this);
    int seParam = mHost->getSeParamNum();
    al::startSeWithParam(this, "プログラムコール", seParam, nullptr);
    kill();
}
bool GreenCoin::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (rc::isMsgPackunEatStart(msg)) return true;
    if (rc::isMsgCoinGet(msg) || rc::isMsgPackunEat(msg)) {
        al::startHitReactionGet(this);
        rc::acquirerItemCoin(this, sender);
        acquirerCoinGreen();
        return true;
    }
    return false;
}
bool GreenCoin::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (rc::tryAcquirerCoinIfTouchAssistTrigger(msg, this, pointer)) {
        acquirerCoinGreen();
        kill();
        return true;
    }
    if (al::isMsgTouchAssistNoPat(msg)) {
        if (!al::isNerve(this, &NrvGreenCoin.Spin)) al::setNerve(this, &NrvGreenCoin.Spin);
        return true;
    }
    return false;
}
int GreenCoin::getCountDownStep() const { return mCollectedFrame; }
int GreenCoin::getTimerFrame() { return 600; }
void GreenCoin::exeAppear() {
    if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, "Wait");
    mFlashing->movement();
    rotate(al::calcNerveEaseOutValue(this, 54, 45.0f, 0.0f));
    al::setNerveAtStep(this, &NrvGreenCoin.CountDown, 54);
}
void GreenCoin::rotate(float speed) {
    mRotateY = al::wrapAngle(mRotateY + rc::getCoinRotateYByFrame(this) + speed);
    if (mConnector) {
        sead::Quatf quat;
        al::rotateQuatYDirDegree(&quat, mBaseQuat, mRotateY);
        al::connectPoseQT(this, mConnector, quat, al::getConnectBaseTrans(mConnector));
    } else {
        al::rotateQuatYDirDegree(this, mBaseQuat, mRotateY);
    }
}
void GreenCoin::exeCountDown() {
    mFlashing->movement();
    rotate(0.0f);
    if (mFlashing->isEnded()) kill();
}
void GreenCoin::exeSpin() {
    if (al::isFirstStep(this)) al::startSe(this, "PgSpin", nullptr);
    mFlashing->movement();
    if (mFlashing->isEnded()) {
        kill();
        return;
    }
    if (al::updateNerveState(this)) al::setNerve(this, &NrvGreenCoin.CountDown);
}
