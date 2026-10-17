#include "LuigiBros/bluetooth.hpp"
#include "LuigiBros/Emulator/cafe.hpp"
#include "LuigiBros/Emulator/inputdevice.hpp"
#include "Library/Controller/InputFunction.hpp"
#include <cstring>

bluetooth m_bluetooth;

/** @brief Initializes stick converters and the inactive controller manager. */
bluetooth::bluetooth() {
    mInitialized = false;
    init(false);
}

/**
 * @brief Resets controller state and enables default input options.
 * @param initialize Whether to mark the manager initialized.
 */
__attribute__((noinline)) void bluetooth::init(bool initialize) {
    if (!mInitialized && initialize) {
        mInitialized = true;
    }
    mAvailable = false;
    for (unsigned int i = 0; i < 12; ++i) {
        mEntryTypes[i] = controller::TYPE_NONE;
        mControllers[i].type = controller::TYPE_NONE;
        mControllers[i].entryType = controller::TYPE_NONE;
        mControllers[i].buttons = 0;
        mControllers[i].moveButtons = 0;
        mControllers[i].homeButton = 0;
        mControllers[i]._71[0] = 0;
        mControllers[i]._71[1] = 0;
        std::memset(&mControllers[i].analog[0], 0, sizeof(sead::Vector2f));
        std::memset(&mControllers[i].analog[1], 0, sizeof(sead::Vector2f));
        mControllers[i]._58 = 0;
        mControllers[i]._54 = 0;
        mControllers[i].dpd[0] = 0;
        mControllers[i].dpd[1] = 0;
        std::memset(&mControllers[i]._78, 0, sizeof(sead::Vector2f));
        mControllers[i].mTimeout = 0;
        mControllers[i].acceleration[0] = 0;
        mControllers[i].acceleration[1] = 0;
        mControllers[i].acceleration[2] = 0;
        mControllers[i].accelerationValue = 0;
        mControllers[i].accelerationSpeed = 0;
        mControllers[i].removeDpd = false;
        mControllers[i].dpdValid = false;
    }
    for (unsigned int i = 0; i < 14; ++i) {
        _91c[i] = 0;
    }
    _916 = true;
    for (unsigned int i = 0; i < 4; ++i) {
        mDrcButtonEnabled[i] = true;
    }
    mEnableRemove = true;
    _95d[2] = 0;
    _95d[0] = 1;
    _95d[1] = 0;
}

/** @brief Destroys the controller manager. */
bluetooth::~bluetooth() = default;

/**
 * @brief Clears transient input for a controller.
 * @param channel Controller channel index.
 * @param resetType Whether to mark the controller disconnected.
 * @param resetDpd Whether to invalidate pointing-device coordinates.
 */
void bluetooth::controllersReset(unsigned int channel, bool resetType, bool resetDpd) {
    if (resetType) {
        mControllers[channel].type = controller::TYPE_NONE;
    }
    if (resetDpd) {
        mControllers[channel].dpdValid = false;
    }
    mControllers[channel].buttons = 0;
    mControllers[channel].moveButtons = 0;
    mControllers[channel].homeButton = 0;
    mControllers[channel]._71[0] = 0;
    mControllers[channel]._71[1] = 0;
    std::memset(&mControllers[channel].analog[0], 0, sizeof(sead::Vector2f));
    std::memset(&mControllers[channel].analog[1], 0, sizeof(sead::Vector2f));
}

/** @brief Polls four Switch controller ports into the legacy GamePad state. */
void bluetooth::main() {
    mAvailable = true;
    mMicrophone = false;
    for (unsigned int i = 0; i < 4; ++i) {
        int error;
        mReadCount = VPADRead(i, mVpad, 16, &error);
        mControllers[i].entryType = controller::TYPE_NONE;
        if (error == 0) {
            mControllers[i].entryType = controller::TYPE_DRC;
            controllersReset(i, false, true);
            if (_916) {
                if (m_inputdevice_controller.addEntry(i, inputdevice_controller::ENTRY_SLOT, i) != 2) {
                    if (mDrcButtonEnabled[i]) {
                        int port = al::getMainControllerPort() + i;
                        if (!al::isPadConnected(port)) {
                            continue;
                        }
                        if (al::isPadHoldUp(port)) {
                            mControllers[i].moveButtons |= 8;
                        }
                        if (al::isPadHoldDown(port)) {
                            mControllers[i].moveButtons |= 4;
                        }
                        if (al::isPadHoldLeft(port)) {
                            mControllers[i].moveButtons |= 1;
                        }
                        if (al::isPadHoldRight(port)) {
                            mControllers[i].moveButtons |= 2;
                        }
                        if (al::isPadHoldA(port)) {
                            mControllers[i].buttons |= 0x1;
                        }
                        if (al::isPadHoldB(port)) {
                            mControllers[i].buttons |= 0x2;
                        }
                        if (al::isPadHoldX(port)) {
                            mControllers[i].buttons |= 0x4;
                        }
                        if (al::isPadHoldY(port)) {
                            mControllers[i].buttons |= 0x8;
                        }
                        if (al::isPadHoldL(port)) {
                            mControllers[i].buttons |= 0x200;
                        }
                        if (al::isPadHoldR(port)) {
                            mControllers[i].buttons |= 0x100;
                        }
                        if (al::isPadHoldZL(port)) {
                            mControllers[i].buttons |= 0x40;
                        }
                        if (al::isPadHoldZR(port)) {
                            mControllers[i].buttons |= 0x80;
                        }
                        if (al::isPadHoldZL(port)) {
                            mControllers[i].buttons |= 0x40;
                        }
                        if (al::isPadHoldZR(port)) {
                            mControllers[i].buttons |= 0x80;
                        }
                        if (al::isPadHoldLeftStick(port)) {
                            mControllers[i].buttons |= 0x4000;
                        }
                        if (al::isPadHoldRightStick(port)) {
                            mControllers[i].buttons |= 0x8000;
                        }
                        sead::Vector2f left = al::getLeftStick(port);
                        sead::Vector2f right = al::getRightStick(port);
                        mControllers[i].analog[0].x = left.x;
                        mControllers[i].analog[0].y = left.y;
                        mControllers[i].analog[1].y = right.y;
                        mControllers[i].analog[1].x = right.x;
                        unsigned int leftButtons = mControllers[i].sticks[0].PAD_Analog(left.x, left.y);
                        if (_91c[0]) {
                            mControllers[i].moveButtons |= leftButtons;
                        }
                        bool useRight = _91c[1];
                        unsigned int rightButtons = mControllers[i].sticks[1].PAD_Analog(right.x, right.y);
                        if (useRight) {
                            mControllers[i].moveButtons |= rightButtons;
                        }
                        if (al::isPadHoldMinus(port)) {
                            mControllers[i].homeButton = 1;
                        }
                    }
                    mControllers[i].type = controller::TYPE_DRC;
                    mControllers[i].volume = mVpad[0].error;
                    mControllers[i].headphone = reinterpret_cast<unsigned char*>(mVpad)[0x61];
                }
                mControllers[i].mTimeout = 10;
            }
        } else if (error == -2) {
            mDrcButtonEnabled[i] = true;
        }
        if (mControllers[i].mTimeout != 0) {
            --mControllers[i].mTimeout;
            if (mControllers[i].mTimeout == 0) {
                controllersReset(i, true, true);
            }
        }
    }
    _95d[2] = 0;
}

/**
 * @brief Gets the controller type when input is available.
 * @param channel Controller channel index.
 * @return Controller type, or the unavailable sentinel.
 */
controller::TYPE bluetooth::getType(unsigned int channel) {
    if (mAvailable) {
        return mControllers[channel].type;
    }
    return controller::TYPE_UNKNOWN;
}

/**
 * @brief Gets the controller type when input is available.
 * @param channel Controller channel index.
 * @return Controller type, or the unavailable sentinel.
 */
controller::TYPE bluetooth::getEntryType(unsigned int channel) {
    if (mAvailable) {
        return mControllers[channel].entryType;
    }
    return controller::TYPE_UNKNOWN;
}

/**
 * @brief Calls the legacy default-heap release function.
 * @param pMemory Memory to release.
 * @return Always true.
 */
bool bluetooth::free(void* pMemory) {
    MEMFreeToDefaultHeap(pMemory);
    return true;
}

/**
 * @brief Gets the held controller buttons.
 * @param channel Controller channel index.
 * @return Button mask.
 */
unsigned int bluetooth::getButton(unsigned int channel) {
    return mControllers[channel].buttons;
}

/**
 * @brief Gets the direction buttons when input is available.
 * @param channel Controller channel index.
 * @return Direction button mask.
 */
unsigned char bluetooth::getMoveButton(unsigned int channel) {
    return mAvailable ? mControllers[channel].moveButtons : 0;
}

/**
 * @brief Writes the direction button mask.
 * @param channel Controller channel index.
 * @param pButtons Output direction mask.
 */
void bluetooth::getMoveButton(unsigned int channel, unsigned char* pButtons) {
    *pButtons = getMoveButton(channel);
}

/**
 * @brief Platform stub for touch coordinates.
 * @param channel Controller channel index.
 * @param pX Unused output coordinate.
 * @param pY Unused output coordinate.
 * @return Always true.
 */
bool bluetooth::getTouchPanel(unsigned int channel, unsigned short* pX, unsigned short* pY) {
    return true;
}

/**
 * @brief Platform stub for touch transitions.
 * @param channel Controller channel index.
 * @return Always false.
 */
bool bluetooth::getTouchON(unsigned int channel) {
    return false;
}

/**
 * @brief Platform stub for touch transitions.
 * @param channel Controller channel index.
 * @return Always false.
 */
bool bluetooth::getTouchOFF(unsigned int channel) {
    return false;
}

/**
 * @brief Gets a valid pointing-device position.
 * @param channel Controller channel index.
 * @param pX Output horizontal pointer coordinate.
 * @param pY Output vertical pointer coordinate.
 * @return Whether coordinates were written.
 */
bool bluetooth::getDPDPos(unsigned int channel, float* pX, float* pY) {
    if (mControllers[channel].dpdValid) {
        *pX = mControllers[channel].dpd[0] * 320.0f;
        *pY = mControllers[channel].dpd[1] * 320.0f;
        return true;
    }
    return false;
}

/**
 * @brief Changes button enablement for a GamePad channel.
 * @param channel Controller channel index.
 * @param enabled Whether to enable buttons.
 * @return Whether the channel is a GamePad.
 */
bool bluetooth::setDRCEnableButton(unsigned int channel, bool enabled) {
    if (getType(channel) == controller::TYPE_DRC) {
        mDrcButtonEnabled[channel] = enabled;
        return true;
    }
    return false;
}

/**
 * @brief Changes vertical suppression for supported channels.
 * @param channel Controller channel index.
 * @param remove Whether to suppress vertical movement.
 */
void bluetooth::setRemoveVertical(unsigned int channel, bool remove) {
    if (channel >= 4 && channel <= 8) {
        mControllers[channel].removeVertical = remove;
    }
}

/**
 * @brief Checks vertical suppression for supported channels.
 * @param channel Controller channel index.
 * @return Whether vertical movement is suppressed.
 */
bool bluetooth::getRemoveVertical(unsigned int channel) {
    if (channel < 4 || channel > 8) {
        return false;
    }
    return mControllers[channel].removeVertical != 0;
}

/**
 * @brief Changes pointing-device suppression for one player.
 * @param channel Controller channel index.
 * @param remove Whether to suppress pointing-device input.
 */
void bluetooth::setRemoveDPD(unsigned int channel, bool remove) {
    mControllers[channel + 4].removeDpd = remove;
}

/**
 * @brief Changes pointing-device suppression for all four players.
 * @param remove Whether to suppress pointing-device input.
 */
void bluetooth::setRemoveDPD(bool remove) {
    for (unsigned int i = 0; i < 4; ++i) {
        mControllers[i + 4].removeDpd = remove;
    }
}

/**
 * @brief Checks pointing-device suppression for one player.
 * @param channel Controller channel index.
 * @return Suppression flag.
 */
bool bluetooth::getRemoveDPD(unsigned int channel) {
    return mControllers[channel + 4].removeDpd;
}

/**
 * @brief Checks pointing-device suppression across all four players.
 * @return Whether any player suppresses pointing-device input.
 */
bool bluetooth::getRemoveDPD() {
    unsigned int remove = mControllers[4].removeDpd;
    remove |= mControllers[5].removeDpd;
    remove |= mControllers[6].removeDpd;
    remove |= mControllers[7].removeDpd;
    return remove != 0;
}

/**
 * @brief Gets the selected analog stick coordinates.
 * @param channel Controller channel index.
 * @param stick Stick to read.
 * @param pX Output horizontal position.
 * @param pY Output vertical position.
 */
void bluetooth::getAnalog(unsigned int channel, controller::ANALOGSTICK stick, int* pX, int* pY) {
    *pX = static_cast<int>(mControllers[channel].analog[stick].x);
    *pY = static_cast<int>(mControllers[channel].analog[stick].y);
}

/**
 * @brief Gets the selected analog stick coordinates.
 * @param channel Controller channel index.
 * @param stick Stick to read.
 * @param pX Output horizontal position.
 * @param pY Output vertical position.
 */
void bluetooth::getAnalog(unsigned int channel, controller::ANALOGSTICK stick, float* pX, float* pY) {
    *pX = static_cast<float>(mControllers[channel].analog[stick].x);
    *pY = static_cast<float>(mControllers[channel].analog[stick].y);
}

/**
 * @brief Checks the GamePad headphone flag.
 * @param channel Controller channel index.
 * @return Whether headphones are connected.
 */
bool bluetooth::IsHeadphone(unsigned int channel) {
    if (mControllers[channel].type == controller::TYPE_DRC) {
        return mControllers[channel].headphone != 0;
    }
    return false;
}

/**
 * @brief Gets the GamePad volume value.
 * @param channel Controller channel index.
 * @return Volume value, or zero for other controller types.
 */
unsigned char bluetooth::getVolume(unsigned int channel) {
    if (mControllers[channel].type == controller::TYPE_DRC) {
        return mControllers[channel].volume;
    }
    return 0;
}

/**
 * @brief Checks the home button for supported controller types.
 * @param channel Controller channel index.
 * @return Whether the home button is held.
 */
bool bluetooth::IsHomeButton(unsigned int channel) {
    switch (static_cast<unsigned int>(getType(channel))) {
    case 0: case 2: case 3: case 5: case 6: case 7: case 8: case 9:
        break;
    default:
        if (!mEnableRemove) {
            return false;
        }
        break;
    }
    return mControllers[channel].homeButton != 0;
}

/**
 * @brief Gets acceleration for controller types that support it.
 * @param channel Controller channel index.
 * @param pX Output X acceleration.
 * @param pY Output Y acceleration.
 * @param pZ Output Z acceleration.
 * @return Whether acceleration was written.
 */
bool bluetooth::getAcc(unsigned int channel, float* pX, float* pY, float* pZ) {
    switch (static_cast<unsigned int>(getType(channel))) {
    case 1: case 4: case 7: case 8:
        *pX = mControllers[channel].acceleration[0];
        *pY = mControllers[channel].acceleration[1];
        *pZ = mControllers[channel].acceleration[2];
        return true;
    default: return false;
    }
}

/**
 * @brief Gets a derived acceleration value for supported controllers.
 * @param channel Controller channel index.
 * @return Acceleration value, or zero if unsupported.
 */
float bluetooth::getAccValue(unsigned int channel) {
    switch (static_cast<unsigned int>(getType(channel))) {
    case 1: case 4: case 7: case 8:
        return mControllers[channel].accelerationValue;
    default: return 0.0f;
    }
}

/**
 * @brief Gets a derived acceleration value for supported controllers.
 * @param channel Controller channel index.
 * @return Acceleration value, or zero if unsupported.
 */
float bluetooth::getAccSpeed(unsigned int channel) {
    switch (static_cast<unsigned int>(getType(channel))) {
    case 1: case 4: case 7: case 8:
        return mControllers[channel].accelerationSpeed;
    default: return 0.0f;
    }
}

/**
 * @brief Platform stub for the GamePad sensor bar.
 * @param enabled Unused sensor-bar setting.
 */
void bluetooth::setDRCSensorBar(bool enabled) {
}

/**
 * @brief Gets horizon orientation for supported controller types.
 * @param channel Controller channel index.
 * @param pX Output horizontal component.
 * @param pY Output vertical component.
 * @return Whether orientation was available.
 */
bool bluetooth::getHorizon(unsigned int channel, float* pX, float* pY) {
    *pX = 0.0f;
    *pY = 0.0f;
    switch (static_cast<unsigned int>(getType(channel))) {
    case 1: case 4: case 7: case 8:
        *pX = mControllers[channel].horizon[0];
        *pY = mControllers[channel].horizon[1];
        return true;
    default: return false;
    }
}
