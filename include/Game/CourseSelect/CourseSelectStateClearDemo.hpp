#pragma once

#include <attributes.h>
#include <basis/seadTypes.h>

#include "Library/Nerve/NerveStateBase.hpp"
#include "Project/Camera/Core/IUseCameraDirector.hpp"

namespace al {
class CameraInfo;
class LayoutInitInfo;
class LiveActor;
class PlayerHolder;
class SceneCameraInfo;
}  // namespace al

class CourseSelectMiniature;
class CourseSelectScene;
class CourseSelectWindowHolder;
class GameDataHolder;
class ICourseSelectActorController;
class ListStampResult;
class RouteOpenCursor;

/**
 * @brief Course select scene state: Plays the demos shown after a course was cleared (clear flag,
 * world clear, opening the roads to the next courses, the casino room) and the messages of the
 * features opened by the clear.
 */
class CourseSelectStateClearDemo : public al::HostStateBase<CourseSelectScene>,
                                   public al::IUseCamera {
public:
    CourseSelectStateClearDemo(CourseSelectScene* pScene, const al::PlayerHolder* pPlayerHolder,
                               al::SceneCameraInfo* pCameraInfo, const al::LayoutInitInfo& rInfo);

    void init() override;
    void appear() override;
    void goToKill();
    bool isFinishOpenNextMiniature() const;

    void exeFirstClearDemo();
    void exeFirstClearDemoEnd();
    void exeWorldClear();
    void exeOpenRouteDokanStart();
    void startDemoCamera(const al::LiveActor* pSubTarget, const al::LiveActor* pTarget,
                         s32 interpoleFrame);
    void exeOpenRouteDokanRoad();
    void exeOpenRouteDokan();
    void exeOpenRoad();
    void exeOpenMiniature();
    void exeOpenCasinoRoomStart();
    void exeOpenCasinoRoom();
    void exeWaitCamera();
    void exeStartMiiverse();
    void exeShowNewOpenFlagMessage();
    void exeEndNewOpenFlagMessage();
    void exeShowGetStamp();
    void exeKill();

    GameDataHolder* getGameDataHolder() const;

    /** @brief Gets the miniature of the last played course. @return The miniature, or nullptr. */
    CourseSelectMiniature* getMiniature() const { return mMiniature; }

    /** @brief Gets the camera info of the scene. @return The camera info. */
    al::SceneCameraInfo* getSceneCameraInfo() const override { return mSceneCameraInfo; }

private:
    NOINLINE inline void goToOpenCasinoRoomOrKill();

    /** Maximum number of nodes opened by one clear. */
    static constexpr s32 cNextNodeNumMax = 4;
    /** Default interpolation of the camera back to the map camera. */
    static constexpr s32 cEndCameraInterpoleFrame = 45;

    s32 mNextNodeNum = 0;  // 0x28
    s32 mOpenedNodeNum = 0;  // 0x2c
    s32 mEndCameraInterpoleFrame = cEndCameraInterpoleFrame;  // 0x30
    ICourseSelectActorController** mNextNodes = nullptr;  // 0x38
    ICourseSelectActorController* mOpeningNode = nullptr;  // 0x40
    bool mIsGreenStarLock = false;  // 0x48
    RouteOpenCursor* mRouteOpenCursor;  // 0x50
    CourseSelectMiniature* mMiniature = nullptr;  // 0x58
    const al::PlayerHolder* mPlayerHolder;  // 0x60
    al::SceneCameraInfo* mSceneCameraInfo;  // 0x68
    al::CameraInfo* mCameraInfo = nullptr;  // 0x70
    CourseSelectWindowHolder* mWindowHolder;  // 0x78
    const char* mBgmLineName = nullptr;  // 0x80
    const char* mMessageLabel = nullptr;  // 0x88
    ListStampResult* mListStampResult = nullptr;  // 0x90
};

static_assert(sizeof(CourseSelectStateClearDemo) == 0x98);
