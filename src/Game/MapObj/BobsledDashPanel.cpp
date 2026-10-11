#include "MapObj/BobsledDashPanel.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
namespace {
    NERVE_DECL(BobsledDashPanel, Wait);
    NERVE_DECL(BobsledDashPanel, Start);
    NERVES_MAKE_NOSTRUCT(BobsledDashPanel, Wait, Start)
}
BobsledDashPanel::BobsledDashPanel(const char* pName) : al::LiveActor(pName) {}
BobsledDashPanel::~BobsledDashPanel() {}
void BobsledDashPanel::init(const al::ActorInitInfo& rInfo) {
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::ByamlIter boxInfo(al::getModelResourceYaml(this, "BoxInfo", nullptr));
    al::tryGetByamlBox3f(&mDashBounds, boxInfo);
    al::setEffectFollowMtxPtr(this, "Blur", &mEffectMatrix);
    al::initNerve(this, &NrvBobsledDashPanelWait, 0);
    if (al::listenStageSwitchOnOff(this, "SwitchTimerAppear",
                                  al::Functor(this, &BobsledDashPanel::appear),
                                  al::Functor(this, &BobsledDashPanel::kill)))
        makeActorDead();
    else
        makeActorAppeared();
}
void BobsledDashPanel::appear() {
    al::setNerve(this, &NrvBobsledDashPanelWait);
    makeActorAppeared();
}
void BobsledDashPanel::kill() { makeActorDead(); }
bool BobsledDashPanel::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor*) {
    if (rc::isMsgAskBobsledDashPanel(pMsg)) {
        sead::Vector3f position;
        al::multVecInvQuat(&position, this, al::getActorTrans(pOther));
        if (mDashBounds.isInside(position)) {
            if (al::isNerve(this, &NrvBobsledDashPanelWait)) {
                al::invalidateClipping(this);
                al::setNerve(this, &NrvBobsledDashPanelStart);
            }
            return true;
        }
    }
    return false;
}
void BobsledDashPanel::exeWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::startAction(this, "Wait");
    }
}
void BobsledDashPanel::exeStart() {
    if (al::isFirstStep(this))
        al::startAction(this, "Start");
    if (al::isGreaterEqualStep(this, 60))
        al::setNerve(this, &NrvBobsledDashPanelWait);
}
