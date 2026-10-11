#include "MapObj/DrcAssistDirector.hpp"
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/PadReplayFunction.hpp"
#include "Library/Controller/ReplayController.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Layout/TouchEffect.hpp"
#include "MapObj/DrcTouchAssistInfo.hpp"
#include "MapObj/DrcTouchPointer.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ControllerEventWatcher.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
/// rc::getControllerStyle() value of a single Joy-Con held sideways the other way round.
constexpr s32 cControllerStyleFlipped = 3;

/**
 * @brief Returns the angle range (in degrees) the gyro pointer covers over the whole screen.
 * @param pWidth receives the horizontal range
 * @param pHeight receives the vertical range
 */
inline void getGyroScreenRange(f32* pWidth, f32* pHeight) {
    bool isDual = al::isPadTypeJoyDual(-1);
    *pWidth = isDual ? 55.0f : 45.0f;
    *pHeight = isDual ? 30.9375f : 25.3125f;
}

/**
 * @brief Checks whether the gyro pointer button (R) was pressed this frame. A lone Joy-Con has no
 *        such button in single mode.
 * @param port pad port
 * @param isSingleMode whether the game is in single mode
 * @return true on a button trigger
 */
inline bool isTriggerGyroButton(s32 port, bool isSingleMode) {
    if (al::isPadTypeJoySingle(port) && isSingleMode) {
        return false;
    }

    return al::isPadTriggerR(port);
}

/**
 * @brief Checks whether the gyro pointer button (R) is held.
 * @param port pad port
 * @param isSingleMode whether the game is in single mode
 * @return true while the button is held
 */
inline bool isHoldGyroButton(s32 port, bool isSingleMode) {
    if (al::isPadTypeJoySingle(port) && isSingleMode) {
        return false;
    }

    return al::isPadHoldR(port);
}

/**
 * @brief Wraps an angle in radians into [-pi, pi).
 * @param angle angle to wrap
 * @return the wrapped angle
 */
inline f32 wrapRadian(f32 angle) {
    return al::modf(angle + sead::Mathf::pi() + sead::Mathf::pi2(), sead::Mathf::pi2()) +
           -sead::Mathf::pi();
}
}  // namespace

/**
 * @brief Constructs the touch state.
 * @param isTouch whether the panel is touched
 * @param isTrigger whether the touch started this frame
 * @param isRelease whether the touch ended this frame
 * @param rTouchPos touched world position
 * @param rLayoutPos touched layout position
 */
DrcTouchAssistInfo::DrcTouchAssistInfo(bool isTouch, bool isTrigger, bool isRelease,
                                       const sead::Vector3f& rTouchPos,
                                       const sead::Vector2f& rLayoutPos)
    : mIsTouch(isTouch), _1(isTrigger), _2(isTrigger), _3(isRelease), mTouchPos(rTouchPos),
      _10(rTouchPos), _1c(rLayoutPos), mScreenPos(sead::Vector2f::zero),
      _2c(sead::Vector2f::zero), _34(sead::Vector2f::zero), _3c(sead::Vector2f::zero), _44(0),
      _48(false), _49(false), mIsUseScreenPos(false), _4b(false) {}

/**
 * @brief Clears the touch positions, the hold counter and the flags.
 */
void DrcTouchAssistInfo::reset() {
    _10 = sead::Vector3f::zero;
    mTouchPos = sead::Vector3f::zero;
    _3c = sead::Vector2f::zero;
    _2c = sead::Vector2f::zero;
    _34 = sead::Vector2f::zero;
    mScreenPos = sead::Vector2f::zero;
    _44 = 0;
    _48 = false;
    _49 = false;
    mIsUseScreenPos = false;
    _4b = false;
}

/**
 * @brief Returns the scene camera used to project the touch position.
 * @return the scene camera info
 */
al::SceneCameraInfo* DrcAssistDirector::getSceneCameraInfo() const {
    return mSceneCameraInfo;
}

/**
 * @brief Constructs the assist of one pad port.
 * @param port player pad port
 * @param isLongRange whether the pointer checks collisions further away
 */
DrcAssistDirector::DrcAssistDirector(s32 port, bool isLongRange)
    : mIsLongRange(isLongRange), mPort(port),
      mTouchPadPort(rc::calcTouchPanelPortByPortNum(port)) {
    mStampDirector = nullptr;
    mIgnoreTriggerFrames = 0;
    mIsDisappearGyro = false;
    mIsPrevGyroTouch = false;
    mIsPlayerExist = true;
    mTouchEffect = nullptr;
}

/**
 * @brief Checks whether a sensor belongs to the touch pointer.
 * @param pSensor sensor to check
 * @return true if the sensor is the pointer's
 */
bool DrcAssistDirector::isDrcDirector(const al::HitSensor* pSensor) const {
    return mTouchPointer->isSensor(pSensor);
}

/**
 * @brief Checks whether a screen pointer belongs to the touch pointer.
 * @param pPointer screen pointer to check
 * @return true if the screen pointer is the pointer's
 */
bool DrcAssistDirector::isDrcDirector(const al::ScreenPointer* pPointer) const {
    return mTouchPointer->isPointer(pPointer);
}

/**
 * @brief Finds the player controlled from this assist's port.
 * @return the player, or nullptr
 */
al::LiveActor* DrcAssistDirector::getPlayer() const {
    s32 port = mPort;
    return rc::tryFindPlayerFromInputPort(mPlayerHolder, port, false);
}

/**
 * @brief Creates the touch state, the pointer actor and its layout effect.
 * @param rInfo actor init info
 * @param pTraceTracker tracker of the pointer's trace effect
 */
void DrcAssistDirector::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo,
                                                   DrcTouchEffectTraceTracker* pTraceTracker) {
    mTouchAssistInfo = new DrcTouchAssistInfo(false, false, false, sead::Vector3f::zero,
                                              sead::Vector2f::zero);
    mSceneCameraInfo = rInfo.getActorSceneInfo().sceneCameraInfo;
    mSceneObjHolder = rInfo.getActorSceneInfo().sceneObjHolder;
    mTouchPointer = new DrcTouchPointer("DRC Touch Pointer", mTouchAssistInfo, mIsMiddleRange,
                                        mIsLongRange, this, pTraceTracker);
    mTouchPointer->init(rInfo);
    al::getPlayerControllerPort(1);
    mTouchEffect = new TouchEffect(al::getLayoutInitInfo(rInfo), mTouchAssistInfo);
    rc::addControllerEventListener(this, controllerEventCallback, this);
}

/**
 * @brief Controller event listener: requests a gyro recalibration when the controllers change.
 * @param event controller event
 * @param pUserData the assist
 */
void DrcAssistDirector::controllerEventCallback(s32 event, void* pUserData) {
    if (event == 0) {
        static_cast<DrcAssistDirector*>(pUserData)->mIsRequestCalib = true;
    }
}

/**
 * @brief Calibrates the gyro pointer while the controller is held still before the start.
 */
void DrcAssistDirector::checkPreStartPose() {
    if (mIsValid && doCheckPoseImpl(true)) {
        mIsPoseChecked = true;
    }
}

/**
 * @brief Recalibrates the gyro pointer if the controller is held in the reference pose.
 * @param isCheckDiff whether to only accept poses closer to the reference than before
 * @return true if the pointer was recalibrated
 */
bool DrcAssistDirector::doCheckPoseImpl(bool isCheckDiff) {
    sead::Vector3f threshold = {0.2f, 0.125f, 0.2f};
    sead::Vector3f limit = {0.6f, -0.35f, 0.6f};
    sead::Vector3f accel;

    if (isPoseExtreme(&accel, threshold, limit) >= 0) {
        return false;
    }

    if (isCheckDiff) {
        f32 diff = sead::Mathf::abs(-1.0f - accel.y);
        if (diff < mMinPoseDiff) {
            mMinPoseDiff = diff;
        } else {
            return false;
        }
    }

    calcJoyConBaseAngle();
    return true;
}

/**
 * @brief Checks whether the touch panel was touched this frame.
 * @return true on a touch trigger
 */
inline bool DrcAssistDirector::isTriggerTouchPanel() const {
    return mIsValid && mTouchPadPort != -1 && al::isPadTriggerTouch(mTouchPadPort);
}

/**
 * @brief Checks whether the touch panel is held.
 * @return true while the panel is touched
 */
inline bool DrcAssistDirector::isHoldTouchPanel() const {
    return mIsValid && mTouchPadPort != -1 && al::isPadHoldTouch(mTouchPadPort);
}

/**
 * @brief Updates the touch state from the touch panel or the gyro pointer, and moves the pointer
 *        actor and its layout effect.
 */
void DrcAssistDirector::update() {
    if (!mIsValid) {
        return;
    }

    if (mIgnoreTriggerFrames > 0) {
        mIgnoreTriggerFrames--;
        return;
    }

    if (mIsSnapshotMode) {
        return;
    }

    if (!mIsEnableUpdate && !mTouchPointer->isSnapshotMode()) {
        return;
    }

    bool isConnected = al::isPadConnected(mPort);
    al::LiveActor* player = nullptr;
    bool isInvalidChar = false;
    s32 character = -1;
    if (mPlayerHolder != nullptr) {
        player = rc::tryFindPlayerFromInputPort(mPlayerHolder, mPort, true);
        bool isPrevPlayerExist = mIsPlayerExist;
        if (player == nullptr) {
            mIsPlayerExist = false;
        }

        bool isActive;
        if (mPlayerAliveWatcher != nullptr) {
            character = mPlayerAliveWatcher->isActivePlayerPort(mPort);
            isActive = character >= 0;
        } else {
            isActive = !(rc::isActiveControlUser(this, mPort - 1) || isConnected);
            character = 9;
        }

        if (isActive || player != nullptr) {
            mIsPlayerExist = true;
        } else {
            mIsPlayerExist = mTouchPadPort != -1;
            if (mTouchPadPort == -1) {
                if (isPrevPlayerExist) {
                    mTouchAssistInfo->_49 = false;
                    mTouchAssistInfo->mIsTouch = false;
                }

                al::killLayoutIfActive(mTouchEffect);
                mTouchPointer->startDisappearForce();
                mIsDisappearGyro = false;
                mIsPrevGyroTouch = false;
                return;
            }

            isInvalidChar = true;
        }
    }

    if (!isConnected) {
        bool isTouchPanel = mTouchPadPort != -1 && (al::isPadTriggerTouch(mTouchPadPort) ||
                                                    al::isPadHoldTouch(mTouchPadPort));
        if (al::isAlive(mTouchPointer) && !isTouchPanel) {
            mTouchPointer->startDisappear();
        }
    }

    if (character >= 0) {
        mTouchPointer->setCharacter(character);
        mTouchEffect->setCharacter(character);
    } else if (isInvalidChar) {
        mTouchPointer->setInvalidChar();
        mTouchEffect->setInvalidChar();
    }

    if (mIsFirstUpdate) {
        if (!mIsPoseChecked) {
            mIsCheckPose = true;
        }

        mIsFirstUpdate = false;
    }

    if (mIsCheckPose) {
        sead::Vector3f threshold = {0.075f, 0.05f, 0.075f};
        sead::Vector3f limit = {0.6f, -0.35f, 0.6f};
        if (isPoseExtreme(nullptr, threshold, limit) < 0) {
            mIsRequestCalib = true;
            mIsCheckPose = false;
        }
    }

    s32 port = mPort;
    bool isGyro = rc::isEnableGyroTouchControl(port);
    bool isRecalib = isGyro && !mIsPrevGyroOrHold && !mIsPoseChecked;
    bool isHoldTouch = isHoldTouchPanel();

    bool isResetTrigger;
    if (al::isPadTypeHandheld(port)) {
        isResetTrigger = false;
    } else {
        bool isSingleMode = GameDataFunction::isSingleMode(this);
        if (al::isPadTypeJoySingle(port)) {
            isResetTrigger = !isSingleMode && al::isPadTriggerPressLeftStick(port);
        } else {
            isResetTrigger = al::isPadTriggerL(port);
        }
    }

    if (isInvalidChar) {
        isGyro = false;
        isResetTrigger = false;
        isRecalib = false;
    }

    if (isGyro) {
        if (isResetTrigger) {
            mTouchPointer->tryResetActiveTime();
        }

        if (isResetTrigger || mIsRequestCalib) {
            isRecalib = true;
            mIsRequestCalib = false;
            mZeroPushAmount = {0.0f, 0.0f};
        }
    }

    bool isGyroTrigger = !al::isPadTypeHandheld(port) && mIsValid && !isInvalidChar &&
                         isTriggerGyroButton(port, GameDataFunction::isSingleMode(this));
    bool isGyroHold = !al::isPadTypeHandheld(port) && mIsValid && !isInvalidChar &&
                      isHoldGyroButton(port, GameDataFunction::isSingleMode(this));
    bool isGyroActive = mTouchPointer->handleGyroButtonInput(isGyroTrigger, isGyroHold);
    mTouchAssistInfo->mIsUseScreenPos = isHoldTouch;
    mTouchAssistInfo->_1 = false;
    mTouchAssistInfo->_3 = false;
    mTouchAssistInfo->_2 = false;
    bool isGyroTouch = isGyro && isGyroActive;
    if (isTriggerTouchPanel()) {
        mTouchAssistInfo->_48 = true;
    } else {
        mTouchAssistInfo->_48 = isGyroTrigger;
    }

    bool isTouching = isHoldTouch || isGyroTouch;
    if (isTouching) {
        mTouchAssistInfo->_4b = false;
        if (mTouchFrame == 0) {
            mTouchAssistInfo->_1 = true;
        }

        if (cTouchKeepFrame - mTouchFrame >= 2) {
            mTouchAssistInfo->_2 = true;
        }

        mTouchFrame = cTouchKeepFrame;
        if (isGyroTouch && !isGyroHold) {
            if (mTouchAssistInfo->mIsTouch) {
                mTouchAssistInfo->_4b = true;
                mTouchAssistInfo->mScreenPos = mTouchAssistInfo->_2c;
                mTouchPointer->handleTouchRelease(mTouchAssistInfo->_44);
                mTouchAssistInfo->mIsTouch = false;
            } else {
                mTouchAssistInfo->_4b = false;
                mTouchAssistInfo->mIsTouch = false;
            }
        } else {
            mTouchAssistInfo->mIsTouch = true;
        }
    } else {
        if (mTouchFrame > 0) {
            if (mTouchFrame == cTouchKeepFrame) {
                mTouchAssistInfo->_3 = true;
            }

            mTouchFrame--;
        } else if (!isGyro) {
            mTouchScreenPos = {0.0f, 0.0f};
            mPrevTouchScreenPos = {0.0f, 0.0f};
        }

        if (mTouchAssistInfo->mIsTouch) {
            mTouchAssistInfo->mIsTouch = false;
            mTouchAssistInfo->_4b = true;
            mTouchAssistInfo->mScreenPos = mTouchAssistInfo->_2c;
            mTouchPointer->handleTouchRelease(mTouchAssistInfo->_44);
            mTouchPointer->forcePrevPos();
            if (mIsPrevGyroTouch) {
                mIsDisappearGyro = true;
            } else {
                mIsDisappearGyro = false;
            }
        } else {
            mTouchAssistInfo->_4b = false;
        }

        if (mTouchPointer->isCompletelyAlive()) {
            if (mIsDisappearGyro) {
                mTouchPointer->startDisappearGyro();
            } else {
                mTouchPointer->startDisappear();
            }
        }
    }

    if (!isGyroTouch) {
        if (!isHoldTouch && !isGyroActive) {
            mBaseMtx.makeIdentity();
            mZeroPushAmount = {0.0f, 0.0f};
            mBaseAngle = {0.0f, 0.0f, 0.0f};
            mDiffAngle = {0.0f, 0.0f, 0.0f};
        }
    } else if (!mIsPrevGyroTouch) {
        bool isDisappearing = mTouchPointer->isDisappearing();
        if (player != nullptr && !isDisappearing) {
            if (mTouchPointer->isGyroStarted()) {
                setJoyConBaseAngle(al::getTrans(player));
            }
        } else {
            mTouchPointer->startAppear(true);
            setJoyConBaseAngle(mTouchAssistInfo->mScreenPos);
        }
    }

    if (isGyroTouch || isHoldTouch || isGyroActive) {
        if (isRecalib) {
            calcJoyConBaseAngle();
        }

        if (isConnected) {
            calcJoyConDiffAngle();
        }
    }

    sead::Vector2f iconPos;
    if (isTouching) {
        sead::Vector2f pos = {0.0f, 0.0f};
        if (mTouchFrame == 0 || isResetTrigger) {
            pos.set(al::getLayoutDisplayWidth() * 0.5f, al::getLayoutDisplayHeight() * 0.5f);
            if (isResetTrigger) {
                mDiffAngle = {0.0f, 0.0f, 0.0f};
            }
        }

        if (isGyro || isHoldTouch) {
            f32 rangeWidth;
            f32 rangeHeight;
            getGyroScreenRange(&rangeWidth, &rangeHeight);
            f32 halfWidth = rangeWidth * 0.5f;
            f32 halfHeight = rangeHeight * 0.5f;

            bool isUpdateTouch = true;
            if (isHoldTouch) {
                al::ReplayController* replay = al::getReplayController(mPort);
                if (replay != nullptr) {
                    sead::Vector2f pointer = replay->getPointer();
                    pos.x = pointer.x * al::getSubDisplayWidth() / al::getLayoutDisplayWidth();
                    pos.y = pointer.y * al::getSubDisplayHeight() / al::getLayoutDisplayHeight();
                } else {
                    al::calcTouchScreenPos(&pos, mTouchPadPort);
                }

                f32 diffX = rangeWidth * (pos.x / al::getLayoutDisplayWidth()) - halfWidth;
                f32 diffY = rangeHeight * (pos.y / al::getLayoutDisplayHeight()) - halfHeight;
                mDiffAngle.x = diffX;
                mDiffAngle.y = diffY;
            } else {
                f32 diffX = mDiffAngle.x;
                f32 diffY = mDiffAngle.y;
                f32 width = al::getLayoutDisplayWidth();
                f32 height = al::getLayoutDisplayHeight();
                const sead::Vector2f minPos = {-0.0f, -0.0f};
                const sead::Vector2f maxPos = {width, height};
                pos.x = (halfWidth + diffX) / rangeWidth * width;
                pos.y = (halfHeight - diffY) / rangeHeight * height;

                sead::Vector2f offscreen;
                calcOffscreenAmount(offscreen, pos, minPos, maxPos);
                updateZeroPushAmount(offscreen, pos, minPos, maxPos);
                pos -= mZeroPushAmount;
                if (pos.x < minPos.x) {
                    pos.x = minPos.x;
                } else if (pos.x > maxPos.x) {
                    pos.x = maxPos.x;
                }

                if (pos.y < minPos.y) {
                    pos.y = minPos.y;
                } else if (pos.y > maxPos.y) {
                    pos.y = maxPos.y;
                }

                if (pos.x < 0.0f || pos.x > width || pos.y < 0.0f || pos.y > height ||
                    !isGyroTouch) {
                    isUpdateTouch = false;
                    mTouchScreenPos = {0.0f, 0.0f};
                    mPrevTouchScreenPos = {0.0f, 0.0f};
                    mTouchAssistInfo->reset();
                }
            }

            if (isUpdateTouch) {
                mPrevTouchScreenPos = mTouchScreenPos;
                mTouchScreenPos = pos;
                mTouchAssistInfo->_34 = mTouchAssistInfo->_2c;
                mTouchAssistInfo->_2c = mTouchAssistInfo->mScreenPos;
                mTouchAssistInfo->_10 = mTouchAssistInfo->mTouchPos;
                if (mTouchAssistInfo->_48) {
                    mTouchAssistInfo->_3c = mTouchScreenPos;
                    mTouchAssistInfo->_34 = mTouchScreenPos;
                    mTouchAssistInfo->_2c = mTouchScreenPos;
                    mTouchAssistInfo->mScreenPos = mTouchScreenPos;
                    mTouchAssistInfo->_44 = 0;
                } else {
                    mTouchAssistInfo->mScreenPos = mTouchScreenPos;
                    if (mTouchAssistInfo->_44 != 0x7fffffff) {
                        mTouchAssistInfo->_44++;
                    }
                }

                al::calcWorldPosFromScreenPosSub(&mTouchAssistInfo->mTouchPos, mTouchPointer, pos,
                                                 al::getCameraLookAt(this));
            }

            iconPos = pos;
        } else {
            iconPos = mTouchScreenPos;
        }

        mTouchAssistInfo->_49 = isGyro;
        const sead::Vector3f& touchPos = mTouchAssistInfo->mTouchPos;
        if (std::isnan(touchPos.x) || std::isnan(touchPos.y) || std::isnan(touchPos.z)) {
            mTouchAssistInfo->reset();
            mTouchScreenPos = {0.0f, 0.0f};
            mPrevTouchScreenPos = {0.0f, 0.0f};
        }
    } else {
        mTouchAssistInfo->_49 = false;
        iconPos = {0.0f, 0.0f};
    }

    mTouchIconPos = iconPos;
    if (isHoldTouch || isGyroActive ||
        (isGyroTouch && !mTouchPointer->isVisible() && !mTouchPointer->isGyroStarted())) {
        if (!mTouchEffect->isAlive()) {
            mTouchEffect->appear();
        }

        if (mTouchPointer->isVisibleAndNotHidden()) {
            al::getTrans(mTouchPointer);
            al::calcLayoutPosFromWorldPos(&mTouchAssistInfo->_1c, getSceneCameraInfo(),
                                          al::getTrans(mTouchPointer), 0);
            mTouchEffect->setTransparent(true);
        } else {
            al::calcLayoutPosFromScreenPos(&mTouchAssistInfo->_1c, mTouchScreenPos);
            mTouchEffect->setTransparent(false);
        }
    } else if (mTouchEffect->isAlive()) {
        mTouchEffect->kill();
    }

    mIsPrevGyroOrHold = isGyro || isHoldTouch;
    mIsPoseChecked = false;
    if (isTouching && !mTouchPointer->isCompletelyAlive()) {
        mTouchPointer->startAppear(!isHoldTouch);
    }

    mTouchAssistInfo->_1 = false;
    mIsPrevGyroTouch = isGyroTouch;
}

/**
 * @brief Hides the touch layout effect.
 */
void DrcAssistDirector::disappearTouchPointerEffect() {
    al::killLayoutIfActive(mTouchEffect);
}

/**
 * @brief Checks the controller pose against the reference pose.
 * @param pAccel receives the acceleration, can be nullptr
 * @param rThreshold maximum allowed gravity deviation
 * @param rLimit acceleration limits
 * @return 0 if the pose is unavailable or deviates, 1 if the acceleration is extreme, -1 if the
 *         controller is held in the reference pose
 */
s32 DrcAssistDirector::isPoseExtreme(sead::Vector3f* pAccel, const sead::Vector3f& rThreshold,
                                     const sead::Vector3f& rLimit) const {
    s32 port = mPort;
    bool isDual = al::isPadTypeJoyDual(-1);
    sead::Matrix34f poseMtx;
    if (!al::tryGetPadPoseMtx(&poseMtx, port, isDual)) {
        return 0;
    }

    sead::Vector3f accel;
    if (!al::tryGetPadAcceleration(&accel, port, isDual)) {
        return 0;
    }

    if (pAccel != nullptr) {
        *pAccel = accel;
    }

    sead::Matrix34f invMtx;
    if (!invMtx.setInverse(poseMtx)) {
        return 0;
    }

    sead::Vector3f gravity = poseMtx * accel;
    gravity.y += 1.0f;
    sead::Vector3f diff = invMtx * gravity;
    if (diff.x <= rThreshold.x && diff.y <= rThreshold.y && diff.z <= rThreshold.z) {
        if (sead::Mathf::abs(accel.x) > rLimit.x || sead::Mathf::abs(accel.z) > rLimit.z ||
            accel.y > rLimit.y) {
            return 1;
        }

        return -1;
    }

    return 0;
}

/**
 * @brief Sets the gyro reference so that the current pose points at a screen position.
 * @param rScreenPos screen position to point at
 */
void DrcAssistDirector::setJoyConBaseAngle(const sead::Vector2f& rScreenPos) {
    f32 rangeWidth;
    f32 rangeHeight;
    getGyroScreenRange(&rangeWidth, &rangeHeight);
    f32 halfWidth = rangeWidth * 0.5f;
    f32 halfHeight = rangeHeight * 0.5f;
    f32 x = rangeWidth * (rScreenPos.x / al::getLayoutDisplayWidth()) - halfWidth;
    f32 y = rangeHeight * (rScreenPos.y / al::getLayoutDisplayHeight()) - halfHeight;
    sead::Vector3f screenAngle = {x * 0.017453292f, y * -0.017453292f, 0.0f};
    sead::Vector3f offset;
    reverseCalcScreenAngle(mPort, &screenAngle, &offset);

    s32 port = mPort;
    sead::Matrix34f poseMtx;
    al::getPadSDKPoseMtx(&poseMtx, port, al::isPadTypeJoyDual(port));
    sead::Vector3f angle;
    poseMtx.getRotation(angle);
    angle -= offset;
    poseMtx.makeR(angle);
    mBaseAngle = -angle;

    sead::Vector3f padAngle;
    calcScreenAngle(port, &angle, &padAngle);
    unrotatePoseMtx(mPort, wrapRadian(padAngle.z), &poseMtx);
    mBaseMtx.setInverse(poseMtx);
    mZeroPushAmount = {0.0f, 0.0f};
}

/**
 * @brief Sets the gyro reference so that the current pose points at a world position.
 * @param rWorldPos world position to point at
 */
void DrcAssistDirector::setJoyConBaseAngle(const sead::Vector3f& rWorldPos) {
    sead::Vector2f screenPos;
    al::calcScreenPosFromWorldPosSubClampInScreen(&screenPos, this, rWorldPos);
    setJoyConBaseAngle(screenPos);
}

/**
 * @brief Makes the current controller pose the gyro reference pose.
 */
void DrcAssistDirector::calcJoyConBaseAngle() {
    s32 port = mPort;
    sead::Matrix34f poseMtx;
    al::getPadSDKPoseMtx(&poseMtx, port, al::isPadTypeJoyDual(port));
    sead::Vector3f angle;
    poseMtx.getRotation(angle);
    mBaseAngle = -angle;

    sead::Vector3f padAngle;
    calcScreenAngle(port, &angle, &padAngle);
    unrotatePoseMtx(port, wrapRadian(padAngle.z), &poseMtx);
    mBaseMtx.setInverse(poseMtx);
}

/**
 * @brief Computes the angle (in degrees) between the current pose and the reference pose.
 */
void DrcAssistDirector::calcJoyConDiffAngle() {
    s32 port = mPort;
    s32 isDual = al::isPadTypeJoyDual(port);
    sead::Vector3f padAngle;
    calcScreenAngle(port, &mBaseAngle, &padAngle);

    sead::Matrix34f poseMtx;
    al::getPadSDKPoseMtx(&poseMtx, port, isDual);
    sead::Vector3f angle;
    poseMtx.getRotation(angle);
    calcScreenAngle(port, &angle, &padAngle);
    if (!al::isPadTypeHandheld(port)) {
        unrotatePoseMtx(port, padAngle.z, &poseMtx);
    }

    poseMtx.setMul(mBaseMtx, poseMtx);
    poseMtx.getRotation(angle);
    calcScreenAngle(port, &angle, &padAngle);
    mDiffAngle.x = padAngle.x * 57.29578f;
    mDiffAngle.y = padAngle.y * 57.29578f;
    mDiffAngle.z = padAngle.z * 57.29578f;
}

/**
 * @brief Computes how far a position lies outside a rectangle.
 * @param rAmount receives the distance outside on each axis (0 when inside)
 * @param rPos position to check
 * @param rMin rectangle minimum
 * @param rMax rectangle maximum
 */
void DrcAssistDirector::calcOffscreenAmount(sead::Vector2f& rAmount, const sead::Vector2f& rPos,
                                            const sead::Vector2f& rMin,
                                            const sead::Vector2f& rMax) {
    if (rPos.x < rMin.x) {
        rAmount.x = rPos.x - rMin.x;
    } else if (rPos.x > rMax.x) {
        rAmount.x = rPos.x - rMax.x;
    } else {
        rAmount.x = 0.0f;
    }

    if (rPos.y < rMin.y) {
        rAmount.y = rPos.y - rMin.y;
    } else if (rPos.y > rMax.y) {
        rAmount.y = rPos.y - rMax.y;
    } else {
        rAmount.y = 0.0f;
    }
}

/**
 * @brief Pushes the gyro pointer's zero point along when the pointer leaves the screen, and pulls
 *        it back once the pointer returns.
 * @param rOffscreen distance the pointer is outside the screen
 * @param rPos pointer position
 * @param rMin screen minimum
 * @param rMax screen maximum
 */
void DrcAssistDirector::updateZeroPushAmount(const sead::Vector2f& rOffscreen,
                                             const sead::Vector2f& rPos,
                                             const sead::Vector2f& rMin,
                                             const sead::Vector2f& rMax) {
    f32 width = rMax.x - rMin.x;
    f32 height = rMax.y - rMin.y;

    if (rOffscreen.x == 0.0f && rOffscreen.y == 0.0f) {
        if (mZeroPushAmount.x > 0.0f) {
            f32 limit = mZeroPushAmount.x - width;
            if (rPos.x < limit) {
                mZeroPushAmount.x = width + rPos.x;
            }
        } else if (mZeroPushAmount.x < 0.0f) {
            f32 limit = width + mZeroPushAmount.x;
            if (rPos.x > limit) {
                mZeroPushAmount.x = rPos.x - width;
            }
        }

        if (mZeroPushAmount.y > 0.0f) {
            f32 limit = mZeroPushAmount.y - height;
            if (rPos.y < limit) {
                mZeroPushAmount.y = height + rPos.y;
            }
        } else if (mZeroPushAmount.y < 0.0f) {
            f32 limit = height + mZeroPushAmount.y;
            if (rPos.y > limit) {
                mZeroPushAmount.y = rPos.y - height;
            }
        }

        return;
    }

    if (rOffscreen.x != 0.0f) {
        if (al::sign(rOffscreen.x) != al::sign(mZeroPushAmount.x)) {
            mZeroPushAmount.x = rOffscreen.x;
        } else if (mZeroPushAmount.x > 0.0f) {
            if (rOffscreen.x > mZeroPushAmount.x) {
                mZeroPushAmount.x = rOffscreen.x;
            } else if (rOffscreen.x > 0.0f && rOffscreen.x < mZeroPushAmount.x - width) {
                mZeroPushAmount.x = width + rOffscreen.x;
            }
        } else if (mZeroPushAmount.x < 0.0f) {
            if (rOffscreen.x < mZeroPushAmount.x) {
                mZeroPushAmount.x = rOffscreen.x;
            } else if (rOffscreen.x < 0.0f && rOffscreen.x > width + mZeroPushAmount.x) {
                mZeroPushAmount.x = rOffscreen.x - width;
            }
        } else {
            mZeroPushAmount.x = rOffscreen.x;
        }
    }

    if (rOffscreen.y != 0.0f) {
        if (al::sign(rOffscreen.y) != al::sign(mZeroPushAmount.y)) {
            mZeroPushAmount.y = rOffscreen.y;
        } else if (mZeroPushAmount.y > 0.0f) {
            if (rOffscreen.y > mZeroPushAmount.y) {
                mZeroPushAmount.y = rOffscreen.y;
            } else if (rOffscreen.y > 0.0f && rOffscreen.y < mZeroPushAmount.y - height) {
                mZeroPushAmount.y = height + rOffscreen.y;
            }
        } else if (mZeroPushAmount.y < 0.0f) {
            if (rOffscreen.y < mZeroPushAmount.y) {
                mZeroPushAmount.y = rOffscreen.y;
            } else if (rOffscreen.y < 0.0f && rOffscreen.y > height + mZeroPushAmount.y) {
                mZeroPushAmount.y = rOffscreen.y - height;
            }
        } else {
            mZeroPushAmount.y = rOffscreen.y;
        }
    }
}

/**
 * @brief Enables the assist.
 */
void DrcAssistDirector::validate() {
    mIsValid = true;
}

/**
 * @brief Hides the pointer if the assist is (or is being made) invalid.
 * @param isForce whether to disable the assist
 */
void DrcAssistDirector::invalidate(bool isForce) {
    if (isForce) {
        mIsValid = false;
    }

    if (!mIsValid) {
        disappearTouchPointer();
    }
}

/**
 * @brief Hides the layout effect and makes the pointer disappear.
 */
void DrcAssistDirector::disappearTouchPointer() {
    al::killLayoutIfActive(mTouchEffect);
    if (al::isAlive(mTouchPointer)) {
        mTouchPointer->startDisappearForce();
    }
}

/**
 * @brief Disables the assist and removes the pointer at once.
 */
void DrcAssistDirector::invalidateImmediate() {
    mIsValid = false;
    disappearTouchPointerImmediately();
}

/**
 * @brief Hides the layout effect and kills the pointer and its effects at once.
 */
void DrcAssistDirector::disappearTouchPointerImmediately() {
    al::killLayoutIfActive(mTouchEffect);
    al::tryKillEmitterAndParticleAll(mTouchPointer);
    if (al::isAlive(mTouchPointer)) {
        mTouchPointer->kill();
    }
}

/**
 * @brief Checks whether the assist is enabled.
 * @return true if enabled
 */
bool DrcAssistDirector::isValid() const {
    return mIsValid;
}

/**
 * @brief Checks whether the screen is (or was just) touched.
 * @return true while touching
 */
bool DrcAssistDirector::isTouch() const {
    return mTouchFrame > 0;
}

/**
 * @brief Checks whether the screen is touched near a world position.
 * @param rPos world position
 * @param radius touch radius on screen
 * @return true if touched within the radius
 */
bool DrcAssistDirector::isTouch(const sead::Vector3f& rPos, f32 radius) const {
    if (mTouchFrame > 0) {
        return al::isTouchPosInCircleByWorldPos(rPos, mTouchPointer, radius, 10.0f,
                                                mTouchPadPort);
    }

    return false;
}

/**
 * @brief Checks whether the screen was touched this frame near a world position.
 * @param rPos world position
 * @param radius touch radius on screen
 * @return true if touched within the radius this frame
 */
bool DrcAssistDirector::isTouchTrigger(const sead::Vector3f& rPos, f32 radius) const {
    if (mTouchAssistInfo->_1) {
        return al::isTouchPosInCircleByWorldPos(rPos, mTouchPointer, radius, 10.0f,
                                                mTouchPadPort);
    }

    return false;
}

/**
 * @brief Checks whether the touch pointer is touching.
 * @return true if the touch state is set and touching
 */
bool DrcAssistDirector::isEnableTouchPointer() const {
    return mTouchAssistInfo != nullptr && mTouchAssistInfo->isTouch();
}

/**
 * @brief Checks whether the pointer can grab an actor.
 * @param pActor actor to grab
 * @return true if grabbable
 */
bool DrcAssistDirector::isEnableTouchPointerGrabItem(const al::LiveActor* pActor) const {
    return mTouchPointer->isEnableGrabItem(pActor);
}

/**
 * @brief Returns the touch control mode (only one mode exists).
 * @return 0
 */
s32 DrcAssistDirector::getTouchControlMode() {
    return 0;
}

/**
 * @brief Switches to the next touch control mode (no-op).
 */
void DrcAssistDirector::incTouchControlMode() {}

/**
 * @brief Toggles touch input (no-op).
 */
void DrcAssistDirector::toggleTouchDisabled() {}

/**
 * @brief Enables or disables touch input (no-op).
 * @param isDisabled whether to disable touch input
 */
void DrcAssistDirector::setTouchDisabled(bool isDisabled) {}

/**
 * @brief Checks whether touch input is disabled.
 * @return always true
 */
bool DrcAssistDirector::isTouchDisabled() {
    return true;
}

/**
 * @brief Returns the touch state.
 * @return the touch state
 */
DrcTouchAssistInfo* DrcAssistDirector::getTouchAssistInfo() {
    return mTouchAssistInfo;
}

/**
 * @brief Returns the touch state.
 * @return the touch state
 */
const DrcTouchAssistInfo* DrcAssistDirector::getTouchAssistInfo() const {
    return mTouchAssistInfo;
}

/**
 * @brief Checks whether the cursor has faded out.
 * @return always false
 */
bool DrcAssistDirector::isCursorFadedOut() const {
    return false;
}

/**
 * @brief Makes the pointer release what it holds.
 */
void DrcAssistDirector::releaseTouchPointerHoldItem() {
    if (mTouchPointer->isCompletelyAlive()) {
        mTouchPointer->startRelease();
    }
}

/**
 * @brief Restarts the touch layout effect.
 */
void DrcAssistDirector::toggleTouchEffect() {
    al::killLayoutIfActive(mTouchEffect);
    al::appearLayoutIfDead(mTouchEffect);
}

/**
 * @brief Returns the touch icon position.
 * @return the icon position on screen
 */
const sead::Vector2f& DrcAssistDirector::getTouchIconPos() const {
    return mTouchIconPos;
}

/**
 * @brief Returns the pointer's world position.
 * @return the pointer position
 */
const sead::Vector3f& DrcAssistDirector::getTouchPointerPosition() const {
    return al::getTrans(mTouchPointer);
}

/**
 * @brief Returns the normal of the surface under the pointer.
 * @return the hit normal
 */
const sead::Vector3f& DrcAssistDirector::getTouchPointerNormal() const {
    return mTouchPointer->getHitNormal();
}

/**
 * @brief Returns the pointer actor.
 * @return the pointer
 */
DrcTouchPointer* DrcAssistDirector::getTouchPointer() const {
    return mTouchPointer;
}

/**
 * @brief Computes the pointer's up direction.
 * @param pUp receives the up direction
 */
void DrcAssistDirector::calcTouchPointerUpDir(sead::Vector3f* pUp) {
    al::calcQuatUp(pUp, al::getQuat(mTouchPointer));
}

/**
 * @brief Computes the touch slide direction on screen.
 * @param pDir receives the slide since the previous frame
 * @return always true
 */
bool DrcAssistDirector::tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f* pDir) {
    pDir->x = mTouchScreenPos.x - mPrevTouchScreenPos.x;
    pDir->y = mTouchScreenPos.y - mPrevTouchScreenPos.y;
    return true;
}

/**
 * @brief Computes the touch slide direction in the world, projecting the current and previous
 *        touch positions onto the collision.
 * @param pDir receives the slide since the previous frame
 * @param pCamera unused
 * @return always true
 */
bool DrcAssistDirector::tryCalcTouchPointerSlideDirOnWorld(sead::Vector3f* pDir,
                                                           const al::IUseCamera* pCamera) {
    sead::Vector3f prevPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f curPos = {0.0f, 0.0f, 0.0f};
    sead::Vector3f cameraPos = al::getCameraPos(mTouchPointer);
    sead::Vector3f dir;
    sead::Vector3f hitPos;
    al::Triangle triangle;

    al::calcWorldPosFromScreenPosSub(&prevPos, mTouchPointer, mPrevTouchScreenPos,
                                     al::getCameraLookAt(this));
    al::normalizeOrZero(&dir, prevPos - cameraPos);
    if (alCollisionUtil::getFirstPolyOnArrow(mTouchPointer, &hitPos, &triangle,
                                             cameraPos + dir * 500.0f, dir * 5000.0f,
                                             "DrcAssist")) {
        prevPos = hitPos;
    }

    al::calcWorldPosFromScreenPosSub(&curPos, mTouchPointer, mTouchScreenPos,
                                     al::getCameraLookAt(this));
    al::normalizeOrZero(&dir, curPos - cameraPos);
    if (alCollisionUtil::getFirstPolyOnArrow(mTouchPointer, &hitPos, &triangle,
                                             cameraPos + dir * 500.0f, dir * 5000.0f,
                                             "DrcAssist")) {
        curPos = hitPos;
    }

    pDir->x = curPos.x - prevPos.x;
    pDir->y = curPos.y - prevPos.y;
    pDir->z = curPos.z - prevPos.z;
    return true;
}

/**
 * @brief Kills the pointer's effects.
 */
void DrcAssistDirector::deleteTouchPointerEffect() {
    al::tryKillEmitterAndParticleAll(mTouchPointer);
}

/**
 * @brief Enters snapshot mode: the main port's pointer becomes the stamp cursor, the others hide.
 */
void DrcAssistDirector::startSnapshotMode() {
    mTouchPointer->startSnapshotMode();
    if (mPort == al::getMainControllerPort()) {
        mTouchEffect->setStampIcon(true);
        mTouchEffect->appear();
    } else {
        mTouchEffect->kill();
        mIsSnapshotMode = true;
    }
}

/**
 * @brief Leaves snapshot mode.
 */
void DrcAssistDirector::endSnapshotMode() {
    mIsSnapshotMode = false;
    mTouchEffect->setStampIcon(false);
    mTouchPointer->endSnapshotMode();
    if (!mIsEnableUpdate && mTouchEffect->isAlive()) {
        mTouchEffect->kill();
    }
}

/**
 * @brief Checks whether the pointer holds a stamp.
 * @return true while holding a stamp
 */
bool DrcAssistDirector::isPlayerHoldingStamp() {
    return mTouchPointer->isHoldStamp();
}

/**
 * @brief Converts a controller rotation into screen axes (x: pitch, y: yaw, z: roll).
 * @param port pad port
 * @param pAngle controller rotation
 * @param pOut receives the screen rotation
 */
void DrcAssistDirector::calcScreenAngle(s32 port, const sead::Vector3f* pAngle,
                                        sead::Vector3f* pOut) const {
    sead::Vector3f angle = *pAngle;
    if (al::isPadTypeHandheld(port)) {
        pOut->x = -angle.y;
        pOut->y = angle.x;
        pOut->z = angle.z;
    } else if (al::isPadTypeJoySingle(port)) {
        f32 sign = rc::getControllerStyle(port) == cControllerStyleFlipped ? -1.0f : 1.0f;
        pOut->x = -angle.z;
        pOut->y = angle.y * sign;
        pOut->z = angle.x;
    } else {
        pOut->x = -angle.z;
        pOut->y = angle.x;
        pOut->z = angle.y;
    }
}

/**
 * @brief Removes the roll around the controller's front axis from a pose.
 * @param port pad port
 * @param angle roll to remove
 * @param pMtx pose to modify
 */
void DrcAssistDirector::unrotatePoseMtx(s32 port, f32 angle, sead::Matrix34f* pMtx) const {
    sead::Vector3f axis;
    getPadTypeFrontAxis(port, &axis);
    sead::Vector3f frontDir = *pMtx * axis;
    sead::Quatf quat;
    quat.setAxisRadian(frontDir, -angle);
    sead::Matrix34f rotMtx;
    rotMtx.fromQuat(quat);
    pMtx->setMul(rotMtx, *pMtx);
}

/**
 * @brief Converts a screen rotation back into controller axes (inverse of calcScreenAngle()).
 * @param port pad port
 * @param pAngle screen rotation
 * @param pOut receives the controller rotation
 */
void DrcAssistDirector::reverseCalcScreenAngle(s32 port, const sead::Vector3f* pAngle,
                                               sead::Vector3f* pOut) const {
    sead::Vector3f angle = *pAngle;
    if (al::isPadTypeHandheld(port)) {
        pOut->x = angle.y;
        pOut->y = -angle.x;
        pOut->z = angle.z;
    } else if (al::isPadTypeJoySingle(port)) {
        f32 sign = rc::getControllerStyle(port) == cControllerStyleFlipped ? -1.0f : 1.0f;
        pOut->x = angle.z;
        pOut->y = angle.y * sign;
        pOut->z = -angle.x;
    } else {
        pOut->x = angle.y;
        pOut->y = angle.z;
        pOut->z = -angle.x;
    }
}

/**
 * @brief Returns the controller axis that points at the screen.
 * @param port pad port
 * @param pAxis receives the front axis
 */
void DrcAssistDirector::getPadTypeFrontAxis(s32 port, sead::Vector3f* pAxis) const {
    if (al::isPadTypeHandheld(port)) {
        *pAxis = sead::Vector3f::ez;
    } else if (al::isPadTypeJoySingle(port)) {
        *pAxis = sead::Vector3f::ex;
    } else {
        *pAxis = sead::Vector3f::ey;
    }
}
