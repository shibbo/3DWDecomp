#pragma once
#include "System/GameDataHolderAccessor.hpp"
class GameDataHolderWriter : public GameDataHolderAccessor {
  public:
    using GameDataHolderAccessor::GameDataHolderAccessor;
    /** @brief Creates a writable handle. @param accessor Existing game-data handle. */
    GameDataHolderWriter(const GameDataHolderAccessor& accessor) : GameDataHolderAccessor(accessor) {}
};
