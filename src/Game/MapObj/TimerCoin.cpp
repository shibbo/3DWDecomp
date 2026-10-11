#include "MapObj/TimerCoin.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/CoinUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
    NERVE_DECL(TimerCoin, Appear);
    NERVE_DECL(TimerCoin, Spin);
    NERVE_DECL(TimerCoin, CountDown);
    NERVES_MAKE_NOSTRUCT(TimerCoin, Appear, Spin, CountDown)
}
TimerCoin::TimerCoin(const char* pName) : al::LiveActor(pName) {}
TimerCoin::~TimerCoin() {}
void TimerCoin::init(const al::ActorInitInfo& rInfo) {
    if (al::isSingleMode(rInfo))
        al::initActorWithArchiveName(this, rInfo, "CoinBlueSM", nullptr);
    else
        al::initActorWithArchiveName(this, rInfo, "CoinBlue", nullptr);
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    al::initNerve(this, &NrvTimerCoinAppear, 1);
    mFlashing = new al::FlashingCtrl(this, true, false);
    mConnector = al::tryCreateMtxConnector(this, rInfo);
    mBaseQuat.set(al::getQuat(this));
    sead::Vector3f up = sead::Vector3f::zero;
    al::calcQuatUp(&up, mBaseQuat);
    sead::Vector3f cross;
    cross.setCross(up, sead::Vector3f::ez);
    if (!al::isNearZero(cross, 0.001f))
        al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f::ez);
    mAssistRotate = new ItemStateAssistRotate(this, CoinUtil::getCoinAssistRotateParam());
    if (mConnector)
        mAssistRotate->setConnector(mConnector, mBaseQuat);
    al::initNerveState(this, mAssistRotate, &NrvTimerCoinSpin, "スピン");
    makeActorDead();
}
void TimerCoin::initAfterPlacement() {
    if (mConnector)
        al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 300.0f);
}
void TimerCoin::appear() {
    al::LiveActor::appear();
    al::showModelIfHide(this);
    mFlashing->start(mTimerFrame);
    al::invalidateClipping(this);
    mIsCounted = false;
    mRotateDegree = 0.0f;
    al::setNerve(this, &NrvTimerCoinAppear);
}
void TimerCoin::control() {
    if (al::isNerve(this, &NrvTimerCoinCountDown))
        CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvTimerCoinSpin);
}
bool TimerCoin::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor*) {
    if (rc::isMsgCoinGet(pMsg) || (al::isSingleMode(this) && rc::isMsgRouteDokanItemGet(pMsg))) {
        mCollectSensor = pOther;
        rc::acquirerItemCoin(this, pOther);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    return false;
}
bool TimerCoin::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget*) {
    if (rc::tryAcquirerCoinIfTouchAssistTrigger(pMsg, this, pPointer)) {
        mCollectSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
        kill();
        return true;
    }
    if (al::isMsgTouchAssistNoPat(pMsg)) {
        if (!al::isNerve(this, &NrvTimerCoinSpin))
            al::setNerve(this, &NrvTimerCoinSpin);
        return true;
    }
    return false;
}
void TimerCoin::rotate(float speed) {
    mRotateDegree = al::wrapAngle(mRotateDegree + rc::getCoinRotateYByFrame(this) + speed);
    if (mConnector) {
        sead::Quatf rotation;
        al::rotateQuatYDirDegree(&rotation, mBaseQuat, mRotateDegree);
        al::connectPoseQT(this, mConnector, rotation, al::getConnectBaseTrans(mConnector));
    } else {
        al::rotateQuatYDirDegree(this, mBaseQuat, mRotateDegree);
    }
}
void TimerCoin::exeAppear() {
    mFlashing->movement();
    rotate(al::calcNerveEaseOutValue(this, 54, 35.0f, 0.0f));
    al::setNerveAtStep(this, &NrvTimerCoinCountDown, 54);
}
void TimerCoin::exeCountDown() {
    rotate(0.0f);
    mFlashing->movement();
}
void TimerCoin::exeSpin() {
    if (al::isFirstStep(this))
        al::startSe(this, "Touched");
    mFlashing->movement();
    al::updateNerveStateAndNextNerve(this, &NrvTimerCoinCountDown);
}
