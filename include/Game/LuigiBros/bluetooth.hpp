#pragma once

#include "LuigiBros/controller.hpp"
#include "LuigiBros/PadAnalogStick.hpp"
#include "LuigiBros/Emulator/cafe.hpp"
#include <basis/seadTypes.h>
#include <math/seadVector.h>

struct BluetoothControllerState {
    controller::TYPE type;
    controller::TYPE entryType;
    u32 buttons;
    u32 _c;
    u8 moveButtons;
    u8 _11;
    u8 homeButton;
    u8 _13;
    float acceleration[3];
    u32 _20;
    float accelerationValue;
    float accelerationSpeed;
    sead::Vector2f analog[2];
    PadAnalogStick sticks[2];
    u32 _54;
    u8 _58;
    u8 volume;
    u8 headphone;
    bool removeDpd;
    bool dpdValid;
    u8 _5d[3];
    float dpd[2];
    float horizon[2];
    u8 removeVertical;
    u8 _71[2];
    u8 _73[5];
    sead::Vector2f _78;
    u8 mTimeout;
    u8 _81[7];
};
static_assert(sizeof(BluetoothControllerState) == 0x88, "Controller state layout");

class bluetooth {
public:
    bluetooth();
    ~bluetooth();
    void init(bool initialize);
    void main();
    controller::TYPE getType(unsigned int channel);
    controller::TYPE getEntryType(unsigned int channel);
    static bool free(void* pMemory);
    void controllersReset(unsigned int channel, bool resetType, bool resetDpd);
    unsigned int getButton(unsigned int channel);
    unsigned char getMoveButton(unsigned int channel);
    void getMoveButton(unsigned int channel, unsigned char* pButtons);
    bool getTouchPanel(unsigned int channel, unsigned short* pX, unsigned short* pY);
    bool getTouchON(unsigned int channel);
    bool getTouchOFF(unsigned int channel);
    bool getDPDPos(unsigned int channel, float* pX, float* pY);
    bool setDRCEnableButton(unsigned int channel, bool enabled);
    void setRemoveVertical(unsigned int channel, bool remove);
    bool getRemoveVertical(unsigned int channel);
    void setRemoveDPD(unsigned int channel, bool remove);
    void setRemoveDPD(bool remove);
    bool getRemoveDPD(unsigned int channel);
    bool getRemoveDPD();
    void getAnalog(unsigned int channel, controller::ANALOGSTICK stick, int* pX, int* pY);
    void getAnalog(unsigned int channel, controller::ANALOGSTICK stick, float* pX, float* pY);
    bool IsHeadphone(unsigned int channel);
    unsigned char getVolume(unsigned int channel);
    bool IsHomeButton(unsigned int channel);
    bool getAcc(unsigned int channel, float* pX, float* pY, float* pZ);
    float getAccValue(unsigned int channel);
    float getAccSpeed(unsigned int channel);
    void setDRCSensorBar(bool enabled);
    bool getHorizon(unsigned int channel, float* pX, float* pY);

    VPADStatus mVpad[18];
    int mReadCount;
    u8 _904[0x10];
    bool mInitialized;
    bool mAvailable;
    bool _916;
    bool mDrcButtonEnabled[4];
    bool mMicrophone;
    u8 _91c[0x10];
    controller::TYPE mEntryTypes[12];
    bool mEnableRemove;
    u8 _95d[3];
    BluetoothControllerState mControllers[12];
    u8 _fc0[0xc0];
};
static_assert(sizeof(bluetooth) == 0x1080, "Bluetooth controller manager layout");
extern bluetooth m_bluetooth;
