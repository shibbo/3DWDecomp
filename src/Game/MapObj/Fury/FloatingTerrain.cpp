#include "MapObj/Fury/FloatingTerrain.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Scene/ProjectActorFactory.hpp"

namespace {
NERVE_DECL(FloatingTerrain, Hidden)
NERVE_DECL(FloatingTerrain, Stay)
NERVE_DECL(FloatingTerrain, Hide)
NERVE_DECL(FloatingTerrain, Rotate)
NERVE_DECL(FloatingTerrain, Appear)
NERVES_MAKE_NOSTRUCT(FloatingTerrain, Hidden, Stay, Hide, Rotate, Appear)
}

FloatingTerrain::FloatingTerrain(const char* name) : al::LiveActor(name) {}
void FloatingTerrain::init(const al::ActorInitInfo& info) {
    const char* modelName = nullptr;
    alPlacementFunction::tryGetModelName(&modelName, info);
    al::initActorWithArchiveName(this, info, modelName, nullptr);
    const al::PlacementInfo& placement = info.getPlacementInfo();
    al::tryGetTrans(&mHome, placement);
    al::tryGetQuat(&mHomeQuat, placement);
    ProjectActorFactory factory;
    mAttachmentCount = al::calcLinkChildNum(info, "AttachObject");
    if (mAttachmentCount >= 1) {
        mAttachments = new Attachment[mAttachmentCount];
        for (s32 i = 0; i < mAttachmentCount; ++i) {
            al::LiveActor* actor = al::createLinksActorFromFactory(factory, info, "AttachObject", i);
            mAttachments[i].actor = actor;
            mAttachments[i].offset = al::getTrans(actor) - mHome;
            actor->makeActorDead();
        }
    }
    al::initNerve(this, &NrvFloatingTerrainHidden, 4);
    using Functor = al::FunctorV0M<FloatingTerrain*, void (FloatingTerrain::*)()>;
    bool hasSwitch = al::listenStageSwitchOnOff(this, "SwitchAppear", Functor(this, &FloatingTerrain::startAppear), Functor(this, &FloatingTerrain::startRotate));
    al::listenStageSwitchOn(this, "SwitchKill", Functor(this, &FloatingTerrain::startRotate));
    al::validateClipping(this);
    if (hasSwitch) {
        makeActorDead();
        al::setTrans(this, sead::Vector3f(mHome.x, mHome.y + 400000.0f, mHome.z));
        syncAttachments(true);
        al::setNerve(this, &NrvFloatingTerrainHidden);
    } else {
        al::setTrans(this, sead::Vector3f(mHome.x, mHome.y + 4000.0f, mHome.z));
        startAppear();
    }
}
void FloatingTerrain::startAppear() {
    al::setTrans(this, sead::Vector3f(mHome.x, mHome.y + 4000.0f, mHome.z));
    if (al::isDead(this)) makeActorAppeared();
    al::invalidateClipping(this);
    al::setNerve(this, &NrvFloatingTerrainAppear);
    syncAttachments(true);
}
void FloatingTerrain::startRotate() { al::setNerve(this, &NrvFloatingTerrainRotate); }
void FloatingTerrain::syncAttachments(bool syncLife) {
    if (syncLife) {
        bool dead = al::isDead(this);
        if (mAttachmentCount >= 1 && dead) for (s32 i = 0; i < mAttachmentCount; ++i) {
            al::LiveActor* actor = mAttachments[i].actor;
            if (al::isDead(actor)) {
                if (al::isDead(actor)) actor->makeActorAppeared();
            } else {
                actor->kill();
            }
        }
    }
    sead::Vector3f axis;
    float degree;
    al::calcQuatRotateAxisAndDegree(&axis, &degree, al::getQuat(this));
}
void FloatingTerrain::control() {}
bool FloatingTerrain::receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) { return false; }
void FloatingTerrain::exeHidden() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::rotateQuatZDirDegree(this, 180.0f);
        al::setTrans(this, sead::Vector3f(mHome.x, mHome.y + 400000.0f, mHome.z));
        kill();
        syncAttachments(true);
    }
}
void FloatingTerrain::exeAppear() {
    sead::Vector3f pos;
    sead::Vector3f start(mHome.x, mHome.y + 4000.0f, mHome.z);
    pos = al::getTrans(this);
    al::lerpVec(&pos, start, mHome, al::easeByType(al::calcNerveRate(this, 100), 0));
    al::setTrans(this, pos);
    syncAttachments(false);
    if (pos.y <= mHome.y) al::setNerve(this, &NrvFloatingTerrainStay);
}
void FloatingTerrain::exeStay() {
    if (al::isFirstStep(this)) {
        al::setTrans(this, mHome);
        al::invalidateClipping(this);
    }
}
void FloatingTerrain::exeHide() {
    sead::Vector3f pos;
    sead::Vector3f end(mHome.x, mHome.y + 4000.0f, mHome.z);
    pos = al::getTrans(this);
    al::lerpVec(&pos, mHome, end, al::easeByType(al::calcNerveRate(this, 100), 0));
    al::setTrans(this, pos);
    syncAttachments(false);
    if (pos.y >= end.y) al::setNerve(this, &NrvFloatingTerrainHidden);
}
void FloatingTerrain::exeRotate() {
    al::rotateQuatZDirDegree(this, al::lerpDegree(0.0f, 180.0f, al::calcNerveRate(this, 9000)));
    syncAttachments(false);
    sead::Vector3f axis;
    float degree;
    al::calcQuatRotateAxisAndDegree(&axis, &degree, al::getQuat(this));
    if ((degree > 0.0f ? degree : -degree) >= 179.0f) al::setNerve(this, &NrvFloatingTerrainHide);
}
void FloatingTerrain::startHide() {
    if (al::isDead(this)) return;
    al::validateClipping(this);
    if (al::isClipped(this)) al::setNerve(this, &NrvFloatingTerrainHidden);
    else al::setNerve(this, &NrvFloatingTerrainHide);
}
