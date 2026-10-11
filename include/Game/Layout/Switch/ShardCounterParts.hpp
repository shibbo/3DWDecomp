#pragma once

#include "Layout/Switch/SingleModeCounterBase.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Bowser's Fury HUD counter of the cat shine shards collected on the current island.
 * @note Only what reconstructed code needs is declared so far.
 */
class ShardCounterParts : public SingleModeCounterBase {
public:
    ShardCounterParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                      al::LayoutActor* pParent);

    void appear() override;
    void control() override;
    virtual void endDemo();
    void addShard(s32 islandId, s32 shardIndex, bool isComplete, bool isDemo);
    void updateString(s32 count, s32 max);
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
    u8 _122[0x130 - 0x122];
    bool mIsShardDemo;  // 0x130
    u8 _131[0x180 - 0x131];
};

static_assert(sizeof(ShardCounterParts) == 0x180);
