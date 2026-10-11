#include "MapObj/GoalPoleRunawayStep.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(GoalPoleRunawayStep, Appear);
    NERVE_DECL(GoalPoleRunawayStep, Wait);
    NERVES_MAKE_NOSTRUCT(GoalPoleRunawayStep, Appear, Wait)
}
GoalPoleRunawayStep::GoalPoleRunawayStep(const sead::Matrix34f* pFollowMtx)
    : al::LiveActor("ゴールポール足場"), mFollowMtx(pFollowMtx) {}
GoalPoleRunawayStep::~GoalPoleRunawayStep() {}
void GoalPoleRunawayStep::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveNameNoPlacementInfo(this, rInfo, "GoalPoleRunawayStep", nullptr);
    al::initNerve(this, &NrvGoalPoleRunawayStepAppear, 0);
    makeActorDead();
}
void GoalPoleRunawayStep::appear() {
    al::updatePoseMtx(this, mFollowMtx);
    al::LiveActor::appear();
    al::hideModel(this);
}
void GoalPoleRunawayStep::exeAppear() {
    if (al::isStep(this, 0)) {
        al::showModel(this);
        al::startAction(this, "Appear");
    }
    if (al::isGreaterStep(this, 0) && al::isActionEnd(this))
        al::setNerve(this, &NrvGoalPoleRunawayStepWait);
}
void GoalPoleRunawayStep::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
}
