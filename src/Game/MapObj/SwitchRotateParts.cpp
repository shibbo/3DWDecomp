#include "MapObj/SwitchRotateParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_ACTION_IMPL(SwitchRotateParts, Wait)
    NERVE_ACTION_IMPL(SwitchRotateParts, Stop)
    NERVE_ACTION_IMPL(SwitchRotateParts, Rotate)
    NERVE_ACTIONS_MAKE_STRUCT(SwitchRotateParts, Wait, Stop, Rotate)
}

SwitchRotateParts::SwitchRotateParts(const char* pName) : al::LiveActor(pName) {
}

void SwitchRotateParts::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Wait", &NrvSwitchRotateParts.collector, 0);
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::invalidateClipping(this);
    makeActorAppeared();
}

void SwitchRotateParts::initFromWatcher(const sead::Matrix34f* pMtx, const sead::Matrix34f& rInverseMtx) {
    mWatcherMtx = pMtx;
    al::makeMtxSRT(&mLocalMtx, this);
    mLocalMtx.setMul(rInverseMtx, mLocalMtx);
}

void SwitchRotateParts::control() {
    sead::Matrix34f mtx;
    mtx.setMul(*mWatcherMtx, mLocalMtx);
    al::updatePoseMtx(this, &mtx);
}

void SwitchRotateParts::requestStop() {
    al::startNerveAction(this, "Stop");
}

void SwitchRotateParts::requestRotate() {
    al::startNerveAction(this, "Rotate");
}

void SwitchRotateParts::exeWait() {
}

void SwitchRotateParts::exeStop() {
}

void SwitchRotateParts::exeRotate() {
}

SwitchRotateParts::~SwitchRotateParts() {
}
