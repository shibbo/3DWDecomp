#pragma once
#include <nn/types.h>

namespace nn::atk::detail {
class VolumeThroughModePacket {
public:
    VolumeThroughModePacket() : mModes(nullptr), mBusCount(0), mUsed(false), mVolume(1.0f) {}
    static size_t GetRequiredMemSize(int busCount);
    bool Initialize(void* memory, size_t size, int busCount);
    void Finalize();
    void Reset();
    VolumeThroughModePacket& operator=(const VolumeThroughModePacket& other);

    /** @brief Checks whether the through modes are active. @return True when used. */
    bool IsUsed() const { return mUsed; }
    /** @brief Activates or deactivates the through modes. @param used New state. */
    void SetUsed(bool used) { mUsed = used; }
    /** @brief Gets the volume of buses in binary mode. @return Linear gain. */
    float GetBinaryVolume() const { return mVolume; }
    /** @brief Sets the volume of buses in binary mode. @param volume Linear gain. */
    void SetBinaryVolume(float volume) { mVolume = volume; }
    /** @brief Gets the number of buses with a through mode. @return Bus count. */
    int GetBusCount() const { return mBusCount; }
    /**
     * @brief Reads the through mode of a bus without a range check.
     * @param bus Bus index.
     * @return The bus's mode.
     */
    u8 GetVolumeThroughMode(int bus) const { return mModes[bus]; }
    /**
     * @brief Writes the through mode of a bus without a range check.
     * @param bus Bus index.
     * @param mode New mode.
     */
    void SetVolumeThroughMode(int bus, u8 mode) { mModes[bus] = mode; }
    /**
     * @brief Reads the through mode of a bus if it exists.
     * @param bus Bus index.
     * @return The bus's mode, or 0 if bus is out of range.
     */
    u8 TryGetVolumeThroughMode(int bus) const { return bus < mBusCount ? mModes[bus] : 0; }
    /**
     * @brief Writes the through mode of a bus if it exists.
     * @param bus Bus index; out-of-range writes are ignored.
     * @param mode New mode.
     */
    void TrySetVolumeThroughMode(int bus, u8 mode) {
        if (bus < mBusCount) {
            mModes[bus] = mode;
        }
    }

private:
    friend class OutputAdditionalParam;
    u8* mModes;
    int mBusCount;
    u8 _0c[4];
    bool mUsed;
    float mVolume;
};
static_assert(sizeof(VolumeThroughModePacket) == 0x18, "VolumeThroughModePacket size");
}
