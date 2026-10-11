#pragma once
#include "Library/Layout/LayoutActor.hpp"
class GameDataHolder;
namespace al {
class LayoutInitInfo;
}
/** @brief Remaining-player counter displayed after a miss. */
class CounterPlayerInfo : public al::LayoutActor {
  public:
    CounterPlayerInfo(const al::LayoutInitInfo& rInfo, const GameDataHolder* pHolder);

  private:
    u8 _128[0x18]; ///< Not reconstructed yet.
};
static_assert(sizeof(CounterPlayerInfo) == 0x140);
