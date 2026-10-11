#pragma once

#include "Project/AreaObj/AreaObj.hpp"

/// Area of an island where Fury Bowser launches spikes during disaster mode.
class DisasterModeArea : public al::AreaObj {
public:
    explicit DisasterModeArea(const char* pName);

    void init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) override;

    /**
     * @brief Get the id of the island the area belongs to.
     * @return The island id, or -1 when the area belongs to no island.
     */
    s32 getIslandID() const { return mZoneID; }

    /**
     * @brief Check whether the player was in the launch spike anticipation area last update.
     * @return True if the player was in the anticipation area.
     */
    bool isPlayerInAnticipation() const { return mIsPlayerInAnticipation; }

    /**
     * @brief Set whether the player is in the launch spike anticipation area.
     * @param isIn True if the player is in the anticipation area.
     */
    void setPlayerInAnticipation(bool isIn) { mIsPlayerInAnticipation = isIn; }

private:
    u8 _84[0x88 - 0x84];
    bool mIsPlayerInAnticipation;  // 0x88
};
