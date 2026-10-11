#include "MapObj/GoalPoleWing.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(GoalPoleWing, Appear);
    NERVE_DECL(GoalPoleWing, Runaway);
    NERVES_MAKE_NOSTRUCT(GoalPoleWing, Appear, Runaway)
}

GoalPoleWing::GoalPoleWing(al::LiveActor* pParent)
    : al::PartsModel("ゴールポール羽"), mParent(pParent) {}

GoalPoleWing::~GoalPoleWing() {}

void GoalPoleWing::init(const al::ActorInitInfo& rInfo) {
    initPartsFixFile(mParent, rInfo, "GoalPoleWing", nullptr, "Wing");
    al::initNerve(this, &NrvGoalPoleWingAppear, 0);
    al::registerSubActorSyncClipping(mParent, this, false);
    makeActorDead();
}

void GoalPoleWing::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear");
    al::setNerveAtActionEnd(this, &NrvGoalPoleWingRunaway);
}

void GoalPoleWing::exeRunaway() {
    if (al::isFirstStep(this))
        al::startAction(this, "Runaway");
}
