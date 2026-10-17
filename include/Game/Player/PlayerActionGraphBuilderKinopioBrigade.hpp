#pragma once

#include "Player/Giga/PlayerActionGraphBuilder.hpp"

/**
 * @brief Builds the action graph of the Captain Toad players.
 */
class PlayerActionGraphBuilderKinopioBrigade : public IUsePlayerActionGraphBuilder {
public:
    PlayerActionGraphBuilderKinopioBrigade();
    PlayerActionGraph* create(Player* pPlayer) override;
};
