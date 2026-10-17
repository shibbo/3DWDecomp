#pragma once

#include <nn/types.h>

namespace nn::hid {
struct VibrationModulation {
    f32 amplitudeLow;
    f32 frequencyLow;
    f32 amplitudeHigh;
    f32 frequencyHigh;
};

// SDK object sizes are recovered from allocations; internal fields remain opaque.
class VibrationNode {
public:
    static void Update();
    void SetModulationTo(const VibrationNode* pDestination, const VibrationModulation& rModulation);
};
struct VibrationFileInfo {
    u8 _0[0xc];
    s32 sampleLength;
    u8 _10[0x10];
};
class VibrationPlayer : public VibrationNode {
public:
    VibrationPlayer();
    VibrationFileInfo GetFileInfo() const;
    void SetCurrentPosition(s32 position);
    void Load(const void* pData, size_t size);
    void Play();
    void Stop();
    void SetLoop(bool loop);
    bool IsPlaying() const;
    bool IsLoop() const;
private:
    alignas(8) u8 mStorage[0xd0];
};
class VibrationNodeConnection {
public:
    VibrationNodeConnection();
    void Connect(VibrationNode* pSource, VibrationNode* pDestination);
    void Disconnect();
    VibrationNodeConnection(VibrationNode* pSource, VibrationNode* pDestination);
    const VibrationNode* GetDestination() const;
    VibrationModulation GetModulation() const;
private:
    alignas(8) u8 mStorage[0x38];
};
enum VibrationMixMode { VibrationMixMode_Max = 1 };
class VibrationMixer : public VibrationNode {
public:
    VibrationMixer();
    void SetMixMode(const VibrationMixMode& rMode);
private:
    alignas(8) u8 mStorage[0x48];
};
struct VibrationDeviceHandle;
class VibrationTarget : public VibrationNode {
public:
    VibrationTarget();
    void SetVibrationDeviceHandle(const VibrationDeviceHandle& rHandle);
    bool IsVibrationDeviceHandleSet() const;
    void UnsetVibrationDeviceHandle();
private:
    alignas(8) u8 mStorage[0x50];
};
struct VibrationValue;
class VibrationWriter : public VibrationNode {
public:
    VibrationWriter();
    void Write(const VibrationValue& rValue);
private:
    alignas(8) u8 mStorage[0x40];
};
static_assert(sizeof(VibrationPlayer) == 0xd0);
static_assert(sizeof(VibrationNodeConnection) == 0x38);
static_assert(sizeof(VibrationMixer) == 0x48);
static_assert(sizeof(VibrationTarget) == 0x50);
static_assert(sizeof(VibrationWriter) == 0x40);
}  // namespace nn::hid
