#include "MapObj/GreenRing.hpp"
#include "MapObj/GreenCoin.hpp"
#include "MapObj/GreenStar.hpp"
#include "Layout/CollectNumber.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
NERVE_DECL(GreenRing, Wait);
NERVE_DECL(GreenRing, CountDown);
NERVES_MAKE_STRUCT(GreenRing, Wait, CountDown)
}
GreenRing::GreenRing(const char* name) : al::LiveActor(name) {}
GreenRing::~GreenRing() {}
void GreenRing::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    al::initNerve(this, &NrvGreenRing.Wait, 0);
    int count = al::calcLinkChildNum(info, "GreenCoin");
    mConnector = al::tryCreateMtxConnector(this, info);
    mBaseQuat.set(al::getQuat(this));
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, mBaseQuat);
    if (!al::isNearZero(up.cross(sead::Vector3f(0.0f, 0.0f, 1.0f)), 0.001f))
        al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f(0.0f, 0.0f, 1.0f));
    mCoins = new al::DeriveActorGroup<GreenCoin>("グリーンコインリスト", count);
    for (int i = 0; i < count; ++i) {
        GreenCoin* coin = new GreenCoin("グリーンコイン");
        al::initLinksActor(coin, info, "GreenCoin", i);
        coin->setHost(this);
        mCoins->registerActor(coin);
    }
    mStar = new GreenStar("グリーンスター", nullptr, false);
    al::initCreateActorWithPlacementInfo(mStar, info);
    mStar->makeActorDead();
    al::setTrans(mStar, al::getTrans(mCoins->getDeriveActor(0)) + sead::Vector3f(0.0f, 80.0f, 0.0f));
    mNumbers.allocBuffer(count, nullptr);
    for (int i = 0; i < mNumbers.capacity(); ++i)
        mNumbers.pushBack(new CollectNumber(al::getLayoutInitInfo(info), "PopGreenCoinNumber", "グリーンコイン取得枚数"));
    makeActorAppeared();
}
void GreenRing::initAfterPlacement() {
    if (mConnector) al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 300.0f);
}
void GreenRing::attackSensor(al::HitSensor*, al::HitSensor*) {}
bool GreenRing::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isMsgItemGetAll(msg)) {
        if (al::isNerve(this, &NrvGreenRing.Wait)) {
            if (al::isSensorHitRingShape(sender, receiver, 40.0f)) {
                rc::addScoreBySystem(this, sender, "コインx1", 0);
                al::setNerve(this, &NrvGreenRing.CountDown);
                return true;
            }
        }
    }
    return false;
}
void GreenRing::appearItem() {
    GreenCoin* lastCoin = mCoins->getDeriveActor(0);
    for (int i = 1; i < mCoins->mNumActors; ++i) {
        GreenCoin* coin = mCoins->getDeriveActor(i);
        if (lastCoin->getCountDownStep() < coin->getCountDownStep()) lastCoin = coin;
    }
    mStar->setNoConnect();
    mStar->appearWithPos(al::getTrans(lastCoin) + sead::Vector3f(0.0f, 80.0f, 0.0f));
}
void GreenRing::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
    if (mConnector) al::connectPoseQT(this, mConnector, mBaseQuat, al::getConnectBaseTrans(mConnector));
}
void GreenRing::exeCountDown() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgStart", nullptr);
        al::hideModelIfShow(this);
        al::invalidateClipping(this);
        al::invalidateHitSensors(this);
        if (al::isActionPlaying(this, "Wait")) al::stopAction(this);
        al::tryDeleteEmitterAndParticleAll(this);
        mCoins->appearAll();
        mCollectedCoins.makeAllZero();
        al::startHitReactionGet(this);
    }
    for (int i = 0; i < mCoins->mNumActors; ++i) {
        GreenCoin* coin = mCoins->getDeriveActor(i);
        if (al::isDead(coin) && !mCollectedCoins.isOnBit(i)) {
            mCollectedCoins.setBit(i);
            CollectNumber* number = mNumbers.at(i);
            ++mCollectedCount;
            if (mCollectedCount == 8) {
                appearItem();
                number->appearComplete(al::getTrans(coin), 8);
                al::startSe(this, "PgComplete", nullptr);
                kill();
                return;
            }
            number->appearNormal(al::getTrans(coin), mCollectedCount);
        }
    }
    if (al::isGreaterEqualStep(this, GreenCoin::getTimerFrame())) {
        mCoins->killAll();
        al::startSe(this, "PgTimerTimeUp", nullptr);
        kill();
    } else if (al::isGreaterEqualStep(this, GreenCoin::getTimerFrame() - 180)) {
        al::holdSe(this, "PgTimerFast", nullptr);
    } else {
        al::holdSe(this, "PgTimerNormal", nullptr);
    }
}
int GreenRing::getSeParamNum() { return mCollectedCount; }
