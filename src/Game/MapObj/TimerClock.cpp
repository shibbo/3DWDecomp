#include "MapObj/TimerClock.hpp"
#include "Layout/TimerClockNumber.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    NERVE_DECL(TimerClock, Wait);
    NERVES_MAKE_NOSTRUCT(TimerClock, Wait)
}

TimerClock::TimerClock(const char* pName) : al::LiveActor(pName) {}
TimerClock::~TimerClock() {}

void TimerClock::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTimerClockWait, 0);
    al::tryAddDisplayOffset(this, rInfo);
    if (al::isObjectName(rInfo, "TimerClock100"))
        mTime = 100;
    else if (al::isObjectName(rInfo, "TimerClock10"))
        mTime = 10;
    mNumber = new TimerClockNumber(al::getLayoutInitInfo(rInfo), mTime);
    al::tryGetArg(&mIsPlacementInRouteDokan, rInfo, "IsPlacementInRouteDokan");
    al::updateEffectMaterialRouteDokan(this, mIsPlacementInRouteDokan);
    if (al::listenStageSwitchOnAppear(this, al::Functor(this, &TimerClock::appearBySwitch))) {
        makeActorDead();
        al::setNerve(this, &NrvTimerClockWait);
    } else {
        makeActorAppeared();
    }
}

void TimerClock::appearBySwitch() { makeActorAppeared(); }
void TimerClock::initAfterPlacement() { al::updateMaterialCodeWater(this); }

bool TimerClock::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (!isEnableMsgItemGet(pMsg))
        return false;
    if (mIsPlacementInRouteDokan)
        al::startHitReaction(this, "取得（土管）");
    else
        al::startHitReactionGet(this);
    GameDataHolderAccessor accessor(this);
    int time = mTime;
    GameDataFunction::turnBackStageTimer(accessor, time);
    mNumber->appearWithWorldPos(al::getTrans(this));
    rc::addScore(this, pOther, 0.0f, 0);
    kill();
    return true;
}

bool TimerClock::isEnableMsgItemGet(const al::SensorMsg* pMsg) const {
    if (mIsPlacementInRouteDokan)
        return rc::isMsgRouteDokanItemGet(pMsg);
    return al::isMsgItemGetAll(pMsg);
}

bool TimerClock::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                        al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}

void TimerClock::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}
