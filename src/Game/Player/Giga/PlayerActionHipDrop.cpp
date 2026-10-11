#include "Player/Giga/PlayerActionHipDrop.hpp"

#include <math/seadVector.h>

#include "Player/IUsePlayerAnimator.hpp"
#include "Player/IUsePlayerCheckArea.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerEventReceiver.hpp"
#include "Player/IUsePlayerFlag.hpp"
#include "Player/IUsePlayerFlagControl.hpp"
#include "Player/IUsePlayerInput.hpp"
#include "Player/IUsePlayerSubAction.hpp"
#include "Player/PlayerActionArg.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerProperty.hpp"

namespace {
/// How fast the player gets pushed into the floor on landing.
constexpr f32 cLandPushSpeed = -10.0f;
}  // namespace

/**
 * @brief Checks if the player stands on something that ends the fall.
 * @return Whether the player is on the floor, in sink sand or on ink.
 */
inline bool PlayerActionHipDrop::isLanding() const {
    return mArg->mCollision->isOnFloor() || mCheckArea->isInSinkSandArea(mArg->mProperty->mTrans) ||
           isInkLimitFlagIsOn();
}

/**
 * @brief Pushes the player into the floor so it stays grounded.
 */
inline void PlayerActionHipDrop::pushToFloor() {
    PlayerProperty* pProperty = mArg->mProperty;
    sead::Vector3f up = pProperty->mGroundUp;
    pProperty->mVelocity = up * cLandPushSpeed;
}

/**
 * @brief Constructs the hip drop action.
 * @param pArg The player systems.
 * @param pFlagControl The flag that is on while the action runs.
 * @param pLandReactionFlag Whether holding the button on the floor bounces again.
 * @param pCheckArea The area checker (for sink sand).
 * @param pFlingPoleDashFlag The fling pole dash flag, cleared when the action ends.
 * @param pInkLimitFlag Whether ink counts as floor.
 */
PlayerActionHipDrop::PlayerActionHipDrop(const PlayerActionArg* pArg,
                                         IUsePlayerFlagControl* pFlagControl,
                                         const IUsePlayerFlag* pLandReactionFlag,
                                         const IUsePlayerCheckArea* pCheckArea,
                                         const IUsePlayerFlag* pFlingPoleDashFlag,
                                         const IUsePlayerFlag* pInkLimitFlag)
    : mArg(pArg), mObserver(new PlayerHipDropObserverNull()), mFlagControl(pFlagControl),
      mLandReactionFlag(pLandReactionFlag), mCheckArea(pCheckArea),
      mFlingPoleDashFlag(pFlingPoleDashFlag), mInkLimitFlag(pInkLimitFlag) {}

/**
 * @brief Destroys the action and its observer.
 */
PlayerActionHipDrop::~PlayerActionHipDrop() {
    delete mObserver;
}

/**
 * @brief Moves the player: snaps to the ground while the land animation plays, falls otherwise.
 */
void PlayerActionHipDrop::move() {
    if (mArg->mCollision->isOnFloor() && mArg->mAnimator->isAnim(getLandAnimName())) {
        mArg->mCollision->snapGround();
    } else {
        mArg->mCollision->solveAir();
    }
}

/**
 * @brief Runs the current phase and advances its step counter.
 */
void PlayerActionHipDrop::update() {
    switch (mState) {
    case EState::Start:
        exeStart();
        break;
    case EState::Loop:
        exeLoop();
        break;
    case EState::LandReady:
        exeLandReady();
        break;
    case EState::Land:
        exeLand();
        break;
    default:
        break;
    }

    mStep++;
}

/**
 * @brief The spin in the air before the fall.
 */
void PlayerActionHipDrop::exeStart() {
    if (mStep == 0) {
        mIsEnd = false;
        mArg->mAnimator->startAnim(getStartAnimName());
        mArg->mAnimator->setAnimRate(getAnimRate());
        mArg->mEventReceiver->onCancelJumpAudio();
        mArg->mEventReceiver->onHipDropStart();
        mArg->mProperty->mVelocity.set(0.0f, 0.0f, 0.0f);
    }

    if (mArg->mAnimator->isAnimEnd()) {
        shiftHipDropLoop();
    }
}

/**
 * @brief The fall, until the player hits the floor.
 */
void PlayerActionHipDrop::exeLoop() {
    if (isLanding()) {
        if (isLandVelocityOffset()) {
            pushToFloor();
        }

        mObserver->notifyOnFloorTrig();
        setState(static_cast<u32>(EState::LandReady));
        mArg->mEventReceiver->onHipDropLand();
    }
}

/**
 * @brief The landing: bounces again while the button is held, falls again if the floor vanished.
 */
void PlayerActionHipDrop::exeLandReady() {
    if (mStep == 1) {
        mLandReactionCount = 0;
    }

    // isLanding() spelled out (the bounce check must not share its button check with the fall)
    if (mArg->mCollision->isOnFloor() || mCheckArea->isInSinkSandArea(mArg->mProperty->mTrans) ||
        isInkLimitFlagIsOn()) {
        if (mArg->getInput()->isHipDropButtonOn() && mLandReactionFlag->isOn()) {
            mLandReadyTimer = mArg->mConstParam->getHipDropMsgInterval();
            if (mLandReactionCount != 0) {
                mArg->mEventReceiver->onHipDropLandLoop();
            }

            mArg->mAnimator->startAnim(getReactionAnimName());
            mArg->mAnimator->setAnimRate(getAnimRate());
            mLandReactionCount++;
        }
    } else {
        if (mArg->getInput()->isHipDropButtonOn()) {
            mLandReadyTimer = 0;
            shiftHipDropLoop();
        } else {
            mIsEnd = true;
        }

        return;
    }

    if (mLandReadyTimer == 0) {
        setState(static_cast<u32>(EState::Land));
        return;
    }

    mLandReadyTimer--;
    if (mLandReadyTimer == 0 && isLanding()) {
        pushToFloor();
        mObserver->notifyOnFloorTrig();
    }
}

/**
 * @brief The end of the landing; the action ends once the animation does or the floor vanishes.
 */
void PlayerActionHipDrop::exeLand() {
    if (mStep == 1) {
        startLand();
    }

    if (!isLanding()) {
        mIsEnd = true;
    }

    if (mArg->mAnimator->isAnimEnd()) {
        mIsEnd = true;
    }
}

/**
 * @brief Starts the action.
 */
void PlayerActionHipDrop::setup() {
    mArg->mSubAction->forceEnd();
    mLandReadyTimer = 0;
    mFlagControl->turnOn();
    setState(static_cast<u32>(EState::Start));
}

/**
 * @brief Switches to another phase.
 * @param state The new phase (an EState).
 */
void PlayerActionHipDrop::setState(u32 state) {
    mState = static_cast<EState>(state);
    mStep = 0;
}

/**
 * @brief Ends the action and clears the fling pole dash.
 */
void PlayerActionHipDrop::teardown() {
    mFlagControl->turnOff();
    if (isFlingPoleDashFlagIsOn()) {
        mArg->mEventReceiver->requestFlingPoleFlagClear();
    }
}

/**
 * @brief Checks the fling pole dash flag.
 * @return Whether the flag exists and is on.
 */
bool PlayerActionHipDrop::isFlingPoleDashFlagIsOn() const {
    return mFlingPoleDashFlag != nullptr ? mFlingPoleDashFlag->isOn() : false;
}

/**
 * @brief Checks if the action finished.
 * @return Whether the action finished.
 */
bool PlayerActionHipDrop::isEnd() const {
    return mIsEnd;
}

/**
 * @brief Checks if the landing has played long enough to be cancelled.
 * @return Whether the land animation passed the cancel frame.
 */
bool PlayerActionHipDrop::isPossibleToCancel() const {
    if (!mArg->mAnimator->isAnim(getLandAnimName())) {
        return false;
    }

    f32 frame = mArg->mAnimator->getAnimFrame();
    return frame >= static_cast<u32>(mArg->mConstParam->getHipDropLandCancelFrame());
}

/**
 * @brief Gets the animation of the spin in the air.
 * @return The animation name.
 */
const char* PlayerActionHipDrop::getStartAnimName() const {
    return "HipDropStart";
}

/**
 * @brief Gets the animation of the fall.
 * @return The animation name.
 */
const char* PlayerActionHipDrop::getLoopAnimName() const {
    return "HipDrop";
}

/**
 * @brief Gets the animation of the landing.
 * @return The animation name.
 */
const char* PlayerActionHipDrop::getLandAnimName() const {
    return "HipDropLand";
}

/**
 * @brief Gets the animation of a bounce on the floor.
 * @return The animation name.
 */
const char* PlayerActionHipDrop::getReactionAnimName() const {
    return "HipDropReaction";
}

/**
 * @brief Gets the falling speed.
 * @return The falling speed.
 */
f32 PlayerActionHipDrop::getHipDropSpeed() const {
    return mArg->mConstParam->getHipDropSpeed();
}

/**
 * @brief Starts the fall.
 */
void PlayerActionHipDrop::shiftHipDropLoop() {
    mArg->mAnimator->startAnim(getLoopAnimName());
    mArg->mAnimator->setAnimRate(getAnimRate());

    PlayerProperty* pProperty = mArg->mProperty;
    const PlayerConstParam* pParam = mArg->mConstParam;
    pProperty->mVelocity =
        -pProperty->mGroundUp *
        (pParam->isOverride() ? pParam->getHipDropSpeed() : getHipDropSpeed());
    setState(static_cast<u32>(EState::Loop));
}

/**
 * @brief Checks the ink limit flag.
 * @return Whether the flag exists and is on.
 */
bool PlayerActionHipDrop::isInkLimitFlagIsOn() const {
    return mInkLimitFlag != nullptr ? mInkLimitFlag->isOn() : false;
}

/**
 * @brief Starts the land animation.
 */
void PlayerActionHipDrop::startLand() {
    mArg->mAnimator->startAnim(getLandAnimName());
    mArg->mAnimator->setAnimRate(getAnimRate());
}

/**
 * @brief Gets the rate of the current animation.
 * @return The start animation rate during the spin, 1 otherwise.
 */
f32 PlayerActionHipDrop::getAnimRate() const {
    if (mState == EState::Start) {
        return mArg->mConstParam->getHipDropStartAnimRate();
    }

    return 1.0f;
}
