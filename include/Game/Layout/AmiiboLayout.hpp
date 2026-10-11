#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}  // namespace al

class PlayerActor;

/// Layout shown next to a player while an amiibo scan is in progress.
class AmiiboLayout : public al::LayoutActor {
public:
    AmiiboLayout(const al::LayoutInitInfo& rInfo, const PlayerActor* pPlayer);

    void appear() override;
    void end(bool isImmediate);
    void exeAppear();
    void updateTrans();

    /**
     * @brief Set the player the layout follows.
     * @param pPlayer The player.
     */
    void setPlayerActor(const PlayerActor* pPlayer) { mPlayer = pPlayer; }

private:
    const PlayerActor* mPlayer;  // 0x128
};
