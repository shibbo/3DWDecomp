#pragma once

#include <container/seadPtrArray.h>
#include "Project/AreaObj/AreaObj.hpp"

class DisasterSpike;
class DisasterSpikeDirector;
namespace al {
class ActorInitInfo;
}

class DisasterSpikeArea : public al::AreaObj {
public:
    DisasterSpikeArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;
    void initSpikes(const al::ActorInitInfo& rInfo, DisasterSpikeDirector* pDirector);
    bool trigger(bool isImmediate);
    void setSpikesKillOutOfView(bool isKill);
    void clearTriggerFlags();
    sead::PtrArray<DisasterSpike>* getDisasterSpikes();
    s32 getInactiveSpikeCount();
    bool canSpawnGoldSpikes() const;

    sead::PtrArray<DisasterSpike> mSpikes;
    bool mCanSpawnGoldSpikes = true;
};
