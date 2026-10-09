#pragma once

#include "System/GameDataHolderWriter.hpp"

namespace PlayerEntryFunction {
bool entryPlayer(GameDataHolderWriter writer, int userId, int characterType);
void startCharacterSelect(GameDataHolderWriter writer);
void shufflePlayerModel(GameDataHolderWriter writer);
void retirePlayer(GameDataHolderWriter writer, int userId);
int calcNextPlayerCharacterType(int characterType, const GameDataHolder* pHolder);
int calcPrevPlayerCharacterType(int characterType, const GameDataHolder* pHolder);
int calcNotUsePlayerCharacterTypeList(int* pList, const GameDataHolder* pHolder);
bool isEnableUsePlayerCharacterType(int characterType, const GameDataHolder* pHolder);
}  // namespace PlayerEntryFunction
