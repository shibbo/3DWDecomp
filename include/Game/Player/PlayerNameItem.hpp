#pragma once
/** @brief Character-name entry used to select an NPC model and voice. */
class PlayerNameItem {
public:
    static const PlayerNameItem* getPlayerNameItem(const char* pName);
    const char* mNames[3];
    int mCharacter;
    int mVoice;
};
