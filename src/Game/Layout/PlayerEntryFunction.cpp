#include "Layout/PlayerEntryFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "System/ControlUserDataHolder.hpp"
#include "System/GameDataFile.hpp"
#include "System/Data/SingleModeData.hpp"
namespace PlayerEntryFunction {
/** @brief Counts character types that are not assigned to a player.
 * @param pHolder Game data containing player assignments. */
int calcNotUsePlayerCharacterTypeNum(const GameDataHolder* pHolder) {
    return calcNotUsePlayerCharacterTypeList(nullptr, pHolder);
}
/** @brief Collects unused character types in ascending order.
 * @param pList Optional output array with room for every character type.
 * @param pHolder Game data containing player assignments.
 * @return Number of unused character types. */
int calcNotUsePlayerCharacterTypeList(int* pList, const GameDataHolder* pHolder) {
    GameDataHolderAccessor accessor(const_cast<GameDataHolder*>(pHolder));
    int max = rc::calcCharacterTypeNumMax(accessor);
    int i = 0;
    int count = 0;
    for (; i < max; i++) {
        if (rc::tryCalcControlUserIdByCharacterType(accessor, i, false) >= 0)
            continue;
        if (pList)
            pList[count] = i;
        count++;
    }
    return count;
}
/** @brief Finds the previous unused character, wrapping at the list boundary.
 * @param characterType Current character type.
 * @param pHolder Game data containing player assignments. */
int calcPrevPlayerCharacterType(int characterType, const GameDataHolder* pHolder) {
    int list[5];
    int count = calcNotUsePlayerCharacterTypeList(list, pHolder);
    if (count < 1) return -1;
    if (count == 1) return list[0];
    int index = 0;
    for (int i = 0; i < count; i++) {
        if (list[i] >= characterType) {
            index = (count + i - 1) % count;
            break;
        }
    }
    return list[index];
}
/** @brief Finds the next unused character, wrapping at the list boundary.
 * @param characterType Current character type.
 * @param pHolder Game data containing player assignments. */
int calcNextPlayerCharacterType(int characterType, const GameDataHolder* pHolder) {
    int list[5];
    int count = calcNotUsePlayerCharacterTypeList(list, pHolder);
    if (count < 1) return -1;
    if (count == 1) return list[0];
    int index = 0;
    for (int i = 0; i < count; i++) {
        if (list[i] == characterType) {
            index = (i + 1) % count;
            break;
        }
        if (list[i] > characterType) {
            index = i;
            break;
        }
    }
    return list[index];
}
/** @brief Tests whether a character is available for entry.
 * @param characterType Character type to check.
 * @param pHolder Game data containing player assignments. */
bool isEnableUsePlayerCharacterType(int characterType, const GameDataHolder* pHolder) {
    int list[5];
    int count = calcNotUsePlayerCharacterTypeList(list, pHolder);
    for (int i = 0; i < count; i++) {
        if (list[i] == characterType)
            return true;
    }
    return false;
}
/** @brief Starts character selection for the current players.
 * @param writer Writable game-data handle. */
void startCharacterSelect(GameDataHolderWriter writer) {
    writer.getHolder()->getControlUserDataHolder()->startCharacterSelect();
}
/** @brief Shuffles character assignments and resets every player.
 * @param writer Writable game-data handle. */
void shufflePlayerModel(GameDataHolderWriter writer) {
    writer.getHolder()->getControlUserDataHolder()->shufflePlayerModel(writer.getHolder());
    writer.getHolder()->getControlUserDataHolder()->resetPlayerAll();
}
/** @brief Enters a player in the active game mode.
 * @param writer Writable game-data handle.
 * @param userId Controller user index.
 * @param characterType Character selected by the player. */
bool entryPlayer(GameDataHolderWriter writer, int userId, int characterType) {
    if (writer.getHolder()->isSingleMode())
        return writer.getHolder()->getSingleFile()->entryPlayer(userId, characterType);
    return writer.getHolder()->getPlayingFile()->entryPlayer(userId, characterType);
}
/** @brief Removes a player from the current assignments.
 * @param writer Writable game-data handle.
 * @param userId Controller user index. */
void retirePlayer(GameDataHolderWriter writer, int userId) {
    writer.getHolder()->getControlUserDataHolder()->retirePlayer(userId);
}
/** @brief Resets all player assignments.
 * @param writer Writable game-data handle. */
void resetPlayerAll(GameDataHolderWriter writer) {
    writer.getHolder()->getControlUserDataHolder()->resetPlayerAll();
}
}
