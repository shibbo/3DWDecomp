#pragma once
#include <prim/seadBitFlag.h>
#include "Library/LiveActor/LiveActor.hpp"
#include "System/ScenarioInfo.hpp"
class KinopioBrigadeNpc;
class KinopioBrigadeWatcher : public al::LiveActor {
public:
    explicit KinopioBrigadeWatcher(const char*);
    ~KinopioBrigadeWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void exeWatch();
    ScenarioInfo getScenarioInfo();
    int getDiscoveredMemberCount() const;
    sead::BitFlag8 getBrigadeCollected() const;

    /** @return Number of brigade members the watcher tracks. */
    int getMemberCount() const { return mMemberCount; }

    /**
     * @brief Sets the scenario of the shine awarded once the whole brigade is assembled.
     * @param shineId Shine (scenario) id, 1-based.
     * @param islandId Island id, 1-based.
     */
    void setScenario(int shineId, int islandId) {
        mShineId = shineId;
        mIslandId = islandId;
    }
private:
    KinopioBrigadeNpc* mMembers[4];
    int mMemberCount = 0;
    int mShineId = -1;
    int mIslandId = -1;
};
static_assert(sizeof(KinopioBrigadeWatcher) == 0x178);
