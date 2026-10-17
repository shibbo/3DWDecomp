#include "LuigiBros/config.hpp"
#include "LuigiBros/Emulator/inputdevice.hpp"
#include "LuigiBros/bluetooth.hpp"
#include "LuigiBros/vessel.hpp"

/**
 * @brief Serializes controller settings into the four-byte configuration header.
 * @param pData Destination buffer of at least four bytes.
 * @return Number of bytes written.
 */
unsigned int configSaveData::saveConfigHeader(unsigned char* pData) {
    mHeader = 0;
    for (unsigned int i = 0; i < 4; ++i) {
        if (m_inputdevice_controller.getRemoveVertical(i)) {
            mHeader |= 1 << i;
        }
    }
    if (m_bluetooth.mEnableRemove) {
        mHeader |= 0x10;
    }
    mHeader |= m_vessel.mDrcDisplay << 5;
    for (unsigned int i = 0; i < 4; ++i) {
        mHeader |= (mCombination[i] & 7) << (8 + i * 3);
    }
    for (unsigned int i = 0; i < 4; ++i) {
        pData[i] = mHeader >> (i * 8);
    }
    return 4;
}

/**
 * @brief Restores controller settings from the configuration header.
 * @param pData Source buffer containing at least four bytes.
 * @return Number of bytes read.
 */
unsigned int configSaveData::loadConfigHeader(unsigned char* pData) {
    mHeader = 0;
    for (unsigned int i = 0; i < 4; ++i) {
        mHeader |= static_cast<unsigned int>(pData[i]) << (i * 8);
    }
    for (unsigned int i = 0; i < 4; ++i) {
        if (mHeader & (1 << i)) {
            m_inputdevice_controller.setRemoveVertical(i, true);
        } else {
            m_inputdevice_controller.setRemoveVertical(i, false);
        }
    }
    m_inputdevice_controller.mRemove = (mHeader & 0x10) != 0;
    m_inputdevice_controller.mDrcPlayer = 0;
    for (unsigned int i = 0; i < 4; ++i) {
        _getCombination(i);
    }
    m_vessel.mDrcDisplay = true;
    return 4;
}

/**
 * @brief Decodes and caches a player's controller combination.
 * @param player Player slot index from zero through three.
 */
void configSaveData::_getCombination(unsigned int player) {
    unsigned int shift = 8;
    switch (player) {
    case 1: shift = 11; break;
    case 2: shift = 14; break;
    case 3: shift = 17; break;
    }
    unsigned int combination = (mHeader >> shift) & 7;
    switch (combination) {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7:
        break;
    default:
        combination = 0;
        break;
    }
    mCombination[player] = static_cast<controller::COMBINATION>(combination);
}

/** @brief Gets the platform's GamePad display setting. @return Always true. */
bool configSaveData::getDRCDisp() {
    return true;
}

/** @brief Gets the saved GamePad player index. @return Player slot from zero through three. */
unsigned int configSaveData::getDRCPlayer() {
    return (mHeader >> 6) & 3;
}
