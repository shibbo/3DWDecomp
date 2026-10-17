#include "MapObj/BoxPropellerPropeller.hpp"
#include "Library/ActorUtil.hpp"
#include <math/seadQuat.h>
BoxPropellerPropeller::BoxPropellerPropeller(const al::LiveActor* parent, const char* joint, const al::ActorInitInfo& info, const char* name)
    : al::LiveActor(name), mParent(parent) {
    mJointMtx = al::getJointMtxPtr(parent, joint);
    bool singleMode = al::isSingleMode(info);
    al::initActorWithArchiveName(this, info, "BoxPropellerPropeller", singleMode ? "SM" : nullptr);
    if (singleMode) alActorSystemFunction::tryCompletelyRemoveFromExecutorDraw(this, &mDrawers);
    makeActorAppeared();
}
BoxPropellerPropeller::~BoxPropellerPropeller() {}
void BoxPropellerPropeller::control() {
    sead::Vector3f pos = mJointMtx->getTranslation();
    if (mAlignUp) {
        sead::Vector3f up(mJointMtx->m[0][1], mJointMtx->m[1][1], mJointMtx->m[2][1]);
        sead::Quatf quat;
        sead::Matrix34f rotation;
        rotation.makeIdentity();
        if (quat.makeVectorRotation(sead::Vector3f(0.0f, 1.0f, 0.0f), up))
            rotation.makeQT(quat, sead::Vector3f(0.0f, 0.0f, 0.0f));
        sead::Matrix34f pose;
        pose.makeIdentity();
        pose = rotation * pose;
        pose.setTranslation(pos);
        al::updatePoseMtx(this, &pose);
    } else al::updatePoseMtx(this, mJointMtx);
    al::setTrans(this, pos);
}
void BoxPropellerPropeller::addToFrontDraw() { alActorSystemFunction::addBackToExecutorDraw(this, &mDrawers); }
void BoxPropellerPropeller::removeFromFrontDraw() { alActorSystemFunction::removeFromExecutorDraw(this, &mDrawers); }
void BoxPropellerPropeller::pause() { mPaused = true; addToFrontDraw(); }
void BoxPropellerPropeller::resume() { mPaused = false; removeFromFrontDraw(); }
