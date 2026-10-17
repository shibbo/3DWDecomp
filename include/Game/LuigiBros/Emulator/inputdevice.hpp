#pragma once
#include "LuigiBros/controller.hpp"

struct InputRepeatState {
    unsigned int pressed;
    unsigned int repeat;
    unsigned int released;
    unsigned int counter = 0;
    unsigned int previous = 0;

    /** @brief Updates edge and repeat masks. @param buttons Current held buttons. */
    void update(unsigned int buttons) {
        pressed = buttons & ~previous;
        released = previous & ~buttons;
        repeat = 0;
        if (pressed != 0) {
            counter = 30;
            repeat = buttons;
        }
        if (buttons != 0) {
            if (counter != 0 && --counter == 0) {
                counter = 4;
                repeat = buttons;
            }
        } else {
            counter = 0;
        }
        previous = buttons;
    }
};

class inputdevice_controller {
public:
    enum ADDENTRY_MODE : unsigned int { ENTRY_FIRST_FREE, ENTRY_SLOT };
    inputdevice_controller();
    ~inputdevice_controller();
    void init();
    void resetEntry();
    void main();
    void setMask();
    void outEmu();
    unsigned char getMoveButton(unsigned int player);
    controller::TYPE getType(unsigned int player);
    bool IsBluetooth(unsigned int player);
    bool IsUSB(unsigned int player);
    unsigned int getButton(unsigned int player);
    unsigned int getDRCButton(unsigned int player);
    unsigned char getDRCMoveButton(unsigned int player);
    void inEmu();
    bool getRemoveVertical(unsigned int player);
    bool IsDRC(unsigned int player);
    bool getButtonOn(unsigned int player, controller::BUTTON button);
    int addEntry(unsigned int channel, ADDENTRY_MODE mode, unsigned int player);
    void deleteEntry(unsigned int channel);
    bool getDPDPos(unsigned int player, float* pX, float* pY);
    bool getAcc(unsigned int player, float* pX, float* pY, float* pZ);
    float getAccValue(unsigned int player);
    float getAccSpeed(unsigned int player);
    bool getHorizon(unsigned int player, float* pX, float* pY);
    bool getTouchPanel(unsigned int player, unsigned short* pX, unsigned short* pY);
    bool getTouchPanel(unsigned short* pX, unsigned short* pY);
    bool getTouchON(unsigned int player);
    bool getTouchON();
    bool getTouchOFF(unsigned int player);
    bool getTouchOFF();
    bool IsHeadphone(unsigned int player);
    unsigned char getVolume(unsigned int player);
    void NonRegistrationDisconnect();
    void backupPlaylist();
    void recoveryPlaylist();
    bool setRemoveVertical(unsigned int player, bool remove);
    void EnableRemove(bool enabled);
    void getAnalog(unsigned int player, controller::ANALOGSTICK stick, int* pX, int* pY);
    bool IsMicrophone();
    bool IsVCM();
    bool IsEnableRemove();

private:
    friend class configSaveData;

    unsigned int mPlaylist[5];
    unsigned int mBackupPlaylist[5];
    unsigned int mPlayerCount;
    unsigned int mVcm;
    unsigned int mDrcPlayer;
    bool mBlocked;
    bool mRemove;
    unsigned int mButtonMasks[5];
    unsigned char mMoveMasks[5];
    unsigned int mOutputButtons[5];
    unsigned char mOutputMove[5];
    bool mDrcBlocked;
    InputRepeatState mRepeat[3];
    float mAnalog[2][2];
};
static_assert(sizeof(inputdevice_controller) == 0xbc, "Input controller layout");
extern inputdevice_controller m_inputdevice_controller;

unsigned int getData(unsigned char* pData, unsigned char size);
void setData(unsigned char* pData, unsigned int value, unsigned char size);
unsigned char* getOffset(unsigned char* pData, unsigned int offset);
