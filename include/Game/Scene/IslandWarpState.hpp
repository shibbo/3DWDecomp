#pragma once

#include <basis/seadTypes.h>
#include "Demo/StageStartEventBase.hpp"

class SingleModeScene;

/**
 * @brief Demo warping the player to another island from the island map.
 * @note Only what reconstructed code needs is declared so far.
 */
class IslandWarpState : public StageStartEventBase {
public:
    explicit IslandWarpState(SingleModeScene* pScene);

    void setIslandWarpDest(s32 islandId);

    s64 getEventType() const override;
    void startDemo() override;
    void endDemo() override;
    bool isEndDemo() const override;
};
