#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Movement/AnimScaleController.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/DrcUtil.hpp"

namespace rc {
al::LiveActor* getFirstTouchingTouchPointer(const al::LiveActor* pActor);
bool tryCalcTouchPointerSlideDirOnScreenByPointer(sead::Vector2f* pDir,
                                                 const al::LiveActor* pPointer);
}

namespace {
NERVE_DECL(ActorStateSupportFreeze, Bind)
NERVES_MAKE_NOSTRUCT(ActorStateSupportFreeze, Bind)
ActorStateSupportFreezeParam cDefaultParam;
al::AnimScaleParam cHardScaleParam(0.2f, 0.91f, 0.2f, 1.8f, 0.06f, 0.12f, 0.91f,
                                   20, 0.25f, 0.98f, 5.2f, 0.02f);
}

/** @brief Creates default touch-freeze settings with stroking disabled. */
ActorStateSupportFreezeParam::ActorStateSupportFreezeParam()
    : mIsStroke(false), mStrokeEffectInterval(15), mIsSyncSubActor(false),
      mIsAppearItem(false), mStrokeFrame(0), mItemOffset(0.0f, 0.0f, 0.0f) {}

/** @brief Creates touch-freeze settings with optional stroking effects.
 * @param isStroke Whether stroking effects are enabled.
 * @param strokeEffectInterval Steps between stroking effects.
 */
ActorStateSupportFreezeParam::ActorStateSupportFreezeParam(bool isStroke, int strokeEffectInterval)
    : mIsStroke(isStroke), mStrokeEffectInterval(strokeEffectInterval), mIsSyncSubActor(false),
      mIsAppearItem(false), mStrokeFrame(0), mItemOffset(0.0f, 0.0f, 0.0f) {}

/** @brief Creates touch-freeze and stroking reward settings.
 * @param isStroke Whether stroking effects are enabled.
 * @param strokeEffectInterval Steps between stroking effects.
 * @param isSyncSubActor Whether to synchronize child actors.
 * @param isAppearItem Whether stroking can produce an item.
 * @param strokeFrame Stroking duration required for an item.
 * @param rItemOffset Item position offset from the host.
 */
ActorStateSupportFreezeParam::ActorStateSupportFreezeParam(bool isStroke, int strokeEffectInterval,
        bool isSyncSubActor, bool isAppearItem, int strokeFrame, const sead::Vector3f& rItemOffset)
    : mIsStroke(isStroke), mStrokeEffectInterval(strokeEffectInterval),
      mIsSyncSubActor(isSyncSubActor), mIsAppearItem(isAppearItem),
      mStrokeFrame(strokeFrame), mItemOffset(rItemOffset) {}

/** @brief Constructs the touch-freeze state and its vibration controller.
 * @param pHost Actor frozen by touch assistance.
 * @param pParam Freeze settings, or nullptr to use defaults.
 */
ActorStateSupportFreeze::ActorStateSupportFreeze(al::LiveActor* pHost,
        const ActorStateSupportFreezeParam* pParam)
    : al::ActorStateBase("サポートフリーズ状態", pHost), mParam(pParam) {
    mScaleController = new al::AnimScaleController(nullptr);
    initNerve(&NrvActorStateSupportFreezeBind, 0);
    if (!mParam) {
        mParam = &cDefaultParam;
    }
}

/** @brief Starts the binding state and captures the actor's original scale. */
void ActorStateSupportFreeze::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvActorStateSupportFreezeBind);
    if (mIsScaleAnim) {
        mScaleController->setOriginalScale(al::getScale(mHostActor));
    }
}

/** @brief Restores saved motion, animation, and scale and stops touch effects. */
void ActorStateSupportFreeze::kill() {
    if (mIsScaleAnim && al::isSklAnimExist(mHostActor) && al::isSklAnimPlaying(mHostActor, 0)) {
        al::setSklAnimFrameRate(mHostActor, mSklAnimFrameRate, 0);
    }
    al::setVelocity(mHostActor, mVelocity);
    if (mIsScaleAnim) {
        mScaleController->stopAndReset();
        al::setScale(mHostActor, mScaleController->getScale());
    }
    al::tryDeleteEffect(mHostActor, "TouchStop");
    al::tryStopSe(mHostActor, "PgTouchStroked");
    if (mParam->mIsSyncSubActor) {
        for (int i = 0; i < al::getSubActorNum(mHostActor); i++) {
            al::LiveActor* pSubActor = al::getSubActor(mHostActor, i);
            if (al::isSklAnimExist(pSubActor) && al::isSklAnimPlaying(pSubActor, 0)) {
                al::setSklAnimFrameRate(pSubActor, mSklAnimFrameRate, 0);
            }
            if (mIsScaleAnim) {
                al::setScale(pSubActor, mScaleController->getScale());
            }
        }
    }
    al::ActorStateBase::kill();
}

/** @brief Switches to the harder vibration settings. */
void ActorStateSupportFreeze::setScaleAnimTypeHard() {
    mScaleController->setAnimScaleParam(&cHardScaleParam);
}

/** @brief Gets the current continuous stroking duration.
 * @return Number of stroking steps.
 */
int ActorStateSupportFreeze::getStrokeFrame() const { return mStrokeFrame; }

/** @brief Refreshes the freeze timer for a touch-assist sensor message.
 * @param pMsg Incoming message.
 * @param pSensor Touch actor sensor.
 * @return Whether the message is touch assistance.
 */
bool ActorStateSupportFreeze::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSensor) {
    if (al::isMsgTouchAssist(pMsg)) {
        setTouchActor(pSensor);
        mStepAfterTouch = 0;
        return true;
    }
    return false;
}

/** @brief Refreshes the freeze timer for screen-pointer assistance.
 * @param pMsg Incoming message.
 * @param pPointer Touch pointer.
 * @param pTarget Target of the screen-pointer message.
 * @return Whether the message is touch assistance.
 */
bool ActorStateSupportFreeze::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
        al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    if (al::isMsgTouchAssist(pMsg)) {
        setTouchActor(pPointer);
        mStepAfterTouch = 0;
        return true;
    }
    return false;
}

/** @brief Finds the actor corresponding to a touch pointer.
 * @param pPointer Touch pointer.
 * @return Whether a touch actor was found.
 */
bool ActorStateSupportFreeze::setTouchActor(al::ScreenPointer* pPointer) {
    mTouchActor = DrcFunction::tryFindDrcTouchActor(mHostActor, pPointer);
    return mTouchActor != nullptr;
}

/** @brief Uses a sensor's host as the touch actor.
 * @param pSensor Touch actor sensor.
 * @return Whether the sensor has a host actor.
 */
bool ActorStateSupportFreeze::setTouchActor(al::HitSensor* pSensor) {
    mTouchActor = al::getSensorHost(pSensor);
    return mTouchActor != nullptr;
}

/** @brief Freezes motion, responds to stroking, and releases after touch expires. */
void ActorStateSupportFreeze::exeBind() {
    if (al::isFirstStep(this)) {
        mStepAfterTouch = 0;
        if (mIsScaleAnim && al::isSklAnimExist(mHostActor) && al::isSklAnimPlaying(mHostActor, 0)) {
            mSklAnimFrameRate = al::getSklAnimFrameRate(mHostActor, 0);
            al::setSklAnimFrameRate(mHostActor, 0.0f, 0);
            if (mParam->mIsSyncSubActor) {
                for (int i = 0; i < al::getSubActorNum(mHostActor); i++) {
                    al::LiveActor* pSubActor = al::getSubActor(mHostActor, i);
                    if (al::isSklAnimExist(pSubActor) && al::isSklAnimPlaying(pSubActor, 0)) {
                        al::setSklAnimFrameRate(pSubActor, 0.0f, 0);
                    }
                }
            }
        } else {
            mSklAnimFrameRate = 1.0f;
        }
        mVelocity = al::getVelocity(mHostActor);
        al::setVelocityZero(mHostActor);
        mScaleController->startVibration();
        al::tryEmitEffect(mHostActor, "TouchStop", nullptr);
    }
    if (al::isExistSeKeeper(mHostActor)) {
        al::holdSeWithParam(mHostActor, "PgTouchFreeze", 1.0f, nullptr);
    }
    al::setVelocityZero(mHostActor);
    if (mParam->mIsStroke) {
        mNoStrokeFrame++;
        if (!mTouchActor) {
            mTouchActor = rc::getFirstTouchingTouchPointer(mHostActor);
        }
        sead::Vector2f slide;
        if (mTouchActor && rc::tryCalcTouchPointerSlideDirOnScreenByPointer(&slide, mTouchActor) &&
            slide.squaredLength() > 1.0f) {
            mStrokeFrame++;
            mNoStrokeFrame = 0;
            if (mStrokeFrame % mParam->mStrokeEffectInterval == 0) {
                al::startHitReactionHitEffect(mHostActor, "撫でる", al::getTrans(mHostActor));
            }
            if (mStrokeFrame == 1) {
                al::tryStartSe(mHostActor, "PgTouchStroked", nullptr);
            }
        }
        al::isExistSeKeeper(mHostActor);
        if (mNoStrokeFrame > 15) {
            al::tryStopSe(mHostActor, "PgTouchStroked");
            mStrokeFrame = 0;
        }
    }
    if (mParam->mIsAppearItem && !mIsItemAppeared && mParam->mStrokeFrame <= mStrokeFrame) {
        al::tryStopSe(mHostActor, "PgTouchStroked");
        mIsItemAppeared = true;
        sead::Vector3f front(0.0f, 0.0f, 0.0f);
        al::calcFrontDir(&front, mHostActor);
        al::appearItemTiming(mHostActor, "撫でる", al::getTrans(mHostActor) + mParam->mItemOffset, front);
    }
    if (mStepAfterTouch++ >= 45 && !mIsKeepFreeze) {
        kill();
    } else if (mIsScaleAnim) {
        mScaleController->update();
        al::setScale(mHostActor, mScaleController->getScale());
        if (mParam->mIsSyncSubActor) {
            for (int i = 0; i < al::getSubActorNum(mHostActor); i++) {
                al::setScale(al::getSubActor(mHostActor, i), mScaleController->getScale());
            }
        }
    }
}
