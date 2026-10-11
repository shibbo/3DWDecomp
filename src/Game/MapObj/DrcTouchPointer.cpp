#include "MapObj/DrcTouchPointer.hpp"
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>
#include "Library/ActorUtil.hpp"
#include "Library/Audio/System/AudioKeeper.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/HitSensor/SensorMsg.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Screen/ScreenPointer.hpp"
#include "Library/Screen/ScreenPointerUtil.hpp"
#include "Library/Se/Function/MeInfoKeeper.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Se/Project/SePlayParamList.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include "MapObj/DrcTouchAssistInfo.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "MapObj/Fury/DrcTouchEffectTraceTracker.hpp"
#include "MapObj/Stamp.hpp"
#include "MapObj/StampDirector.hpp"
#include "MapObj/TouchPointTransparent.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace al {
SENSOR_MSG(TouchReleaseItem);
SENSOR_MSG(TouchAssist);
SENSOR_MSG(TouchAssistTrig);
SENSOR_MSG(TouchAssistTrigNoPat);
SENSOR_MSG(TouchCarryItem);
SENSOR_MSG(TouchAssistBurn);
SENSOR_MSG(TouchAssistNoPat);
}  // namespace al

namespace rc {
SENSOR_MSG(TouchAssistBurnPeto);
}  // namespace rc

namespace {
NERVE_DECL(DrcTouchPointer, Wait);
NERVE_DECL(DrcTouchPointer, ReleaseItem);
NERVE_DECL(DrcTouchPointer, ThrowItem);

class DrcTouchPointerNrvDisAppearForce : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DrcTouchPointer>()->exeDisAppear();
    }
};

NERVE_DECL(DrcTouchPointer, HiddenGyroNoTouch);
NERVE_DECL(DrcTouchPointer, FadeToGyroNoTouch);
NERVE_DECL(DrcTouchPointer, HiddenGyroTouch);
NERVE_DECL(DrcTouchPointer, Knock);
NERVE_DECL(DrcTouchPointer, BurnStart);
NERVE_DECL(DrcTouchPointer, Burn);

class DrcTouchPointerNrvGrabStampStart : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DrcTouchPointer>()->exeGrabItemStart();
    }
};

NERVE_DECL(DrcTouchPointer, GrabItemStart);
NERVE_DECL(DrcTouchPointer, Stroke);
NERVE_DECL(DrcTouchPointer, Hold);
NERVE_DECL(DrcTouchPointer, TouchObj);
NERVE_DECL(DrcTouchPointer, WaitGyroNoTouch);
NERVE_DECL(DrcTouchPointer, DisAppear);

class DrcTouchPointerNrvReleaseStamp : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DrcTouchPointer>()->exeReleaseItem();
    }
};

NERVE_DECL(DrcTouchPointer, GrabItem);

class DrcTouchPointerNrvGrabStamp : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DrcTouchPointer>()->exeGrabItem();
    }
};

// Non-const nerve objects: they are merged into one data block, so neighbouring nerves are
// addressed relative to each other (the game addresses DisAppear as Wait + 0x80).
#define NERVE_MAKE_MUTABLE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

FOR_EACH(NERVE_MAKE_MUTABLE, DrcTouchPointer, Wait, ReleaseItem, ThrowItem, DisAppearForce,
         HiddenGyroNoTouch, FadeToGyroNoTouch, HiddenGyroTouch, Knock, BurnStart, Burn,
         GrabStampStart, GrabItemStart, Stroke, Hold, TouchObj, WaitGyroNoTouch, DisAppear,
         GrabItem, GrabStamp)
NERVES_MAKE_NOSTRUCT(DrcTouchPointer, ReleaseStamp)

/// Hit flags collected by DrcTouchPointer::control().
enum HitFlag : s32 {
    HitFlag_Collision = 0x2,
    HitFlag_Target = 0x4,
    HitFlag_CarryItem = 0x10,
    HitFlag_Stroke = 0x20,
    HitFlag_Knock = 0x40,
    HitFlag_Hold = 0x80,
    HitFlag_Burn = 0x100,
    HitFlag_TouchObj = 0x200,
};

/**
 * @brief Places the pointer at the hit position, turned towards the hit normal and the camera,
 *        and copies the pose to the transparent model.
 * @param pActor pointer actor
 * @param rPos hit position
 * @param rNormal hit normal
 * @param pTransparent transparent model following the pointer
 */
void updatePointerPose(al::LiveActor* pActor, const sead::Vector3f& rPos,
                       const sead::Vector3f& rNormal, al::LiveActor* pTransparent) {
    al::setTrans(pActor, rPos);
    sead::Quatf* quat = al::getQuatPtr(pActor);
    al::turnQuatYDirRadian(quat, *quat, rNormal,
                           al::isDead(pActor) ? sead::Mathf::pi() : sead::Mathf::deg2rad(8.0f));

    sead::Vector3f up = sead::Vector3f::ey;
    al::calcUpDir(&up, pActor);
    sead::Vector3f front = al::getCameraPos(pActor) - al::getTrans(pActor);
    al::verticalizeVec(&front, up, front);

    if (!al::normalizeOrZero(&front)) {
        sead::Quatf* frontQuat = al::getQuatPtr(pActor);
        al::turnQuatZDirRadian(frontQuat, *frontQuat, front,
                               al::isDead(pActor) ? sead::Mathf::pi() :
                                                    sead::Mathf::deg2rad(10.0f));
    }

    al::copyPose(pTransparent, pActor);
}
}  // namespace

/**
 * @brief Shows the model unless the pointer is drawn reversed, and marks it active.
 */
inline void DrcTouchPointer::showModel() {
    if (!isReverseTransparent()) {
        al::showModelIfHide(this);
    }

    mIsModelActive = true;
}

/**
 * @brief Stops the trace effect if it is still emitting.
 */
inline void DrcTouchPointer::tryDeleteTraceEffect() {
    if (al::isEffectEmitting(this, "Trace")) {
        al::deleteEffect(this, "Trace");
    }
}

/**
 * @brief Freezes the skeletal animation on its first frame.
 */
inline void DrcTouchPointer::stopSklAnim() {
    if (al::isSklAnimPlaying(this, 0)) {
        al::setSklAnimFrame(this, 0.0f, 0);
    }
}

/**
 * @brief Tells the carried item that it was released.
 */
inline void DrcTouchPointer::sendMsgReleaseItem() {
    al::sendMsgScreenPointTarget(al::SensorMsgTouchReleaseItem(), mScreenPointer,
                                 al::getScreenPointTarget(mGrabActor, "Body"));
}

/**
 * @brief Constructs the pointer.
 * @param pName actor name
 * @param pTouchInfo touch-screen state
 * @param isMiddleRange use the middle check length
 * @param isLongRange use the long check length
 * @param pDirector owning assist director
 * @param pTraceTracker trace effect tracker shared by the pointers
 */
DrcTouchPointer::DrcTouchPointer(const char* pName, const DrcTouchAssistInfo* pTouchInfo,
                                 bool isMiddleRange, bool isLongRange,
                                 DrcAssistDirector* pDirector,
                                 DrcTouchEffectTraceTracker* pTraceTracker)
    : al::LiveActor(pName), mTouchInfo(pTouchInfo), mDirector(pDirector),
      mTraceTracker(pTraceTracker) {
    mCheckLength = isLongRange ? 125000.0f : isMiddleRange ? 8000.0f : 5000.0f;
}

/**
 * @brief Starts an action and stops the trace effect.
 * @param pActionName action name
 * @param isKeepTrace keep the trace effect playing
 */
void DrcTouchPointer::startAction(const char* pActionName, bool isKeepTrace) {
    al::startAction(this, pActionName);

    if (mTraceTracker != nullptr && !isKeepTrace) {
        mTraceTracker->endPlayTraceEffect(this);
    }
}

/**
 * @brief Hides the model and stops the trace effect.
 */
void DrcTouchPointer::hideModelIfShow() {
    mIsModelActive = false;
    al::hideModelIfShow(this);

    if (mTraceTracker != nullptr) {
        mTraceTracker->endPlayTraceEffect(this);
    }
}

/**
 * @brief Moves the pointer back to the previous frame's hit position.
 */
void DrcTouchPointer::forcePrevPos() {
    mHitPos = mPrevHitPos;
    mHitNormal = mPrevHitNormal;
    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
}

/**
 * @brief Initializes the model, the transparent model and the screen pointer.
 * @param rInfo actor init info
 */
void DrcTouchPointer::init(const al::ActorInitInfo& rInfo) {
    mIsSingleMode = al::isSingleMode(rInfo);
    al::initActorWithArchiveName(this, rInfo, "TouchPoint", mIsSingleMode ? "SM" : nullptr);
    al::initNerve(this, &NrvDrcTouchPointerWait, 0);
    al::invalidateClipping(this);
    mTransparent = new TouchPointTransparent("DRC Touch Pointer(Transparent)");
    mTransparent->init(rInfo);

    if (mIsSingleMode && mCheckLength == 5000.0f) {
        mCheckLength = 125000.0f;
    }

    mScreenPointer = new al::ScreenPointer(rInfo, this, al::getTransPtr(this));
    makeActorDead();

    if (al::isNear(mCheckLength, 125000.0f, 0.001f)) {
        al::setScaleAll(this, 10.0f);
    }
}

/**
 * @brief Checks whether a screen pointer is this pointer's.
 * @param pPointer screen pointer
 * @return true if it is the own screen pointer
 */
bool DrcTouchPointer::isPointer(const al::ScreenPointer* pPointer) const {
    return mScreenPointer == pPointer;
}

/**
 * @brief Checks whether a sensor is this pointer's.
 * @param pSensor hit sensor
 * @return true if it is the own sensor
 */
bool DrcTouchPointer::isSensor(const al::HitSensor* pSensor) const {
    return al::getHitSensor(this, nullptr) == pSensor;
}

/**
 * @brief Resets the character texture of both models.
 * @param character character index (unused)
 */
void DrcTouchPointer::setCharacter(s32 character) {
    if (mCharacter != -1) {
        mCharacter = -1;
        al::startMclAnimAndSetFrameAndStop(this, "reset", 0.0f);
        al::startMclAnimAndSetFrameAndStop(mTransparent, "reset", 0.0f);
    }
}

/**
 * @brief Resets the character texture of both models.
 */
void DrcTouchPointer::setInvalidChar() {
    if (mCharacter != -1) {
        mCharacter = -1;
        al::startMclAnimAndSetFrameAndStop(this, "reset", 0.0f);
        al::startMclAnimAndSetFrameAndStop(mTransparent, "reset", 0.0f);
    }
}

/**
 * @brief Finds what the pointer touches, sends the touch messages and picks the next nerve.
 */
void DrcTouchPointer::control() {
    mIsGyroStarted = false;
    mHitFlags = 0;
    mHitTarget = nullptr;

    if (mThrowCooldown > 0) {
        mThrowCooldown--;
    }

    if (al::isNerve(this, &NrvDrcTouchPointerReleaseItem) ||
        al::isNerve(this, &NrvDrcTouchPointerThrowItem) ||
        al::isNerve(this, &NrvDrcTouchPointerDisAppearForce)) {
        return;
    }

    if (al::isNerve(this, &NrvDrcTouchPointerDisAppear) && !mTouchInfo->isTouch()) {
        return;
    }

    if (isGyroNoTouchNerve()) {
        if (isPhysicallyTouchingScreen()) {
            startAppear(false);
            return;
        }

        if (calcHitPosAndNormal()) {
            if (al::isNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch)) {
                al::setNerve(this, &NrvDrcTouchPointerFadeToGyroNoTouch);
            }

            mHitFlags = 0;

            if (!al::isHideModel(this) && mTraceTracker->tryPlayTraceEffect(this)) {
                al::tryUpdateEffectMaterialCode(this, "Dummy");
                al::tryUpdateSeMaterialCode(this, "Dummy");
            } else {
                tryDeleteTraceEffect();
            }

            if (mIsSnapshotMode) {
                sead::Vector3f cameraPos = al::getCameraPos(this);
                al::hitCheckSegmentScreenPointTarget(mScreenPointer, cameraPos, mHitPos);

                for (s32 i = 0; i < mScreenPointer->getHitTargetNum(); i++) {
                    al::ScreenPointTarget* target =
                        mScreenPointer->getHitTargetAndSetPosAndNormal(i);
                    sead::Vector3f targetPos = al::getScreenPointTargetPos(target);
                    sead::Vector3f diff = targetPos - al::getCameraPos(this);

                    if (diff.length() < 500.0f || diff.length() > 8500.0f) {
                        continue;
                    }

                    if (rc::Stamp::isStampActor(al::getScreenPointTargetHost(target))) {
                        static_cast<rc::Stamp*>(al::getScreenPointTargetHost(target))
                            ->startHover();
                        break;
                    }
                }
            }
        } else {
            al::setNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch);
        }

        mPrevHitPos = mHitPos;
        mPrevHitNormal = mHitNormal;
        return;
    }

    mPrevHitPos = mHitPos;
    mPrevHitNormal = mHitNormal;
    bool isHit = calcHitPosAndNormal();
    bool isHidden = al::isNerve(this, &NrvDrcTouchPointerHiddenGyroTouch);

    if (isHit) {
        if (isHidden) {
            al::setNerve(this, &NrvDrcTouchPointerWait);
        }

        initialAppear();
        mHitFlags |= HitFlag_Collision;

        if (!al::isHideModel(this) && mTraceTracker->tryPlayTraceEffect(this)) {
            al::tryUpdateEffectMaterialCode(this,
                                            al::getMaterialCodeName(mHitArrowInfo->mTriangle));
            al::tryUpdateSeMaterialCode(this, al::getMaterialCodeName(mHitArrowInfo->mTriangle));
        } else {
            tryDeleteTraceEffect();
        }

        if (al::isEqualString(al::getFloorCodeName(mHitArrowInfo->mTriangle), "DamageFire")) {
            mHitFlags |= HitFlag_Burn;
        }
    } else if (isHidden) {
        return;
    }

    showModel();

    if (isItemGrab()) {
        if ((mHitFlags & HitFlag_Collision) == 0 && !mIsSnapshotMode) {
            al::setNerve(this, &NrvDrcTouchPointerReleaseItem);
            sendMsgReleaseItem();
        }

        return;
    }

    sead::Vector3f cameraPos = al::getCameraPos(this);
    sead::Vector3f hitPos = mHitPos;
    al::hitCheckSegmentScreenPointTarget(mScreenPointer, cameraPos, mHitPos);

    if (mIsSnapshotMode) {
        mIsSnapshotTouched = false;

        if (mTouchInfo->_49 && !mTouchInfo->isTouch()) {
            return;
        }

        if (mGrabActor != nullptr) {
            return;
        }

        rc::Stamp* topStamp = nullptr;
        s32 topDepth = -1;

        for (s32 i = 0; i < mScreenPointer->getHitTargetNum(); i++) {
            al::ScreenPointTarget* target = mScreenPointer->getHitTargetAndSetPosAndNormal(i);
            sead::Vector3f targetPos = al::getScreenPointTargetPos(target);
            sead::Vector3f diff = targetPos - al::getCameraPos(this);

            if (diff.length() < 500.0f || diff.length() > 8500.0f) {
                continue;
            }

            if (rc::Stamp::isStampActor(al::getScreenPointTargetHost(target))) {
                auto* stamp = static_cast<rc::Stamp*>(al::getScreenPointTargetHost(target));

                if (stamp->getDepth() > topDepth) {
                    topDepth = stamp->getDepth();
                    topStamp = stamp;
                }
            }
        }

        if (topStamp != nullptr) {
            if (!mTouchInfo->_48) {
                topStamp->startHover();
            } else if (topStamp->canGrab()) {
                mGrabCandidate = topStamp;
            }
        }

        if (mGrabCandidate == nullptr && mTouchInfo->_48) {
            startNewStamp();
        }

        if (mGrabCandidate != nullptr && mTouchInfo->_44 >= 8) {
            if (al::sendMsgScreenPointTarget(al::SensorMsgTouchCarryItem(), mScreenPointer,
                                             al::getScreenPointTarget(mGrabCandidate, 0))) {
                startStampGrab(mGrabCandidate, false);
            } else {
                mGrabCandidate = nullptr;
            }
        }
    } else {
        if (mTouchInfo->_49 && !mTouchInfo->isTouch()) {
            return;
        }

        f32 minDistance = sead::Mathf::maxNumber();

        for (s32 i = 0; i < mScreenPointer->getHitTargetNum(); i++) {
            al::ScreenPointTarget* target = mScreenPointer->getHitTargetAndSetPosAndNormal(i);
            sead::Vector3f targetPos = al::getScreenPointTargetPos(target);
            f32 distance = (targetPos - al::getCameraPos(this)).length();

            if (distance < 500.0f || distance > mCheckLength + 500.0f) {
                continue;
            }

            if (mIsSingleMode) {
                if (al::sendMsgScreenPointTargetSM(al::SensorMsgTouchAssist(), mScreenPointer,
                                                   target)) {
                    mHitFlags |= HitFlag_Target | HitFlag_Stroke;
                    mHitPos.set(al::getHitScreenPointTargetPos(mScreenPointer));
                    mHitNormal.set(al::getHitScreenPointTargetNormal(mScreenPointer));
                    mHitTarget = mScreenPointer->getHitTargetAndSetPosAndNormal(i);
                    break;
                }

                if (distance < minDistance) {
                    mHitFlags |= HitFlag_Target | HitFlag_Stroke;
                    mHitPos.set(al::getHitScreenPointTargetPos(mScreenPointer));
                    mHitNormal.set(al::getHitScreenPointTargetNormal(mScreenPointer));
                    mHitTarget = mScreenPointer->getHitTargetAndSetPosAndNormal(i);
                    minDistance = distance;
                }

                continue;
            }

            if (mTouchInfo->_48) {
                if (al::sendMsgScreenPointTarget(al::SensorMsgTouchAssistTrig(), mScreenPointer,
                                                 target)) {
                    mHitFlags |= HitFlag_Target | HitFlag_Knock;
                    mKnockActionName = "Knock";
                    mHitPos.set(al::getHitScreenPointTargetPos(mScreenPointer));
                    mHitNormal.set(al::getHitScreenPointTargetNormal(mScreenPointer));
                    break;
                }

                if (al::sendMsgScreenPointTarget(al::SensorMsgTouchAssistTrigNoPat(),
                                                 mScreenPointer, target)) {
                    mHitFlags |= HitFlag_Target | HitFlag_Knock;
                    mKnockActionName = "ObjPointWait";
                    mHitPos.set(al::getHitScreenPointTargetPos(mScreenPointer));
                    mHitNormal.set(al::getHitScreenPointTargetNormal(mScreenPointer));
                    break;
                }
            } else if (al::sendMsgScreenPointTarget(al::SensorMsgTouchAssist(), mScreenPointer,
                                                    target)) {
                mHitFlags |= HitFlag_Target | HitFlag_Stroke;
                mHitPos.set(al::getHitScreenPointTargetPos(mScreenPointer));
                mHitNormal.set(al::getHitScreenPointTargetNormal(mScreenPointer));
                break;
            }

            if (!al::isNerve(this, &NrvDrcTouchPointerKnock) && mTouchInfo->isTouch() &&
                al::sendMsgScreenPointTarget(al::SensorMsgTouchCarryItem(), mScreenPointer,
                                             target)) {
                mHitFlags |= HitFlag_Target | HitFlag_CarryItem;
                mGrabActor = al::getScreenPointTargetHost(target);
                break;
            }

            if (al::sendMsgScreenPointTarget(rc::SensorMsgTouchAssistBurnPeto(), mScreenPointer,
                                             target)) {
                sead::Vector3f up = sead::Vector3f::ey;
                al::calcUpDir(&up, al::getScreenPointTargetHost(target));

                if (al::isNear(up, mHitNormal, 0.01f)) {
                    mHitFlags |= HitFlag_Burn;
                    mHitPos.set(hitPos);
                    mHitPos -= mHitNormal * 60.0f;
                    break;
                }
            }

            if (al::sendMsgScreenPointTarget(al::SensorMsgTouchAssistBurn(), mScreenPointer,
                                             target)) {
                mHitFlags |= HitFlag_Burn;
                mHitPos.set(al::getHitScreenPointTargetPos(mScreenPointer));
                mHitNormal.set(al::getHitScreenPointTargetNormal(mScreenPointer));
                break;
            }

            if (al::sendMsgScreenPointTarget(al::SensorMsgTouchAssistNoPat(), mScreenPointer,
                                             target)) {
                mHitPos.set(al::getHitScreenPointTargetPos(mScreenPointer));
                mHitFlags |= HitFlag_Target | HitFlag_TouchObj;
                break;
            }
        }

        if (!mIsSingleMode &&
            (mHitFlags & (HitFlag_Collision | HitFlag_Target)) == HitFlag_Collision) {
            if (mTouchInfo->_48) {
                if (al::sendMsgTouchAssistTrig(mHitSensor, al::getHitSensor(this, nullptr))) {
                    mHitFlags |= HitFlag_Knock;
                }
            } else if (al::sendMsgTouchAssist(mHitSensor, al::getHitSensor(this, nullptr))) {
                mHitFlags |= HitFlag_Hold;
            } else if (al::sendMsgStrokeTransparent(mHitSensor, al::getHitSensor(this, nullptr))) {
                mHitFlags |= HitFlag_Stroke;
            }
        }
    }

    if (al::isNerve(this, &NrvDrcTouchPointerKnock)) {
        if (mHitFlags & HitFlag_Knock) {
            al::setNerve(this, &NrvDrcTouchPointerKnock);
        }

        return;
    }

    if (mHitFlags & HitFlag_Burn) {
        if (!al::isNerve(this, &NrvDrcTouchPointerBurnStart) &&
            !al::isNerve(this, &NrvDrcTouchPointerBurn)) {
            al::setNerve(this, &NrvDrcTouchPointerBurnStart);
        }
    } else if (mHitFlags & HitFlag_CarryItem) {
        if (!isItemGrab()) {
            if (mIsSnapshotMode) {
                al::setNerve(this, &NrvDrcTouchPointerGrabStampStart);
            } else {
                al::setNerve(this, &NrvDrcTouchPointerGrabItemStart);
            }
        }
    } else if (mHitFlags & HitFlag_Knock) {
        if (!al::isNerve(this, &NrvDrcTouchPointerKnock)) {
            al::setNerve(this, &NrvDrcTouchPointerKnock);
        }
    } else if (mHitFlags & HitFlag_Stroke) {
        if (!al::isNerve(this, &NrvDrcTouchPointerStroke)) {
            al::setNerve(this, &NrvDrcTouchPointerStroke);
        }
    } else if (mHitFlags & HitFlag_Hold) {
        if (!al::isNerve(this, &NrvDrcTouchPointerHold)) {
            al::setNerve(this, &NrvDrcTouchPointerHold);
        }
    } else if (mHitFlags & HitFlag_TouchObj) {
        if (!al::isNerve(this, &NrvDrcTouchPointerTouchObj)) {
            al::setNerve(this, &NrvDrcTouchPointerTouchObj);
        }
    } else if (mHitFlags & (HitFlag_Collision | HitFlag_Target)) {
        if (!al::isNerve(this, &NrvDrcTouchPointerWait)) {
            al::setNerve(this, &NrvDrcTouchPointerWait);
        }
    } else if (!al::isNerve(this, &NrvDrcTouchPointerDisAppear)) {
        setDisappearNerve(true);
    }

    sead::Vector2f slideDir = {0.0f, 0.0f};

    if (mDirector != nullptr) {
        mDirector->tryCalcTouchPointerSlideDirOnScreen(&slideDir);
    }

    f32 slideSpeed = slideDir.length();
    f32 prevSlideSpeed = mSlideSpeed;
    f32 seParam = (slideSpeed + prevSlideSpeed) * 0.5f;

    if (sead::Mathf::abs(slideSpeed) / (sead::Mathf::abs(prevSlideSpeed) + 1.0f) > 2.0f) {
        seParam = 0.0f;
    }

    if (al::isNerve(this, &NrvDrcTouchPointerWait) || al::isNerve(this, &NrvDrcTouchPointerHold) ||
        al::isNerve(this, &NrvDrcTouchPointerStroke) ||
        al::isNerve(this, &NrvDrcTouchPointerTouchObj)) {
        al::holdSeWithParam(this, "Touched", seParam, nullptr);
    }

    mSlideSpeed = slideSpeed;
}

/**
 * @brief Checks whether the pointer is disappearing.
 * @return true in the DisAppear nerve
 */
bool DrcTouchPointer::isDisappearNerve() const {
    return al::isNerve(this, &NrvDrcTouchPointerDisAppear);
}

/**
 * @brief Checks whether the pointer is in one of the untouched gyro nerves.
 * @return true while fading to, waiting in or hidden in the untouched gyro state
 */
bool DrcTouchPointer::isGyroNoTouchNerve() const {
    return al::isNerve(this, &NrvDrcTouchPointerFadeToGyroNoTouch) ||
           al::isNerve(this, &NrvDrcTouchPointerWaitGyroNoTouch) ||
           al::isNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch);
}

/**
 * @brief Checks whether the touch screen is physically touched.
 * @return true if the touch pad port is triggered or held
 */
bool DrcTouchPointer::isPhysicallyTouchingScreen() const {
    s32 port = mDirector->getTouchPadPort();

    if (port == -1) {
        return false;
    }

    return al::isPadTriggerTouch(port) || al::isPadHoldTouch(port);
}

/**
 * @brief Makes the pointer appear.
 * @param isGyro appearing from the gyro state
 */
void DrcTouchPointer::startAppear(bool isGyro) {
    if (al::isNerve(this, &NrvDrcTouchPointerDisAppearForce)) {
        return;
    }

    if (!isGyro) {
        mGyroState = GyroTouchState_None;
    } else if (mGyroState == GyroTouchState_NoTouch) {
        return;
    }

    al::LiveActor::appear();

    if (isReverseTransparent()) {
        hideModelIfShow();
    } else {
        al::showModelIfHide(this);
    }

    mIsModelActive = true;
    al::setNerve(this, &NrvDrcTouchPointerWait);
    control();

    if (al::isNerve(this, &NrvDrcTouchPointerDisAppear)) {
        if (al::isAlive(mTransparent)) {
            mTransparent->kill();
        }

        kill();
    } else if (al::isNerve(this, &NrvDrcTouchPointerHiddenGyroTouch)) {
        if (al::isAlive(mTransparent)) {
            mTransparent->kill();
        }

        hideModelIfShow();
    } else {
        initialAppear();
    }
}

/**
 * @brief Casts the touch ray into the stage collision.
 * @return true if a collision was hit
 */
bool DrcTouchPointer::calcHitPosAndNormal() {
    sead::Vector3f cameraPos = al::getCameraPos(this);
    sead::Vector3f dir;
    al::normalizeOrZero(&dir, mTouchInfo->getTouchPos() - cameraPos);
    f32 checkLength = mIsSnapshotMode ? 8000.0f : mCheckLength;
    mHitPos = cameraPos + dir * checkLength;
    al::CollisionPartsFilterSpecialPurpose partsFilter("DrcAssist");
    rc::TriangleWallFilter wallFilter;
    rc::TriangleWallNoCodeFilter noCodeFilter;
    const al::TriangleFilterBase* triFilter = nullptr;

    if (mIsSnapshotMode) {
        rc::StampDirector* stampDirector = mDirector->getStampDirector();

        if (stampDirector != nullptr) {
            if (stampDirector->isUseNoCodeWallFilter()) {
                triFilter = &noCodeFilter;
            } else {
                triFilter = &wallFilter;
            }
        }
    }

    s32 hitNum = alCollisionUtil::checkStrikeArrow(this, cameraPos + dir * 500.0f,
                                                   dir * checkLength, &partsFilter, triFilter);

    if (hitNum == 0) {
        return false;
    }

    al::HitSensor* sensor = al::getHitSensor(this, nullptr);
    f32 minDistance = sead::Mathf::maxNumber();
    const al::ArrowHitInfo* nearest = nullptr;

    for (s32 i = 0; i != hitNum; i++) {
        const al::ArrowHitInfo* hitInfo = alCollisionUtil::getStrikeArrowInfo(this, i);

        if (hitInfo->mTriangle.isValid() && al::isFloorCode("IgnoreTouch", hitInfo->mTriangle)) {
            continue;
        }

        if (al::sendMsgScreenPointInvalidCollisionParts(hitInfo->mTriangle.getSensor(), sensor)) {
            continue;
        }

        if (minDistance > hitInfo->_70) {
            mHitPos.e = hitInfo->mPos.e;
            minDistance = hitInfo->_70;
            nearest = hitInfo;
        }
    }

    if (nearest == nullptr) {
        return false;
    }

    mHitSensor = nearest->mTriangle.getSensor();
    mHitNormal.set(*nearest->mTriangle.getFaceNormal());
    al::normalizeOrZero(&mHitNormal);
    mHitArrowInfo = nearest;
    return true;
}

/**
 * @brief Places the pointer at the hit position and emits the echo.
 */
void DrcTouchPointer::initialAppear() {
    if (al::isAlive(mTransparent)) {
        mTransparent->kill();
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
    al::resetPosition(this, false);

    if (!mIsSnapshotMode) {
        bool isEcho = rc::emitEcho(this, al::getTrans(this), 500.0f, 70, false);

        if ((isEcho || mIsSingleMode) && !mIsSnapshotMode) {
            al::startSe(this, "EchoBlockTouch");
        }
    }
}

/**
 * @brief Checks whether an item or stamp is held.
 * @return true while grabbing an item or a stamp
 */
bool DrcTouchPointer::isItemGrab() const {
    if (mGrabActor == nullptr) {
        return false;
    }

    return al::isNerve(this, &NrvDrcTouchPointerGrabItemStart) ||
           al::isNerve(this, &NrvDrcTouchPointerGrabItem) ||
           al::isNerve(this, &NrvDrcTouchPointerGrabStampStart) ||
           al::isNerve(this, &NrvDrcTouchPointerGrabStamp);
}

/**
 * @brief Gets a hit target of the screen pointer.
 * @param index hit target index
 * @return the hit target
 */
al::ScreenPointTarget* DrcTouchPointer::getHitTarget(s32 index) const {
    return mScreenPointer->getHitTargetAndSetPosAndNormal(index);
}

/**
 * @brief Takes a new stamp from the stamp director and grabs it.
 */
void DrcTouchPointer::startNewStamp() {
    rc::StampDirector* stampDirector = mDirector->getStampDirector();

    if (stampDirector != nullptr) {
        mGrabActor = stampDirector->getNewStamp();

        if (mGrabActor != nullptr) {
            startStampGrab(mGrabActor, true);
        }
    }
}

/**
 * @brief Grabs a stamp.
 * @param pStamp stamp to grab
 * @param isKeepRotation keep the stamp rotation
 */
void DrcTouchPointer::startStampGrab(al::LiveActor* pStamp, bool isKeepRotation) {
    mHitFlags = HitFlag_Target | HitFlag_CarryItem;
    mGrabActor = pStamp;
    mGrabCandidate = nullptr;
    static_cast<rc::Stamp*>(pStamp)->startGrab(isKeepRotation);
}

/**
 * @brief Sets the nerve to disappear with, depending on the gyro state.
 * @param isKeepGyro stay in the gyro state
 */
void DrcTouchPointer::setDisappearNerve(bool isKeepGyro) {
    switch (mGyroState) {
    case GyroTouchState_Touch:
        if (isKeepGyro) {
            al::setNerve(this, &NrvDrcTouchPointerHiddenGyroTouch);
        } else {
            changeGyroState(GyroTouchState_NoTouch);
            al::setNerve(this, &NrvDrcTouchPointerFadeToGyroNoTouch);
        }

        break;
    case GyroTouchState_NoTouch:
        if (isKeepGyro) {
            al::setNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch);
        } else {
            changeGyroState(GyroTouchState_None);
            al::setNerve(this, &NrvDrcTouchPointerDisAppear);
        }

        break;
    default:
        al::setNerve(this, &NrvDrcTouchPointerDisAppear);
        break;
    }
}

/**
 * @brief Drops a new stamp when the touch is released in snapshot mode.
 * @param frame frames the touch was held
 */
void DrcTouchPointer::handleTouchRelease(s32 frame) {
    if (frame > 7 || !mIsSnapshotMode || mGrabActor != nullptr || mThrowCooldown != 0) {
        return;
    }

    mGrabCandidate = nullptr;
    startNewStamp();

    if (mGrabActor != nullptr) {
        mDirector->getStampDirector()->releaseStamp(static_cast<rc::Stamp*>(mGrabActor));
        mGrabActor = nullptr;
    }
}

/**
 * @brief Restarts the gyro fade timer in snapshot mode.
 */
void DrcTouchPointer::tryResetActiveTime() {
    if (mIsSnapshotMode && mGyroTimer < 240) {
        mGyroTimer = 0;
    }
}

/**
 * @brief Checks whether the model is drawn reversed (snapshot or single mode).
 * @return true in snapshot or single mode
 */
bool DrcTouchPointer::isReverseTransparent() const {
    return mIsSnapshotMode || mIsSingleMode;
}

/**
 * @brief Changes the gyro state, restarting the timer when entering the untouched state.
 * @param state new gyro state
 */
void DrcTouchPointer::changeGyroState(GyroTouchState state) {
    if (state == GyroTouchState_NoTouch && mGyroState != GyroTouchState_NoTouch) {
        mGyroTimer = 0;
    }

    mGyroState = state;
}

/**
 * @brief Enters snapshot mode, releasing what is held.
 */
void DrcTouchPointer::startSnapshotMode() {
    if (mGrabActor != nullptr) {
        sendMsgReleaseItem();
        mGrabActor = nullptr;
    }

    mThrowCooldown = 0;
    mGrabCandidate = nullptr;
    setDisappearNerve(true);
    mIsSnapshotMode = true;
    al::tryDeleteEffectAndParticle(this, "Trace");
    kill();
}

/**
 * @brief Leaves snapshot mode, releasing what is held.
 */
void DrcTouchPointer::endSnapshotMode() {
    mIsSnapshotMode = false;

    if (mGrabActor != nullptr) {
        sendMsgReleaseItem();
        mGrabActor = nullptr;
    }

    mGrabCandidate = nullptr;
    setDisappearNerve(true);
}

/**
 * @brief Checks whether something is held.
 * @return true if an item or stamp is held
 */
bool DrcTouchPointer::isHoldStamp() {
    return mGrabActor != nullptr;
}

/**
 * @brief Gets the number of screen point targets hit.
 * @return number of hit targets
 */
s32 DrcTouchPointer::getHitTargetTableSize() const {
    return mScreenPointer->getHitTargetNum();
}

/**
 * @brief Disappears slowly, as when the gyro pointer is released.
 */
void DrcTouchPointer::startDisappearGyro() {
    startDisappear();
    mIsSlowDisappear = true;
}

/**
 * @brief Releases or throws what is held, or disappears.
 */
void DrcTouchPointer::startDisappear() {
    mIsSlowDisappear = false;

    if (isItemGrab()) {
        if (mIsSnapshotMode) {
            tryThrowReleaseStamp();
            return;
        }

        sead::Vector3f slideDir = {0.0f, 0.0f, 0.0f};

        if (mDirector == nullptr ||
            !mDirector->tryCalcTouchPointerSlideDirOnWorld(&slideDir, this) ||
            slideDir.length() < 15.0f) {
            al::setNerve(this, &NrvDrcTouchPointerReleaseItem);
            return;
        }

        al::normalize(&slideDir);
        al::makeQuatFrontUp(al::getQuatPtr(this), -slideDir, sead::Vector3f::ey);
        al::setNerve(this, &NrvDrcTouchPointerThrowItem);
        return;
    }

    if (al::isNerve(this, &NrvDrcTouchPointerWait) ||
        al::isNerve(this, &NrvDrcTouchPointerStroke) ||
        al::isNerve(this, &NrvDrcTouchPointerBurnStart) ||
        al::isNerve(this, &NrvDrcTouchPointerBurn) || al::isNerve(this, &NrvDrcTouchPointerHold) ||
        al::isNerve(this, &NrvDrcTouchPointerTouchObj)) {
        setDisappearNerve(false);
    }
}

/**
 * @brief Checks whether the pointer is visible.
 * @return true if alive and not disappeared or hidden
 */
bool DrcTouchPointer::isVisible() const {
    if (!al::isAlive(this)) {
        return false;
    }

    if (al::isNerve(this, &NrvDrcTouchPointerDisAppear) ||
        al::isNerve(this, &NrvDrcTouchPointerDisAppearForce) ||
        al::isNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch)) {
        return false;
    }

    return !al::isNerve(this, &NrvDrcTouchPointerHiddenGyroTouch);
}

/**
 * @brief Checks whether the pointer is visible and its model shown.
 * @return true if visible and shown
 */
bool DrcTouchPointer::isVisibleAndNotHidden() const {
    if (isReverseTransparent()) {
        return al::isAlive(this) && mIsModelActive;
    }

    return isVisible() && !al::isHideModel(this);
}

/**
 * @brief Checks whether the pointer is alive and not hidden in a gyro state.
 * @return true if alive and not hidden
 */
bool DrcTouchPointer::isCompletelyAlive() const {
    if (al::isDead(this)) {
        return false;
    }

    if (al::isNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch)) {
        return false;
    }

    return !al::isNerve(this, &NrvDrcTouchPointerHiddenGyroTouch);
}

/**
 * @brief Checks whether the pointer is releasing, throwing or disappearing.
 * @return true while disappearing
 */
bool DrcTouchPointer::isDisappearing() const {
    if (!al::isAlive(this)) {
        return false;
    }

    return al::isNerve(this, &NrvDrcTouchPointerReleaseItem) ||
           al::isNerve(this, &NrvDrcTouchPointerThrowItem) ||
           al::isNerve(this, &NrvDrcTouchPointerDisAppear) ||
           al::isNerve(this, &NrvDrcTouchPointerDisAppearForce);
}

/**
 * @brief Throws the held stamp if the touch was flicked, otherwise releases it.
 * @return true if the stamp was thrown
 */
bool DrcTouchPointer::tryThrowReleaseStamp() {
    rc::StampDirector* stampDirector = mDirector->getStampDirector();
    bool isThrow;

    if (stampDirector != nullptr) {
        sead::Vector2f flick = mTouchInfo->_2c - mTouchInfo->_34;

        if (flick.squaredLength() < 3025.0f) {
            mThrowCooldown = 1;
            mDirector->getStampDirector()->releaseStamp(static_cast<rc::Stamp*>(mGrabActor));
            isThrow = false;
        } else {
            sead::Vector3f velocity = mTouchInfo->mTouchPos - mTouchInfo->_10;
            stampDirector->throwStamp(static_cast<rc::Stamp*>(mGrabActor), velocity);
            isThrow = true;
        }
    } else {
        sendMsgReleaseItem();
        isThrow = false;
    }

    mGrabActor = nullptr;
    al::setNerve(this, &NrvDrcTouchPointerReleaseStamp);
    return isThrow;
}

/**
 * @brief Disappears immediately.
 */
void DrcTouchPointer::startDisappearForce() {
    mGyroState = GyroTouchState_None;
    mIsSlowDisappear = false;

    if (!al::isNerve(this, &NrvDrcTouchPointerDisAppearForce)) {
        al::setNerve(this, &NrvDrcTouchPointerDisAppearForce);
    }

    mGrabActor = nullptr;
}

/**
 * @brief Releases the held item or stamp.
 */
void DrcTouchPointer::startRelease() {
    if (!isItemGrab()) {
        return;
    }

    if (mIsSnapshotMode) {
        tryThrowReleaseStamp();
        return;
    }

    al::setNerve(this, &NrvDrcTouchPointerReleaseItem);
    sendMsgReleaseItem();
}

/**
 * @brief Fades the pointer out through the transparent model.
 */
void DrcTouchPointer::exeDisAppear() {
    if (al::isFirstStep(this) && !isReverseTransparent()) {
        mTransparent->setSlowDisappear(mIsSlowDisappear);
        mTransparent->appear();
        al::copyPose(mTransparent, this);

        if (!isReverseTransparent()) {
            al::copyAction(mTransparent, this);
        }
    }

    if (al::isStep(this, 2)) {
        hideModelIfShow();
    }

    if (al::isDead(mTransparent)) {
        al::setNerve(this, &NrvDrcTouchPointerWait);
        kill();
    }
}

/**
 * @brief Handles the gyro pointing button.
 * @param rIsPress button pressed this frame, cleared when consumed
 * @param rIsRelease button released this frame, cleared when consumed
 * @return true if the input was handled
 */
bool DrcTouchPointer::handleGyroButtonInput(bool& rIsPress, bool& rIsRelease) {
    if (mIsSnapshotMode && mGyroState == GyroTouchState_None &&
        !al::isNerve(this, &NrvDrcTouchPointerWait) &&
        !al::isNerve(this, &NrvDrcTouchPointerDisAppear)) {
        rIsRelease = false;
        rIsPress = false;
        return false;
    }

    if (rIsPress) {
        switch (mGyroState) {
        case GyroTouchState_None:
            mIsGyroStarted = true;
            changeGyroState(GyroTouchState_NoTouch);
            rIsRelease = false;
            rIsPress = false;

            if (al::isDead(this)) {
                al::LiveActor::appear();
                hideModelIfShow();
            }

            al::setNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch);
            return true;
        case GyroTouchState_NoTouch:
            changeGyroState(GyroTouchState_Touch);

            if (al::isNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch)) {
                if (al::isDead(this)) {
                    al::LiveActor::appear();
                    hideModelIfShow();
                }

                al::setNerve(this, &NrvDrcTouchPointerHiddenGyroTouch);
            } else {
                startAppear(true);
            }

            return true;
        default:
            return true;
        }
    }

    if (rIsRelease) {
        if (mGyroState != GyroTouchState_NoTouch) {
            return mGyroState == GyroTouchState_Touch;
        }
    } else if (mGyroState != GyroTouchState_NoTouch) {
        if (mGyroState != GyroTouchState_Touch) {
            return false;
        }

        rIsRelease = false;
        rIsPress = false;

        if (al::isNerve(this, &NrvDrcTouchPointerHiddenGyroTouch)) {
            changeGyroState(GyroTouchState_NoTouch);
            al::setNerve(this, &NrvDrcTouchPointerHiddenGyroNoTouch);
        } else {
            startDisappearGyro();
        }

        return true;
    }

    rIsRelease = false;
    rIsPress = false;
    return true;
}

/**
 * @brief Fades the pointer out into the untouched gyro state.
 */
void DrcTouchPointer::exeFadeToGyroNoTouch() {
    if (al::isFirstStep(this)) {
        getAudioKeeper()->kill();
        getEffectKeeper()->deleteAndClearEffectAll();
        al::deleteEffectAll(this);
    }

    if (al::isStep(this, 2)) {
        hideModelIfShow();
        al::setNerve(this, &NrvDrcTouchPointerWaitGyroNoTouch);
    }

    stopSklAnim();

    if (al::calcDistance(mTransparent, mHitPos) > 750.0f) {
        stopSklAnim();
        al::tryDeleteEmitterAndParticleAll(this);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
}

/**
 * @brief Fades the transparent model out over time and kills the pointer at the end.
 */
void DrcTouchPointer::updateTransparentGyro() {
    s32 time = mIsSnapshotMode ? 240 : 480;

    if (mGyroTimer++ < time) {
        mTransparent->setGyroDisappearAlpha(1.0f - mGyroTimer / time);
        return;
    }

    mGyroState = GyroTouchState_None;
    mTransparent->kill();
    al::setNerve(this, &NrvDrcTouchPointerWait);
    kill();
}

/**
 * @brief Waits in the untouched gyro state.
 */
void DrcTouchPointer::exeWaitGyroNoTouch() {
    if (al::isFirstStep(this)) {
        getAudioKeeper()->kill();
        getEffectKeeper()->deleteAndClearEffectAll();
        al::tryStartAction(this, "PointWait");
        al::deleteEffectAll(this);
    }

    f32 distance = al::calcDistance(mTransparent, mHitPos);
    stopSklAnim();

    if (distance > 750.0f) {
        stopSklAnim();
        al::tryDeleteEmitterAndParticleAll(this);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
    updateTransparentGyro();
}

/**
 * @brief Stays hidden in the untouched gyro state.
 */
void DrcTouchPointer::exeHiddenGyroNoTouch() {
    if (al::isFirstStep(this)) {
        getAudioKeeper()->kill();
        getEffectKeeper()->deleteAndClearEffectAll();
        al::tryStartAction(this, "PointWait");
        hideModelIfShow();

        if (al::isAlive(mTransparent)) {
            mTransparent->kill();
        }
    }

    updateTransparentGyro();
}

/**
 * @brief Stays hidden in the touched gyro state.
 */
void DrcTouchPointer::exeHiddenGyroTouch() {
    if (al::isFirstStep(this)) {
        if (!mIsSnapshotMode) {
            startAction("PointWait", false);
        }

        getAudioKeeper()->kill();
        getEffectKeeper()->deleteAndClearEffectAll();
        al::tryStartAction(this, "PointWait");
        hideModelIfShow();

        if (al::isAlive(mTransparent)) {
            mTransparent->kill();
        }
    }
}

/**
 * @brief Follows the touch position and plays the slide sound.
 */
void DrcTouchPointer::exeWait() {
    if (al::isFirstStep(this)) {
        if (!mIsSnapshotMode) {
            al::startAction(this, "PointWait");

            if (mTraceTracker != nullptr && !mTraceTracker->tryPlayTraceEffect(this)) {
                tryDeleteTraceEffect();
            }
        }

        mSeLocalVariable = -1;
    }

    if (al::calcDistance(this, mHitPos) > 750.0f) {
        stopSklAnim();
        al::tryDeleteEmitterAndParticleAll(this);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
    sead::Vector2f slideDir = {0.0f, 0.0f};

    if (mDirector != nullptr) {
        mDirector->tryCalcTouchPointerSlideDirOnScreen(&slideDir);
    }

    f32 slideSpeed = slideDir.length();
    al::MeInfo meInfo;
    meInfo._0 = -1;
    s32 localVariable = mSeLocalVariable;
    meInfo.mChordOffset = localVariable >= 0 ? -1 : 0;
    meInfo.mScaleOffset = 0;
    meInfo.mPitchOffset = localVariable;
    al::SePlayParamList* paramList = al::holdSeWithParam(this, "PointWait", slideSpeed, &meInfo);

    if (paramList != nullptr) {
        s32 value = 0;
        bool isGet = paramList->tryGetLocalVariable(value, 0);

        if (localVariable < 0 && isGet) {
            mSeLocalVariable = value;
        }
    }
}

/**
 * @brief Starts grabbing an item or a stamp.
 */
void DrcTouchPointer::exeGrabItemStart() {
    if (al::isFirstStep(this) && !mIsSnapshotMode) {
        startAction("GrabBallStart", false);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);

    if (mIsSnapshotMode || al::isActionEnd(this)) {
        if (al::isNerve(this, &NrvDrcTouchPointerGrabStampStart)) {
            al::setNerve(this, &NrvDrcTouchPointerGrabStamp);
        } else {
            al::setNerve(this, &NrvDrcTouchPointerGrabItem);
        }
    }
}

/**
 * @brief Holds an item or a stamp.
 */
void DrcTouchPointer::exeGrabItem() {
    if (al::isFirstStep(this) && !mIsSnapshotMode) {
        startAction("GrabBallLoop", false);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
}

/**
 * @brief Releases the held item or stamp, then disappears.
 */
void DrcTouchPointer::exeReleaseItem() {
    if (al::isFirstStep(this)) {
        if (!mIsSnapshotMode) {
            startAction("GrabBallEnd", false);
        }

        if (al::isNerve(this, &NrvDrcTouchPointerReleaseItem)) {
            al::updatePoseRotate(mGrabActor, sead::Vector3f::zero);
        }
    }

    if (mIsSnapshotMode || al::isActionEnd(this)) {
        setDisappearNerve(false);
        mGrabActor = nullptr;
    }
}

/**
 * @brief Throws the held item, then disappears.
 */
void DrcTouchPointer::exeThrowItem() {
    if (al::isFirstStep(this)) {
        startAction("Throw", false);
    }

    if (al::isActionEnd(this)) {
        setDisappearNerve(false);
        mGrabActor = nullptr;
    }
}

/**
 * @brief Strokes what is touched.
 */
void DrcTouchPointer::exeStroke() {
    if (al::isFirstStep(this)) {
        startAction("Stroke", false);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
}

/**
 * @brief Holds what is touched.
 */
void DrcTouchPointer::exeHold() {
    if (al::isFirstStep(this)) {
        startAction("Hold", false);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
}

/**
 * @brief Knocks on what is touched, then waits.
 */
void DrcTouchPointer::exeKnock() {
    if (al::isFirstStep(this)) {
        startAction(mKnockActionName.cstr(), false);
        mKnockStep = 21;
        al::resetPosition(this, mHitPos, false);
        bool isKnock = al::isEqualString(mKnockActionName.cstr(), "Knock");
        sead::Quatf* quat = al::getQuatPtr(this);

        if (isKnock) {
            al::makeQuatFrontUp(quat, -sead::Vector3f::ex, sead::Vector3f::ez);
        } else {
            al::makeQuatFrontUp(quat, sead::Vector3f::ey, sead::Vector3f::ez);
        }
    }

    if (al::isGreaterEqualStep(this, mKnockStep)) {
        al::setNerve(this, &NrvDrcTouchPointerWait);
    }
}

/**
 * @brief Starts burning on a fire floor.
 */
void DrcTouchPointer::exeBurnStart() {
    if (al::isFirstStep(this)) {
        startAction("Burn", false);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvDrcTouchPointerBurn);
    }
}

/**
 * @brief Burns on a fire floor.
 */
void DrcTouchPointer::exeBurn() {
    if (al::isFirstStep(this)) {
        startAction("BurnLoop", false);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
}

/**
 * @brief Touches an object.
 */
void DrcTouchPointer::exeTouchObj() {
    if (al::isFirstStep(this)) {
        startAction("ObjPointWait", false);
    }

    updatePointerPose(this, mHitPos, mHitNormal, mTransparent);
}

/**
 * @brief Checks whether an item may be grabbed.
 * @param pActor item to grab
 * @return true if nothing else is held, or the item is the one being held
 */
bool DrcTouchPointer::isEnableGrabItem(const al::LiveActor* pActor) const {
    if (mIsSnapshotMode) {
        return false;
    }

    if (mIsSingleMode) {
        return false;
    }

    if (al::isNerve(this, &NrvDrcTouchPointerGrabItemStart) ||
        al::isNerve(this, &NrvDrcTouchPointerGrabItem) ||
        al::isNerve(this, &NrvDrcTouchPointerReleaseItem) ||
        al::isNerve(this, &NrvDrcTouchPointerThrowItem)) {
        return mGrabActor == pActor;
    }

    return true;
}
