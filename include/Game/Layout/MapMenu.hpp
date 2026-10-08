#pragma once

#include <basis/seadTypes.h>

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class GraphicsSystemInfo;
class LayoutInitInfo;
class SceneCameraInfo;
class WipeSimple;
}  // namespace al

class ButtonGroup;
class CourseSelectDirector;
class GameDataHolder;
class MapMenuPlayerParts;
class RCSControlGuideBar;

/**
 * @brief World map menu of the course select map, used to warp to another world.
 *
 * Shows one button per world; worlds that aren't open yet are invalidated. Deciding a world
 * closes a white wipe and moves the player to the start of that world.
 */
class MapMenu : public al::LayoutActor {
public:
    MapMenu(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder,
            al::SceneCameraInfo* pCameraInfo, CourseSelectDirector* pDirector,
            al::GraphicsSystemInfo* pGraphicsSystemInfo, RCSControlGuideBar* pGuideBar);

    void kill() override;
    void control() override;

    void exeAppear();
    void exeWait();
    void resetButtonValidation();
    bool isInTransition() const;
    void exeEnd();
    void exeWorldJumpStart();
    void exeWorldJumpEnd();

    void startAppear(s32 port, s32 worldId);
    void forceEnd();
    void transitionOut(bool isLeft);
    void transitionIn(bool isLeft);
    bool isStartEnd() const;
    bool isEnd() const;
    bool isWorldJumpStart() const;
    bool isWorldJumpFinish() const;
    bool isDecideAny() const;
    void setControlGuideBar(RCSControlGuideBar* pGuideBar);
    al::SceneCameraInfo* getSceneCameraInfo() const override;

    /**
     * Gets the world shown by the map.
     * @return The world id.
     */
    s32 getWorldId() const { return mWorldId; }

private:
    GameDataHolder* mGameDataHolder;                // 0x128
    al::SceneCameraInfo* mSceneCameraInfo;          // 0x130
    CourseSelectDirector* mDirector;                // 0x138
    s32 mPort = -1;                                 // 0x140
    s32 mWorldId = 1;                               // 0x144
    s32 mOpenWorldIdMax = 1;                        // 0x148
    s32 _14c = 1;                                   // 0x14c
    ButtonGroup* mButtonGroup = nullptr;            // 0x150
    MapMenuPlayerParts* mPlayerParts = nullptr;     // 0x158
    al::WipeSimple* mWipe = nullptr;                // 0x160
    al::GraphicsSystemInfo* mGraphicsSystemInfo;    // 0x168
    RCSControlGuideBar* mGuideBar;                  // 0x170
    bool mIsActive = true;                          // 0x178
    bool mIsDecided = false;                        // 0x179
};

static_assert(sizeof(MapMenu) == 0x180);
