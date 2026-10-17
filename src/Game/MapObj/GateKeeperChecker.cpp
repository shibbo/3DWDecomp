#include "MapObj/GateKeeperChecker.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/GoalObjStateGoalDemo.hpp"
#include "MapObj/WarpObjUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(GateKeeperChecker, Checking)
NERVE_DECL(GateKeeperChecker, GoalDemo)
NERVE_DECL(GateKeeperChecker, End)
NERVES_MAKE_STRUCT(GateKeeperChecker, Checking, GoalDemo, End)
}
GateKeeperChecker::GateKeeperChecker(const char* name) : al::LiveActor(name) {}
void GateKeeperChecker::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorAudioKeeperWithout3D(this, info, nullptr, nullptr);
    al::initExecutorWatchObj(this, info);
    al::initStageSwitch(this, info);
    al::initNerve(this, &NrvGateKeeperChecker.Checking, 1);
    mGoalParam = new GoalObjStateGoalDemoParam;
    mGoalParam->mGoalPoseAction = "GoalPoseShort";
    mGoalParam->_30 = 190.0f;
    mGoalParam->_34 = 0.0f;
    mGoalParam->_28 = 880.0f;
    mGoalParam->_2c = 0.0f;
    mGoalParam->_1c = 140.0f;
    mGoalParam->_20 = 0.0f;
    mGoalParam->_24 = 200.0f;
    mGoalParam->_14 = 135;
    mGoalDemo = new GoalObjStateGoalDemo(this, info, false, mGoalParam);
    mGoalDemo->setKinopioBrigadeDemo(true);
    al::initNerveState(this, mGoalDemo, &NrvGateKeeperChecker.GoalDemo, "ゴールデモ");
    GreenStar* star = new GreenStar("グリーンスター", nullptr, false);
    al::initLinksActor(star, info, "GreenStar", 0);
    mStar = star;
    mGoalDemo->setAudioDirector(al::getAudioDirector(info));
    makeActorAppeared();
}
bool GateKeeperChecker::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvGateKeeperChecker.GoalDemo))
        return mGoalDemo->receiveMsg(msg, sender, receiver);
    return false;
}
bool GateKeeperChecker::isGoal() const {
    return (al::isNerve(this, &NrvGateKeeperChecker.GoalDemo) && !mGoalDemo->isDemoBefore()) || al::isNerve(this, &NrvGateKeeperChecker.End);
}
bool GateKeeperChecker::isEndGoalDemo() const { return al::isNerve(this, &NrvGateKeeperChecker.End); }
void GateKeeperChecker::exeChecking() {
    if (rc::isAcquiredGreenStarInScene(mStar)) {
        WarpObjUtil::stopStageTimer(this);
        al::setNerve(this, &NrvGateKeeperChecker.GoalDemo);
    }
}
void GateKeeperChecker::exeGoalDemo() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvGateKeeperChecker.End);
}
void GateKeeperChecker::exeEnd() {}
