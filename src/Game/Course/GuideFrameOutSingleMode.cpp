#include "Course/GuideFrameOutSingleMode.hpp"
#include "Layout/LayoutFontUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <gfx/seadProjection.h>
#include <cmath>

namespace {
NERVE_DECL(GuideFrameOutSingleMode, Disappear);
NERVE_DECL(GuideFrameOutSingleMode, StartDisappear);
NERVE_DECL(GuideFrameOutSingleMode, StartAppear);
NERVE_DECL(GuideFrameOutSingleMode, Appear);
NERVES_MAKE_STRUCT(GuideFrameOutSingleMode, Disappear, StartDisappear, StartAppear, Appear)
}

/**
 * @brief Creates the off-screen actor indicator.
 * @param rInfo Layout initialization context.
 * @param pActor Actor whose screen position is tracked.
 * @param pCharacterAction Initial character icon action.
 */
GuideFrameOutSingleMode::GuideFrameOutSingleMode(const al::LayoutInitInfo& rInfo,
                                                const al::LiveActor* pActor,
                                                const char* pCharacterAction)
    : al::LayoutActor("画面外ガイド"), mpActor(pActor) {
    al::initLayoutActor(this, rInfo, "GuideFrameOut", nullptr);
    initNerve(&NrvGuideFrameOutSingleMode.Disappear, 0);
    al::startAction(this, pCharacterAction, "Character");
    sead::WFormatFixedSafeString<6> icon(u"%s", u"K");
    al::setPaneString(this, "TxtFrameIcon", icon.cstr());
    kill();
}

/** @brief Activates the layout with its root pane initially hidden. */
void GuideFrameOutSingleMode::appear() {
    al::LayoutActor::appear();
    mShowing = false;
    al::hidePaneRootNoRecursive(this);
    al::setNerve(this, &NrvGuideFrameOutSingleMode.Disappear);
}

/** @brief Hides the indicator during a demo. */
void GuideFrameOutSingleMode::startDemo() { mInDemo = true; updateVisibility(true); }

/**
 * @brief Reconciles layout activity or pane visibility with the enable flags.
 * @param isPaneOnly True to update the root pane while retaining layout activity.
 */
void GuideFrameOutSingleMode::updateVisibility(bool isPaneOnly) {
    if (isEnable()) {
        if (isPaneOnly) {
            al::LayoutActor::appear();
            al::showPaneRootNoRecursive(this);
        } else if (!isAlive()) {
            appear();
        }
    } else {
        if (isPaneOnly) {
            mScreenOutFrame = 0;
            al::hidePaneRootNoRecursive(this);
        } else if (isAlive()) {
            kill();
        }
    }
}

/** @brief Restores visibility after a demo. */
void GuideFrameOutSingleMode::endDemo() { mInDemo = false; updateVisibility(true); }
/** @brief Hides the indicator while paused. */
void GuideFrameOutSingleMode::startPause() { mPaused = true; updateVisibility(true); }
/** @brief Restores visibility after unpausing. */
void GuideFrameOutSingleMode::endPause() { mPaused = false; updateVisibility(true); }
/** @brief Enables tracking and resets the off-screen duration. */
void GuideFrameOutSingleMode::setEnable() {
    mEnabled = true;
    mScreenOutFrame = 0;
    updateVisibility(false);
}
/** @brief Disables tracking and reconciles activity. */
void GuideFrameOutSingleMode::setDisable() { mEnabled = false; updateVisibility(false); }
/** @brief Updates the bubble enable flag when it changes. */
void GuideFrameOutSingleMode::setEnableBubble(bool enabled) {
    if (mBubbleEnabled != enabled) {
        mBubbleEnabled = enabled;
        updateVisibility(false);
    }
}
/** @brief Enables or resets the warp-control hint state. */
void GuideFrameOutSingleMode::setEnableWarpGuide(bool enabled) {
    if (enabled) {
        if (mWarpGuideState == 0) mWarpGuideState = 1;
    } else {
        mWarpGuideState = 0;
    }
}
/** @brief Tests the visibility enable conditions. */
bool GuideFrameOutSingleMode::isEnable() const {
    return mEnabled && !mInDemo && !mPaused && mBubbleEnabled;
}
/** @brief Reports whether the actor has spent any frames outside the screen. */
bool GuideFrameOutSingleMode::isScreenOut() const { return mScreenOutFrame > 0; }
/** @brief Returns the number of consecutive off-screen frames. */
s32 GuideFrameOutSingleMode::getScreenOutFrame() const { return mScreenOutFrame; }

/** @brief Projects the actor, positions its edge indicator and updates its state. */
void GuideFrameOutSingleMode::control() {
    if (!mpActor) {
        al::setNerve(this, &NrvGuideFrameOutSingleMode.StartDisappear);
        return;
    }
    if (!mEnabled) return;
    sead::Vector3f world = al::getTrans(mpActor);
    world.y += 70.0f;
    f32 halfWidth = al::getLayoutDisplayWidth() * 0.5f;
    f32 halfHeight = al::getLayoutDisplayHeight() * 0.5f;
    const sead::Matrix34f& view = al::getCameraViewMtx(mpActor->mActorSceneInfo->sceneCameraInfo);
    const sead::Matrix44f& projection =
        al::getCameraProjection(mpActor->mActorSceneInfo->sceneCameraInfo)->getDeviceProjectionMatrix();
    sead::Vector3f camera;
    camera.setMul(view, world);
    sead::Vector3f screen;
    screen.setMul(projection, camera);
    f32 screenX = screen.x;
    f32 screenY = screen.y;
    bool outside;
    if (camera.z > 0.0f) {
        screenX = -screenX;
        screenY = -screenY;
        outside = true;
    } else {
        outside = (screenX > 1.0f) | (screenX < -1.0f) |
                  (screenY > 1.0f) | (screenY < -1.0f);
    }
    f32 maxAbs = std::fabs(screenX) > std::fabs(screenY) ? std::fabs(screenX) : std::fabs(screenY);
    f32 inverse = 1.0f / maxAbs;
    sead::Vector2f edge;
    edge.y = screenY * (halfHeight * inverse);
    edge.x = screenX * (halfWidth * inverse);
    mScreenOutFrame = outside ? mScreenOutFrame + 1 : 0;
    if (al::isDead(mpActor) || (al::getAlivePlayerNum(mpActor) <= 1 && !mFixedIcon)) {
        outside = false;
        mScreenOutFrame = 0;
    }
    sead::Vector2f position = edge;
    sead::Vector2f limit(halfWidth - 100.0f, halfHeight - 100.0f);
    al::clampV2f(&position, sead::Vector2f(-limit.x, -limit.y), limit);
    sead::Vector2f direction = edge - position;
    if (al::isNearZero(direction, 0.001f)) direction.set(0.0f, 1.0f);
    else al::normalize(&direction);
    f32 dot = direction.x * 0.0f + direction.y;
    if (dot < -1.0f) dot = -1.0f;
    else if (dot > 1.0f) dot = 1.0f;
    sead::Vector3f rotation(0.0f, 0.0f, std::acos(dot) * 57.2957763671875f);
    if (direction.y * 0.0f - direction.x < 0.0f) rotation.z = -rotation.z;
    bool delayed = false;
    if (mDelayAppear) delayed = mScreenOutFrame <= 45;
    if (mCenterHorizontally && !delayed && outside) {
        position.x = al::lerpValue(0.02f, al::getLocalTrans(this).x, 0.0f);
    }
    if (outside || mForcePosition) {
        al::setLocalTrans(this, position);
        al::setPaneLocalRotate(this, "Arrow", rotation);
        if (mWarpGuideState != 0) updateWarpGuide(std::fabs(rotation.z));
        if (outside) {
            if (al::isNerve(this, &NrvGuideFrameOutSingleMode.Disappear) && !delayed)
                al::setNerve(this, &NrvGuideFrameOutSingleMode.StartAppear);
            return;
        }
    }
    if (al::isNerve(this, &NrvGuideFrameOutSingleMode.Appear)) {
        if (mDelayAppear) {
            mShowing = false;
            updateVisibility(true);
        }
        al::setNerve(this, &NrvGuideFrameOutSingleMode.StartDisappear);
    }
}

/** @brief Selects the warp hint for the arrow's angle and controller style. */
void GuideFrameOutSingleMode::updateWarpGuide(f32 angle) {
    bool single = al::isPadTypeJoySingle(rc::getPadPortByUserId(1));
    s32 state = mWarpGuideState;
    if (angle > 90.0f) {
        if (state != 2) {
            if (state == 3) {
                al::IUseLayoutAction* layout = this;
                const char* action = single ? "SLSR_BottomToTop" : "LR_BottomToTop";
                al::startAction(layout, action, "Controls");
            } else {
                al::IUseLayoutAction* layout = this;
                const char* action = single ? "SLSR_Top" : "LR_Top";
                al::startAction(layout, action, "Controls");
            }
            state = 2;
        }
    } else {
        if (state != 3) {
            if (state == 2) {
                al::IUseLayoutAction* layout = this;
                const char* action = single ? "SLSR_TopToBottom" : "LR_TopToBottom";
                al::startAction(layout, action, "Controls");
            } else {
                al::IUseLayoutAction* layout = this;
                const char* action = single ? "SLSR_Bottom" : "LR_Bottom";
                al::startAction(layout, action, "Controls");
            }
            state = 3;
        }
    }
    mWarpGuideState = state;
}

/** @brief Clears the showing flag and immediately enters disappearance. */
void GuideFrameOutSingleMode::disappearImmediate() {
    mShowing = false;
    updateVisibility(true);
    al::setNerve(this, &NrvGuideFrameOutSingleMode.StartDisappear);
}
/** @brief Initializes the icon and plays the appear transition. */
void GuideFrameOutSingleMode::exeStartAppear() {
    if (al::isFirstStep(this)) {
        mShowing = true;
        updateVisibility(true);
        if (!mFixedIcon) initIcon();
        al::startAction(this, "Appear");
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGuideFrameOutSingleMode.Appear);
}
/** @brief Derives the character icon from the followed actor when needed. */
void GuideFrameOutSingleMode::initIcon() {
    if (GameDataFunction::isCurrentStageKinopioBrigade(this) || mCharacterType >= 0) return;
    mCharacterType = rc::getPlayerCharaType(mpActor);
    const char* characterName = rc::getPlayerCharacterNameTrue(mCharacterType);
    al::startAction(this, characterName, "Character");
    al::setPaneString(this, "TxtFrameIcon",
        sead::WFormatFixedSafeString<6>(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType)).cstr());
}
/** @brief Starts the visible wait animation. */
void GuideFrameOutSingleMode::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait");
}
/** @brief Plays the end animation at the configured rate. */
void GuideFrameOutSingleMode::exeStartDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End");
        al::setActionFrameRate(this, mDisappearRate);
    }
    if (al::isActionEnd(this)) al::setNerve(this, &NrvGuideFrameOutSingleMode.Disappear);
}
/** @brief Clears the showing flag and reconciles pane visibility. */
void GuideFrameOutSingleMode::exeDisappear() {
    if (al::isFirstStep(this)) {
        mShowing = false;
        updateVisibility(true);
    }
}
