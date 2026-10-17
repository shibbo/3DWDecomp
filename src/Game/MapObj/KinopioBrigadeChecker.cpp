#include "MapObj/KinopioBrigadeChecker.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/GoalObjStateGoalDemo.hpp"
#include "MapObj/WarpObjUtil.hpp"
#include "Demo/DemoStartPosition.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/CourseInfoHolder.hpp"
#include "Util/DemoUtil.hpp"
namespace {
NERVE_DECL(KinopioBrigadeChecker, Checking);
NERVE_DECL(KinopioBrigadeChecker, GoalDemo);
NERVE_DECL(KinopioBrigadeChecker, End);
NERVES_MAKE_STRUCT(KinopioBrigadeChecker, Checking, GoalDemo, End)
}
KinopioBrigadeChecker::KinopioBrigadeChecker(const char* name) : al::LiveActor(name) {}
KinopioBrigadeChecker::~KinopioBrigadeChecker() {}
void KinopioBrigadeChecker::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorAudioKeeperWithout3D(this, info, nullptr, nullptr);
    al::initExecutorWatchObj(this, info);
    al::initStageSwitch(this, info);
    mGoalParam = new GoalObjStateGoalDemoParam;
    mGoalParam->_0 = 0;
    mGoalParam->_4 = 120;
    mGoalParam->_10 = 25;
    mGoalParam->_14 = 96;
    mGoalParam->_18 = 235;
    mGoalParam->_30 = 150.0f;
    mDemoStar = new al::LiveActor("デモ用グリーンスター");
    al::initActorWithArchiveName(mDemoStar, info, "GreenStar", nullptr);
    mDemoStar->makeActorDead();
    mDemoStarEmpty = new al::LiveActor("デモ用グリーンスター(空)");
    al::initActorWithArchiveName(mDemoStarEmpty, info, "GreenStarEmpty", nullptr);
    mDemoStarEmpty->makeActorDead();
    al::LiveActor* item = mDemoStar;
    if (CourseInfoFunction::isClear(GameDataHolderAccessor(this), GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(this)))) item = mDemoStarEmpty;
    al::initNerve(this, &NrvKinopioBrigadeChecker.Checking, 1);
    mGoalDemo = new GoalObjStateGoalDemo(this, info, true, mGoalParam);
    mGoalDemo->setKinopioBrigadeDemo(true);
    mGoalDemo->setGoalItem(item);
    mGoalDemo->setGoalAction("KinopioBrigadeGoalPose");
    al::initNerveState(this, mGoalDemo, &NrvKinopioBrigadeChecker.GoalDemo, "ゴールデモ");
    int count = al::calcLinkChildNum(info, "GreenStar");
    for (int i = 0; i < count; ++i) {
        GreenStar* star = new GreenStar("グリーンスター", nullptr, false);
        al::initLinksActor(star, info, "GreenStar", i);
        mStars.pushBack(star);
    }
    int positions = al::calcLinkChildNum(info, "DemoStartPosition");
    if (positions > 0) {
        mPositions.allocBuffer(positions, nullptr);
        for (int i = 0; i < positions; ++i) {
            DemoStartPosition* position = new DemoStartPosition("デモ開始位置");
            al::initLinksActor(position, info, "DemoStartPosition", i);
            mPositions.pushBack(position);
        }
    }
    mGoalDemo->setAudioDirector(al::getAudioDirector(info));
    makeActorAppeared();
}
bool KinopioBrigadeChecker::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvKinopioBrigadeChecker.GoalDemo)) return mGoalDemo->receiveMsg(msg, sender, receiver);
    return false;
}
bool KinopioBrigadeChecker::isGoal() const {
    return (al::isNerve(this, &NrvKinopioBrigadeChecker.GoalDemo) && !mGoalDemo->isDemoBefore()) || al::isNerve(this, &NrvKinopioBrigadeChecker.End);
}
bool KinopioBrigadeChecker::isEndGoalDemo() const { return al::isNerve(this, &NrvKinopioBrigadeChecker.End); }
DemoStartPosition* KinopioBrigadeChecker::findNearestDemoStartPosition() const {
    al::LiveActor* player = al::getPlayerActor(this, 0);
    const sead::Vector3f& playerPos = al::getTrans(player);
    sead::Vector3f playerUp;
    al::calcUpDir(&playerUp, player);
    float nearestDistance = 100000.0f;
    DemoStartPosition* nearest = nullptr;
    int count = mPositions.size();
    for (int i = 0; i < count; ++i) {
        sead::Vector3f pos = al::getTrans(mPositions.at(i));
        sead::Vector3f up;
        al::calcUpDir(&up, mPositions.at(i));
        if (playerUp.dot(up) < 0.70710677f) continue;
        float distance = (pos - playerPos).length();
        if (!(distance >= nearestDistance)) {
            nearest = mPositions.at(i);
            nearestDistance = distance;
        }
    }
    return nearest;
}
void KinopioBrigadeChecker::exeChecking() {
    sead::BitFlag32 previous = mAcquired;
    int acquired = 0;
    for (int i = 0; i < mStars.size(); ++i) {
        if (mStars[i]->isAcquiredInScene()) {
            ++acquired;
            mAcquired.setBit(i);
        }
    }
    if (acquired < mStars.size()) return;
    for (int i = 0; i < mStars.size(); ++i) {
        if (!previous.isOnBit(i)) {
            mGoalPlayer = al::getSensorHost(mStars[i]->getAcquirerSensor());
            break;
        }
    }
    mGoalDemo->setGoalPlayer(mGoalPlayer);
    al::tryOnStageSwitch(this, "SwitchAllGetOn");
    WarpObjUtil::stopStageTimer(this);
    DemoStartPosition* position = findNearestDemoStartPosition();
    mGoalDemo->setDemoStartPosition(position);
    for (int i = 0; i < mStars.size(); ++i) rc::addDemoActor(mStars.at(i));
    al::setNerve(this, &NrvKinopioBrigadeChecker.GoalDemo);
}
void KinopioBrigadeChecker::exeGoalDemo() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvKinopioBrigadeChecker.End);
}
void KinopioBrigadeChecker::exeEnd() {}
