#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/Layout/LayoutActor.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include "Project/Camera/Main/IUseCameraDirector_RS.hpp"
#include "System/ScenarioInfo.hpp"

namespace al {
class ActorInitInfo;
class CameraDirector_RS;
class GraphicsSystemInfo;
class LayoutInitInfo;
class LiveActor;
class PlayerHolder;
class WipeSimple;
}  // namespace al

class GameDataHolder;
class IslandCounterParts;
class MapOceanShineParts;
class MapPlayerTrackerParts;
class PlayerAliveWatcher;

/**
 * @brief The island map layout of Bowser's Fury.
 *
 * Shows a flag icon for every island, the ocean shines, the player's trail and the giga bells,
 * lets the player scroll and zoom the map and warp to a visited island.
 */
class IslandMap : public al::LayoutActor, public al::ISceneObj, public al::IUseCamera_RS {
public:
    /// One island flag icon on the map.
    struct FlagIcon {
        /**
         * @brief Creates an icon that has no island assigned yet.
         * @param pActor Layout parts actor of the icon.
         */
        explicit FlagIcon(al::LayoutActor* pActor) : mActor(pActor), mName(nullptr) {}

        al::LayoutActor* mActor;  // 0x0
        const char16_t* mName;    // 0x8
        s32 mIslandId;            // 0x10
        s32 mLayerId;             // 0x14
    };

    /// First map index used by the ocean shine icons; smaller indices are island flags.
    static constexpr s32 cOceanIconIdxOffset = 49;
    /// Selection index meaning "nothing selected".
    static constexpr s32 cInvalidIdx = -5;

    static IslandMap* tryGetIslandMap(const al::IUseSceneObjHolder* pUser);
    static void setIslandWarpEnable(const al::IUseSceneObjHolder* pUser, bool isEnable);
    static void setIslandMapEnable(const al::IUseSceneObjHolder* pUser, bool isEnable);

    IslandMap(const char* pName, const al::LayoutInitInfo& rLayoutInfo,
              const al::ActorInitInfo& rActorInfo, const GameDataHolder* pGameData,
              al::CameraDirector_RS* pCameraDirector, al::PlayerHolder* pPlayerHolder,
              const PlayerAliveWatcher* pPlayerAliveWatcher, al::WipeSimple* pWipe,
              al::GraphicsSystemInfo* pGraphicsSystemInfo);

    void setWarpEnabled(bool isEnable);
    void control() override;
    void kill() override;

    void exeAppear();
    void exeAppearDemo();
    void exeInkDemo();
    void exeWaitDemo();
    void exeWait();
    bool updateMapScale();
    void exeSnapToPoint();
    s32 getNearestFlagIconIdx();
    bool canWarp();
    void endOnWarp();
    void exeIdle();
    void exeEnd();
    void exeWarp();
    void exeWarpEnd();

    void startAppear(s32 port, bool isDemo);
    void updateInkPatches(s32 phase, bool isDemo);
    void resetScale(bool isDefault);
    bool isEnd() const;
    bool isEndOnWarp() const;
    void updatePlayerTracker(bool isForce);

    /**
     * @brief Check whether the map can be opened.
     * @return True when the map is enabled.
     */
    bool isMapEnable() const { return mIsMapEnable; }

    /**
     * @brief Get the island chosen as the warp destination.
     * @return The island id.
     */
    s32 getWarpIslandId() const { return mWarpIslandId; }
    void updatePlayerTrackerForBonusArea(sead::Vector3f trans);
    void addSpecialShineLocation(al::LiveActor* pActor, sead::Vector3f trans,
                                 ScenarioInfo scenarioInfo);
    void removeSpecialShineLocation(al::LiveActor* pActor);
    void setSpecialShineIconComplete(al::LiveActor* pActor, bool isComplete);

    al::SceneCameraInfo* getSceneCameraInfo() const override;

    /**
     * @brief Name of the island map scene object.
     * @return The scene object name.
     */
    const char* getSceneObjName() const override { return "IslandMap"; }

    /**
     * @brief Access the camera director the map was created with.
     * @return The camera director.
     */
    al::CameraDirector_RS* getCameraDirector_RS() const override { return mCameraDirector; }

    /**
     * @brief Set whether a player went into a bonus area.
     * @param isInBonusArea True while a player is in a bonus area.
     */
    void setIsPlayerInBonusArea(bool isInBonusArea) { mIsPlayerInBonusArea = isInBonusArea; }

    /**
     * @brief Check whether a player is in a cloud bonus area.
     * @return True while a player is in a bonus area.
     */
    bool isPlayerInBonusArea() const { return mIsPlayerInBonusArea; }

private:
    bool isIconBlinkTiming();
    bool isSelectFlagIcon() const;
    void hideIslandInfo();
    sead::Vector2f calcClampedParchmentTrans(const sead::Vector2f& rTrans);

    const GameDataHolder* mGameDataHolder;            // 0x138
    u8 _140[8];                                       // 0x140
    const PlayerAliveWatcher* mPlayerAliveWatcher;    // 0x148
    s32 mPadPort = -1;                                // 0x150
    al::WipeSimple* mWipe;                            // 0x158
    al::GraphicsSystemInfo* mGraphicsSystemInfo;      // 0x160
    al::PlayerHolder* mPlayerHolder;                  // 0x168
    sead::Vector3f mMarioIconTrans;                   // 0x170
    al::LayoutActor* mControlGuideBar = nullptr;      // 0x180
    al::LayoutActor* mMarioIcon = nullptr;            // 0x188
    IslandCounterParts* mIslandCounter = nullptr;     // 0x190
    MapOceanShineParts* mOceanShine;                  // 0x198
    MapPlayerTrackerParts* mPlayerTracker;            // 0x1a0
    s32 mSelectedIdx = cInvalidIdx;                   // 0x1a8
    sead::Vector2f mSnapTrans = sead::Vector2f::zero;      // 0x1ac
    sead::Vector2f mSnapTarget = sead::Vector2f::zero;     // 0x1b4
    sead::Vector2f mScrollVelocity = sead::Vector2f::zero; // 0x1bc
    f32 mZoomVelocity = 0.0f;                         // 0x1c4
    s32 mZoomHoldFrames = 0;                          // 0x1c8
    s32 mWarpIslandId;                                // 0x1cc
    al::CameraDirector_RS* mCameraDirector;           // 0x1d0
    s32 _1d8 = 0;                                     // 0x1d8
    bool _1dc = false;                                // 0x1dc
    bool _1dd = false;                                // 0x1dd
    bool _1de = false;                                // 0x1de
    bool _1df = false;                                // 0x1df
    bool mIsSnapping = false;                         // 0x1e0
    bool mIsPlayerInBonusArea = false;                // 0x1e1
    bool mIsMapEnable = false;                        // 0x1e2
    s32 mWarpDisableCount = 0;                        // 0x1e4
    sead::Vector2f mPicPhase1Trans;                   // 0x1e8
    sead::Vector2f mPicPhase2Trans;                   // 0x1f0
    sead::Vector2f mPicPhase3Trans;                   // 0x1f8
    sead::PtrArray<FlagIcon> mFlagIcons;              // 0x200
};

static_assert(sizeof(IslandMap) == 0x210);
