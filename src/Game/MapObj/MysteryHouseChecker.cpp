#include "MapObj/MysteryHouseChecker.hpp"
#include "MapObj/MysteryBox.hpp"
#include "MapObj/GreenStar.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/GoalObjStateGoalDemo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
namespace {
NERVE_DECL(MysteryHouseChecker, Checking);
NERVE_DECL(MysteryHouseChecker, GoalDemo);
NERVE_DECL(MysteryHouseChecker, End);
NERVE_DECL(MysteryHouseChecker, Miss);
NERVES_MAKE_STRUCT(MysteryHouseChecker, Checking, GoalDemo, End, Miss)
}
MysteryHouseChecker::MysteryHouseChecker(const char* name) : al::LiveActor(name) {}
MysteryHouseChecker::~MysteryHouseChecker() {}
void MysteryHouseChecker::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorAudioKeeperWithout3D(this, info, nullptr, nullptr);
    al::initExecutorWatchObj(this, info);
    al::initStageSwitch(this, info);
    al::initNerve(this, &NrvMysteryHouseChecker.Checking, 1);
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
    al::initNerveState(this, mGoalDemo, &NrvMysteryHouseChecker.GoalDemo, "ゴールデモ");
    al::tryGetArg(&mRequiredStars, info, "GreenStarNum");
    mGoalDemo->setAudioDirector(al::getAudioDirector(info));
    makeActorAppeared();
    al::setSceneObj(this, this, 13);
}
bool MysteryHouseChecker::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isNerve(this, &NrvMysteryHouseChecker.GoalDemo)) return mGoalDemo->receiveMsg(msg, sender, receiver);
    return false;
}
bool MysteryHouseChecker::isGoal() const {
    return (al::isNerve(this, &NrvMysteryHouseChecker.GoalDemo) && !mGoalDemo->isDemoBefore()) || al::isNerve(this, &NrvMysteryHouseChecker.End);
}
bool MysteryHouseChecker::isEndGoalDemo() const { return al::isNerve(this, &NrvMysteryHouseChecker.End); }
bool MysteryHouseChecker::isMiss() const { return al::isNerve(this, &NrvMysteryHouseChecker.Miss); }
void MysteryHouseChecker::registerGreenStar(const GreenStar* star) {
    auto* entry = new MysteryHouseStarInfo(star);
    mStars.pushBack(entry);
}
void MysteryHouseChecker::registerMysteryBox(al::LiveActor* actor) {
    auto* box = static_cast<MysteryBox*>(actor);
    box->setAutoCountDownCancel(false);
    mBoxes.pushBack(box);
}
int MysteryHouseChecker::tryCalcLastAcquirerUserId() const { return rc::tryFindRelativeControlUserId(mLastAcquirer); }
void MysteryHouseChecker::exeChecking() {
    for (int i = 0; i < mStars.size(); ++i) {
        if (!mStars[i]->acquired && rc::isAcquiredGreenStarInScene(mStars[i]->star)) {
            mLastAcquirer = rc::getAcquirerSensor(mStars[i]->star);
            mStars[i]->acquired = true;
            ++mAcquiredCount;
        }
    }
    for (int i = 0; i < mBoxes.size(); ++i) {
        if (al::isDead(mBoxes.at(i))) continue;
        if (mRequiredStars <= mAcquiredCount) {
            mAcquiredCount = 0;
            if (!mBoxes.at(i)->tryCancelCountDown(true)) mBoxes[i]->kill();
        } else if (mBoxes.at(i)->isCountDownEnd()) {
            al::setNerve(this, &NrvMysteryHouseChecker.Miss);
            return;
        }
    }
    for (int i = 0; i < mStars.size(); ++i) if (!mStars[i]->acquired) return;
    al::setNerve(this, &NrvMysteryHouseChecker.GoalDemo);
}
void MysteryHouseChecker::exeGoalDemo() {
    if (al::updateNerveState(this)) al::setNerve(this, &NrvMysteryHouseChecker.End);
}
void MysteryHouseChecker::exeEnd() {}
void MysteryHouseChecker::exeMiss() {}
namespace MysteryHouseCheckerFunction {
void tryRegisterGreenStar(const GreenStar* star) {
    if (al::isExistSceneObj(star, 13)) static_cast<MysteryHouseChecker*>(al::getSceneObj(star, 13))->registerGreenStar(star);
}
void tryRegisterMysteryBox(al::LiveActor* box) {
    if (al::isExistSceneObj(box, 13)) static_cast<MysteryHouseChecker*>(al::getSceneObj(box, 13))->registerMysteryBox(box);
}
}
