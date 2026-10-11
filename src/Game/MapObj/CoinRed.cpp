#include "MapObj/CoinRed.hpp"
#include "MapObj/IUseRedCoin.hpp"
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
#include "Util/DrcUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(CoinRed, Appear);
NERVE_DECL(CoinRed, Spin);
NERVE_DECL(CoinRed, Stop);
NERVE_DECL(CoinRed, CountDown);
NERVES_MAKE_STRUCT(CoinRed, Appear, Spin, Stop, CountDown)
}
CoinRed::CoinRed(const char* name, IUseRedCoin* host) : al::LiveActor(name), mHost(host) {}
CoinRed::~CoinRed() {}
void CoinRed::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    al::initNerve(this, &NrvCoinRed.Appear, 1);
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
    al::initNerveState(this, mAssistRotate, &NrvCoinRed.Spin, "DRC回転ステート");
    makeActorDead();
}
void CoinRed::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 300.0f);
}
void CoinRed::appear() {
    al::LiveActor::appear();
    mFlashing->start(600);
}
void CoinRed::control() {
    if (!al::isNerve(this, &NrvCoinRed.Stop)) ++mCollectedFrame;
    if (al::isNerve(this, &NrvCoinRed.CountDown))
        CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvCoinRed.Spin);
}
void CoinRed::acquirerCoinRed() {
    int maxCoins = mHost->getMaxCoinNum();
    int seParam = mHost->getSeParamNum() - maxCoins + 1;
    al::startHitReactionGet(this);
    if (seParam != 0) al::startSeWithParam(this, "Collect", seParam, nullptr);
    kill();
}
bool CoinRed::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (rc::isMsgCoinGet(msg)) {
        mCollector = sender;
        rc::acquirerItemCoin(this, sender);
        acquirerCoinRed();
        return true;
    }
    return false;
}
bool CoinRed::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (rc::tryAcquirerCoinIfTouchAssistTrigger(msg, this, pointer)) {
        mCollector = DrcFunction::tryFindDrcPlayerSensor(this, pointer);
        acquirerCoinRed();
        return true;
    }
    if (al::isMsgTouchAssistNoPat(msg)) {
        if (!al::isNerve(this, &NrvCoinRed.Spin)) al::setNerve(this, &NrvCoinRed.Spin);
        return true;
    }
    return false;
}
void CoinRed::disableCountdown() { al::setNerve(this, &NrvCoinRed.Stop); }
int CoinRed::getTimerFrame() { return 600; }
void CoinRed::exeAppear() {
    if (al::isFirstStep(this)) al::startHitReactionAppear(this);
    mFlashing->movement();
    rotate(al::calcNerveEaseOutValue(this, 54, 45.0f, 0.0f));
    al::setNerveAtStep(this, &NrvCoinRed.CountDown, 54);
}
void CoinRed::rotate(float speed) {
    mRotateY = al::wrapAngle(mRotateY + rc::getCoinRotateYByFrame(this) + speed);
    if (mConnector) {
        sead::Quatf quat;
        al::rotateQuatYDirDegree(&quat, mBaseQuat, mRotateY);
        al::connectPoseQT(this, mConnector, quat, al::getConnectBaseTrans(mConnector));
    } else {
        al::rotateQuatYDirDegree(this, mBaseQuat, mRotateY);
    }
}
void CoinRed::exeCountDown() {
    mFlashing->movement();
    rotate(0.0f);
    if (mFlashing->isEnded()) al::setNerve(this, &NrvCoinRed.Stop);
}
void CoinRed::exeSpin() {
    if (al::isFirstStep(this)) al::startSe(this, "Rotate", nullptr);
    mFlashing->movement();
    if (mFlashing->isEnded()) {
        al::setNerve(this, &NrvCoinRed.Stop);
        return;
    }
    al::updateNerveStateAndNextNerve(this, &NrvCoinRed.CountDown);
}
void CoinRed::exeStop() { rotate(0.0f); }
