#include "LuigiBros/Emulator/inputdevice.hpp"
#include "LuigiBros/bluetooth.hpp"
#include "LuigiBros/Emulator/cafe.hpp"
#include <cstdlib>

/** @brief Initializes repeat state and clears input suppression flags. */
inputdevice_controller::inputdevice_controller() {
    mBlocked = false;
    mDrcBlocked = false;
}

/** @brief Destroys the input controller. */
inputdevice_controller::~inputdevice_controller() = default;

/** @brief Initializes controller input, player entries, and masks. */
void inputdevice_controller::init() {
    mPlayerCount = 1;
    m_bluetooth.init(true);
    resetEntry();
    mVcm = 0xff;
    mDrcPlayer = 0;
    mBlocked = false;
    mRepeat[0].counter = 0;
    mRepeat[0].previous = 0;
    mRepeat[1].counter = 0;
    mRepeat[1].previous = 0;
    mDrcBlocked = false;
    for (unsigned int i = 0; i < 5; ++i) {
        mButtonMasks[i] = 0;
        mMoveMasks[i] = 0;
    }
}

/** @brief Clears player assignments and enables the primary GamePad buttons. */
void inputdevice_controller::resetEntry() {
    for (unsigned int i = 0; i < 5; ++i) {
        mPlaylist[i] = 0xff;
    }
    m_bluetooth.setDRCEnableButton(0, true);
}

/**
 * @brief Gets the assigned controller type.
 * @param player Player slot index.
 * @return Controller type, or the disconnected sentinel.
 */
controller::TYPE inputdevice_controller::getType(unsigned int player) {
    if (IsBluetooth(player)) {
        return m_bluetooth.getType(mPlaylist[player]);
    }
    return controller::TYPE_NONE;
}

/**
 * @brief Checks whether a player has a supported controller channel.
 * @param player Player slot index.
 * @return Whether the assigned channel is in range.
 */
bool inputdevice_controller::IsBluetooth(unsigned int player) {
    return mPlaylist[player] < 12;
}

/**
 * @brief Platform stub for USB controller detection.
 * @param player Player slot index.
 * @return Always false.
 */
bool inputdevice_controller::IsUSB(unsigned int player) {
    return false;
}

/**
 * @brief Gets held buttons after applying the release mask.
 * @param player Player slot index.
 * @return Unmasked button bits.
 */
unsigned int inputdevice_controller::getButton(unsigned int player) {
    if (mBlocked) {
        return 0;
    }
    unsigned int buttons = 0;
    switch (static_cast<unsigned int>(getType(player))) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 8: case 9:
        buttons = m_bluetooth.getButton(mPlaylist[player]);
        break;
    }
    unsigned int result = buttons & ~mButtonMasks[player];
    mButtonMasks[player] &= buttons;
    return result;
}

/**
 * @brief Gets GamePad input after applying the release mask.
 * @param player Player slot index.
 * @return Unmasked input bits.
 */
unsigned int inputdevice_controller::getDRCButton(unsigned int player) {
    unsigned int buttons = 0;
    if (m_bluetooth.mControllers[0].type == controller::TYPE_DRC && mDrcPlayer == player && !mDrcBlocked) {
        buttons = m_bluetooth.getButton(0);
    }
    unsigned int result = buttons & ~mButtonMasks[4];
    mButtonMasks[4] &= buttons;
    return result;
}

/**
 * @brief Gets GamePad input after applying the release mask.
 * @param player Player slot index.
 * @return Unmasked input bits.
 */
unsigned char inputdevice_controller::getDRCMoveButton(unsigned int player) {
    unsigned char buttons = 0;
    if (m_bluetooth.mControllers[0].type == controller::TYPE_DRC && mDrcPlayer == player && !mDrcBlocked) {
        buttons = m_bluetooth.getMoveButton(0);
    }
    unsigned char result = buttons & ~mMoveMasks[4];
    mMoveMasks[4] &= buttons;
    return result;
}

/**
 * @brief Releases masks for buttons returned by the emulator.
 */
void inputdevice_controller::inEmu() {
    for (unsigned int i = 0; i < 5; ++i) {
        mButtonMasks[i] = mButtonMasks[i] & ~mOutputButtons[i];
        mMoveMasks[i] = mMoveMasks[i] & ~mOutputMove[i];
    }
}

/**
 * @brief Checks vertical input suppression.
 * @param player Player slot index.
 * @return Whether vertical input is suppressed.
 */
bool inputdevice_controller::getRemoveVertical(unsigned int player) {
    return m_bluetooth.getRemoveVertical(player + 4);
}

/**
 * @brief Checks whether the primary GamePad serves this player.
 * @param player Player slot index.
 * @return Whether a GamePad is assigned.
 */
bool inputdevice_controller::IsDRC(unsigned int player) {
    switch (player) {
    case 0: case 1: case 2: case 3:
        if (mDrcPlayer != player) {
            return false;
        }
        break;
    case 0xff:
        break;
    default:
        return false;
    }
    return m_bluetooth.mControllers[0].type == controller::TYPE_DRC;
}

/**
 * @brief Tests held button bits.
 * @param player Player slot index.
 * @param button Button bits to test.
 * @return Whether any requested button is held.
 */
bool inputdevice_controller::getButtonOn(unsigned int player, controller::BUTTON button) {
    return (getButton(player) & button) != 0;
}

/**
 * @brief Registers a controller channel in the player list.
 * @param channel Controller channel to register.
 * @param mode Whether to find a free slot or replace a specific slot.
 * @param player Destination slot in explicit-slot mode.
 * @return Zero on success, one if already registered, two if unavailable, or three for channel zero.
 */
int inputdevice_controller::addEntry(unsigned int channel, ADDENTRY_MODE mode, unsigned int player) {
    if (channel == 0) {
        return 3;
    }
    switch (mode) {
    case ENTRY_FIRST_FREE: {
        for (unsigned int i = 0; i < 5; ++i) {
            if (mPlaylist[i] == channel) {
                return 1;
            }
        }
        int count = mPlayerCount;
        if (mPlayerCount >= 5) { count = 5; }
        for (int i = 0; i < count; ++i) {
            if (mPlaylist[i] == 0xff) {
                mPlaylist[i] = channel;
                return 0;
            }
        }
        break;
    }
    case ENTRY_SLOT:
        if (player < (mPlayerCount < 5 ? mPlayerCount : 5)) {
            mPlaylist[player] = channel;
            return 0;
        }
        break;
    }
    return 2;
}

/**
 * @brief Removes a controller from all player slots.
 * @param channel Controller channel to remove.
 */
void inputdevice_controller::deleteEntry(unsigned int channel) {
    for (unsigned int i = 0; i < 5; ++i) {
        if (mPlaylist[i] == channel) {
            mPlaylist[i] = 0xff;
        }
    }
}

/**
 * @brief Reads controller sensor data for an assigned channel.
 * @param player Player slot index.
 * @param pX Output horizontal pointer position.
 * @param pY Output vertical pointer position.
 * @return Sensor result, or zero if no supported channel is assigned.
 */
bool inputdevice_controller::getDPDPos(unsigned int player, float* pX, float* pY) {
    if (IsBluetooth(player)) {
        return m_bluetooth.getDPDPos(mPlaylist[player], pX, pY);
    }
    return false;
}

/**
 * @brief Reads controller sensor data for an assigned channel.
 * @param player Player slot index.
 * @param pX Output X acceleration.
 * @param pY Output Y acceleration.
 * @param pZ Output Z acceleration.
 * @return Sensor result, or zero if no supported channel is assigned.
 */
bool inputdevice_controller::getAcc(unsigned int player, float* pX, float* pY, float* pZ) {
    if (IsBluetooth(player)) {
        return m_bluetooth.getAcc(mPlaylist[player], pX, pY, pZ);
    }
    return false;
}

/**
 * @brief Reads controller sensor data for an assigned channel.
 * @param player Player slot index.
 * @return Sensor result, or zero if no supported channel is assigned.
 */
float inputdevice_controller::getAccValue(unsigned int player) {
    if (IsBluetooth(player)) {
        return m_bluetooth.getAccValue(mPlaylist[player]);
    }
    return 0.0f;
}

/**
 * @brief Reads controller sensor data for an assigned channel.
 * @param player Player slot index.
 * @return Sensor result, or zero if no supported channel is assigned.
 */
float inputdevice_controller::getAccSpeed(unsigned int player) {
    if (IsBluetooth(player)) {
        return m_bluetooth.getAccSpeed(mPlaylist[player]);
    }
    return 0.0f;
}

/**
 * @brief Reads controller sensor data for an assigned channel.
 * @param player Player slot index.
 * @param pX Output horizontal component.
 * @param pY Output vertical component.
 * @return Sensor result, or zero if no supported channel is assigned.
 */
bool inputdevice_controller::getHorizon(unsigned int player, float* pX, float* pY) {
    if (IsBluetooth(player)) {
        return m_bluetooth.getHorizon(mPlaylist[player], pX, pY);
    }
    return false;
}

/**
 * @brief Reads a GamePad property for an assigned player.
 * @param player Player slot index.
 * @param pX Output touch X coordinate.
 * @param pY Output touch Y coordinate.
 * @return Property value, or zero for other controller types.
 */
bool inputdevice_controller::getTouchPanel(unsigned int player, unsigned short* pX, unsigned short* pY) {
    if (getType(player) == controller::TYPE_DRC) {
        return m_bluetooth.getTouchPanel(mPlaylist[player], pX, pY);
    }
    return 0;
}

/**
 * @brief Reads a property from the primary GamePad.
 * @param pX Output touch X coordinate.
 * @param pY Output touch Y coordinate.
 * @return Property value, or false if no GamePad is present.
 */
bool inputdevice_controller::getTouchPanel(unsigned short* pX, unsigned short* pY) {
    if (m_bluetooth.mControllers[0].type == controller::TYPE_DRC) {
        return m_bluetooth.getTouchPanel(0, pX, pY);
    }
    return false;
}

/**
 * @brief Reads a GamePad property for an assigned player.
 * @param player Player slot index.
 * @return Property value, or zero for other controller types.
 */
bool inputdevice_controller::getTouchON(unsigned int player) {
    if (getType(player) == controller::TYPE_DRC) {
        return m_bluetooth.getTouchON(mPlaylist[player]);
    }
    return 0;
}

/**
 * @brief Reads a property from the primary GamePad.
 * @return Property value, or false if no GamePad is present.
 */
bool inputdevice_controller::getTouchON() {
    if (m_bluetooth.mControllers[0].type == controller::TYPE_DRC) {
        return m_bluetooth.getTouchON(0);
    }
    return false;
}

/**
 * @brief Reads a GamePad property for an assigned player.
 * @param player Player slot index.
 * @return Property value, or zero for other controller types.
 */
bool inputdevice_controller::getTouchOFF(unsigned int player) {
    if (getType(player) == controller::TYPE_DRC) {
        return m_bluetooth.getTouchOFF(mPlaylist[player]);
    }
    return 0;
}

/**
 * @brief Reads a property from the primary GamePad.
 * @return Property value, or false if no GamePad is present.
 */
bool inputdevice_controller::getTouchOFF() {
    if (m_bluetooth.mControllers[0].type == controller::TYPE_DRC) {
        return m_bluetooth.getTouchOFF(0);
    }
    return false;
}

/**
 * @brief Reads a GamePad property for an assigned player.
 * @param player Player slot index.
 * @return Property value, or zero for other controller types.
 */
bool inputdevice_controller::IsHeadphone(unsigned int player) {
    if (getType(player) == controller::TYPE_DRC) {
        return m_bluetooth.IsHeadphone(mPlaylist[player]);
    }
    return 0;
}

/**
 * @brief Reads a GamePad property for an assigned player.
 * @param player Player slot index.
 * @return Property value, or zero for other controller types.
 */
unsigned char inputdevice_controller::getVolume(unsigned int player) {
    if (getType(player) == controller::TYPE_DRC) {
        return m_bluetooth.getVolume(mPlaylist[player]);
    }
    return 0;
}

/**
 * @brief Disables the primary GamePad buttons when it is not registered.
 */
void inputdevice_controller::NonRegistrationDisconnect() {
    for (unsigned int i = 0; i < 5; ++i) {
        if (mPlaylist[i] == 0) {
            return;
        }
    }
    m_bluetooth.setDRCEnableButton(0, false);
}

/**
 * @brief Copies the player list to or from its backup.
 */
void inputdevice_controller::backupPlaylist() {
    for (unsigned int i = 0; i < 5; ++i) {
        mBackupPlaylist[i] = mPlaylist[i];
    }
}

/**
 * @brief Copies the player list to or from its backup.
 */
void inputdevice_controller::recoveryPlaylist() {
    for (unsigned int i = 0; i < 5; ++i) {
        mPlaylist[i] = mBackupPlaylist[i];
    }
}

/**
 * @brief Changes vertical input suppression.
 * @param player Player slot index.
 * @param remove Whether to suppress vertical input.
 * @return Always true.
 */
bool inputdevice_controller::setRemoveVertical(unsigned int player, bool remove) {
    m_bluetooth.setRemoveVertical(player + 4, remove);
    return true;
}

/**
 * @brief Changes the controller removal flag.
 * @param enabled Whether removal behavior is enabled.
 */
void inputdevice_controller::EnableRemove(bool enabled) {
    m_bluetooth.mEnableRemove = enabled;
}

/**
 * @brief Gets analog stick coordinates for an assigned player.
 * @param player Player slot index.
 * @param stick Stick to read.
 * @param pX Output horizontal position.
 * @param pY Output vertical position.
 */
void inputdevice_controller::getAnalog(unsigned int player, controller::ANALOGSTICK stick, int* pX, int* pY) {
    *pX = 0;
    *pY = 0;
    if (getType(player) != controller::TYPE_NONE) {
        m_bluetooth.getAnalog(mPlaylist[player], stick, pX, pY);
    }
}

/**
 * @brief Checks the microphone flag.
 * @return Whether microphone input is active.
 */
bool inputdevice_controller::IsMicrophone() {
    return m_bluetooth.mMicrophone;
}

/**
 * @brief Checks whether a controller-menu request is pending.
 * @return Whether a request is pending.
 */
bool inputdevice_controller::IsVCM() {
    return mVcm != 0xff;
}

/**
 * @brief Checks the controller removal flag.
 * @return Whether removal behavior is enabled.
 */
bool inputdevice_controller::IsEnableRemove() {
    return m_bluetooth.mEnableRemove;
}


/**
 * @brief Reads a little-endian integer from bytes.
 * @param pData Input byte buffer.
 * @param size Number of bytes, at most four for a 32-bit result.
 * @return Decoded integer.
 */
unsigned int getData(unsigned char* pData, unsigned char size) {
    unsigned int value = 0;
    for (unsigned int i = 0; i < size; ++i) {
        value += *pData++ << (i * 8);
    }
    return value;
}

/**
 * @brief Writes an integer in little-endian byte order.
 * @param pData Output byte buffer.
 * @param value Integer to write.
 * @param size Number of bytes to write; additional bytes are zero.
 */
void setData(unsigned char* pData, unsigned int value, unsigned char size) {
    for (unsigned int i = 0; i < size; ++i) {
        *pData++ = value;
        value >>= 8;
    }
}

/**
 * @brief Resolves a stored 32-bit offset within a byte buffer.
 * @param pData Buffer base.
 * @param offset Location of the little-endian offset field.
 * @return Resolved pointer, or nullptr for a zero offset.
 */
unsigned char* getOffset(unsigned char* pData, unsigned int offset) {
    unsigned int relative = getData(pData + offset, 4);
    return relative ? pData + relative : nullptr;
}

/**
 * @brief Gets direction buttons, applies controller orientation, and updates release masks.
 * @param player Player slot index.
 * @return Unmasked direction buttons.
 */
unsigned char inputdevice_controller::getMoveButton(unsigned int player) {
    if (mBlocked) {
        return 0;
    }
    unsigned char buttons = 0;
    if (IsBluetooth(player)) {
        buttons = m_bluetooth.getMoveButton(mPlaylist[player]);
        switch (static_cast<unsigned int>(getType(player))) {
        case 0: case 2: case 3: case 5: case 6: case 9:
            break;
        case 1: case 4: case 7: case 8:
            if (!getRemoveVertical(player)) {
                unsigned char rotated = (buttons >> 3) & 1;
                rotated |= (buttons << 2) & 12;
                rotated |= (buttons >> 1) & 2;
                buttons = rotated;
            }
            break;
        default:
            buttons = 0;
            break;
        }
    }
    if (player < 4) {
        unsigned char mask = mMoveMasks[player];
        mMoveMasks[player] &= buttons;
        buttons &= ~mask;
    }
    return buttons;
}

/** @brief Masks every currently held button until it is released. */
void inputdevice_controller::setMask() {
    for (unsigned int i = 0; i < 4; ++i) {
        mButtonMasks[i] |= getButton(i);
        mMoveMasks[i] |= getMoveButton(i);
    }
    mButtonMasks[4] |= getDRCButton(mDrcPlayer);
    mMoveMasks[4] |= getDRCMoveButton(mDrcPlayer);
}

/** @brief Captures unmasked controller input for the emulator. */
void inputdevice_controller::outEmu() {
    for (unsigned int i = 0; i < 4; ++i) {
        mOutputButtons[i] = getButton(i);
        mOutputMove[i] = getMoveButton(i);
    }
    mOutputButtons[4] = getDRCButton(mDrcPlayer);
    mOutputMove[4] = getDRCMoveButton(mDrcPlayer);
}

/** @brief Polls controller input and updates menu button edges and repeat timers. */
void inputdevice_controller::main() {
    mVcm = 0xff;
    m_bluetooth.main();
    if (m_bluetooth.getTouchOFF(0)) {
        mVcm = mDrcPlayer + 10;
    }
    mAnalog[0][0] = 0;
    mAnalog[0][1] = 0;
    mAnalog[1][0] = 0;
    mAnalog[1][1] = 0;
    PADStatus status[4];
    PADRead(status);
    unsigned int buttons = 0;
    for (unsigned int i = 0; i < 4; ++i) {
        if (status[i].error == 0) {
            unsigned int padButtons = status[i].buttons;
            int leftX = status[i].stickX;
            int leftY = status[i].stickY;
            int rightX = status[i].substickX;
            int rightY = status[i].substickY;
            if (static_cast<unsigned int>(leftX + 9) < 19) { leftX = 0; }
            if (static_cast<unsigned int>(leftY + 9) < 19) { leftY = 0; }
            if (static_cast<unsigned int>(rightX + 9) < 19) { rightX = 0; }
            if (static_cast<unsigned int>(rightY + 9) < 19) { rightY = 0; }
            mAnalog[0][0] += leftX;
            mAnalog[0][1] += leftY;
            mAnalog[1][0] += rightX;
            mAnalog[1][1] += rightY;
            buttons |= padButtons;
        }
    }
    unsigned int drcButtons = m_bluetooth.getButton(0);
    mRepeat[1].update(drcButtons);
    if (mDrcBlocked) {
        if (drcButtons & 0x100) { buttons = 0x800; }
        if (drcButtons & 1) { buttons = 1; }
        if (drcButtons & 2) { buttons = 2; }
        if (drcButtons & 0x20) { buttons = 0x4000; }
        if (drcButtons & 8) { buttons = 0x10; }
        if (drcButtons & 4) { buttons = 8; }
        if ((drcButtons & 0x4000) && (mRepeat[1].pressed & 0x8000)) { buttons = 0x800; }
        unsigned int move = m_bluetooth.getMoveButton(0);
        mRepeat[2].update(move);
        if (move & 1) { buttons = 0x40000; }
        if (move & 2) { buttons = 0x80000; }
        if (move & 4) { buttons = 0x20000; }
        if (move & 8) { buttons = 0x10000; }
        m_bluetooth.getAnalog(0, controller::STICK_LEFT, &mAnalog[0][0], &mAnalog[0][1]);
        m_bluetooth.getAnalog(0, controller::STICK_RIGHT, &mAnalog[1][0], &mAnalog[1][1]);
        mAnalog[0][0] = mAnalog[0][0] * 100.0f;
        mAnalog[0][1] = mAnalog[0][1] * 100.0f;
        mAnalog[1][0] = mAnalog[1][0] * 100.0f;
        mAnalog[1][1] = mAnalog[1][1] * 100.0f;
    }
    mRepeat[0].update(buttons);
}
