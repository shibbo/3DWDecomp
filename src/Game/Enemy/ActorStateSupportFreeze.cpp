#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Movement/AnimScaleController.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/DrcUtil.hpp"
#include <basis/seadNew.h>

namespace rc {
al::LiveActor* getFirstTouchingTouchPointer(const al::LiveActor*);
bool tryCalcTouchPointerSlideDirOnScreenByPointer(sead::Vector2f*, const al::LiveActor*);
}

namespace {
NERVE_DECL(ActorStateSupportFreeze, Bind)
NERVES_MAKE_NOSTRUCT(ActorStateSupportFreeze, Bind)
ActorStateSupportFreezeParam sDefaultParam;
al::AnimScaleParam sHardScaleParam(0.2f, 0.91f, 0.2f, 1.8f, 0.06f, 0.12f, 0.91f,
                                  20, 0.25f, 0.98f, 5.2f, 0.02f);
}

ActorStateSupportFreezeParam::ActorStateSupportFreezeParam()
    : mEnableStroke(false), mStrokeEffectInterval(15), mFreezeSubActors(false), mEmitItem(false),
      mItemStrokeCount(0), mItemOffset(0.0f, 0.0f, 0.0f) {}
ActorStateSupportFreezeParam::ActorStateSupportFreezeParam(bool stroke, int interval)
    : mEnableStroke(stroke), mStrokeEffectInterval(interval), mFreezeSubActors(false), mEmitItem(false),
      mItemStrokeCount(0), mItemOffset(0.0f, 0.0f, 0.0f) {}
ActorStateSupportFreezeParam::ActorStateSupportFreezeParam(bool stroke, int interval, bool subActors,
    bool item, int itemCount, const sead::Vector3f& offset)
    : mEnableStroke(stroke), mStrokeEffectInterval(interval), mFreezeSubActors(subActors),
      mEmitItem(item), mItemStrokeCount(itemCount), mItemOffset(offset) {}

ActorStateSupportFreeze::ActorStateSupportFreeze(al::LiveActor* host, const ActorStateSupportFreezeParam* param)
    : al::ActorStateBase("サポートフリーズ状態", host), mParam(param) {
    mScaleController = new al::AnimScaleController(nullptr);
    initNerve(&NrvActorStateSupportFreezeBind, 0);
    if (!mParam) mParam = &sDefaultParam;
}
void ActorStateSupportFreeze::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvActorStateSupportFreezeBind);
    if (mAnimateScale) mScaleController->setOriginalScale(al::getScale(mHostActor));
}
void ActorStateSupportFreeze::kill() {
    if (mAnimateScale && al::isSklAnimExist(mHostActor) && al::isSklAnimPlaying(mHostActor, 0))
        al::setSklAnimFrameRate(mHostActor, mAnimFrameRate, 0);
    al::setVelocity(mHostActor, mVelocity);
    if (mAnimateScale) {
        mScaleController->stopAndReset();
        al::setScale(mHostActor, mScaleController->getScale());
    }
    al::tryDeleteEffect(mHostActor, "TouchStop");
    al::tryStopSe(mHostActor, "PgTouchStroked");
    if (mParam->mFreezeSubActors) {
        for (int i = 0; i < al::getSubActorNum(mHostActor); ++i) {
            al::LiveActor* actor = al::getSubActor(mHostActor, i);
            if (al::isSklAnimExist(actor) && al::isSklAnimPlaying(actor, 0))
                al::setSklAnimFrameRate(actor, mAnimFrameRate, 0);
            if (mAnimateScale) al::setScale(actor, mScaleController->getScale());
        }
    }
    al::NerveStateBase::kill();
}
void ActorStateSupportFreeze::setScaleAnimTypeHard() { mScaleController->setAnimScaleParam(&sHardScaleParam); }
int ActorStateSupportFreeze::getStrokeFrame() const { return mStrokeFrame; }
bool ActorStateSupportFreeze::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sensor) {
    if (!al::isMsgTouchAssist(msg)) return false;
    mTouchActor = al::getSensorHost(sensor);
    mReleaseTimer = 0;
    return true;
}
bool ActorStateSupportFreeze::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer, al::ScreenPointTarget*) {
    if (!al::isMsgTouchAssist(msg)) return false;
    mTouchActor = DrcFunction::tryFindDrcTouchActor(mHostActor, pointer);
    mReleaseTimer = 0;
    return true;
}
bool ActorStateSupportFreeze::setTouchActor(al::ScreenPointer* pointer) {
    mTouchActor = DrcFunction::tryFindDrcTouchActor(mHostActor, pointer);
    return mTouchActor != nullptr;
}
bool ActorStateSupportFreeze::setTouchActor(al::HitSensor* sensor) {
    mTouchActor = al::getSensorHost(sensor);
    return mTouchActor != nullptr;
}
void ActorStateSupportFreeze::exeBind() {
    if (al::isFirstStep(this)) {
        mReleaseTimer = 0;
        if (mAnimateScale && al::isSklAnimExist(mHostActor) && al::isSklAnimPlaying(mHostActor, 0)) {
            mAnimFrameRate = al::getSklAnimFrameRate(mHostActor, 0);
            al::setSklAnimFrameRate(mHostActor, 0.0f, 0);
            if (mParam->mFreezeSubActors) {
                for (int i = 0; i < al::getSubActorNum(mHostActor); ++i) {
                    al::LiveActor* actor = al::getSubActor(mHostActor, i);
                    if (al::isSklAnimExist(actor) && al::isSklAnimPlaying(actor, 0))
                        al::setSklAnimFrameRate(actor, 0.0f, 0);
                }
            }
        } else mAnimFrameRate = 1.0f;
        mVelocity = al::getVelocity(mHostActor);
        al::setVelocityZero(mHostActor);
        mScaleController->startVibration();
        al::tryEmitEffect(mHostActor, "TouchStop", nullptr);
    }
    if (al::isExistSeKeeper(mHostActor)) al::holdSeWithParam(mHostActor, "PgTouchFreeze", 1.0f, nullptr);
    al::setVelocityZero(mHostActor);
    if (mParam->mEnableStroke) {
        ++mStrokeIdleTimer;
        if (!mTouchActor) mTouchActor = rc::getFirstTouchingTouchPointer(mHostActor);
        sead::Vector2f direction;
        if (mTouchActor && rc::tryCalcTouchPointerSlideDirOnScreenByPointer(&direction, mTouchActor) && direction.squaredLength() > 1.0f) {
            mStrokeIdleTimer = 0;
            ++mStrokeFrame;
            if (mStrokeFrame % mParam->mStrokeEffectInterval == 0)
                al::startHitReactionHitEffect(mHostActor, "撫でる", al::getTrans(mHostActor));
            if (mStrokeFrame == 1) al::tryStartSe(mHostActor, "PgTouchStroked", nullptr);
        }
        al::isExistSeKeeper(mHostActor);
        if (mStrokeIdleTimer >= 16) {
            al::tryStopSe(mHostActor, "PgTouchStroked");
            mStrokeFrame = 0;
        }
    }
    if (mParam->mEmitItem && !mItemEmitted && mParam->mItemStrokeCount <= mStrokeFrame) {
        al::tryStopSe(mHostActor, "PgTouchStroked");
        mItemEmitted = true;
        sead::Vector3f front(0.0f, 0.0f, 0.0f);
        al::calcFrontDir(&front, mHostActor);
        al::appearItemTiming(mHostActor, "撫でる", al::getTrans(mHostActor) + mParam->mItemOffset, front);
    }
    if (mReleaseTimer++ >= 45) {
        if (!mKeepFrozen) {
            kill();
            return;
        }
    }
    if (mAnimateScale) {
        mScaleController->update();
        al::setScale(mHostActor, mScaleController->getScale());
        if (mParam->mFreezeSubActors)
            for (int i = 0; i < al::getSubActorNum(mHostActor); ++i)
                al::setScale(al::getSubActor(mHostActor, i), mScaleController->getScale());
    }
}
