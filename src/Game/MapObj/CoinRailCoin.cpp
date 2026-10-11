#include "MapObj/CoinRailCoin.hpp"
#include "MapObj/ItemStateAssistRotate.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Rail/Rail.hpp"
#include "Library/Rail/RailKeeper.hpp"
#include "Library/Rail/RailRider.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/CoinUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
NERVE_DECL(CoinRailCoin, Move);
NERVE_DECL(CoinRailCoin, Standby);
NERVE_DECL(CoinRailCoin, SpinDrc);
NERVE_DECL(CoinRailCoin, Stop);
NERVES_MAKE_STRUCT(CoinRailCoin, Stop, Move, Standby, SpinDrc)
}
CoinRailCoin::CoinRailCoin(const char* name, int index, int delay, float speed, float spacing)
    : al::LiveActor(name), mCoinIndex(index), mDelay(delay), mSpeed(speed), mSpacing(spacing) {}
CoinRailCoin::~CoinRailCoin() {}
void CoinRailCoin::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "Coin", nullptr);
    mCoinRailKeeper = al::tryCreateRailKeeper(al::getPlacementInfo(info), "RailWithEffect");
    if (al::isExistRail(this)) {
        al::setRailPosToStart(this);
        al::moveRail(this, mCoinIndex * mSpacing);
        al::syncRailTrans(this);
        al::setRailClippingInfo(&mClippingCenter, this, 100.0f, 100.0f);
    } else if (mCoinRailKeeper) {
        mCoinRailKeeper->getRailRider()->moveToRailStart();
        mCoinRailKeeper->getRailRider()->setSpeed(mCoinIndex * mSpacing);
        mCoinRailKeeper->getRailRider()->move();
        al::setTrans(this, mCoinRailKeeper->getRailRider()->getPosition());
        al::setRailClippingInfo(&mClippingCenter, this, mCoinRailKeeper, 100.0f, 100.0f);
        mSection = mCoinRailKeeper->getRail()->getIncludedSectionIndex(mCoinRailKeeper->getRailRider()->getCoord());
    } else {
        makeActorDead();
        return;
    }
    al::initNerve(this, &NrvCoinRailCoin.Move, 1);
    if (mDelay >= 1) al::setNerve(this, &NrvCoinRailCoin.Standby);
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    mBaseQuat.set(al::getQuat(this));
    al::calcQuatUp(&up, mBaseQuat);
    if (!al::isNearZero(up.cross(sead::Vector3f(0.0f, 0.0f, 1.0f)), 0.001f))
        al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f(0.0f, 0.0f, 1.0f));
    mAssistRotate = new ItemStateAssistRotate(this, CoinUtil::getCoinAssistRotateParam());
    al::initNerveState(this, mAssistRotate, &NrvCoinRailCoin.SpinDrc, "DRC回転");
    al::tryGetArg(&mInRouteDokan, info, "IsPlacementInRouteDokan");
    al::updateEffectMaterialRouteDokan(this, mInRouteDokan);
    al::updateSeMaterialBeyondWall(this, mInRouteDokan);
    makeActorAppeared();
}
void CoinRailCoin::initAfterPlacement() { al::updateMaterialCodeWater(this); }
bool CoinRailCoin::isEnableMsgItemGet(const al::SensorMsg* msg) const {
    return mInRouteDokan ? rc::isMsgRouteDokanItemGet(msg) : rc::isMsgCoinGet(msg);
}
bool CoinRailCoin::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (isEnableMsgItemGet(msg)) {
        rc::acquirerItemCoin(this, sender);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    return false;
}
bool CoinRailCoin::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (rc::tryAcquirerCoinIfTouchAssistTrigger(msg, this, pointer)) {
        kill();
        return true;
    }
    if (al::isMsgTouchAssistNoPat(msg)) {
        if (!al::isNerve(this, &NrvCoinRailCoin.Move)) return false;
        if (!al::isNerve(this, &NrvCoinRailCoin.SpinDrc)) al::setNerve(this, &NrvCoinRailCoin.SpinDrc);
        return true;
    }
    return false;
}
void CoinRailCoin::control() {
    if (!al::isNerve(this, &NrvCoinRailCoin.SpinDrc))
        al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this));
    al::addTransOffsetLocalDir(this, 70.0f, 1);
}
void CoinRailCoin::exeStop() {
    if (mCoinRailKeeper) {
        if (mFollowOffset) al::setTrans(this, mCoinRailKeeper->getRailRider()->getPosition() + *mFollowOffset);
        else al::setTrans(this, mCoinRailKeeper->getRailRider()->getPosition());
    } else al::syncRailTrans(this);
}
void CoinRailCoin::exeStandby() {
    exeStop();
    if (al::isGreaterEqualStep(this, mDelay)) al::setNerve(this, &NrvCoinRailCoin.Move);
}
void CoinRailCoin::exeMove() {
    moveCoinRail();
    CoinUtil::tryStartSpinCoinIfMicInputOn(this, &NrvCoinRailCoin.SpinDrc);
}
void CoinRailCoin::moveCoinRail() {
    if (mCoinRailKeeper) {
        mCoinRailKeeper->getRailRider()->setSpeed(mSpeed);
        mCoinRailKeeper->getRailRider()->move();
        if (mFollowOffset) al::setTrans(this, mCoinRailKeeper->getRailRider()->getPosition() + *mFollowOffset);
        else al::setTrans(this, mCoinRailKeeper->getRailRider()->getPosition());
        int section = mCoinRailKeeper->getRail()->getIncludedSectionIndex(mCoinRailKeeper->getRailRider()->getCoord());
        if (mSection != section) {
            const al::PlacementInfo* point = mCoinRailKeeper->getRail()->getRailPoint(section);
            bool throughWater = false;
            bool throughBottom = false;
            al::tryGetArg(&throughWater, *point, "ThroughWater");
            al::tryGetArg(&throughBottom, *point, "ThroughWaterBottom");
            if (throughWater) al::startHitReaction(this, "水面通過");
            else if (throughBottom) al::startHitReaction(this, "水面通過[底面]");
        }
        mSection = section;
    } else {
        if (mFollowOffset) al::moveSyncRailOffset(this, mSpeed, *mFollowOffset);
        else al::moveSyncRail(this, mSpeed);
    }
    rc::startHitReactionIfThroughWater(this);
}
void CoinRailCoin::exeSpinDrc() {
    if (al::isFirstStep(this)) al::startSe(this, "PgTouched", nullptr);
    moveCoinRail();
    if (al::updateNerveState(this)) al::setNerve(this, &NrvCoinRailCoin.Move);
}
void CoinRailCoin::setStop() { al::setNerve(this, &NrvCoinRailCoin.Stop); }
void CoinRailCoin::setMove() { al::setNerve(this, &NrvCoinRailCoin.Move); }
