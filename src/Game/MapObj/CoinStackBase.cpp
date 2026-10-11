#include "MapObj/CoinStackBase.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
    NERVE_DECL(CoinStackBase, Wait);
    NERVE_DECL(CoinStackBase, Fall);
    NERVE_DECL(CoinStackBase, FloatWait);
    NERVE_DECL(CoinStackBase, Land);
    NERVES_MAKE_NOSTRUCT(CoinStackBase, Wait, Fall, FloatWait, Land)
}
CoinStackBase::CoinStackBase(const char* pName, bool isMoving) : al::LiveActor(pName), mIsMoving(isMoving) {}
CoinStackBase::~CoinStackBase() {}
void CoinStackBase::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, mIsMoving ? "CoinStackMoving" : "CoinStack", nullptr);
    mStackHeight = 80.0f;
    al::invalidateClipping(this);
    al::initNerve(this, &NrvCoinStackBaseWait, 0);
    makeActorAppeared();
}
void CoinStackBase::control() { al::connectPoseQT(this, mConnector, mRotation, mLocalPosition); }
bool CoinStackBase::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isDead(this) || mIsCollected) return false;
    if (al::isMsgPlayerItemGet(pMsg) || al::isMsgKillerItemGet(pMsg) ||
        (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) && al::isMsgItemGetByObjAll(pMsg))) {
        al::acquirerItem(this, pSender, "コインx5[自動取得]");
        al::startHitReactionGet(this);
        mIsCollected = true;
        return true;
    }
    if (al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerClimbRollingAttack(pMsg) || al::isMsgPlayerClimbSlidingAttack(pMsg) ||
        al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgPlayerGiantAttack(pMsg) || al::isMsgPlayerKouraAttack(pMsg) || al::isMsgKickKouraAttack(pMsg)) {
        al::acquirerItem(this, pSender, "コインx5[自動取得]");
        al::startHitReactionGet(this);
        mIsCollected = true;
        rc::requestHitReactionToAttacker(pMsg, pReceiver, pSender);
        return true;
    }
    return false;
}
bool CoinStackBase::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget*) {
    if (al::isDead(this)) return false;
    if (mIsCollected) return false;
    if (al::isMsgTouchAssistTrigNoPat(pMsg)) {
        al::acquirerItem(this, DrcFunction::tryFindDrcPlayerSensor(this, pPointer), "コインx5[自動取得]");
        al::startHitReactionGet(this);
        mIsCollected = true;
        return true;
    }
    return false;
}
void CoinStackBase::initPosition(const sead::Vector3f& rPosition) {
    al::setTrans(this, rPosition);
    mLandingY = rPosition.y;
    mInitialPosition = rPosition;
    mLocalPosition.x = rPosition.x + al::getRandom(-5.0f, 5.0f);
    mLocalPosition.z = rPosition.z + al::getRandom(-5.0f, 5.0f);
    mLocalPosition.y = rPosition.y;
    mRotation.set(al::getQuat(this));
    al::rotateQuatYDirDegree(&mRotation, mRotation, al::getRandomDegree());
}
void CoinStackBase::initConnector(const al::MtxConnector* pConnector) { mConnector = pConnector; }
void CoinStackBase::requestFall(int index) {
    if (al::isDead(this)) return;
    mLandingY -= 80.0f;
    mFallDelay = index * 5;
    if (al::isNerve(this, &NrvCoinStackBaseFall) || al::isNerve(this, &NrvCoinStackBaseFloatWait)) return;
    al::setNerve(this, &NrvCoinStackBaseFloatWait);
}
void CoinStackBase::exeWait() { if (al::isFirstStep(this)) al::startAction(this, "Wait"); }
void CoinStackBase::exeFloatWait() { if (al::isGreaterEqualStep(this, mFallDelay)) al::setNerve(this, &NrvCoinStackBaseFall); }
void CoinStackBase::exeFall() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Fall");
        mSlideOffset.x = al::getRandom(-5.0f, 5.0f);
        mSlideOffset.z = al::getRandom(-5.0f, 5.0f);
    }
    slide();
    mVelocity.y -= 0.7f;
    mVelocity *= 0.97f;
    mLocalPosition += mVelocity;
    if (mLandingY > mLocalPosition.y) al::setNerve(this, &NrvCoinStackBaseLand);
}
void CoinStackBase::slide() {
    int step = al::getNerveStep(this);
    float rate = step / 10;
    if (step >= 20) rate = 1.0f;
    mLocalPosition.x = mInitialPosition.x + mSlideOffset.x * rate;
    mLocalPosition.z = mInitialPosition.z + mSlideOffset.z * rate;
}
void CoinStackBase::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Land");
        al::startHitReaction(this, "着地");
        mVelocity.set(0.0f, 0.0f, 0.0f);
        mLocalPosition.y = mLandingY;
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvCoinStackBaseWait);
}
