#include "AreaObj/DisasterSpikeArea.hpp"
#include "MapObj/DisasterSpike.hpp"
#include "MapObj/Fury/DisasterSpikeBouncy.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Project/Base/StringUtil.hpp"

/**
 * @brief Constructs an area that can spawn gold spikes.
 * @param pName Area name.
 */
DisasterSpikeArea::DisasterSpikeArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Loads the area's gold spike setting.
 * @param rInfo Area placement and switch information.
 */
void DisasterSpikeArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
    al::tryGetArg(&mCanSpawnGoldSpikes, rInfo.mPlacementInfo, "CanSpawnGoldSpikes");
}

/**
 * @brief Creates linked spike actors that are enabled for the current scenario.
 * @param rInfo Host actor initialization information.
 * @param pDirector Director assigned to each created spike.
 */
void DisasterSpikeArea::initSpikes(const al::ActorInitInfo& rInfo,
                                  DisasterSpikeDirector* pDirector) {
    s32 count = al::calcLinkChildNum(*mPlacementInfo, "DisasterSpike");
    if (count == 0) {
        return;
    }
    mSpikes.allocBuffer(count, nullptr);
    for (s32 i = 0; i < count; ++i) {
        al::ActorInitInfo spikeInfo;
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, *mPlacementInfo, "DisasterSpike", i);
        spikeInfo.initViewIdSelf(&placementInfo, rInfo);
        if (SingleModeDataFunction::isActorInPlessieChaseDisabled(spikeInfo)) {
            continue;
        }
        const char* pObjectName = nullptr;
        DisasterSpike* pSpike;
        if (al::tryGetObjectName(&pObjectName, spikeInfo) &&
            al::isEqualString(pObjectName, "DisasterSpikeBouncy")) {
            pSpike = new DisasterSpikeBouncy("DisasterSpikeBouncy");
        } else {
            pSpike = new DisasterSpike("DisasterSpike");
        }
        pSpike->setDisasterSpikeDirector(pDirector);
        pSpike->init(spikeInfo);
        pSpike->kill();
        mSpikes.pushBack(pSpike);
    }
}

/**
 * @brief Attempts to make every untriggered spike appear.
 * @param isImmediate Appearance mode forwarded to each spike.
 * @return Whether all attempted appearances succeeded.
 */
bool DisasterSpikeArea::trigger(bool isImmediate) {
    bool success = true;
    for (s32 i = 0; i < mSpikes.size(); ++i) {
        if (!mSpikes[i]->isTriggered()) {
            success &= mSpikes[i]->tryAppear(isImmediate);
        }
    }
    return success;
}

/**
 * @brief Sets whether each spike is killed outside the view.
 * @param isKill Out-of-view kill setting.
 */
void DisasterSpikeArea::setSpikesKillOutOfView(bool isKill) {
    for (s32 i = 0; i < mSpikes.size(); ++i) {
        mSpikes[i]->setKillOutOfView(isKill);
    }
}

/** @brief Clears the trigger flag on every spike. */
void DisasterSpikeArea::clearTriggerFlags() {
    for (s32 i = 0; i < mSpikes.size(); ++i) {
        mSpikes[i]->setTriggered(false);
    }
}

/**
 * @brief Returns the linked spike array.
 * @return Array of spikes owned by this area.
 */
sead::PtrArray<DisasterSpike>* DisasterSpikeArea::getDisasterSpikes() {
    return &mSpikes;
}

/**
 * @brief Counts spikes whose actors are dead.
 * @return Number of inactive spike actors.
 */
s32 DisasterSpikeArea::getInactiveSpikeCount() {
    s32 count = 0;
    for (s32 i = 0; i < mSpikes.size(); ++i) {
        count += al::isDead(mSpikes[i]);
    }
    return count;
}

/**
 * @brief Queries the area's gold spike setting.
 * @return Whether gold spikes may spawn.
 */
bool DisasterSpikeArea::canSpawnGoldSpikes() const {
    return mCanSpawnGoldSpikes;
}
