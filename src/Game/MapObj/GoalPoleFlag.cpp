#include "MapObj/GoalPoleFlag.hpp"
#include "MapObj/GoalPole.hpp"
#include "MapObj/GoalPoleBindPuppeteer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/CourseInfoHolder.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
    NERVE_DECL(GoalPoleFlag, Wait);
    class GoalPoleFlagNrvWaitRunaway : public al::Nerve {
        void execute(al::NerveKeeper* keeper) const override { keeper->getParent<GoalPoleFlag>()->exeWait(); }
    };
    NERVES_MAKE_NOSTRUCT(GoalPoleFlag, WaitRunaway)
    NERVE_DECL(GoalPoleFlag, Upward);
    NERVES_MAKE_NOSTRUCT(GoalPoleFlag, Upward)
    class GoalPoleFlagNrvLerp : public al::Nerve {
        void execute(al::NerveKeeper* keeper) const override { keeper->getParent<GoalPoleFlag>()->exeWait(); }
    };
    NERVE_DECL(GoalPoleFlag, ReachTop);
    NERVES_MAKE_NOSTRUCT(GoalPoleFlag, ReachTop)
    NERVES_MAKE_STRUCT(GoalPoleFlag, Wait, Lerp)
    bool isLargePole(const GoalPole* pole) { return pole->isLast() || pole->isSuper(); }
    float getTopHeight(const GoalPole* pole) { return isLargePole(pole) ? 600.0f : 700.0f; }
    float getBottomHeight(const GoalPole* pole) { return isLargePole(pole) ? 250.0f : 150.0f; }
}
GoalPoleFlag::GoalPoleFlag(GoalPole* pole)
    : al::PartsModel(pole->isSuper() ? "スーパーゴールポール旗" : pole->isLast() ? "ラストゴールポール旗" : "ゴールポール旗"), mPole(pole), mFollowMtx(sead::Matrix34f::ident) {}
void GoalPoleFlag::init(const al::ActorInitInfo& info) {
    al::makeMtxRT(&mFollowMtx, mPole);
    mHeight = getTopHeight(mPole);
    initPartsMtx(mPole, info, isLargePole(mPole) ? "GoalPoleSuperFlag" : "GoalPoleFlag", &mFollowMtx, false);
    al::initNerve(this, &NrvGoalPoleFlag.Wait, 0);
    al::registerSubActorSyncClipping(mPole, this, false);
    al::startAction(this, mPole->isLast() ? "WaitLast" : "Wait");
    int course = GameDataFunction::getPlayingCourseId(GameDataHolderAccessor(this));
    if (CourseInfoFunction::isClear(GameDataHolderAccessor(this), course)) {
        float rate = CourseInfoFunction::getHighestGoalPolePosition(GameDataHolderAccessor(this), course);
        mHeight = al::lerpValue(rate, getBottomHeight(mPole), getTopHeight(mPole));
        bool golden = CourseInfoFunction::isClearFlagTop(GameDataHolderAccessor(this), course);
        const char* pattern = golden ? "FlagPatternGolden" : "FlagPattern";
        const char* visibility = golden ? "AfterGolden" : "After";
        rc::setPlayerColorAnimByCharacterType(this, CourseInfoFunction::getClearTopCharacter(GameDataHolderAccessor(this), course), pattern);
        al::startVisAnim(this, visibility);
    } else {
        rc::setPlayerColorAnimDefault(this, "FlagPattern");
        al::startVisAnim(this, "Before");
    }
    makeActorAppeared();
}
void GoalPoleFlag::initAfterPlacement() { if (mPole->isRunaway()) al::setNerve(this, &NrvGoalPoleFlagWaitRunaway); }
void GoalPoleFlag::kill() { al::startHitReactionDisappear(this); al::LiveActor::kill(); }
void GoalPoleFlag::control() { updateFollowMtx(); al::PartsModel::control(); }
void GoalPoleFlag::updateFollowMtx() {
    if (mPole->isRunaway()) {
        if (al::isNerve(this, &NrvGoalPoleFlag.Wait) || al::isNerve(this, &NrvGoalPoleFlag.Lerp)) {
            sead::Vector3f front(0, 0, 0);
            mPole->getBaseMtx()->getBase(front, 2);
            al::rotateVectorDegreeY(&front, 270.0f);
            if (al::isNerve(this, &NrvGoalPoleFlag.Lerp)) {
                if (al::turnMtxZDirDegree(&mFollowMtx, mFollowMtx, front, 5.0f)) al::setNerve(this, &NrvGoalPoleFlag.Wait);
            } else if (al::isNerve(this, &NrvGoalPoleFlag.Wait)) al::makeMtxFrontUp(&mFollowMtx, front, sead::Vector3f::ey);
        }
        sead::Vector3f trans(0, 0, 0);
        al::calcJointPos(&trans, mPole, "JointRoot");
        mFollowMtx.setTranslation(trans);
    } else {
        mFollowMtx.operator=(*mPole->getBaseMtx());
        al::rotateMtxYDirDegree(&mFollowMtx, mFollowMtx, mRotateY);
    }
    const sead::Vector3f offset(0, mHeight, 0);
    mFollowMtx(0, 3) += offset.x;
    mFollowMtx(1, 3) += offset.y;
    mFollowMtx(2, 3) += offset.z;
}
void GoalPoleFlag::appearBottom(int user) {
    mHeight = getBottomHeight(mPole);
    if (mPole->isRunaway()) {
        mFollowMtx = mPole->getRunawayBaseMtx();
        al::rotateMtxYDirDegree(&mFollowMtx, mFollowMtx, mRotateY);
        al::setNerve(this, &NrvGoalPoleFlagWaitRunaway);
    }
    updateFollowMtx();
    al::LiveActor::appear();
    rc::setPlayerColorAnimByControlUserId(this, user, "FlagPattern");
    al::startVisAnim(this, "After");
    al::startHitReactionAppear(this);
}
void GoalPoleFlag::startUpward(float rate) {
    mTargetHeight = al::lerpValue(rate, getBottomHeight(mPole), getTopHeight(mPole));
    al::setNerve(this, &NrvGoalPoleFlagUpward);
}
void GoalPoleFlag::startLerp() { al::setNerve(this, &NrvGoalPoleFlag.Lerp); }
void GoalPoleFlag::exeWait() { if (al::isFirstStep(this)) al::tryStartActionIfNotPlaying(this, mPole->isLast() ? "WaitLast" : "Wait"); }
void GoalPoleFlag::exeUpward() {
    float remaining = mTargetHeight - mHeight;
    float speed = (getTopHeight(mPole) - getBottomHeight(mPole)) / GoalPoleBindPuppeteer::getFallFrameMax();
    if (speed < remaining) mHeight += speed;
    else {
        mHeight = mTargetHeight;
        if (al::isNearZero(getTopHeight(mPole) - mHeight, 0.001f)) al::setNerve(this, &NrvGoalPoleFlagReachTop);
    }
}
void GoalPoleFlag::exeReachTop() {
    if (al::isFirstStep(this)) {
        al::startVisAnim(this, "AfterGolden");
        al::startMtpAnimAndSetFrameAndStop(this, "FlagPatternGolden", al::getMtpAnimFrame(this));
        al::tryDeleteEffect(this, "Appear");
        al::startHitReaction(this, "ゴールデン");
    }
}
