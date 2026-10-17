#include "AreaObj/NeonAreaWatcher.hpp"
#include "Util/AreaObjUtil.hpp"

/**
 * @brief Constructs a watcher for players in neon areas.
 * @param pDirector Area object director.
 * @param pPlayerHolder Player holder to query.
 */
NeonAreaWatcher::NeonAreaWatcher(al::AreaObjDirector* pDirector,
                               const al::PlayerHolder* pPlayerHolder)
    : mPlayerHolder(pPlayerHolder), mAreaObjDirector(pDirector) {}

/**
 * @brief Checks whether a player is inside a neon area.
 * @return Whether the player-area query succeeds.
 */
bool NeonAreaWatcher::isInPlayer() {
    return rc::isInAreaObjPlayerOne(this, rc::AreaObjType::NeonArea, mPlayerHolder);
}
