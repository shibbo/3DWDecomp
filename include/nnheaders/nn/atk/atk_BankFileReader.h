#pragma once

#include <nn/types.h>
#include <nn/atk/atk_BankFile.h>

namespace nn::atk::detail {
struct VelocityRegionInfo {
    /** @brief Initializes the ADSHR parameters before reading a bank region. */
    VelocityRegionInfo() : attack(0), decay(0), sustain(0), hold(0), release(0) {}
    WaveId waveId;
    float pitch;
    u8 attack, decay, sustain, hold, release;
    u8 originalKey, volume, pan;
    u8 ignoreNoteOff, keyGroup, interpolationType;
};
static_assert(sizeof(VelocityRegionInfo) == 0x18, "VelocityRegionInfo size");

class BankFileReader {
  public:
    BankFileReader();
    explicit BankFileReader(const void* pBankFile);
    void Initialize(const void* pBankFile);
    void Finalize();

    DISABLE_TAIL_CALLS const WaveIdTable* GetWaveIdTable() const;
    bool ReadVelocityRegionInfo(VelocityRegionInfo* pInfo, int program, int key, int velocity) const;

    /** @brief Tests whether a bank file is attached. @return Whether Initialize() accepted a file. */
    bool IsInitialized() const { return mInitialized; }
    /** @brief Gets the attached bank file. @return Start of the bank file, or nullptr. */
    const void* GetBankFileAddress() const { return mHeader; }

  private:
    const BankFile::FileHeader* mHeader;
    const BankFile::InfoBlockBody* mInfo;
    bool mInitialized;
};
} // namespace nn::atk::detail
