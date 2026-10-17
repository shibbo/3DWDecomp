#include "MapObj/TouchCarryItemState.hpp"
#include "MapObj/DrcTouchPointer.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Screen/ScreenPointer.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
namespace {
    NERVE_DECL(TouchCarryItemState, Hold);
    NERVE_DECL(TouchCarryItemState, Release);
    NERVE_DECL(TouchCarryItemState, Throw);
    NERVES_MAKE_NOSTRUCT(TouchCarryItemState, Hold, Release, Throw)
    const TouchCarryItemStateParam cDefaultParam(18.0f, 10.0f, 20.0f, 60.0f, 15.0f, 50.0f);
}
TouchCarryItemStateParam::TouchCarryItemStateParam()
    : throwSpeed(18.0f), throwLift(10.0f), releaseLift(20.0f), holdOffset(60.0f), throwThreshold(15.0f), releaseOffset(50.0f) {}
TouchCarryItemStateParam::TouchCarryItemStateParam(float a, float b, float c, float d, float e, float f)
    : throwSpeed(a), throwLift(b), releaseLift(c), holdOffset(d), throwThreshold(e), releaseOffset(f) {}
TouchCarryItemState::TouchCarryItemState(al::LiveActor* host, const TouchCarryItemStateParam* param)
    : al::ActorStateBase("DRCドラッグによる運び状態", host), mParam(param) {
    initNerve(&NrvTouchCarryItemStateHold, 0);
    if (!mParam) mParam = &cDefaultParam;
}
void TouchCarryItemState::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvTouchCarryItemStateHold);
}
void TouchCarryItemState::kill() {
    al::NerveStateBase::kill();
    mPointer = nullptr;
}
bool TouchCarryItemState::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (!mPointer && rc::isEnableTouchPointerGrabItem(mHostActor) && al::isMsgTouchCarryItem(msg)) {
        mPointer = static_cast<const DrcTouchPointer*>(pointer->getHost());
        return true;
    }
    if (al::isNerve(this, &NrvTouchCarryItemStateHold) && al::isMsgTouchReleaseItem(msg))
        al::setNerve(this, &NrvTouchCarryItemStateRelease);
    return false;
}
void TouchCarryItemState::exeHold() {
    al::LiveActor* host = mHostActor;
    if (al::isFirstStep(this)) {
        al::validateHitSensors(host);
        al::setVelocityZero(host);
        al::setColliderRadius(host, 30.0f);
        al::offCollide(host);
    }
    sead::Vector3f up = sead::Vector3f::zero;
    if (rc::isTouchDrcAssistByPointer(mPointer)) {
        sead::Vector3f pos = al::getTrans(mPointer);
        rc::getTouchPointerUpDir(&up, mPointer);
        al::copyPose(host, mPointer);
        al::resetPosition(host, pos + mParam->holdOffset * up, false);
    } else {
        sead::Vector3f slide = sead::Vector3f::zero;
        if (!rc::tryCalcTouchPointerSlideDirOnWorldByPointer(&slide, mPointer) || slide.length() < mParam->throwThreshold) {
            al::setNerve(this, &NrvTouchCarryItemStateRelease);
        } else {
            sead::Vector3f velocity = slide;
            velocity.y = 0.0f;
            al::normalize(&velocity);
            velocity *= mParam->throwSpeed;
            velocity += sead::Vector3f(0.0f, mParam->throwLift, 0.0f);
            mThrowVelocity.set(velocity);
            al::setNerve(this, &NrvTouchCarryItemStateThrow);
        }
    }
}
void TouchCarryItemState::exeRelease() {
    if (al::isFirstStep(this)) {
        setFinalPos();
        al::onCollide(mHostActor);
        al::setColliderRadius(mHostActor, 30.0f);
        mReleaseVelocity.set(0.0f, mParam->releaseLift, 0.0f);
    }
    kill();
}
void TouchCarryItemState::setFinalPos() {
    al::LiveActor* host = mHostActor;
    al::getTrans(host);
    sead::Vector3f normal = mPointer->getHitNormal();
    const sead::Vector3f& pos = al::getTrans(mPointer);
    al::resetPosition(host, pos + (mParam->releaseOffset + 2.0f) * normal, false);
}
void TouchCarryItemState::exeThrow() {
    if (al::isFirstStep(this)) {
        setFinalPos();
        al::onCollide(mHostActor);
        al::setColliderRadius(mHostActor, 30.0f);
    }
    kill();
}
bool TouchCarryItemState::isItemThrow() const { return al::isNerve(this, &NrvTouchCarryItemStateThrow); }
