#include "MapObj/PlessieTerrain.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
namespace {
NERVE_DECL(PlessieTerrain, Hidden);
NERVE_DECL(PlessieTerrain, Hide);
NERVE_DECL(PlessieTerrain, Stay);
NERVE_DECL(PlessieTerrain, Appear);
NERVES_MAKE_STRUCT(PlessieTerrain, Hidden, Hide)
NERVES_MAKE_NOSTRUCT(PlessieTerrain, Stay, Appear)
}
PlessieTerrain::PlessieTerrain(const char* name) : al::LiveActor(name) {}
PlessieTerrain::~PlessieTerrain() {}
void PlessieTerrain::init(const al::ActorInitInfo& info) {
    const char* model = nullptr;
    alPlacementFunction::tryGetModelName(&model, info);
    al::initActorWithArchiveName(this, info, model, nullptr);
    al::tryGetTrans(&mBasePos, *info.mPlacementInfo);
    mAttachments.init(info, true);
    al::initNerve(this, &NrvPlessieTerrain.Hidden, 4);
    bool switchAppear = al::listenStageSwitchOnOff(this, "SwitchAppear", al::FunctorV0M(this, &PlessieTerrain::startRise), al::FunctorV0M(this, &PlessieTerrain::hideInstant));
    al::listenStageSwitchOn(this, "SwitchKill", al::FunctorV0M(this, &PlessieTerrain::hideInstant));
    al::validateClipping(this);
    mIsPlessieChase = rc::isPlessieChase(SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(this)));
    if (switchAppear) {
        makeActorDead();
        al::setTrans(this, sead::Vector3f(mBasePos.x, mBasePos.y + -400000.0f, mBasePos.z));
        syncAttachments(true);
        al::setNerve(this, &NrvPlessieTerrain.Hidden);
    } else {
        al::setTrans(this, sead::Vector3f(mBasePos.x, mBasePos.y + -4000.0f, mBasePos.z));
        startRise();
    }
}
void PlessieTerrain::startRise() {
    if (mIsPlessieChase) al::setTrans(this, mBasePos);
    else al::setTrans(this, sead::Vector3f(mBasePos.x, mBasePos.y + -4000.0f, mBasePos.z));
    if (al::isDead(this)) makeActorAppeared();
    al::invalidateClipping(this);
    al::setNerve(this, &NrvPlessieTerrainAppear);
    syncAttachments(true);
}
void PlessieTerrain::hideInstant() {
    if (al::isDead(this)) return;
    al::validateClipping(this);
    if (mCollisionParts) al::invalidateCollisionPartsBySystem(this);
    al::setTrans(this, sead::Vector3f(mBasePos.x, mBasePos.y + -400000.0f, mBasePos.z));
    kill();
    syncAttachments(true);
    al::setNerve(this, &NrvPlessieTerrain.Hidden);
}
void PlessieTerrain::syncAttachments(bool syncAlive) {
    if (syncAlive) {
        bool dead = al::isDead(this);
        for (int i = 0; i < mAttachments.getObjectNum(); ++i) {
            al::LiveActor* actor = mAttachments.getActor(i);
            if (dead) {
                if (!al::isDead(actor)) actor->kill();
            } else if (al::isDead(actor)) actor->reappear();
        }
    }
    mAttachments.syncObjectsToPosition(al::getTrans(this));
}
void PlessieTerrain::startClipped() {
    if (al::isNerve(this, &NrvPlessieTerrain.Hide)) al::setNerve(this, &NrvPlessieTerrain.Hidden);
    al::LiveActor::startClipped();
}
void PlessieTerrain::endClipped() { al::LiveActor::endClipped(); }
bool PlessieTerrain::receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) { return false; }
void PlessieTerrain::exeHidden() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::setTrans(this, sead::Vector3f(mBasePos.x, mBasePos.y + -400000.0f, mBasePos.z));
        kill();
        syncAttachments(true);
    }
}
void PlessieTerrain::exeAppear() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        if (mCollisionParts) al::validateCollisionPartsBySystem(this);
    }
    sead::Vector3f pos;
    sead::Vector3f start(mBasePos.x, mBasePos.y + -4000.0f, mBasePos.z);
    pos = al::getTrans(this);
    float rate = al::calcNerveRate(this, 200);
    if (mIsPlessieChase) rate = 1.0f;
    al::lerpVec(&pos, start, mBasePos, al::easeByType(rate, 0));
    al::setTrans(this, pos);
    syncAttachments(false);
    if (pos.y >= mBasePos.y) al::setNerve(this, &NrvPlessieTerrainStay);
}
void PlessieTerrain::exeStay() {
    if (al::isFirstStep(this)) {
        al::setTrans(this, mBasePos);
        al::validateClipping(this);
    }
}
void PlessieTerrain::exeHide() {
    sead::Vector3f pos;
    sead::Vector3f end(mBasePos.x, mBasePos.y + -4000.0f, mBasePos.z);
    pos = al::getTrans(this);
    al::lerpVec(&pos, mBasePos, end, al::easeByType(al::calcNerveRate(this, 1), 0));
    al::setTrans(this, pos);
    syncAttachments(false);
    if (pos.y <= end.y) {
        if (mCollisionParts) al::invalidateCollisionPartsBySystem(this);
        al::setNerve(this, &NrvPlessieTerrain.Hidden);
    }
}
void PlessieTerrain::startHide() {
    if (al::isDead(this)) return;
    al::validateClipping(this);
    if (al::isClipped(this)) {
        if (mCollisionParts) al::invalidateCollisionPartsBySystem(this);
        al::setNerve(this, &NrvPlessieTerrain.Hidden);
    } else al::setNerve(this, &NrvPlessieTerrain.Hide);
}
