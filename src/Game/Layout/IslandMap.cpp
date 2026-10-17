#include "Layout/IslandMap.hpp"

#include <attributes.h>
#include <cmath>
#include <math/seadMathCalcCommon.h>

#include "Course/MapOceanShineParts.hpp"
#include "Course/MapPlayerTrackerParts.hpp"
#include "Layout/Switch/IslandCounterParts.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/Fury/GigaBell.hpp"
#include "MapObj/Fury/GigaBellManager.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "Library/Camera/CameraPoser_RS.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "MapObj/Lighthouse.hpp"
#include "MapObj/TimerManager.hpp"
#include "NPC/IslandHolder.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/IslandDataList.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(IslandMap, End);
NERVE_DECL(IslandMap, Wait);
NERVE_DECL(IslandMap, SnapToPoint);
NERVE_DECL(IslandMap, Idle);
NERVE_DECL(IslandMap, WaitDemo);
NERVE_DECL(IslandMap, InkDemo);
NERVE_DECL(IslandMap, WarpEnd);
NERVE_DECL(IslandMap, AppearDemo);
NERVE_DECL(IslandMap, Appear);
NERVE_DECL(IslandMap, Warp);
NERVES_MAKE_NOSTRUCT(IslandMap, End, Wait, SnapToPoint, Idle, WaitDemo, InkDemo, WarpEnd,
                     AppearDemo, Appear, Warp)

/// Scene object id of the disaster mode controller.
constexpr s32 cSceneObjIdDisasterModeController = 58;
/// Scene object id of the giga bell manager.
constexpr s32 cSceneObjIdGigaBellManager = 50;

/// Last map index used by the island flag icons.
constexpr s32 cFlagIconIdxMax = IslandMap::cOceanIconIdxOffset - 1;
/// Number of "Flag_%d" panes in the layout.
constexpr s32 cFlagPaneNum = 50;
/// Number of "GigaBell_%d" panes in the layout.
constexpr s32 cGigaBellPaneNum = 3;

/// Squared stick length below which the map scroll input is ignored.
constexpr f32 cScrollInputMin = 0.0001f;
/// Maximum scroll speed of the map.
constexpr f32 cScrollSpeedMax = 10.0f;

constexpr f32 cZoomScaleMin = 1.25f;
constexpr f32 cZoomScaleMax = 4.0f;

/// Half the size of the world (in world units) that the map shows.
constexpr f32 cWorldSize = 30000.0f;
/// Half the size of the map (in layout units).
constexpr f32 cMapSize = 85.5f;

/**
 * @brief Converts a world position into a position on the map.
 * @param rTrans World position.
 * @return The position on the map.
 */
inline sead::Vector2f calcMapPos(const sead::Vector3f& rTrans) {
    return {rTrans.x / cWorldSize * cMapSize, 0.0f - rTrans.z / cWorldSize * cMapSize};
}

/**
 * @brief Checks whether both scenarios uncovering the top of island 9 are complete.
 * @param pActor Layout actor used to access the game data.
 * @return True if the top of island 9 is free of ink.
 */
inline bool isIsland9TopCleared(const al::LayoutActor* pActor) {
    return SingleModeDataFunction::isScenarioComplete(pActor, 8, 0) &&
           SingleModeDataFunction::isScenarioComplete(pActor, 11, 0);
}
}  // namespace

/**
 * @brief Finds the island map scene object.
 * @param pUser Scene object holder user.
 * @return The island map, or nullptr when the scene has none.
 */
IslandMap* IslandMap::tryGetIslandMap(const al::IUseSceneObjHolder* pUser) {
    return al::tryGetSceneObj<IslandMap>(pUser, SceneObjID_IslandMap);
}

/**
 * @brief Allows or forbids warping from the island map of the scene.
 * @param pUser Scene object holder user.
 * @param isEnable True to allow warping, false to forbid it.
 */
void IslandMap::setIslandWarpEnable(const al::IUseSceneObjHolder* pUser, bool isEnable) {
    IslandMap* islandMap = tryGetIslandMap(pUser);
    if (islandMap != nullptr) {
        islandMap->setWarpEnabled(isEnable);
    }
}

/**
 * @brief Allows or forbids warping from the map; every forbid needs its own allow.
 * @param isEnable True to allow warping, false to forbid it.
 */
void IslandMap::setWarpEnabled(bool isEnable) {
    if (isEnable) {
        mWarpDisableCount--;
        if (mWarpDisableCount < 0) {
            mWarpDisableCount--;
        }
    } else {
        mWarpDisableCount++;
    }
}

/**
 * @brief Allows or forbids opening the island map of the scene.
 * @param pUser Scene object holder user.
 * @param isEnable True to allow opening the map.
 */
void IslandMap::setIslandMapEnable(const al::IUseSceneObjHolder* pUser, bool isEnable) {
    IslandMap* islandMap = tryGetIslandMap(pUser);
    if (islandMap != nullptr) {
        islandMap->mIsMapEnable = isEnable;
    }
}

/**
 * @brief Creates the island map with all its parts and one flag icon per island.
 * @param pName Layout archive name.
 * @param rLayoutInfo Layout initialization context.
 * @param rActorInfo Actor initialization context (unused).
 * @param pGameData Game data holder.
 * @param pCameraDirector Camera director of the scene.
 * @param pPlayerHolder Holder of the players.
 * @param pPlayerAliveWatcher Watcher of the players' alive state.
 * @param pWipe Wipe used when warping.
 * @param pGraphicsSystemInfo Graphics system info of the scene.
 */
IslandMap::IslandMap(const char* pName, const al::LayoutInitInfo& rLayoutInfo,
                     const al::ActorInitInfo& rActorInfo, const GameDataHolder* pGameData,
                     al::CameraDirector_RS* pCameraDirector, al::PlayerHolder* pPlayerHolder,
                     const PlayerAliveWatcher* pPlayerAliveWatcher, al::WipeSimple* pWipe,
                     al::GraphicsSystemInfo* pGraphicsSystemInfo)
    : al::LayoutActor(pName), mGameDataHolder(pGameData), mPlayerAliveWatcher(pPlayerAliveWatcher),
      mWipe(pWipe), mGraphicsSystemInfo(pGraphicsSystemInfo), mPlayerHolder(pPlayerHolder),
      mCameraDirector(pCameraDirector) {
    al::initLayoutActor(this, rLayoutInfo, pName, nullptr);

    mMarioIcon = new al::LayoutActor("MapDotIconMarioParts");
    al::initLayoutPartsActor(mMarioIcon, this, rLayoutInfo, "icon_Mario", nullptr);
    mMarioIconTrans = al::getPaneLocalTrans(this, "icon_Mario");
    initNerve(&NrvIslandMapEnd, 0);

    mControlGuideBar = new al::LayoutActor("RCS_ControlGuideBarParts");
    al::initLayoutPartsActor(mControlGuideBar, this, rLayoutInfo, "InfoBar", nullptr);
    mIslandCounter =
        new IslandCounterParts(rLayoutInfo, "IslandCounterParts", "parIslandInfo", this, nullptr);

    IslandKeeper* islandKeeper = al::tryGetSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper);
    if (islandKeeper != nullptr) {
        mFlagIcons.allocBuffer(islandKeeper->getIndexHolderNum(), nullptr);
        for (s32 i = 0; i < islandKeeper->getIndexHolderNum(); i++) {
            auto* icon = new FlagIcon(new al::LayoutActor("MapDotIconFlagParts"));
            al::initLayoutPartsActor(icon->mActor, this, rLayoutInfo,
                                     al::StringTmp<32>("Flag_%d", i).cstr(), nullptr);
            mFlagIcons.pushBack(icon);
        }
    }

    mPlayerTracker =
        new MapPlayerTrackerParts(rLayoutInfo, "MapPlayerTrackerParts", "ParPlayerTracker", this);
    mOceanShine = new MapOceanShineParts(rLayoutInfo, "MapOceanShineParts", "ParOceanShines", this,
                                         mGameDataHolder);

    sead::Vector3f trans = al::getPaneLocalTrans(this, "PicPhase1");
    mPicPhase1Trans.set(trans.x, trans.y);
    trans = al::getPaneLocalTrans(this, "PicPhase2");
    mPicPhase2Trans.set(trans.x, trans.y);
    trans = al::getPaneLocalTrans(this, "PicPhase3");
    mPicPhase3Trans.set(trans.x, trans.y);
}

/** @brief Updates the control guide when the controller assignment changes while browsing. */
void IslandMap::control() {
    if (rc::isControllerAssignmentChanged() &&
        (al::isNerve(this, &NrvIslandMapWait) || al::isNerve(this, &NrvIslandMapSnapToPoint) ||
         al::isNerve(this, &NrvIslandMapIdle))) {
        if (al::isPadTypeJoySingle(mPadPort)) {
            al::setPaneSystemMessage(mControlGuideBar, "TxtInfo", "RCS_ControlGuideBar",
                                     "SingleModeMap_SingleJoycon");
            al::setPaneSystemMessage(this, "TxtWarp", "IslandMap", "IslandMap_Warp_SingleJoycon");
        } else {
            al::setPaneSystemMessage(mControlGuideBar, "TxtInfo", "RCS_ControlGuideBar",
                                     "SingleModeMap");
            al::setPaneSystemMessage(this, "TxtWarp", "IslandMap", "IslandMap_Warp");
        }
    }
}

/** @brief Remembers the current zoom in the save data and hides the map. */
void IslandMap::kill() {
    SingleModeDataFunction::setMapZoomRatio(this, al::getPaneLocalScale(this, "ScalePane").x);
    al::LayoutActor::kill();
}

/** @brief Plays the appear animation, then lets the player browse the map. */
void IslandMap::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        al::startSe(this, "PgAppear", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvIslandMapIdle);
    }
}

/** @brief Plays the appear demo shown after a new phase was unlocked and hides the new flags. */
void IslandMap::exeAppearDemo() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AppearDemo", "Demo");
        al::startSe(this, "PgAppear", nullptr);
        al::enableChangeSituation(this);
        al::enableVolumeChange(this);

        s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
        s32 newLayerA;
        s32 newLayerB;
        if (phase == 3) {
            newLayerA = 16;
            newLayerB = 18;
        } else if (phase == 8) {
            mOceanShine->startIconBlink();
            al::setNerve(this, &NrvIslandMapWaitDemo);
            return;
        } else if (phase == 5) {
            newLayerA = 19;
            newLayerB = 22;
        } else {
            al::setNerve(this, &NrvIslandMapIdle);
            return;
        }

        for (s32 i = 0; i < mFlagIcons.size(); i++) {
            al::StringTmp<32> paneName("Flag_%d", i);
            const char* name = paneName.cstr();
            if (mFlagIcons(i)->mLayerId == newLayerA) {
                al::hidePaneNoRecursive(this, name);
            }

            if (mFlagIcons(i)->mLayerId == newLayerB) {
                al::hidePaneNoRecursive(this, name);
            }
        }
    }

    if (al::isActionEnd(this, "Demo") && al::isGreaterEqualStep(this, 90)) {
        al::setNerve(this, &NrvIslandMapInkDemo);
    }
}

/** @brief Washes away the ink of the unlocked phase, then shows the new flags blinking. */
void IslandMap::exeInkDemo() {
    if (al::isFirstStep(this)) {
        s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
        const char* actionName = phase == 3 ? "InkPhase2Demo" :
                                 phase == 5 ? "InkPhase3Demo" :
                                              nullptr;
        al::startAction(this, actionName, "InkBlots");
        al::startSe(this, "InkDemo", nullptr);
    }

    if (!al::isActionEnd(this, "InkBlots")) {
        return;
    }

    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    s32 newLayerA;
    s32 newLayerB;
    if (phase == 3) {
        newLayerA = 16;
        newLayerB = 18;
    } else if (phase == 8) {
        al::showPaneRootNoRecursive(mOceanShine);
        mOceanShine->startIconBlink();
        al::setNerve(this, &NrvIslandMapWaitDemo);
        return;
    } else if (phase == 5) {
        newLayerA = 19;
        newLayerB = 22;
    } else {
        al::setNerve(this, &NrvIslandMapIdle);
        return;
    }

    for (s32 i = 0; i < mFlagIcons.size(); i++) {
        al::StringTmp<32> paneName("Flag_%d", i);
        const char* name = paneName.cstr();
        if (mFlagIcons(i)->mLayerId == newLayerA) {
            if (mFlagIcons(i)->mName != nullptr) {
                al::showPaneNoRecursive(this, name);
            }

            al::startAction(mFlagIcons(i)->mActor, "icon_BLINK", "Scale");
        }

        if (mFlagIcons(i)->mLayerId == newLayerB) {
            if (mFlagIcons(i)->mName != nullptr) {
                al::showPaneNoRecursive(this, name);
            }

            al::startAction(mFlagIcons(i)->mActor, "icon_BLINK", "Scale");
        }
    }

    al::setNerve(this, &NrvIslandMapWaitDemo);
}

/**
 * @brief Checks whether the new icons are at the middle of a blink.
 * @return True at the frame a blink sound should play.
 */
ALWAYS_INLINE inline bool IslandMap::isIconBlinkTiming() {
    if (SingleModeDataFunction::getUnlockedPhase(this) == 8) {
        return mOceanShine->isBlinkAnimDone();
    }

    for (s32 i = 0; i < mFlagIcons.size(); i++) {
        if (al::isActionPlaying(mFlagIcons(i)->mActor, "icon_BLINK", "Scale") &&
            al::getActionFrame(mFlagIcons(i)->mActor, "Scale") ==
                al::getActionFrameMax(mFlagIcons(i)->mActor, "icon_BLINK", "Scale") * 0.5f) {
            return true;
        }
    }

    return false;
}

/** @brief Lets the new icons blink for a while, then ends the demo. */
void IslandMap::exeWaitDemo() {
    if (isIconBlinkTiming()) {
        al::startSe(this, "PgBlink", nullptr);
    }

    if (!al::isGreaterEqualStep(this, 180) || !isIconBlinkTiming()) {
        return;
    }

    al::startAction(this, "AppearDemo", "AfterDemo");
    al::startAction(mControlGuideBar, "Appear_Icon", "Player");
    if (SingleModeDataFunction::getUnlockedPhase(this) == 8) {
        mOceanShine->endIconBlink();
    } else {
        for (s32 i = 0; i < mFlagIcons.size(); i++) {
            if (al::isActionPlaying(mFlagIcons(i)->mActor, "icon_BLINK", "Scale")) {
                al::startFreezeActionEnd(mFlagIcons(i)->mActor, "icon_DEFAULT", "Scale");
            }
        }
    }

    al::startAction(mMarioIcon, "iconMario_BLINK", nullptr);
    al::setNerve(this, &NrvIslandMapIdle);
}

/**
 * @brief Clamps a parchment position so the map never scrolls out of the screen.
 * @param rTrans Wanted parchment position.
 * @return The clamped parchment position.
 */
inline sead::Vector2f IslandMap::calcClampedParchmentTrans(const sead::Vector2f& rTrans) {
    f32 halfHeight = al::getLayoutDisplayHeight() * 0.5f;
    f32 halfWidth = al::getLayoutDisplayWidth() * 0.5f;
    sead::Vector2f size(0.0f, 0.0f);
    sead::Vector2f mapScale = al::getPaneLocalScale(this, "ScalePane");
    al::getPaneLocalSize(&size, this, "PicDeepWater");
    size *= mapScale.x * al::getPaneLocalScale(this, "PicDeepWater").x;

    f32 limitX = (halfWidth + size.x * 0.5f + -640.0f) / mapScale.x;
    f32 limitY = (halfHeight + size.y * 0.5f + -360.0f) / mapScale.y;
    return {fmaxf(-limitX, fminf(limitX, rTrans.x)), fmaxf(-limitY, fminf(limitY, rTrans.y))};
}

/** @brief Scrolls the map with the stick until it is released. */
void IslandMap::exeWait() {
    if (rc::isPadTriggerUiCancelByPort(mPadPort) || rc::isPadTriggerUiMapByPort(mPadPort)) {
        al::setNerve(this, &NrvIslandMapEnd);
        return;
    }

    sead::Vector3f parchmentTrans = al::getPaneLocalTrans(this, "Parchment");
    sead::Vector2f trans(parchmentTrans.x, parchmentTrans.y);
    sead::Vector2f scroll = rc::getPadUiMapScrollByPort(mPadPort);
    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (scroll.squaredLength() > cScrollInputMin) {
        scroll *= cScrollSpeedMax;
        scroll *= 1.0f / al::getPaneLocalScale(this, "ScalePane").x;
        f32 rate = fminf((al::getNerveStep(this) + 1.0f) / cScrollSpeedMax, 1.0f);
        mScrollVelocity.set(scroll.x * rate, scroll.y * rate);
        if (sead::Mathf::abs(mScrollVelocity.x) > cScrollSpeedMax) {
            mScrollVelocity.x = mScrollVelocity.x < 0.0f ? -cScrollSpeedMax : cScrollSpeedMax;
        }

        if (sead::Mathf::abs(mScrollVelocity.y) > cScrollSpeedMax) {
            mScrollVelocity.y = mScrollVelocity.y < 0.0f ? -cScrollSpeedMax : cScrollSpeedMax;
        }

        trans = calcClampedParchmentTrans(trans - mScrollVelocity);
        al::setPaneLocalTrans(this, "Parchment", trans);
    } else if (phase == 10 || phase == 7) {
        al::setNerve(this, &NrvIslandMapIdle);
    } else {
        al::setNerve(this, &NrvIslandMapSnapToPoint);
    }

    updateMapScale();
}

/**
 * @brief Zooms the map with the zoom input and rescales every icon accordingly.
 * @return True while the player is zooming.
 */
bool IslandMap::updateMapScale() {
    if (!rc::isEnablePadUiMapZoom(mPadPort)) {
        return false;
    }

    sead::Vector2f zoom = rc::getPadUiMapZoomByPort(mPadPort);
    sead::Vector2f scale = al::getPaneLocalScale(this, "ScalePane");
    f32 scaleValue = scale.x;
    bool isZooming;
    if (sead::Vector2f(0.0f, zoom.y).squaredLength() > cScrollInputMin &&
        !(zoom.y > 0.0f ? al::isNear(scaleValue, cZoomScaleMax, 0.001f) :
                          al::isNear(scaleValue, cZoomScaleMin, 0.001f))) {
        f32 rate = fminf(mZoomHoldFrames / 10.0f, 1.0f);
        mZoomVelocity = zoom.y * 0.035f * rate;
        mZoomHoldFrames++;
        isZooming = true;
    } else {
        mZoomVelocity *= 0.7f;
        mZoomHoldFrames = 0;
        isZooming = false;
    }

    scaleValue = fmaxf(fminf(mZoomVelocity + scaleValue, cZoomScaleMax), cZoomScaleMin);
    scale.set(scaleValue, scaleValue);
    if (!(al::getPaneLocalScale(this, "ScalePane") == scale) && isZooming) {
        al::tryHoldSeWithParam(this, "PgZoom",
                               (scaleValue - cZoomScaleMin) / (cZoomScaleMax - cZoomScaleMin),
                               nullptr);
    }

    al::setPaneLocalScale(this, "ScalePane", scale);
    f32 invScale = 1.0f / scale.x;
    scale.set(invScale, invScale);
    for (s32 i = 0; i < cGigaBellPaneNum; i++) {
        al::StringTmp<32> paneName("GigaBell_%d", i);
        al::setPaneLocalScale(this, paneName.cstr(), scale);
    }

    for (s32 i = 0; i < mFlagIcons.size(); i++) {
        al::setPaneLocalScale(mFlagIcons(i)->mActor, "img_mapDot_Beak", scale);
        al::setPaneLocalScale(mFlagIcons(i)->mActor, "img_mapDot", scale);
        al::setPaneLocalScale(mFlagIcons(i)->mActor, "target_scale", scale);
    }

    al::setPaneLocalScale(this, "icon_Mario", scale);
    mOceanShine->updateScale(scale);
    mPlayerTracker->updateScale(scale);
    return isZooming;
}

/**
 * @brief Checks whether an island flag icon is selected.
 * @return True if the selection is an island flag.
 */
inline bool IslandMap::isSelectFlagIcon() const {
    return mSelectedIdx >= 0 && mSelectedIdx <= cFlagIconIdxMax;
}

/** @brief Hides the island name and warp message if they are shown. */
inline void IslandMap::hideIslandInfo() {
    if (al::isActionPlaying(this, "ShowIslandInfo", "IslandInfo")) {
        al::startAction(this, "HideIslandInfo", "IslandInfo");
        al::startAction(this, "HideWarpMessage", "Warp");
    }
}

/** @brief Selects the icon nearest to the map center and scrolls the map onto it. */
void IslandMap::exeSnapToPoint() {
    if (al::isFirstStep(this)) {
        s32 idx = getNearestFlagIconIdx();
        if (idx < cInvalidIdx + 1) {
            mSelectedIdx = idx;
            updateMapScale();
            al::setNerve(this, &NrvIslandMapIdle);
            return;
        }

        mScrollVelocity = sead::Vector2f::zero;
        mIsSnapping = true;
        if (idx >= 0 && idx != mSelectedIdx) {
            if (idx <= cFlagIconIdxMax) {
                mIslandCounter->setIslandName(mFlagIcons(idx)->mName);
                mIslandCounter->updateShineCount(mFlagIcons(idx)->mIslandId);
            } else {
                mIslandCounter->updateShineCountOcean(
                    mGameDataHolder, mOceanShine->getScenarioInfo(idx - cOceanIconIdxOffset));
            }
        }

        mSelectedIdx = idx;
        const sead::Matrix34f& parchmentMtx = al::getPaneMtx(this, "Parchment");
        f32 parchmentX = parchmentMtx.m[0][3];
        f32 parchmentY = parchmentMtx.m[1][3];
        if (!al::isActionPlaying(this, "ShowIslandInfo", "IslandInfo")) {
            al::startAction(this, "ShowIslandInfo", "IslandInfo");
        }

        if (mSelectedIdx >= 0) {
            if (mSelectedIdx <= cFlagIconIdxMax) {
                const sead::Matrix34f& targetMtx =
                    al::getPaneMtx(mFlagIcons(mSelectedIdx)->mActor, "Target");
                mSnapTarget.set(parchmentX - targetMtx.m[0][3], parchmentY - targetMtx.m[1][3]);
                if (canWarp()) {
                    al::startAction(this, "ShowWarpMessage", "Warp");
                }
            } else {
                const sead::Matrix34f& targetMtx = al::getPaneMtx(
                    mOceanShine->getIcon(mSelectedIdx - cOceanIconIdxOffset), "Target");
                mSnapTarget.set(parchmentX - targetMtx.m[0][3], parchmentY - targetMtx.m[1][3]);
            }
        }
    }

    if (rc::isPadTriggerUiCancelByPort(mPadPort) || rc::isPadTriggerUiMapByPort(mPadPort)) {
        al::setNerve(this, &NrvIslandMapEnd);
        return;
    }

    if (rc::getPadUiMapScrollByPort(mPadPort).squaredLength() > cScrollInputMin) {
        hideIslandInfo();
        al::setNerve(this, &NrvIslandMapWait);
        return;
    }

    updateMapScale();
    if (mSelectedIdx < cInvalidIdx + 1) {
        return;
    }

    u32 layoutHeight = al::getLayoutDisplayHeight();
    u32 displayHeight = al::getDisplayHeight();
    al::getPaneLocalScale(this, "ScalePane");
    sead::Vector3f parchmentTrans = al::getPaneLocalTrans(this, "Parchment");
    sead::Vector2f trans(parchmentTrans.x, parchmentTrans.y);
    if (mSelectedIdx >= 0) {
        f32 ratio = static_cast<f32>(layoutHeight) / static_cast<f32>(displayHeight);
        al::LayoutActor* icon = mSelectedIdx <= cFlagIconIdxMax ?
                                    mFlagIcons(mSelectedIdx)->mActor :
                                    mOceanShine->getIcon(mSelectedIdx - cOceanIconIdxOffset);
        const sead::Matrix34f& targetMtx = al::getPaneMtx(icon, "Target");
        mSnapTarget.set(trans.x - ratio * targetMtx.m[0][3], trans.y - ratio * targetMtx.m[1][3]);
    }

    if (rc::isPadTriggerUiDecideByPort(mPadPort) && al::isGreaterEqualStep(this, 8) &&
        isSelectFlagIcon() && al::isNear(trans, mSnapTarget, 4.0f) && canWarp()) {
        endOnWarp();
        return;
    }

    if (al::isNear(trans, mSnapTarget, 0.25f)) {
        al::setPaneLocalTrans(this, "Parchment", mSnapTarget);
        al::setNerve(this, &NrvIslandMapIdle);
        return;
    }

    if (mIsSnapping && al::isNear(trans, mSnapTarget, 3.7f)) {
        if (mSelectedIdx >= 0) {
            if (mSelectedIdx <= cFlagIconIdxMax) {
                al::startAction(mFlagIcons(mSelectedIdx)->mActor, "icon_SNAP", "Scale");
            } else {
                mOceanShine->snapIcon(mSelectedIdx - cOceanIconIdxOffset);
            }
        }

        mIsSnapping = false;
    }

    al::lerpVec(&mSnapTrans, trans, mSnapTarget,
                0.25f / al::getPaneLocalScale(this, "ScalePane").x);
    al::setPaneLocalTrans(this, "Parchment", mSnapTrans);
}

/**
 * @brief Finds the visited island or ocean shine icon nearest to the map center.
 * @return The icon's map index, or cInvalidIdx when there is none.
 */
s32 IslandMap::getNearestFlagIconIdx() {
    f32 minDistance = 1500.0f;
    s32 nearestIdx = cInvalidIdx;
    for (s32 i = 0; i < mFlagIcons.size(); i++) {
        if (mFlagIcons(i)->mName == nullptr ||
            SingleModeDataFunction::isIslandFirstVisit(this, mFlagIcons(i)->mIslandId - 1)) {
            continue;
        }

        const sead::Matrix34f& targetMtx = al::getPaneMtx(mFlagIcons(i)->mActor, "Target");
        sead::Vector3f target(targetMtx.m[0][3], targetMtx.m[1][3], targetMtx.m[2][3]);
        f32 distance = target.squaredLength();
        if (distance < minDistance) {
            nearestIdx = i;
            minDistance = distance;
        }
    }

    s32 oceanIdx = mOceanShine->getNearestIconIdx(minDistance);
    return oceanIdx >= 0 ? oceanIdx + cOceanIconIdxOffset : nearestIdx;
}

/**
 * @brief Checks whether the player may warp to the selected island right now.
 * @return True if warping is possible.
 */
bool IslandMap::canWarp() {
    if (SingleModeDataFunction::getUnlockedPhase(this) == 8 && mWarpDisableCount > 0) {
        return false;
    }

    DisasterModeController* disasterController = DisasterModeController::tryGetController(this);
    if (disasterController != nullptr && disasterController->isWipeActive()) {
        return false;
    }

    s32 islandId = mFlagIcons(mSelectedIdx)->mIslandId;
    IslandKeeper* islandKeeper = al::tryGetSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper);
    sead::Vector3f startPos;
    sead::Vector3f startFront;
    if (islandKeeper == nullptr ||
        !islandKeeper->getIslandStartPos(islandId, startPos, startFront)) {
        return false;
    }

    TimerManager* timerManager = TimerManager::tryGetTimerManager(this);
    if (timerManager != nullptr && !timerManager->canCancel()) {
        return false;
    }

    al::LiveActor* player = al::tryFindAlivePlayerActorFirst(mPlayerHolder);
    if (player != nullptr && rc::isAnyActiveDemo(player)) {
        return false;
    }

    PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(this);
    if (koopaJr != nullptr && !koopaJr->canDoIslandWarp()) {
        return false;
    }

    return !mIsPlayerInBonusArea && mPlayerAliveWatcher->isEnableIslandWarp(0);
}

/** @brief Starts warping to the selected island. */
void IslandMap::endOnWarp() {
    mWarpIslandId = mFlagIcons(mSelectedIdx)->mIslandId;
    al::startSe(this, "PgWarp", nullptr);
    al::setNerve(this, &NrvIslandMapWarp);
}

/** @brief Keeps the selected icon centered while zooming and lets the map drift to a halt. */
void IslandMap::exeIdle() {
    if (al::isFirstStep(this)) {
        al::isActionPlaying(this, "ShowIslandInfo", "IslandInfo");
    }

    sead::Vector2f scroll = rc::getPadUiMapScrollByPort(mPadPort);
    bool isZooming = updateMapScale();
    if (scroll.squaredLength() > cScrollInputMin) {
        hideIslandInfo();
        al::setNerve(this, &NrvIslandMapWait);
        return;
    }

    if (isZooming && mSelectedIdx >= cInvalidIdx + 1) {
        f32 mapScale = al::getPaneLocalScale(this, "ScalePane").x;
        sead::Vector3f parchmentTrans = al::getPaneLocalTrans(this, "Parchment");
        const sead::Matrix34f* targetMtx;
        f32 invScale;
        if (mSelectedIdx <= cFlagIconIdxMax) {
            invScale = 1.0f / mapScale;
            targetMtx = &al::getPaneMtx(mFlagIcons(mSelectedIdx)->mActor, "Target");
        } else {
            al::getPaneMtx(mOceanShine->getIcon(mSelectedIdx - cOceanIconIdxOffset), "Target");
            invScale = 1.0f / mapScale;
            targetMtx = &al::getPaneMtx(
                mOceanShine->getIcon(mSelectedIdx - cOceanIconIdxOffset), "Target");
        }

        sead::Vector2f trans(parchmentTrans.x - invScale * targetMtx->m[0][3],
                             parchmentTrans.y - invScale * targetMtx->m[1][3]);
        al::setPaneLocalTrans(this, "Parchment", trans);
    } else {
        bool isMoving = isZooming;
        if (!al::isNear(mScrollVelocity.x, 0.0f, cScrollInputMin)) {
            mScrollVelocity.x *= 0.6f;
            isMoving = true;
        }

        if (!al::isNear(mScrollVelocity.y, 0.0f, cScrollInputMin)) {
            mScrollVelocity.y *= 0.6f;
            isMoving = true;
        }

        if (isMoving) {
            sead::Vector3f parchmentTrans = al::getPaneLocalTrans(this, "Parchment");
            sead::Vector2f trans = calcClampedParchmentTrans(
                {parchmentTrans.x - mScrollVelocity.x, parchmentTrans.y - mScrollVelocity.y});
            al::setPaneLocalTrans(this, "Parchment", trans);
        } else {
            mScrollVelocity = sead::Vector2f::zero;
        }
    }

    if (rc::isPadTriggerUiDecideByPort(mPadPort) && isSelectFlagIcon() && canWarp()) {
        endOnWarp();
        return;
    }

    if (!al::isActionPlaying(this, "ShowIslandInfo", "IslandInfo") ||
        al::isActionEnd(this, "IslandInfo")) {
        if (rc::isPadTriggerUiCancelByPort(mPadPort) || rc::isPadTriggerUiMapByPort(mPadPort)) {
            al::setNerve(this, &NrvIslandMapEnd);
        }
    }
}

/** @brief Plays the close animation, then hides the map. */
void IslandMap::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
        al::startSe(this, "PgEnd", nullptr);
    }

    if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}

/** @brief Closes the wipe and fades the music before warping to the selected island. */
void IslandMap::exeWarp() {
    if (al::isFirstStep(this)) {
        mWipe->startClose(-1);
        auto* disasterController = al::tryGetSceneObj<DisasterModeController>(
            this, cSceneObjIdDisasterModeController);
        if (disasterController != nullptr && !disasterController->isDisasterMode()) {
            IslandKeeper* islandKeeper =
                al::tryGetSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper);
            bool isKeepVolume = false;
            if (islandKeeper != nullptr) {
                s32 currentIslandId = islandKeeper->getCurrentIslandId();
                s32 warpIslandId = mFlagIcons(mSelectedIdx)->mIslandId;
                isKeepVolume = !IslandDataFunction::isGigaBellIsland(currentIslandId) &&
                               !IslandDataFunction::isGigaBellIsland(warpIslandId) &&
                               currentIslandId >= 0 && warpIslandId >= 0;
            }

            al::changeIslandMapBgmVolume(this, -1, 30, isKeepVolume);
        }
    }

    if (mWipe->isCloseEnd()) {
        al::setNerve(this, &NrvIslandMapWarpEnd);
    }
}

/** @brief Hides the map once the warp wipe is closed. */
void IslandMap::exeWarpEnd() {
    kill();
}

/**
 * @brief Opens the map for a player, placing every icon and centering on the player.
 * @param port Controller port of the player opening the map.
 * @param isDemo True to play the demo shown after a new phase was unlocked.
 */
void IslandMap::startAppear(s32 port, bool isDemo) {
    s32 phase = SingleModeDataFunction::getUnlockedPhase(this);
    if (al::isPadTypeJoySingle(port)) {
        al::setPaneSystemMessage(mControlGuideBar, "TxtInfo", "RCS_ControlGuideBar",
                                 "SingleModeMap_SingleJoycon");
        al::setPaneSystemMessage(this, "TxtWarp", "IslandMap", "IslandMap_Warp_SingleJoycon");
    } else {
        al::setPaneSystemMessage(this, "TxtWarp", "IslandMap", "IslandMap_Warp");
        al::setPaneSystemMessage(mControlGuideBar, "TxtInfo", "RCS_ControlGuideBar",
                                 "SingleModeMap");
    }

    if (al::getMainControllerPort() == port) {
        al::startAction(mControlGuideBar, "SetIcon_Mario", "Player");
    } else {
        al::startAction(mControlGuideBar, "SetIcon_KoopaJr", "Player");
    }

    updateInkPatches(phase, isDemo);
    al::startAction(this, "HideWarpMessage", "Warp");
    mPadPort = port;
    al::setPaneLocalTrans(this, "Parchment", sead::Vector2f::zero);
    resetScale(isDemo);
    mSelectedIdx = cInvalidIdx;
    al::startAction(this, "ForceHideIslandInfo", "IslandInfo");

    sead::Vector3f mapTrans = al::getPaneLocalTrans(this, "map");
    f32 mapScale = al::getPaneLocalScale(this, "map").x;
    sead::Vector3f blackSunTrans = al::getPaneLocalTrans(this, "PicBlackSun");
    sead::Vector2f mapPos(0.0f, 0.0f);
    if (mIsPlayerInBonusArea) {
        mPlayerTracker->getLastKnownPos(&mapPos);
        al::hidePaneNoRecursive(mMarioIcon, "PicPointer");
    } else {
        al::showPaneNoRecursive(mMarioIcon, "PicPointer");
        al::LiveActor* player =
            rc::tryFindPlayerFromInputPort(mPlayerHolder, al::getMainControllerPort(), false);
        if (player != nullptr) {
            const sead::Vector3f& playerTrans =
                rc::getPlayerTrans(static_cast<PlayerActor*>(player));
            mapPos.x = playerTrans.x / cWorldSize * cMapSize;
            mapPos.y = 0.0f - playerTrans.z / cWorldSize * cMapSize;

            sead::Vector3f front = rc::getPlayerFront(player);
            const sead::Vector3f up(0.0f, 0.0f, -1.0f);
            f32 cosAngle = front.dot(up);
            sead::Vector3f cross;
            cross.setCross(front, up);
            f32 angle = atan2f(cross.length(), cosAngle) * 57.295776f;
            al::setPaneLocalRotate(mMarioIcon, "PicPointer",
                                   {0.0f, 0.0f, front.x > 0.0f ? -angle : angle});
        }
    }

    al::setPaneLocalTrans(this, "icon_Mario", mapPos);

    sead::Vector2f focusPos;
    if (isDemo) {
        focusPos.set(mapTrans.x + mapScale * blackSunTrans.x,
                     mapTrans.y + mapScale * blackSunTrans.y);
    } else {
        focusPos = mapPos;
        al::startAction(mMarioIcon, "iconMario_BLINK", nullptr);
    }

    auto* gigaBellManager =
        al::tryGetSceneObj<GigaBellManager>(this, cSceneObjIdGigaBellManager);
    if (gigaBellManager != nullptr) {
        s32 gigaBellNum = gigaBellManager->getGigaBellCount();
        s32 shownNum = gigaBellNum < cGigaBellPaneNum ? gigaBellNum : cGigaBellPaneNum;
        for (s32 i = 0; i < shownNum; i++) {
            const sead::Vector3f& gigaBellTrans = al::getTrans(gigaBellManager->getGigaBell(i));
            mapPos.x = gigaBellTrans.x / cWorldSize * cMapSize;
            mapPos.y = 0.0f - gigaBellTrans.z / cWorldSize * cMapSize;
            al::StringTmp<32> paneName("GigaBell_%d", i);
            al::setPaneLocalTrans(this, paneName.cstr(), mapPos);
            al::showPaneNoRecursive(this, paneName.cstr());
        }

        for (s32 i = gigaBellNum; i < cGigaBellPaneNum; i++) {
            al::StringTmp<32> paneName("GigaBell_%d", i);
            al::hidePaneNoRecursive(this, paneName.cstr());
        }
    }

    IslandKeeper* islandKeeper = al::tryGetSceneObj<IslandKeeper>(this, SceneObjID_IslandKeeper);
    if (phase == 7 || phase == 10) {
        for (s32 i = 0; i < cFlagPaneNum; i++) {
            al::StringTmp<32> paneName("Flag_%d", i);
            al::hidePaneNoRecursive(this, paneName.cstr());
        }

        al::hidePaneNoRecursive(this, "ParOceanShines");
        al::hidePaneNoRecursive(this, "GigaBells");
    } else {
        for (s32 i = 0; i < islandKeeper->getIndexHolderNum(); i++) {
            FlagIcon* icon = mFlagIcons[i];
            Lighthouse* lighthouse = islandKeeper->getIslandHolderIndex(i)->getLighthouse();
            al::LiveActor* islandFlag = islandKeeper->getIslandHolderIndex(i)->getIslandFlag();
            const char* paneName = al::StringTmp<32>("Flag_%d", i).cstr();
            mFlagIcons(i)->mLayerId =
                islandKeeper->getIslandHolderIndex(i)->mPlacementHolder->getLayerId();
            if (lighthouse == nullptr && islandFlag == nullptr) {
                mFlagIcons(i)->mName = nullptr;
                al::hidePaneNoRecursive(this, paneName);
                continue;
            }

            if (lighthouse != nullptr) {
                mFlagIcons(i)->mIslandId = lighthouse->mPlacementHolder->getZoneNo();
            } else if (islandFlag != nullptr) {
                mFlagIcons(i)->mIslandId = islandFlag->mPlacementHolder->getZoneNo();
            }

            sead::Vector3f startPos;
            sead::Vector3f startFront;
            islandKeeper->getIslandStartPos(mFlagIcons(i)->mIslandId, startPos, startFront);
            mapPos = calcMapPos(startPos);
            al::setPaneLocalTrans(this, al::StringTmp<32>("Flag_%d", i).cstr(), mapPos);
            mFlagIcons(i)->mName = reinterpret_cast<const char16_t*>(
                IslandDataFunction::getIslandName(this, this, mFlagIcons(i)->mIslandId));

            if (SingleModeDataFunction::allScenariosComplete(this, mFlagIcons(i)->mIslandId - 1)) {
                al::startAction(icon->mActor, "icon_ALLCOMPLETE", nullptr);
                continue;
            }

            if (SingleModeDataFunction::isIslandFirstVisit(this, mFlagIcons(i)->mIslandId - 1)) {
                al::startAction(icon->mActor, "icon_DEFAULT", nullptr);
            } else if (phase <= 7 && mFlagIcons(i)->mIslandId == 1) {
                al::startAction(icon->mActor, "icon_INPROGRESS", nullptr);
            } else {
                s32 scenarioIdx = SingleModeDataFunction::getCurActiveScenarioIndex(
                    this, mFlagIcons(i)->mIslandId - 1);
                if (scenarioIdx >= 0 && SingleModeDataFunction::isScenarioComplete(
                                            this, mFlagIcons(i)->mIslandId - 1, scenarioIdx)) {
                    al::startAction(icon->mActor, "icon_INPROGRESS", nullptr);
                } else {
                    al::startAction(icon->mActor, "icon_NEW", nullptr);
                }
            }

            al::showPaneNoRecursive(this, paneName);
        }

        for (s32 i = islandKeeper->getIndexHolderNum(); i < cFlagPaneNum; i++) {
            al::StringTmp<32> paneName("Flag_%d", i);
            al::hidePaneNoRecursive(this, paneName.cstr());
        }

        al::showPaneNoRecursive(this, "ParOceanShines");
        al::showPaneNoRecursive(this, "GigaBells");
        mOceanShine->setPanePositions({0.00285f, -0.00285f}, 4.0f);
    }

    const sead::Vector2f offset(0.0f, 0.0f);
    sead::Vector2f picPhaseTrans = mPicPhase1Trans + offset;
    al::setPaneLocalTrans(this, "PicPhase1", picPhaseTrans);
    picPhaseTrans = mPicPhase2Trans + offset;
    al::setPaneLocalTrans(this, "PicPhase2", picPhaseTrans);
    picPhaseTrans = mPicPhase3Trans + offset;
    al::setPaneLocalTrans(this, "PicPhase3", picPhaseTrans);
    mPlayerTracker->setPanePositions();
    al::LayoutActor::appear();

    sead::Vector3f parchmentTrans = al::getPaneLocalTrans(this, "Parchment");
    picPhaseTrans.set(parchmentTrans.x - focusPos.x, parchmentTrans.y - focusPos.y);
    al::setPaneLocalTrans(this, "Parchment", picPhaseTrans);
    if (isDemo) {
        al::setNerve(this, &NrvIslandMapAppearDemo);
        return;
    }

    if (mSelectedIdx >= 0) {
        mIslandCounter->setIslandName(mFlagIcons(mSelectedIdx)->mName);
        mIslandCounter->updateShineCount(mFlagIcons(mSelectedIdx)->mIslandId);
        al::startAction(this, "ShowIslandInfo", "IslandInfo");
    }

    al::setNerve(this, &NrvIslandMapAppear);
}

/**
 * @brief Shows the ink patches of the islands that are still covered in ink.
 * @param phase Unlocked phase of the game.
 * @param isDemo True to freeze the ink at the start of the phase's wash-away demo.
 */
void IslandMap::updateInkPatches(s32 phase, bool isDemo) {
    static const s32 cPatchIslandIds[] = {2, 6, 7, 8, 9, 11};

    al::StringTmp<32> paneName;
    s32 openPatchNum;
    if (phase >= 5) {
        if (isIsland9TopCleared(this)) {
            al::hidePaneNoRecursive(this, "PicPatch_Island9Top");
        } else {
            al::showPaneNoRecursive(this, "PicPatch_Island9Top");
        }

        openPatchNum = 5;
    } else {
        al::hidePaneNoRecursive(this, "PicPatch_Island9Top");
        openPatchNum = phase > 2 ? 2 : 0;
    }

    for (s32 i = 0; i < 6; i++) {
        paneName.format("PicPatch_Island%d", cPatchIslandIds[i]);
        bool isCleared = i > openPatchNum ||
                         SingleModeDataFunction::isScenarioComplete(this, cPatchIslandIds[i], 0);
        if (isCleared) {
            al::hidePaneNoRecursive(this, paneName.cstr());
        } else {
            al::showPaneNoRecursive(this, paneName.cstr());
        }
    }

    const char* actionName;
    switch (phase) {
    case 1:
        actionName = "InkPhase1";
        break;
    case 3:
        actionName = isDemo ? "InkPhase2Demo" : "InkPhase2";
        break;
    case 5:
    case 7:
        actionName = isDemo ? "InkPhase3Demo" : "InkPhase3";
        break;
    case 8:
    case 10:
        actionName = "InkPhase4";
        break;
    default:
        return;
    }

    if (isDemo) {
        al::startFreezeAction(this, actionName, 0.0f, "InkBlots");
    } else {
        al::startAction(this, actionName, "InkBlots");
    }
}

/**
 * @brief Restores the zoom of the map and rescales every icon accordingly.
 * @param isDefault True to use the default zoom instead of the saved one.
 */
void IslandMap::resetScale(bool isDefault) {
    sead::Vector2f scale(cZoomScaleMin, cZoomScaleMin);
    if (!isDefault) {
        f32 zoomRatio = SingleModeDataFunction::getMapZoomRatio(this);
        if (zoomRatio == 0.0f) {
            scale.set(2.1f, 2.1f);
        } else {
            scale.set(zoomRatio, zoomRatio);
        }
    }

    f32 invScale = 1.0f / scale.x;
    sead::Vector2f iconScale(invScale, invScale);
    al::setPaneLocalScale(this, "ScalePane", scale);
    mZoomVelocity = 0.0f;
    for (s32 i = 0; i < cGigaBellPaneNum; i++) {
        al::StringTmp<32> paneName("GigaBell_%d", i);
        al::setPaneLocalScale(this, paneName.cstr(), iconScale);
    }

    for (s32 i = 0; i < mFlagIcons.size(); i++) {
        al::setPaneLocalScale(mFlagIcons(i)->mActor, "img_mapDot_Beak", iconScale);
        al::setPaneLocalScale(mFlagIcons(i)->mActor, "img_mapDot", iconScale);
        al::setPaneLocalScale(mFlagIcons(i)->mActor, "target_scale", iconScale);
    }

    al::setPaneLocalScale(this, "icon_Mario", iconScale);
    mPlayerTracker->updateScale(iconScale);
    mOceanShine->updateScale(iconScale);
}

/**
 * @brief Checks whether the map finished closing.
 * @return True once the map is closed or closing for a warp.
 */
bool IslandMap::isEnd() const {
    if (al::isNerve(this, &NrvIslandMapWarpEnd)) {
        return true;
    }

    return al::isNerve(this, &NrvIslandMapEnd) && al::isGreaterEqualStep(this, 1) &&
           al::isActionEnd(this, nullptr);
}

/**
 * @brief Checks whether the map closed for a warp.
 * @return True once the warp wipe closed.
 */
bool IslandMap::isEndOnWarp() const {
    return al::isNerve(this, &NrvIslandMapWarpEnd);
}

/**
 * @brief Adds the current player position to the player trail.
 * @param isForce True to add the position right away instead of waiting for the trail timer.
 */
void IslandMap::updatePlayerTracker(bool isForce) {
    if (!isForce && (mIsPlayerInBonusArea || !mPlayerTracker->updateTimer())) {
        return;
    }

    al::LiveActor* player = rc::tryFindPlayerFromInputPort(mPlayerHolder, mPadPort, false);
    if (player != nullptr) {
        const sead::Vector3f& playerTrans = rc::getPlayerTrans(static_cast<PlayerActor*>(player));
        sead::Vector2f pos;
        pos.x = playerTrans.x / cWorldSize * cMapSize;
        pos.y = 0.0f - playerTrans.z / cWorldSize * cMapSize;
        mPlayerTracker->addPlayerPos(pos);
    }
}

/**
 * @brief Adds the position of a player inside a bonus area to the player trail.
 * @param trans World position the player entered the bonus area from.
 */
void IslandMap::updatePlayerTrackerForBonusArea(sead::Vector3f trans) {
    mPlayerTracker->addPlayerPos(calcMapPos(trans));
}

/**
 * @brief Shows a special shine on the map.
 * @param pActor Actor holding the shine.
 * @param trans World position of the shine.
 * @param scenarioInfo Scenario the shine belongs to.
 */
void IslandMap::addSpecialShineLocation(al::LiveActor* pActor, sead::Vector3f trans,
                                        ScenarioInfo scenarioInfo) {
    mOceanShine->addSpecialShineLocation(pActor, trans, scenarioInfo);
}

/**
 * @brief Removes a special shine from the map.
 * @param pActor Actor holding the shine.
 */
void IslandMap::removeSpecialShineLocation(al::LiveActor* pActor) {
    mOceanShine->removeSpecialShineLocation(pActor);
}

/**
 * @brief Marks a special shine icon as collected or not.
 * @param pActor Actor holding the shine.
 * @param isComplete True if the shine was collected.
 */
void IslandMap::setSpecialShineIconComplete(al::LiveActor* pActor, bool isComplete) {
    mOceanShine->setSpecialShineIconComplete(pActor, isComplete);
}

/**
 * @brief Access the scene camera info of the layout scene.
 * @return The scene camera info.
 */
al::SceneCameraInfo* IslandMap::getSceneCameraInfo() const {
    return getLayoutSceneInfo()->getSceneCameraInfo();
}
