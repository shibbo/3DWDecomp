#pragma once

#include "Layout/Switch/SingleModeCounterBase.hpp"
#include "Layout/TextBoxTextInfo.hpp"
class SingleModeSceneLayout;

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Bowser's Fury HUD counter of the cat shine shards collected on the current island.
 */
class ShardCounterParts : public SingleModeCounterBase {
public:
    ShardCounterParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                      al::LayoutActor* pParent);

    void appear() override;
    void control() override;
    virtual void endDemo();
    void addShard(s32 islandId, s32 shardIndex, bool isComplete, bool isDemo);
    void updateString(s32 islandId, s32 excludedShard);
    void updateCount(s32 islandId);
    void endShardDemo();

    void exeAdd();
    void exeComplete();
    void exeCompleteEnd();

    /** @brief Mark the counter as being shown by a shard collection demo. */
    void startShardDemo() { mIsShardDemo = true; }

    /**
     * @brief Check whether the counter is being shown by a shard collection demo.
     * @return True during a shard demo.
     */
    bool isShardDemo() const { return mIsShardDemo; }

private:
    SingleModeSceneLayout* mParent;
    bool mIsShardDemo = false;
    s32 mPendingShard = -1;
    fix::TextBoxTextInfo mTextInfo;
    u8 mUnreconstructed158[0x20];
    s32 mIslandId = -1;
};

static_assert(sizeof(ShardCounterParts) == 0x180);
