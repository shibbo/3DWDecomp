#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ActionPadAndCameraCtrl;
class ActorInitInfo;
class AreaObj;
}  // namespace al

class GoalItem;
class GuideBalloon;
class Lighthouse;

/**
 * Root actor of an island ("ZoneHolder"/"Island"). Owns every actor and area placed on the island,
 * shows the actors that belong to the active scenario and raises the island out of the sea.
 */
class IslandHolder : public al::LiveActor {
public:
    explicit IslandHolder(const char* pName);

    static void initDemoActions();
    static void addDemoActions(const char* pSuffix);

    void init(const al::ActorInitInfo& rInfo) override;
    virtual void endInit();

    void hideZone();
    void onStartRiseSwitch();
    void onEndRiseSwitch();
    void startDemoOnActors();
    void endDemoOnActors();
    void exeIdle();
    void exeDelayRise();
    void exeRise();
    void allocBuffer(s32 zoneActorNum, s32 unused);
    void showZone(bool isPlayAppear);
    void setScenarioID(s32 scenarioId, bool isNoRespawn);
    void setIslandStartPos(const al::ActorInitInfo& rInfo);
    void turnOnCameraArea();
    void setLODDisable(bool isDisable);
    void setIslandName(const char* pName);
    void hideMovingActors();
    void linkActor(al::LiveActor* pActor);
    void addActor(al::LiveActor* pActor);
    bool isNewScenarioOpen(s32 islandId);
    void showEntranceBalloon(bool isShow);
    bool tryAddArea(al::AreaObj* pArea);
    GoalItem* findGoalItem(s32 shineId);
    void startIntro();

    /**
     * @brief Access the island's flag actor.
     * @return The flag actor, or nullptr when the island has none.
     */
    al::LiveActor* getIslandFlag() const { return mIslandFlag; }

    /**
     * Gets the id of the island.
     * @return The island id.
     */
    s32 getIslandId() const { return mIslandId; }

    /**
     * Gets the name of the island stage.
     * @return The island name.
     */
    const char* getIslandName() const { return mIslandName; }

    /**
     * Gets the placement object name of the island holder.
     * @return The object name.
     */
    const char* getObjectName() const { return mObjectName; }

    /**
     * Sets the id of the island.
     * @param islandId The island id.
     */
    void setIslandId(s32 islandId) { mIslandId = islandId; }

    /**
     * Sets the position of the effect played when the island appears.
     * @param rPos The effect position.
     */
    void setAppearEffectPos(const sead::Vector3f& rPos) { mAppearEffectPos = rPos; }

    /**
     * @brief Access the island's lighthouse.
     * @return The lighthouse, or nullptr when the island has none.
     */
    Lighthouse* getLighthouse() const { return mLighthouse; }

private:
    typedef sead::PtrArray<al::LiveActor> ActorArray;
    typedef sead::PtrArray<al::AreaObj> AreaArray;

    /// Number of scenario layers (layers 4-7); index 4 is the "scenario 2 and later" layer (8).
    static constexpr s32 cScenarioLayerNum = 4;

    ActorArray mZoneActors;                                         // 0x148
    ActorArray mScenarioActors[cScenarioLayerNum + 1];              // 0x158
    ActorArray mOtherScenarioActors[cScenarioLayerNum];             // 0x1a8
    sead::PtrArray<GoalItem> mGoalItems;                            // 0x1e8
    ActorArray mLinkedActors;                                       // 0x1f8
    AreaArray mScenarioAreas[cScenarioLayerNum + 1];                // 0x208
    AreaArray mOtherScenarioAreas[cScenarioLayerNum];               // 0x258
    al::LiveActor* mIslandFlag = nullptr;                           // 0x298
    Lighthouse* mLighthouse = nullptr;                              // 0x2a0
    al::LiveActor* mFixedPart;                                      // 0x2a8
    GuideBalloon* mGuideBalloon;                                    // 0x2b0
    al::ActorInitInfo* mIslandStartInfo = nullptr;                  // 0x2b8
    al::AreaObj* mStartCameraArea = nullptr;                        // 0x2c0
    const char* mObjectName = nullptr;                              // 0x2c8
    s32 mIslandId = 0;                                              // 0x2d0
    s32 _2d4;                                                       // 0x2d4
    s32 _2d8;                                                       // 0x2d8
    s32 mScenarioId = -1;                                           // 0x2dc
    bool mIsAllowEchoSounds = false;                                // 0x2e0
    bool mIsHidingZone = false;                                     // 0x2e1
    sead::Vector3f mAppearEffectPos = sead::Vector3f::zero;         // 0x2e4
    al::ActionPadAndCameraCtrl* mActionPadAndCameraCtrl = nullptr;  // 0x2f0
    sead::FixedSafeString<128> mDemoRiseName;                       // 0x2f8
    sead::FixedSafeString<128> mDemoDelayRiseName;                  // 0x390
    sead::FixedSafeString<128> mDemoEndRiseName;                    // 0x428
    f32 mRiseOffsetY = 0.0f;                                        // 0x4c0
    f32 mRiseStartY = 0.0f;                                         // 0x4c4
    s32 mRiseTime = 0;                                              // 0x4c8
    s32 mRiseDelayTime = 0;                                         // 0x4cc
    char mIslandName[128];                                          // 0x4d0
    s32 mRiseSeFrameStart = 0;                                      // 0x550
};

static_assert(sizeof(IslandHolder) == 0x558);
