#include "AreaObj/TorpedoSpikeRestrictionArea.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

/**
 * @brief Constructs a torpedo spike restriction area.
 * @param pName Area name.
 */
TorpedoSpikeRestrictionArea::TorpedoSpikeRestrictionArea(const char* pName) : al::AreaObj(pName) {}

/**
 * @brief Loads the explosion and sinking restrictions.
 * @param rInfo Area placement and switch information.
 */
void TorpedoSpikeRestrictionArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
    al::tryGetArg(&mExplode, rInfo.mPlacementInfo, "Explode");
    al::tryGetArg(&mSink, rInfo.mPlacementInfo, "Sink");
}

/**
 * @brief Queries the explosion restriction.
 * @return Whether spikes should explode in this area.
 */
bool TorpedoSpikeRestrictionArea::isExplode() const {
    return mExplode;
}

/**
 * @brief Queries the sinking restriction.
 * @return Whether spikes should sink in this area.
 */
bool TorpedoSpikeRestrictionArea::isSink() const {
    return mSink;
}
