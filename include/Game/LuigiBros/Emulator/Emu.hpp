#pragma once

class EmuController;

class Emu {
public:
    enum SaveData : unsigned int {};
    virtual ~Emu();
    virtual unsigned int Init(unsigned char* pRom, EmuController* pController) = 0;
    virtual unsigned int Main() = 0;
    virtual void Reset() = 0;
    virtual void MakeScreenTextue(unsigned int* pPixels) = 0;
    virtual unsigned int GetSaveData(SaveData type, const void* pData, unsigned int size,
        bool flag, int* pResult) = 0;
    virtual unsigned int SetSaveData(SaveData type, void* pData, unsigned int size, bool flag) = 0;
    virtual void SoundCallback(unsigned char* pSamples);
    virtual void GetScreenSize(unsigned int* pWidth, unsigned int* pHeight) = 0;
    virtual void Pause(bool paused) = 0;
    virtual bool IsNeedSave() const = 0;
    virtual void checkVCM() = 0;
    virtual bool IsAccessSRAM();
    virtual void SetAlwaysFilter();
    virtual void Debug(unsigned int flags) = 0;
    virtual void Finalize() = 0;
};
