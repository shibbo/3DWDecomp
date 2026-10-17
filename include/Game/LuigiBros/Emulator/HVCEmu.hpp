#pragma once
#include "LuigiBros/Emulator/Emu.hpp"

class HVCEmu : public Emu {
public:
    unsigned int Init(unsigned char* pRom, EmuController* pController) override;
    unsigned int Main() override;
    void Reset() override;
    void MakeScreenTextue(unsigned int* pPixels) override;
    unsigned int GetSaveData(SaveData type, const void* pData, unsigned int size,
        bool flag, int* pResult) override;
    unsigned int SetSaveData(SaveData type, void* pData, unsigned int size, bool flag) override;
    void GetScreenSize(unsigned int* pWidth, unsigned int* pHeight) override;
    void Pause(bool paused) override;
    bool IsNeedSave() const override;
    void checkVCM() override;
    void Debug(unsigned int flags) override;
    void Finalize() override;
    unsigned char _08[0x72];
    unsigned short mState7a;
    unsigned char _7c[0x1c];
};
extern HVCEmu m_HVCEmu;
static_assert(sizeof(HVCEmu) == 0x98, "HVC emulator layout");
