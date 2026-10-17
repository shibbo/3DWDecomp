#include "Enemy/ChorobonNode.hpp"
#include "Library/Rail/RailUtil.hpp"

/** @brief Constructs a node used by a group of Fuzzies.
 * @param pName Actor name.
 * @param pParam External float data retained by the node; not read by this unit.
 */
ChorobonNode::ChorobonNode(const char* pName, const float* pParam)
    : al::LiveActor(pName), mParam(pParam) {}

/** @brief Places the node on its rail, reflecting coordinates beyond an open rail's end.
 * @param coord Distance along the rail.
 */
void ChorobonNode::setRailCoord(float coord) {
    if (!al::isLoopRail(this) && al::getRailTotalLength(this) < coord) {
        coord = 2.0f * al::getRailTotalLength(this) - coord;
    }
    al::setRailPosToCoord(this, coord);
    al::syncRailTrans(this);
}
