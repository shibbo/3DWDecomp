#include "MapObj/GreenStarStand.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
namespace {
    NERVE_DECL(GreenStarStand, BeforeWait);
    NERVE_DECL(GreenStarStand, AfterWait);
    NERVE_DECL(GreenStarStand, Appear);
    NERVES_MAKE_NOSTRUCT(GreenStarStand, BeforeWait, AfterWait, Appear)
}
GreenStarStand::GreenStarStand(const char* pName) : al::LiveActor(pName) {}
GreenStarStand::~GreenStarStand() {}
void GreenStarStand::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvGreenStarStandBeforeWait, 0);
    al::listenStageSwitchOnStart(this, al::Functor(this, &GreenStarStand::switchOn));
    makeActorAppeared();
}
void GreenStarStand::switchOn() {
    if (al::isNerve(this, &NrvGreenStarStandBeforeWait))
        al::setNerve(this, &NrvGreenStarStandAppear);
}
void GreenStarStand::exeBeforeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "BeforeWait");
}
void GreenStarStand::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvGreenStarStandAfterWait);
}
void GreenStarStand::exeAfterWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "AfterWait");
}
