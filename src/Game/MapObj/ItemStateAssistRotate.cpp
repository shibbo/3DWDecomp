#include "MapObj/ItemStateAssistRotate.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

struct ItemAssistRotateParam {
    int mSpinFrames;
    float mSpinSpeed;
    bool mUseLerp;
    float mEndSpeed;
    int mLerpFrames;
};

namespace {
    const ItemAssistRotateParam sDefaultParam = {60, 20.0f, true, 3.0f, 60};
    NERVE_DECL(ItemStateAssistRotate, Spin);
    NERVE_DECL(ItemStateAssistRotate, Lerp);
    NERVES_MAKE_NOSTRUCT(ItemStateAssistRotate, Spin, Lerp)
}

ItemStateAssistRotate::ItemStateAssistRotate(al::LiveActor* pActor,
    const ItemAssistRotateParam* pParam)
    : al::ActorStateBase("アシスト回転状態", pActor), mParam(pParam) {
    if (!mParam)
        mParam = &sDefaultParam;
}

void ItemStateAssistRotate::init() {
    initNerve(&NrvItemStateAssistRotateSpin, 0);
}

void ItemStateAssistRotate::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvItemStateAssistRotateSpin);
}

void ItemStateAssistRotate::setRotateDegree(float degree) {
    mLocalRotateDegree = al::wrapAngle(degree);
}

void ItemStateAssistRotate::exeSpin() {
    if (al::isFirstStep(this) && !mRotateDegree)
        mRotateDegree = &mLocalRotateDegree;
    rotate(mParam->mSpinSpeed);
    if (al::isGreaterEqualStep(this, mParam->mSpinFrames)) {
        if (mParam->mUseLerp)
            al::setNerve(this, &NrvItemStateAssistRotateLerp);
        else
            kill();
    }
}

void ItemStateAssistRotate::rotate(float speed) {
    if (mConnector) {
        *mRotateDegree += speed;
        sead::Quatf rotation;
        al::rotateQuatYDirDegree(&rotation, mBaseQuat, *mRotateDegree);
        al::connectPoseQT(mHostActor, mConnector, rotation, al::getConnectBaseTrans(mConnector));
    } else {
        al::rotateQuatYDirDegree(mHostActor, al::getQuat(mHostActor), speed);
    }
}

void ItemStateAssistRotate::exeLerp() {
    rotate(al::lerpValue(al::calcNerveEaseOutRate(this, mParam->mLerpFrames),
                         mParam->mSpinSpeed, mParam->mEndSpeed));
    if (al::isGreaterEqualStep(this, mParam->mLerpFrames))
        kill();
}
