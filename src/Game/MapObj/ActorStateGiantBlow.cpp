#include "MapObj/ActorStateGiantBlow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
    const ActorStateGiantBlowParam sDefaultParam(30, 18.0f, 30.0f, 0.99f, 1.1f, -15.0f, 0);
    NERVE_DECL(ActorStateGiantBlow, Blow);
    NERVES_MAKE_NOSTRUCT(ActorStateGiantBlow, Blow)
}

ActorStateGiantBlowParam::ActorStateGiantBlowParam()
    : ActorStateGiantBlowParam(30, 18.0f, 30.0f, 0.99f, 1.1f, -15.0f, 0) {}

ActorStateGiantBlow::ActorStateGiantBlow(al::LiveActor* pActor,
    const ActorStateGiantBlowParam* pParam, al::LiveActor* pBreakModel, al::LiveActor* pTraceModel)
    : al::ActorStateBase("巨大マリオで吹き飛び状態", pActor), mParam(pParam),
      mBreakModel(pBreakModel), mTraceModel(pTraceModel) {
    initNerve(&NrvActorStateGiantBlowBlow, 0);
    if (!mParam)
        mParam = &sDefaultParam;
}

void ActorStateGiantBlow::appear() {
    al::ActorStateBase::appear();
    if (mBreakModel)
        mBreakModel->appear();
    if (mTraceModel) {
        mTraceModel->appear();
        al::startHitReactionAppear(mTraceModel);
    }
    al::setNerve(this, &NrvActorStateGiantBlowBlow);
}

bool ActorStateGiantBlow::tryStartBlow(const al::SensorMsg* pMsg,
    al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isMsgPlayerGiantTouch(pMsg)) {
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        al::calcDirBetweenSensorsH(&mBlowDirection, pOther, pSelf);
        if (al::isNearZero(mBlowDirection, 0.001f)) {
            mBlowDirection.set(sead::Vector3f::ez);
            al::rotateVectorDegreeY(&mBlowDirection, al::getRandomDegree());
        }
        return true;
    }
    return false;
}

void ActorStateGiantBlow::exeBlow() {
    al::LiveActor* actor = mBreakModel ? mBreakModel : mHostActor;
    if (al::isFirstStep(this)) {
        if (al::isNearZero(mBlowDirection, 0.001f)) {
            kill();
            return;
        }
        sead::Vector3f front(-mBlowDirection.x, 0.0f, -mBlowDirection.z);
        al::normalize(&front);
        al::faceToDirection(actor, front);
        sead::Vector3f velocity = mBlowDirection;
        velocity *= mParam->mSpeed;
        velocity.y = mParam->mUpSpeed;
        al::setVelocity(actor, velocity);
        al::startHitReaction(actor, "巨大マリオ衝突");
    }
    al::addVelocityToGravity(actor, mParam->mGravity);
    al::scaleVelocity(actor, mParam->mVelocityScale);
    al::rotateQuatLocalDirDegree(actor, mParam->mRotateAxis, mParam->mRotateSpeed);
    if (al::isGreaterEqualStep(this, mParam->mDuration)) {
        if (mBreakModel) {
            al::startHitReactionBreak(mBreakModel);
            mBreakModel->kill();
        }
        kill();
    }
}
