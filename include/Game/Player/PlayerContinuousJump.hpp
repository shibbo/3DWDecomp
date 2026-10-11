#pragma once

#include <basis/seadTypes.h>

class PlayerActionGraph;
class PlayerConstParam;

/// Counts the jumps done in a row (for the double and triple jump).
class PlayerContinuousJump {
public:
    PlayerContinuousJump(const PlayerConstParam* pConstParam);
    void update();
    void clear();
    void countUp();
    s32 getContinuousCount() const;

    void setActionGraph(const PlayerActionGraph* pActionGraph) { mActionGraph = pActionGraph; }

private:
    const PlayerConstParam* mConstParam;  // 0x0
    const PlayerActionGraph* mActionGraph;  // 0x8
    u8 _10[0x18 - 0x10];
};
static_assert(sizeof(PlayerContinuousJump) == 0x18);
