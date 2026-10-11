#pragma once

#include <container/seadPtrArray.h>

#include "Project/AreaObj/AreaObj.hpp"

namespace al {
class ActorInitInfo;
}  // namespace al

class DisasterSpike;
class DisasterSpikeDirector;

/// Area holding the disaster spikes that fall on one part of an island.
class DisasterSpikeArea : public al::AreaObj {
public:
    explicit DisasterSpikeArea(const char* pName);

    void initSpikes(const al::ActorInitInfo& rInfo, DisasterSpikeDirector* pDirector);
    void clearTriggerFlags();
    bool trigger(bool isFirst);
    void setSpikesKillOutOfView(bool isKill);
    bool canSpawnGoldSpikes() const;
    s32 getInactiveSpikeCount();
    sead::PtrArray<DisasterSpike>* getDisasterSpikes();

    /**
     * @brief Get the id of the island the area belongs to.
     * @return The island id, or -1 when the area belongs to no island.
     */
    s32 getIslandID() const { return mZoneID; }
};
