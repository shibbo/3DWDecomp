#pragma once
#include <nn/types.h>

namespace nn::atk::detail {
struct OutputBusMixVolume {
    float volumes[2][24];
};

class BusMixVolumePacket {
public:
    BusMixVolumePacket() : mUsed(false), mEnabled(nullptr) {}
    static size_t GetRequiredMemSize(int busCount);
    bool Initialize(void* memory, size_t size, int busCount);
    void Finalize();
    void Reset();
    // used controls whether the packet's mixing values are active.
    void SetUsed(bool used) { mUsed = used; }
    // bus selects a flag and enabled supplies the new per-bus state.
    void SetEnabledForBus(int bus, bool enabled) { mEnabled[bus] = enabled; }

    /** @brief Checks whether the packet's mixing values are active. @return True when used. */
    bool IsUsed() const { return mUsed; }
    /** @brief Gets the number of buses with an enable flag. @return Bus count. */
    int GetBusCount() const { return mBusCount; }
    /** @brief Checks the enable flag of a bus. @param bus Bus index. @return Flag state. */
    bool IsEnabledForBus(int bus) const { return mEnabled[bus]; }
    /** @brief Gets the per-channel bus volumes. @return Volume table. */
    OutputBusMixVolume& GetVolume() { return mVolume; }

private:
    friend class OutputAdditionalParam;
    OutputBusMixVolume mVolume;
    bool mUsed;
    bool* mEnabled;
    int mBusCount;
};
static_assert(sizeof(BusMixVolumePacket) == 0xd8, "BusMixVolumePacket size");
}
