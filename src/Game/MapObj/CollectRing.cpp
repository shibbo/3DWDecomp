#include "MapObj/CollectRing.hpp"
#include "MapObj/CollectRingHolder.hpp"
#include "Layout/CollectNumber.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ScoreUtil.hpp"
namespace {
    NERVE_DECL(CollectRing, Wait);
    NERVES_MAKE_NOSTRUCT(CollectRing, Wait)
}
CollectRing::CollectRing(const char* pName) : al::LiveActor(pName) {}
CollectRing::~CollectRing() {}
void CollectRing::setHost(CollectRingHolder* pHost) { mHost = pHost; }
void CollectRing::appearNumberComplete(int count) { mNumber->appearComplete(al::getTrans(this), count); }
void CollectRing::appearNumber(int count) { mNumber->appearNormal(al::getTrans(this), count); }
void CollectRing::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvCollectRingWait, 0);
    mNumber = new CollectNumber(al::getLayoutInitInfo(rInfo), "PopRedCoinNumber", "コレクトリング枚数表示");
    makeActorDead();
}
bool CollectRing::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgItemGetAll(pMsg)) {
        if (!al::isSensorHitRingShape(pOther, pSelf, 40.0f))
            return false;
        rc::addScore(this, pOther, 0.0f, 0);
        al::startHitReactionGet(this);
        mCollector = pOther;
        if (mHost)
            mHost->noticeGet(this);
        kill();
        return true;
    }
    return false;
}
void CollectRing::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvCollectRingWait);
}
void CollectRing::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}
