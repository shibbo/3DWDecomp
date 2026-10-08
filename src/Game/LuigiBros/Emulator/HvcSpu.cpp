#include "LuigiBros/Emulator/HvcSpu.hpp"
#include "LuigiBros/Emulator/Hvc.hpp"
#include "LuigiBros/Utility/Serialize.hpp"
#include <cstring>

/**
 * @brief The Luigi Bros. emulator front end.
 */
struct HVCEmu {
    u8 _00[0x58];
    u8* mDiskImage;
    u8 _60[0x1a];
    u16 mTitleType;
    u8 _7c[0x1c];
};

extern HVCEmu m_HVCEmu;

namespace Vessel::Emulator::Virtual::PlatformHvc {

namespace {

/// Offset of the disk image inside the machine memory.
constexpr u64 cMemory_DiskImage = 0x100000;

/// Offset of the expansion sound registers ($5000-) read by the CPU.
constexpr u64 cMemory_ExRegister = 0x303c10;

/// Offset of the last values written to the expansion sound registers ($5000-).
constexpr u64 cMemory_ExShadow = 0x304010;

/// Size of the tag in front of a serialized state.
constexpr u64 cSerializeTagSize = 0x10;

/// Size reserved for the disk drive in a serialized state.
constexpr u64 cSerializeDiskSize = 0x40;

/// Mapper number of the Famicom Disk System.
constexpr u8 cMapper_Disk = 0xff;

/// Mapper number of MMC5.
constexpr u8 cMapper_Mmc5 = 5;

/// Mapper number of VRC6.
constexpr u8 cMapper_Vrc6 = 24;

/// Square duty cycles, one bit per step.
const u16 cDutyTable[4] = {0x000c, 0x003c, 0x03fc, 0xffc3};

/// Length counter loads, halved.
const u8 cLengthTable[32] = {5,  127, 10, 1,  20, 2,  40, 3,  80, 4,  30, 5,  7,  6,  13, 7,
                             6,  8,   12, 9,  24, 10, 48, 11, 96, 12, 36, 13, 8,  14, 16, 15};

/// Largest square period kept by an upward sweep, for each sweep shift.
const s16 cSweepPeriodMax[8] = {0x3ff, 0x555, 0x666, 0x71c, 0x787, 0x7c1, 0x7e0, 0x7f0};

/// Triangle wave steps.
const s8 cTriangleTable[32] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                               15, 14, 13, 12, 11, 10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0};

/// Noise periods.
const u16 cNoisePeriodTable[16] = {4,   8,   16,  32,  64,  96,   128,  160,
                                   202, 254, 380, 508, 762, 1016, 2034, 4068};

/// Delta modulation periods.
const u16 cDeltaPeriodTable[16] = {428, 380, 340, 320, 286, 254, 226, 214,
                                   190, 160, 142, 128, 106, 84,  72,  54};

/// Modulator steps of the disk system sound (0x7f resets the counter).
const s8 cModStepTable[8] = {0, 1, 2, 4, 0x7f, -4, -2, -1};

/// Wave phase advance per sample, rate and frequency unit.
constexpr f64 cWavePhaseScale = 0x1.006d1204b83b1p-26;

/// Modulator phase advance per sample, rate and frequency unit.
constexpr f64 cModPhaseScale = 0x1.006d1204b83b1p-16;

/**
 * @brief Check whether the running title is a disk system title.
 * @return Whether the disk drive is emulated.
 */
bool isDiskSystem() {
    return (m_HVCEmu.mTitleType & 0xfff0) == 0xf0f0;
}

/**
 * @brief Check whether the custom parameters select the alternative behavior of a channel.
 * @param pRender The custom parameters.
 * @param index The channel index.
 * @return Whether the option is set.
 */
u8 getChannelOption(const CPlatformHvcUnit::_SCustomParameter* pRender, u8 index) {
    return reinterpret_cast<const u8*>(&pRender->mPanMaster)[index];
}

/**
 * @brief Move an output level towards its target, by the smoothing step of its channel.
 * @param rOutput The output level.
 * @param target The target level.
 * @param pEntity The machine state.
 * @param index The channel index.
 */
inline void approachOutput(s16& rOutput, s16 target, const CPlatformHvcUnit::_SEntity* pEntity,
                           u8 index) {
    s32 step = 1 << pEntity->mTitleRender->mPan[index];
    s16 output = rOutput;
    if (output + step < target) {
        rOutput = output + step;
    } else if (target + step < output) {
        rOutput = output - step;
    } else {
        rOutput = target;
    }
}

/**
 * @brief Compute the clock period of a disk system envelope.
 * @param speed The envelope speed.
 * @param master The master envelope speed.
 * @return The period.
 */
inline u64 getEnvelopePeriod(u8 speed, u8 master) {
    return static_cast<u64>(speed + 1) * master * 0x7453fc / 3712;
}

/**
 * @brief Get the program counter of the CPU of a machine.
 * @param pHvc The machine.
 * @return The program counter.
 */
inline u16 getProgramCounter(const CHvc* pHvc) {
    return *reinterpret_cast<const u16*>(&pHvc->mCpu._48[0x1006]);
}

/**
 * @brief Get the buttons held on the controllers of a machine.
 * @param pHvc The machine.
 * @return The buttons.
 */
inline u8 getGioButton(const CHvc* pHvc) {
    return pHvc->mGio._48[0x10];
}

/**
 * @brief Get the byte written by a bus access.
 * @param pArgument The transfer.
 * @return The byte.
 */
inline u8 getAccessData(const CPlatformUnit::UArgument* pArgument) {
    return *static_cast<u8*>(pArgument->mAccess.mData);
}

/**
 * @brief Clear a register and its shadow.
 * @param pRegister The registers.
 * @param pShadow The shadow registers.
 * @param index The register index.
 */
inline void clearRegister(u8* pRegister, u8* pShadow, s32 index) {
    pShadow[index] = 0;
    pRegister[index] = 0;
}

/**
 * @brief Pad the data serialized since a position to a multiple of 16 bytes.
 * @param rSerializer The serializer.
 * @param start The offset where the padded block starts.
 */
inline void alignSerializer(CHvcSpu::Serializer& rSerializer, u64 start) {
    u32 size = rSerializer.GetOffset() - start;
    rSerializer.SetOffset(start + ((size + 15) & ~0xfu));
}

}  // namespace

/**
 * @brief Clock the length counter.
 */
inline void CHvcSpu::_SWave::clockLength() {
    if (mIsLengthEnabled && mLength > 0) {
        mLength--;
    }
}

/**
 * @brief Run the timer for one sample.
 * @param rate The number of clocks in the sample.
 * @return The number of timer periods that elapsed.
 */
inline s64 CHvcSpu::_SWave::stepTimer(s16 rate) {
    s64 count = 0;
    if (mIsTimerActive && mPeriod >= 4) {
        if (mIsTimerReload) {
            mPhase = mPeriod;
            mIsTimerReload = false;
        } else {
            while (mPhase <= rate) {
                mPhase += mPeriod;
                count++;
            }

            mPhase -= rate;
        }
    }

    return count;
}

/**
 * @brief Move the output towards the target level.
 * @param pEntity The machine state.
 */
inline void CHvcSpu::_SWave::approachTarget(const _SEntity* pEntity) {
    approachOutput(mOutput, mTarget, pEntity, mIndex);
}

/**
 * @brief Save or load the state shared by every channel.
 * @param rSerializer The serializer.
 */
ALWAYS_INLINE inline void CHvcSpu::_SWave::serializeWave(Serializer& rSerializer) {
    rSerializer.Entry(mKind);
    rSerializer.Entry(mIndex);
    rSerializer.Entry(mIsEnabled);
    rSerializer.Entry(mStatus);
    rSerializer.Entry(mOutput);
    rSerializer.Entry(mTarget);
    rSerializer.Entry(mLevel);
    rSerializer.Entry(mIsTimerActive);
    rSerializer.Entry(mIsTimerReload);
    rSerializer.Entry(_0c);
    rSerializer.Entry(mPhase);
    rSerializer.Entry(mPeriod);
    rSerializer.Entry(mIsLengthEnabled);
    rSerializer.Entry(mIsLengthLoaded);
    rSerializer.Entry(mLength);
    rSerializer.Entry(mLengthReload);
}

/**
 * @brief Clock the volume envelope.
 */
inline void CHvcSpu::_SWaveEnvelope::clockEnvelope() {
    if (mIsEnvelopeStart) {
        mEnvelopeCounter = mEnvelopePeriod;
        mIsEnvelopeStart = false;
        mEnvelopeVolume = 15;
    } else {
        if (mEnvelopeCounter > 0) {
            mEnvelopeCounter--;
        }

        if (mEnvelopeCounter == 0) {
            mEnvelopeCounter = mEnvelopePeriod;
            if (mEnvelopeVolume > 0) {
                mEnvelopeVolume--;
            } else if (mEnvelopeVolume == 0 && mIsEnvelopeLoop) {
                mEnvelopeVolume = 15;
            }
        }
    }
}

/**
 * @brief Write the envelope register.
 * @param data The value written.
 */
inline void CHvcSpu::_SWaveEnvelope::setEnvelope(u8 data) {
    mIsLengthEnabled = !(data & 0x20);
    mIsEnvelopeEnabled = !(data & 0x10);
    mEnvelopePeriod = data & 0xf;
    mIsEnvelopeLoop = (data >> 5) & 1;
}

/**
 * @brief Get the current volume, from the envelope or the constant volume.
 * @return The volume, scaled by 256.
 */
inline s64 CHvcSpu::_SWaveEnvelope::getEnvelopeVolume() const {
    return (mIsEnvelopeEnabled ? mEnvelopeVolume : mEnvelopePeriod) << 8;
}

/**
 * @brief Save or load the envelope state.
 * @param rSerializer The serializer.
 */
ALWAYS_INLINE inline void CHvcSpu::_SWaveEnvelope::serializeEnvelope(Serializer& rSerializer) {
    rSerializer.Entry(mIsEnvelopeEnabled);
    rSerializer.Entry(mIsEnvelopeStart);
    rSerializer.Entry(mEnvelopeVolume);
    rSerializer.Entry(mIsEnvelopeLoop);
    rSerializer.Entry(mEnvelopeCounter);
    rSerializer.Entry(mEnvelopePeriod);
}

/**
 * @brief Reset the square channel.
 * @param kind The channel kind.
 * @param index The channel index.
 */
void CHvcSpu::_SWavwR::Reset(u8 kind, u8 index) {
    mKind = kind;
    mIndex = index;
    mIsEnabled = false;
    mStatus = 0;
    mOutput = 0;
    mTarget = 0;
    mLevel = 0;
    mIsTimerActive = false;
    mIsTimerReload = false;
    _0c = 0;
    mPhase = 0;
    mPeriod = 0;
    mIsLengthEnabled = true;
    mIsLengthLoaded = false;
    mLength = 1;
    mLengthReload = 0;
    mIsEnvelopeEnabled = false;
    mIsEnvelopeStart = false;
    mEnvelopeVolume = 0;
    mIsEnvelopeLoop = false;
    mEnvelopeCounter = 0;
    mEnvelopePeriod = 0;
    mIsSweepEnabled = false;
    mIsSweepReload = false;
    mSweepShift = 0;
    mSweepDirection = -1;
    mSweepCounter = 0;
    mSweepPeriod = 0;
    mDuty = cDutyTable[2];
    mDutyPosition = 0;
}

/**
 * @brief Write a register of the square channel.
 * @param reg The register ($4000-$4003 as 0-3, or cRegister_Enable).
 * @param data The value written.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SWavwR::Setup(u8 reg, u8 data, _SEntity* pEntity) {
    switch (reg) {
    case 0:
        mIsLengthEnabled = !(data & 0x20);
        mIsEnvelopeEnabled = !(data & 0x10);
        mDuty = cDutyTable[data >> 6];
        mEnvelopePeriod = data & 0xf;
        mIsEnvelopeLoop = (data >> 5) & 1;
        break;
    case 1:
        mIsSweepEnabled = data >> 7;
        mSweepShift = data & 7;
        mSweepPeriod = ((data >> 4) & 7) + 1;
        mSweepDirection = (data & 8) ? 1 : -1;
        mIsSweepReload = true;
        break;
    case 2:
        mPeriod = (mPeriod & 0x700) | data;
        break;
    case 3:
        mPeriodHigh = data & 7;
        mLength = mLengthReload = cLengthTable[data >> 3] * 2;
        if (mIsEnabled) {
            mIsTimerActive = true;
            mIsTimerReload = true;
        }

        mIsLengthLoaded = true;
        mIsEnvelopeStart = true;
        mIsSweepReload = true;
        mDutyPosition = 0;
        mStatus = 1 << mIndex;
        break;
    case cRegister_Enable:
        if (data & (1 << mIndex)) {
            mIsEnabled = true;
        } else {
            mIsTimerActive = false;
            mLength = 0;
            mLengthReload = 0;
            mIsEnabled = false;
            mStatus = 0;
        }

        break;
    }
}

/**
 * @brief Run the square channel for one sample.
 * @param rate The number of clocks in the sample, scaled by 256.
 * @param events The frame counter events that happened.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SWavwR::Pulse(s16 rate, u64 events, _SEntity* pEntity) {
    bool isEnabled = mIsEnabled;
    s16 period = mPeriod;
    s16 target = period;
    if (pEntity->mHalfFrameEvents & events) {
        clockLength();
        if (mIsSweepEnabled && mSweepPeriod != 0) {
            if (mIsSweepReload) {
                mSweepCounter = mSweepPeriod;
                mIsSweepReload = false;
            }

            if (mSweepCounter > 0) {
                mSweepCounter--;
            }

            if (mSweepCounter == 0) {
                mSweepCounter = mSweepPeriod;
                if (mSweepShift != 0) {
                    s16 delta = period >> mSweepShift;
                    if (mSweepDirection > 0) {
                        target = period - delta - (mIndex == 0);
                    } else if (mSweepDirection < 0) {
                        target = period + delta;
                    }
                }
            }
        }
    }

    if (pEntity->mQuarterFrameEvents & events) {
        clockEnvelope();
    }

    const _SCustomParameter* pRender = pEntity->mTitleRender;
    const s16* pPeriodMax =
        mSweepDirection <= 0 ? &cSweepPeriodMax[mSweepShift] : &pRender->mSweepPeriodMax;
    s64 count = 0;
    if (pRender->mSweepPeriodMin <= target && target <= *pPeriodMax) {
        if (period >= 4 && mIsTimerActive) {
            if (mIsTimerReload) {
                mPhase = period;
                mIsTimerReload = false;
            } else {
                while (mPhase <= rate) {
                    mPhase += period;
                    count++;
                }

                mPhase -= rate;
            }
        }
    }

    if (target != period) {
        mPeriod = target;
        mPhase = target;
    }

    if (mLength == 0) {
        if (mIsLengthEnabled) {
            mStatus = 0;
        }

        if (getChannelOption(pRender, mIndex)) {
            isEnabled = false;
        } else {
            count = 0;
        }
    }

    if (count != 0) {
        s64 volume = getEnvelopeVolume();
        s64 high = 0;
        s64 low = 0;
        for (; count > 0; count--) {
            mDutyPosition = (mDutyPosition + 1) & 0xf;
            if (mDuty & (1 << mDutyPosition)) {
                high++;
            } else {
                low++;
            }
        }

        mLevel = high * volume / (low + high);
    }

    mTarget = mLevel * isEnabled;
    approachTarget(pEntity);
}

/**
 * @brief Save or load the state of the square channel.
 * @param rSerializer The serializer.
 */
void CHvcSpu::_SWavwR::Serialize(Serializer& rSerializer) {
    u64 start = rSerializer.GetOffset();
    serializeWave(rSerializer);
    serializeEnvelope(rSerializer);
    rSerializer.Entry(mIsSweepEnabled);
    rSerializer.Entry(mIsSweepReload);
    rSerializer.Entry(mSweepShift);
    rSerializer.Entry(mSweepDirection);
    rSerializer.Entry(mSweepCounter);
    rSerializer.Entry(mSweepPeriod);
    rSerializer.Entry(mDuty);
    rSerializer.Entry(mDutyPosition);
    alignSerializer(rSerializer, start);
}

/**
 * @brief Reset the triangle channel.
 * @param kind The channel kind.
 * @param index The channel index.
 */
void CHvcSpu::_SWavwT::Reset(u8 kind, u8 index) {
    mKind = kind;
    mIndex = index;
    mIsEnabled = false;
    mStatus = 0;
    mOutput = 0;
    mTarget = 0;
    mLevel = 0;
    mIsTimerActive = false;
    mIsTimerReload = false;
    _0c = 0;
    mPhase = 0;
    mPeriod = 0;
    mIsLengthEnabled = false;
    mIsLengthLoaded = false;
    mLength = 1;
    mLengthReload = 0;
    mIsLinearEnabled = false;
    mIsLinearReload = false;
    mLinearCounter = 0;
    mLinearReload = 0;
    _20 = 0;
    mStep = 0;
}

/**
 * @brief Write a register of the triangle channel.
 * @param reg The register ($4008-$400b as 0-3, or cRegister_Enable).
 * @param data The value written.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SWavwT::Setup(u8 reg, u8 data, _SEntity* pEntity) {
    switch (reg) {
    case 0:
        mIsLinearEnabled = mIsLengthEnabled = !(data & 0x80);
        mLinearReload = data & 0x7f;
        break;
    case 2:
        mPeriod = (mPeriod & 0x700) | data;
        break;
    case 3:
        mPeriodHigh = data & 7;
        mLength = mLengthReload = cLengthTable[data >> 3] * 2;
        if (mIsEnabled) {
            mIsTimerActive = true;
            mIsTimerReload = true;
        }

        mIsLengthLoaded = true;
        mIsLinearReload = true;
        mStatus = 1 << mIndex;
        break;
    case cRegister_Enable:
        if (data & (1 << mIndex)) {
            mIsEnabled = true;
        } else {
            mIsTimerActive = false;
            mLength = 0;
            mLengthReload = 0;
            mLinearCounter = 0;
            mLinearReload = 0;
            mIsEnabled = false;
            mStatus = 0;
        }

        break;
    }
}

/**
 * @brief Run the triangle channel for one sample.
 * @param rate The number of clocks in the sample, scaled by 256.
 * @param events The frame counter events that happened.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SWavwT::Pulse(s16 rate, u64 events, _SEntity* pEntity) {
    if (pEntity->mHalfFrameEvents & events) {
        clockLength();
    }

    if (pEntity->mQuarterFrameEvents & events) {
        if (mIsLinearReload) {
            mLinearCounter = mLinearReload;
            mIsLinearReload = false;
        }

        if (mIsLinearEnabled && mLinearCounter > 0) {
            mLinearCounter--;
        }

        if (mLinearReload == 0) {
            mLinearCounter = 0;
        }
    }

    s64 count = stepTimer(rate);
    if (mIsLengthEnabled && mIsLinearEnabled && (mLength == 0 || mLinearCounter == 0)) {
        mStatus = 0;
    }

    s16 level;
    if (mLength != 0 && count != 0 && mLinearCounter != 0) {
        s64 sum = 0;
        for (s64 i = 0; i < count; i++) {
            mStep++;
            sum += cTriangleTable[mStep & 0x1f];
        }

        level = sum / count;
    } else {
        level = cTriangleTable[mStep & 0x1f];
    }

    mLevel = level;
    mTarget = level << 8;
    approachTarget(pEntity);
}

/**
 * @brief Save or load the state of the triangle channel.
 * @param rSerializer The serializer.
 */
void CHvcSpu::_SWavwT::Serialize(Serializer& rSerializer) {
    u64 start = rSerializer.GetOffset();
    serializeWave(rSerializer);
    rSerializer.Entry(mIsLinearEnabled);
    rSerializer.Entry(mIsLinearReload);
    rSerializer.Entry(mLinearCounter);
    rSerializer.Entry(mLinearReload);
    rSerializer.Entry(_20);
    rSerializer.Entry(mStep);
    alignSerializer(rSerializer, start);
}

/**
 * @brief Reset the noise channel.
 * @param kind The channel kind.
 * @param index The channel index.
 */
void CHvcSpu::_SNoise::Reset(u8 kind, u8 index) {
    mKind = kind;
    mIndex = index;
    mIsEnabled = false;
    mStatus = 0;
    mOutput = 0;
    mTarget = 0;
    mLevel = 0;
    mIsTimerActive = false;
    mIsTimerReload = false;
    _0c = 0;
    mPhase = 0;
    mPeriod = 0;
    mIsLengthEnabled = false;
    mIsLengthLoaded = false;
    mLength = 1;
    mLengthReload = 0;
    mIsEnvelopeEnabled = false;
    mIsEnvelopeStart = false;
    mEnvelopeVolume = 0;
    mIsEnvelopeLoop = false;
    mEnvelopeCounter = 0;
    mEnvelopePeriod = 0;
    _20 = 0;
    _28 = 0;
    mShift = 1;
    mTapA = 0;
    mTapB = 6;
}

/**
 * @brief Write a register of the noise channel.
 * @param reg The register ($400c-$400f as 0-3, or cRegister_Enable).
 * @param data The value written.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SNoise::Setup(u8 reg, u8 data, _SEntity* pEntity) {
    switch (reg) {
    case 0:
        setEnvelope(data);
        break;
    case 2: {
        u8 option = getChannelOption(pEntity->mTitleRender, mIndex);
        mPeriod = cNoisePeriodTable[data & 0xf];
        if (option) {
            if (!(data & 0x80)) {
                if (mTapB == 6) {
                    mShift = 1;
                }

                mTapA = 0;
                mTapB = 1;
            } else {
                if (mTapB == 1) {
                    mShift = 1;
                }

                mTapA = 0;
                mTapB = 6;
            }
        } else {
            mTapA = 0;
            if (!(data & 0x80)) {
                mTapB = 3;
            } else {
                mTapB = 6;
            }
        }

        break;
    }
    case 3:
        mLength = mLengthReload = cLengthTable[data >> 3] * 2;
        if (mIsEnabled) {
            mIsTimerActive = true;
            mIsTimerReload = true;
            mStatus = 1 << mIndex;
        }

        mIsLengthLoaded = true;
        mIsEnvelopeStart = true;
        _28 = 0;
        break;
    case cRegister_Enable:
        if (data & (1 << mIndex)) {
            mIsEnabled = true;
        } else {
            mIsTimerActive = false;
            mLength = 0;
            mLengthReload = 0;
            mIsEnabled = false;
            mStatus = 0;
        }

        break;
    }
}

/**
 * @brief Run the noise channel for one sample.
 * @param rate The number of clocks in the sample, scaled by 256.
 * @param events The frame counter events that happened.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SNoise::Pulse(s16 rate, u64 events, _SEntity* pEntity) {
    bool isEnabled = mIsEnabled;
    if (pEntity->mHalfFrameEvents & events) {
        clockLength();
    }

    if (pEntity->mQuarterFrameEvents & events) {
        clockEnvelope();
    }

    s64 count = stepTimer(rate);
    if (mLength == 0) {
        u8 status = 0;
        if (mIsEnabled && !mIsLengthEnabled) {
            status = 1 << mIndex;
        }

        mStatus = status;
        isEnabled = false;
    }

    if (count != 0) {
        s64 volume = getEnvelopeVolume();
        s64 high = 0;
        s64 low = 0;
        u64 shift = mShift;
        if (getChannelOption(pEntity->mTitleRender, mIndex)) {
            do {
                shift >>= 1;
                if (shift == 0) {
                    shift = 1;
                }

                shift |= (((shift >> mTapB) ^ (shift >> mTapA)) & 1) << 14;
                high += shift & 1;
                low += !(shift & 1);
            } while (--count != 0);
        } else {
            s32 width = mTapB == 6 ? 15 : 17;
            do {
                shift |= (((shift >> mTapA) ^ (shift >> mTapB)) & 1) << width;
                shift >>= 1;
                if (shift == 0) {
                    shift = 1;
                }

                high += shift & 1;
                low += !(shift & 1);
            } while (--count != 0);
        }

        mShift = shift;
        mLevel = low * volume / (high + low);
    }

    mTarget = mLevel * isEnabled;
    approachTarget(pEntity);
}

/**
 * @brief Save or load the state of the noise channel.
 * @param rSerializer The serializer.
 */
void CHvcSpu::_SNoise::Serialize(Serializer& rSerializer) {
    u64 start = rSerializer.GetOffset();
    serializeWave(rSerializer);
    serializeEnvelope(rSerializer);
    rSerializer.Entry(_20);
    rSerializer.Entry(_28);
    rSerializer.Entry(mShift);
    rSerializer.Entry(mTapA);
    rSerializer.Entry(mTapB);
    alignSerializer(rSerializer, start);
}

/**
 * @brief Reset the delta modulation channel.
 * @param kind The channel kind.
 * @param index The channel index.
 */
void CHvcSpu::_SDelta::Reset(u8 kind, u8 index) {
    mKind = kind;
    mIndex = index;
    mIsEnabled = false;
    mStatus = 0;
    mOutput = 0;
    mTarget = 0;
    mLevel = 0;
    mIsTimerActive = false;
    mIsTimerReload = false;
    _0c = 0;
    mPhase = 0;
    mPeriod = 0;
    mIsLengthEnabled = false;
    mIsLengthLoaded = false;
    mLength = 0;
    mLengthReload = 0;
    mCounter = 0;
    mCounterReload = 0;
    mAddress = 0xc000;
    mAddressReload = 0xc000;
    mBuffer = 0xaa;
    mBit = 0;
    mIsLoop = false;
    mIsIrqEnabled = false;
    mIsIrq = false;
    _25 = 0;
    mFetchCount = 0;
}

/**
 * @brief Write a register of the delta modulation channel.
 * @param reg The register ($4010-$4013 as 0-3, or cRegister_Enable).
 * @param data The value written.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SDelta::Setup(u8 reg, u8 data, _SEntity* pEntity) {
    switch (reg) {
    case 0:
        mIsIrqEnabled = data >> 7;
        mIsLoop = (data >> 6) & 1;
        mIsTimerActive = true;
        mIsTimerReload = true;
        mPeriod = cDeltaPeriodTable[data & 0xf];
        if (!mIsIrqEnabled) {
            mIsIrq = false;
        }

        break;
    case 1:
        mCounter = mCounterReload = data & 0x7f;
        break;
    case 2:
        mAddress = mAddressReload = 0xc000 | (data << 6);
        break;
    case 3:
        mIsLengthEnabled = true;
        mIsLengthLoaded = true;
        mLength = mLengthReload = (data << 7) | 8;
        break;
    case cRegister_Enable:
        if (data & (1 << mIndex)) {
            mStatus = 1 << mIndex;
            mIsEnabled = true;
        } else {
            mIsIrq = false;
            mIsEnabled = false;
        }

        break;
    }
}

/**
 * @brief Run the delta modulation channel for one sample.
 * @param rate The number of clocks in the sample, scaled by 256.
 * @param events The frame counter events that happened.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SDelta::Pulse(s16 rate, u64 events, _SEntity* pEntity) {
    s64 count = stepTimer(rate);
    if (mIsLengthLoaded && mIsLengthEnabled) {
        mBit = 0;
        mIsLengthLoaded = false;
    }

    s16 level;
    if (count != 0) {
        s64 sum = 0;
        for (s64 i = 0; i < count; i++) {
            if (mBit >= 8) {
                u16 address = mAddress + 1;
                if (static_cast<s16>(address) >= 0) {
                    address -= 0x8000;
                }

                mBit = 0;
                mAddress = address;
            }

            if (mBit == 0) {
                u64 address = mAddress;
                mBuffer = (pEntity->mMemory + pEntity->mCpuPages[address >> 12].mOffset)
                    [address & 0xfff];
                mFetchCount++;
            }

            if (mLength > 0 && mIsEnabled) {
                if (mBuffer & (1 << mBit)) {
                    if (mCounter <= 0x7d) {
                        mCounter += 2;
                    }
                } else if (mCounter >= 2) {
                    mCounter -= 2;
                }

                mBit++;
                mLength--;
            }

            if (mLength == 0 && mIsLengthEnabled) {
                if (mIsLoop) {
                    mLength = mLengthReload;
                    mIsLengthEnabled = true;
                    mIsLengthLoaded = true;
                    mAddress = mAddressReload;
                    mCounter = mCounterReload;
                } else {
                    mIsLengthEnabled = false;
                    mIsLengthLoaded = false;
                    mIsIrq = true;
                    mStatus = 0;
                }
            }

            sum += mCounter;
        }

        level = sum / count;
    } else {
        level = mCounter;
    }

    mLevel = level;
    mTarget = level << 8;
    approachTarget(pEntity);
}

/**
 * @brief Save or load the state of the delta modulation channel.
 * @param rSerializer The serializer.
 */
void CHvcSpu::_SDelta::Serialize(Serializer& rSerializer) {
    u64 start = rSerializer.GetOffset();
    serializeWave(rSerializer);
    rSerializer.Entry(mCounter);
    rSerializer.Entry(mCounterReload);
    rSerializer.Entry(mAddress);
    rSerializer.Entry(mAddressReload);
    rSerializer.Entry(mBuffer);
    rSerializer.Entry(mBit);
    rSerializer.Entry(mIsLoop);
    rSerializer.Entry(mIsIrqEnabled);
    rSerializer.Entry(mIsIrq);
    rSerializer.Entry(_25);
    rSerializer.Entry(_27);
    alignSerializer(rSerializer, start);
}

/**
 * @brief Reset the disk system sound.
 * @param kind The channel kind.
 * @param index The channel index.
 */
void CHvcSpu::_SQdMfm::Reset(u8 kind, u8 index) {
    mKind = kind;
    mIndex = index;
    mOutput = 0;
    mIsSoundEnabled = false;
    mIsEnvelopeDisabled = false;
    mEnvelopeSpeed = 0xe8;
    mIsPlaying = false;
    mIsWaveLocked = false;
    mFrequency = 0;
    mWavePhase = 0.0;
    for (s32 i = 0; i < 64; i++) {
        mWave[i] = 0;
    }

    mVolumeGain = 0;
    mVolume = 0;
    mVolumeSpeed = 0;
    mVolumeCounter = 0;
    mIsVolumeEnvelope = false;
    mIsVolumeIncrease = false;
    mVolumePeriod = 0;
    mMasterVolume = 0;
    mModFrequency = 0;
    mModPhase = -1.0;
    mModCounter = 0;
    mModWriteIndex = 0;
    mIsModHalted = false;
    for (s32 i = 0; i < 64; i++) {
        mModTable[i] = 0;
    }

    mModPosition = -1;
    mModGain = 0;
    mModSpeed = 0;
    mModCounterClock = 0;
    mIsModEnvelope = false;
    mIsModIncrease = false;
    mModPeriod = 0;
}

/**
 * @brief Write a register of the disk system sound.
 * @param reg The low byte of the register address ($4023, $4040-$408a).
 * @param data The value written.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SQdMfm::Setup(u8 reg, u8 data, _SEntity* pEntity) {
    switch (reg) {
    case 0x23:
        mIsSoundEnabled = (data >> 1) & 1;
        break;
    case 0x80:
        if (data & 0x80) {
            mIsVolumeEnvelope = false;
            mVolumeGain = data & 0x3f;
        } else {
            mIsVolumeEnvelope = true;
            mIsVolumeIncrease = (data >> 6) & 1;
            mVolumeSpeed = data & 0x3f;
            mVolumePeriod = getEnvelopePeriod(mVolumeSpeed, mEnvelopeSpeed);
            mVolumeCounter = 0;
        }

        break;
    case 0x82:
        mFrequencyLow = data;
        break;
    case 0x83: {
        mFrequency = (mFrequency & 0xf0ff) | ((data & 0xf) << 8);
        u8 isPlaying = mIsPlaying;
        mIsPlaying = !(data & 0x80);
        if (!isPlaying && mIsSoundEnabled) {
            mWavePhase = 0.0;
            mVolume = mVolumeGain;
        }

        mIsEnvelopeDisabled = (data >> 6) & 1;
        break;
    }
    case 0x84:
        if (data & 0x80) {
            mIsModEnvelope = false;
            mModGain = data & 0x3f;
        } else {
            mIsModEnvelope = true;
            mIsModIncrease = (data >> 6) & 1;
            mModSpeed = data & 0x3f;
            mModPeriod = getEnvelopePeriod(mModSpeed, mEnvelopeSpeed);
            mModCounterClock = 0;
        }

        break;
    case 0x85:
        mModPhase = -1.0;
        mModCounter = (data & 0x40) ? (data | 0xc0) : (data & 0x3f);
        mModPosition = -1;
        break;
    case 0x86:
        mModFrequencyLow = data;
        break;
    case 0x87:
        mModFrequency = (mModFrequency & 0xf0ff) | ((data & 0xf) << 8);
        mIsModHalted = data >> 7;
        break;
    case 0x88:
        if (mIsModHalted) {
            s8 step = cModStepTable[data & 7];
            u8 index = mModWriteIndex;
            mModTable[index] = step;
            mModTable[static_cast<u8>(index + 1)] = step;
            mModWriteIndex = (index + 2) & 0x3f;
        }

        break;
    case 0x89:
        mIsWaveLocked = !(data & 0x80);
        mMasterVolume = data & 3;
        break;
    case 0x8a:
        mVolumePeriod = getEnvelopePeriod(mVolumeSpeed, data);
        mEnvelopeSpeed = data;
        mModPeriod = getEnvelopePeriod(mModSpeed, data);
        break;
    default:
        if (reg >= 0x40 && reg < 0x80 && !mIsWaveLocked) {
            mWave[reg - 0x40] = data & 0x3f;
        }

        break;
    }
}

/**
 * @brief Run the disk system sound for one sample.
 * @param rate The number of clocks in the sample, scaled by 256.
 * @param events The frame counter events that happened.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SQdMfm::Pulse(s16 rate, u64 events, _SEntity* pEntity) {
    u8 isOutput = 0;
    if (mIsSoundEnabled && mIsPlaying) {
        isOutput = mIsWaveLocked;
    }

    if (!mIsEnvelopeDisabled) {
        if (mIsVolumeEnvelope && mEnvelopeSpeed != 0) {
            u32 period = mVolumePeriod;
            u64 counter = mVolumeCounter;
            if (counter >= period) {
                if (mIsVolumeIncrease) {
                    do {
                        if (mVolumeGain < 0x20) {
                            mVolumeGain++;
                        }

                        counter -= period;
                    } while (counter >= period);
                } else {
                    do {
                        if (mVolumeGain != 0) {
                            mVolumeGain--;
                        }

                        counter -= period;
                    } while (counter >= period);
                }

                mVolumeCounter = counter;
            }

            mVolumeCounter = counter + (rate << 8);
        }

        if (mIsModEnvelope && mEnvelopeSpeed != 0) {
            u32 period = mModPeriod;
            u64 counter = mModCounterClock;
            if (counter >= period) {
                if (mIsModIncrease) {
                    do {
                        if (mModGain < 0x20) {
                            mModGain++;
                        }

                        counter -= period;
                    } while (counter >= period);
                } else {
                    do {
                        if (mModGain != 0) {
                            mModGain--;
                        }

                        counter -= period;
                    } while (counter >= period);
                }

                mModCounterClock = counter;
            }

            mModCounterClock = counter + (rate << 8);
        }
    }

    s64 modGain = mModGain < 0x20 ? mModGain : 0x20;
    s64 mod;
    if (!mIsModHalted && mModFrequency != 0) {
        f64 phase = mModPhase;
        s64 position = static_cast<s64>(phase);
        s8 counter = mModCounter;
        if (position >= 0) {
            while (mModPosition != position) {
                s8 next = mModPosition + 1;
                if (next > 63) {
                    next = 0;
                }

                mModPosition = next;
                s8 step = mModTable[(mModWriteIndex + next) & 0x3f];
                s8 value = step == 0x7f ? 0 : mModCounter + step;
                s8 wrapped = value + 64;
                if (wrapped < 0) {
                    value = (wrapped & 0x7f) - 64;
                }

                mModCounter = value;
            }
        }

        phase += rate * cModPhaseScale * mModFrequency;
        if (phase >= 64.0) {
            phase -= 64.0;
        }

        mModPhase = phase;
        mod = counter * modGain;
    } else {
        mod = mModCounter * modGain;
    }

    if (mod < 0) {
        mod -= 8;
    } else if (mod > 0) {
        mod += 8;
    }

    if (mod >= 0xc00) {
        mod -= 0x1000;
    } else if (mod < -0x400) {
        mod += 0x1000;
    }

    s64 sample = 0;
    if (mFrequency != 0) {
        f64 phase = mWavePhase;
        s64 pitch = mod + 0x400;
        s64 volume = mVolume < 0x20 ? mVolume : 0x20;
        sample = volume * mWave[static_cast<s64>(phase)];
        phase += rate * cWavePhaseScale * mFrequency * pitch;
        mWavePhase = phase;
        if (phase >= 64.0) {
            mVolume = mVolumeGain;
            mWavePhase = phase - 64.0;
        }
    }

    switch (mMasterVolume) {
    case 1:
        sample = sample * 2 / 3;
        break;
    case 2:
        sample = sample >> 1;
        break;
    case 3:
        sample = sample * 2 / 5;
        break;
    }

    approachOutput(mOutput, sample * isOutput, pEntity, mIndex);
}

/**
 * @brief Save or load the state of the disk system sound.
 * @param rSerializer The serializer.
 */
void CHvcSpu::_SQdMfm::Serialize(Serializer& rSerializer) {
    u64 start = rSerializer.GetOffset();
    rSerializer.Entry(mKind);
    rSerializer.Entry(mIndex);
    rSerializer.Entry(mOutput);
    rSerializer.Entry(mIsSoundEnabled);
    rSerializer.Entry(mIsEnvelopeDisabled);
    rSerializer.Entry(mEnvelopeSpeed);
    rSerializer.Entry(mIsPlaying);
    rSerializer.Entry(mIsWaveLocked);
    rSerializer.Entry(mFrequency);
    rSerializer.Entry(mWavePhase);
    for (s32 i = 0; i < 64; i++) {
        rSerializer.Entry(mWave[i]);
    }

    rSerializer.Entry(mVolumeGain);
    rSerializer.Entry(mVolume);
    rSerializer.Entry(mVolumeSpeed);
    rSerializer.Entry(mIsVolumeEnvelope);
    rSerializer.Entry(mIsVolumeIncrease);
    rSerializer.Entry(mVolumeCounter);
    rSerializer.Entry(mVolumePeriod);
    rSerializer.Entry(mMasterVolume);
    rSerializer.Entry(mModFrequency);
    rSerializer.Entry(mModPhase);
    rSerializer.Entry(mModCounter);
    rSerializer.Entry(mModWriteIndex);
    rSerializer.Entry(mIsModHalted);
    for (s32 i = 0; i < 64; i++) {
        rSerializer.Entry(mModTable[i]);
    }

    rSerializer.Entry(mModPosition);
    rSerializer.Entry(mModGain);
    rSerializer.Entry(mModSpeed);
    rSerializer.Entry(mIsModEnvelope);
    rSerializer.Entry(mIsModIncrease);
    rSerializer.Entry(mModCounterClock);
    rSerializer.Entry(mModPeriod);
    alignSerializer(rSerializer, start);
}

/**
 * @brief Reset the disk drive.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SQdDisk::Reset(_SEntity* pEntity) {
    u8* pRegister = pEntity->mMemory + cMemory_IoRegister;
    mOffset = 0;
    mIsOffsetChanged = false;
    mTimer = 0;
    mStatusMask = 0x47;
    mMediaCount = 0x78;
    mIsTimerActive = false;
    pRegister[0x32] = 0x47;
    pRegister[0x33] = 0x80;
}

/**
 * @brief Run the disk drive for one sample: transfer the current byte and run the timer IRQ.
 * @param rate The number of clocks in the sample.
 * @param events The frame counter events that happened.
 * @param pEntity The machine state.
 */
void CHvcSpu::_SQdDisk::Pulse(s16 rate, u64 events, _SEntity* pEntity) {
    u8* pMemory = pEntity->mMemory;
    u8* pShadow = pMemory + cMemory_IoShadow;
    u8* pRegister = pMemory + cMemory_IoRegister;
    u8 timerControl = pShadow[0x22];
    if (mIsOffsetChanged) {
        s8 control = pShadow[0x25];
        if (control & 4) {
            pRegister[0x31] = pMemory[cMemory_DiskImage + mOffset];
        }

        if (control < 0) {
            pEntity->mOrderController._08 |= 1;
            mIsOffsetChanged = false;
        }
    }

    if (mIsTimerActive) {
        for (s32 i = 0; i < rate; i++) {
            if (mTimer == 0) {
                if (!(timerControl & 1)) {
                    mIsTimerActive = false;
                    return;
                }

                SetTimer(pEntity);
            }

            mTimer--;
            if (mTimer == 0 && (timerControl & 2)) {
                pRegister[0x30] |= 1;
                pEntity->mOrderController._08 |= 1;
            }
        }
    }
}

/**
 * @brief Reload the timer from its reload registers ($4020-$4021).
 * @param pEntity The machine state.
 */
void CHvcSpu::_SQdDisk::SetTimer(_SEntity* pEntity) {
    mTimer = *reinterpret_cast<u16*>(pEntity->mMemory + cMemory_IoShadow + 0x20);
}

/**
 * @brief Get the position of the disk head.
 * @return The offset in the disk image.
 */
u64 CHvcSpu::_SQdDisk::GetQdOffset() {
    return mOffset;
}

/**
 * @brief Move the disk head.
 * @param offset The offset in the disk image.
 */
void CHvcSpu::_SQdDisk::SetQdOffset(u64 offset) {
    mOffset = offset;
    mIsOffsetChanged = true;
}

/**
 * @brief Move the disk head inside the current side.
 * @param offset The offset in the side.
 */
void CHvcSpu::_SQdDisk::SetQdOffsetHalf(u64 offset) {
    mOffset = (mOffset & 0xffff0000) | (offset & 0xffff);
    mIsOffsetChanged = true;
}

/**
 * @brief Advance the disk head by one byte inside the current side.
 */
void CHvcSpu::_SQdDisk::IncQdOffsetHalf() {
    mOffset = (mOffset & 0xffff0000) | ((mOffset + 1) & 0xffff);
    mIsOffsetChanged = true;
}

/**
 * @brief Start the timer IRQ ($4022).
 * @param pEntity The machine state.
 */
void CHvcSpu::_SQdDisk::SetTimerStart(_SEntity* pEntity) {
    SetTimer(pEntity);
    mIsTimerActive = true;
}

/**
 * @brief Mark the disk as written.
 * @param isDirty Whether the disk was written.
 */
void CHvcSpu::_SQdDisk::SetQdDirty(bool isDirty) {
    mIsDirty = isDirty;
}

/**
 * @brief Advance the disk insertion counter, holding it while the buttons are pressed.
 */
void CHvcSpu::_SQdDisk::IncMediaCount() {
    u8 count = mMediaCount;
    if (count >= 0xc4) {
        mMediaCount = count + 1;
    } else if (count < 0x78) {
        mMediaCount = count + 1;
    } else if (getGioButton(GetCHvc()) & 9) {
        mMediaCount = 0xc4;
    }
}

/**
 * @brief Construct the sound processing unit.
 */
CHvcSpu::CHvcSpu() : CPlatformHvcUnit(0, 5, 1) {
    std::strncpy(mDescriptor.mName, "HVC SPU        ", sizeof(mDescriptor.mName) - 1);
}

/**
 * @brief Destroy the sound processing unit.
 */
CHvcSpu::~CHvcSpu() {}

/**
 * @brief Reset the clocks and every channel.
 */
inline void CHvcSpu::resetChannels() {
    mClock = 0;
    mSampleClock = 0;
    mEvents = 0;
    mSample = 0;
    mLastSample = 0;
    mSquare[0].Reset(0, 0);
    mSquare[1].Reset(0, 1);
    mTriangle.Reset(0, 2);
    mNoise.Reset(0, 3);
    mDelta.Reset(0, 4);
    mQdMfm.Reset(0, 5);
    mExSquare[0].Reset(0, 6);
    mExSquare[1].Reset(0, 7);
    mExWave.Reset(0, 8);
}

/**
 * @brief Reset the channels before running.
 * @return The result (always success).
 */
int CHvcSpu::_Prologue() {
    resetChannels();
    return cResult_Success;
}

/**
 * @brief Nothing to do after running.
 * @return The result (always success).
 */
int CHvcSpu::_Epilogue() {
    return cResult_Success;
}

/**
 * @brief Read or write a sound register.
 * @param pUnit The calling unit.
 * @param channel The bus channel.
 * @param operation cOperation_Read or cOperation_Write.
 * @param pArgument The transfer.
 * @return cResult_Success, or cResult_Unhandled for an address that is not a sound register.
 */
int CHvcSpu::Access(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) {
    _SEntity* pEntity = mEntity;
    u8* pMemory = pEntity->mMemory;
    u64 address = pArgument->mAccess.mAddress;
    u8* pShadow = pMemory + cMemory_IoShadow;
    u8* pDiskStatus = pMemory + cMemory_IoRegister + 0x32;

    if (operation == cOperation_Write) {
        switch (address) {
        case 0x4000:
        case 0x4001:
        case 0x4002:
        case 0x4003: {
            u8 data = getAccessData(pArgument);
            pShadow[address & 0xff] = data;
            mSquare[0].Setup(address, data, mEntity);
            return cResult_Success;
        }
        case 0x4004:
        case 0x4005:
        case 0x4006:
        case 0x4007: {
            u8 data = getAccessData(pArgument);
            pShadow[address & 0xff] = data;
            mSquare[1].Setup(address - 4, data, mEntity);
            return cResult_Success;
        }
        case 0x4008:
        case 0x4009:
        case 0x400a:
        case 0x400b: {
            u8 data = getAccessData(pArgument);
            pShadow[address & 0xff] = data;
            mTriangle.Setup(address - 8, data, mEntity);
            return cResult_Success;
        }
        case 0x400c:
        case 0x400d:
        case 0x400e:
        case 0x400f: {
            u8 data = getAccessData(pArgument);
            pShadow[address & 0xff] = data;
            mNoise.Setup(address - 0xc, data, mEntity);
            return cResult_Success;
        }
        case 0x4010:
        case 0x4011:
        case 0x4012:
        case 0x4013: {
            u8 data = getAccessData(pArgument);
            pShadow[address & 0xff] = data;
            mDelta.Setup(address - 0x10, data, mEntity);
            return cResult_Success;
        }
        case 0x4015:
            pShadow[0x15] = getAccessData(pArgument);
            mSquare[0].Setup(cRegister_Enable, pShadow[0x15], mEntity);
            mSquare[1].Setup(cRegister_Enable, pShadow[0x15], mEntity);
            mTriangle.Setup(cRegister_Enable, pShadow[0x15], mEntity);
            mNoise.Setup(cRegister_Enable, pShadow[0x15], mEntity);
            mDelta.Setup(cRegister_Enable, pShadow[0x15], mEntity);
            mChannelFlags |= ~pShadow[0x15] & 0xf;
            mChannelFlags |= 0x10;
            return cResult_Success;
        }
    } else if (address == 0x4015) {
        *static_cast<u8*>(pArgument->mAccess.mData) = pMemory[cMemory_IoRegister + 0x15];
        return cResult_Success;
    } else {
        *static_cast<u8*>(pArgument->mAccess.mData) = 0x40;
        pEntity = mEntity;
    }

    switch (pEntity->mMapper) {
    case cMapper_Mmc5: {
        u8* pMemory = pEntity->mMemory;
        u64 address = pArgument->mAccess.mAddress;
        if (operation == cOperation_Write) {
            u8* pExShadow = pMemory + cMemory_ExShadow;
            switch (address) {
            case 0x5000:
            case 0x5002:
            case 0x5003: {
                u8 data = getAccessData(pArgument);
                pExShadow[address & 0xff] = data;
                mExSquare[0].Setup(address, data, mEntity);
                return cResult_Success;
            }
            case 0x5004:
            case 0x5006:
            case 0x5007: {
                u8 data = getAccessData(pArgument);
                pExShadow[address & 0xff] = data;
                mExSquare[1].Setup(address - 4, data, mEntity);
                return cResult_Success;
            }
            case 0x5010:
                pExShadow[0x10] = getAccessData(pArgument);
                return cResult_Success;
            case 0x5011:
                pExShadow[0x11] = getAccessData(pArgument);
                return cResult_Success;
            case 0x5015: {
                u8 data = getAccessData(pArgument);
                pExShadow[0x15] = data;
                mChannelFlags |= ~data & 3;
                return cResult_Success;
            }
            default:
                return cResult_Unhandled;
            }
        }

        if (address == 0x5015) {
            *static_cast<u8*>(pArgument->mAccess.mData) = pMemory[cMemory_ExRegister + 0x15];
            return cResult_Success;
        }

        return cResult_Unhandled;
    }
    case cMapper_Vrc6: {
        int result = cResult_Success;
        if (operation == cOperation_Write) {
            u64 address = pArgument->mAccess.mAddress;
            if (((address - 0x9000) >> 12) <= 2) {
                u64 page = ((address & 0x3000) - 0x1000) >> 10;
                u64 reg = (address & 3) == 0 ? page : page | ((address + 1) & 3);
                reg |= 0x5000;

                u8* pMemory = pEntity->mMemory;
                u8* pExShadow = pMemory + cMemory_ExShadow;
                switch (reg) {
                case 0x5000: {
                    u8 data = getAccessData(pArgument);
                    u8 control = ((data >> 3) & 0x10) | (data & 0xf) | 0x20;
                    pExShadow[reg & 0xff] = control;
                    mExSquare[0].Setup(reg, control, mEntity);
                    u8 sweep = (getAccessData(pArgument) >> 4) & 7;
                    pExShadow[0x01] = sweep;
                    mExSquare[0].Setup(reg + 1, sweep, mEntity);
                    return cResult_Success;
                }
                case 0x5002:
                case 0x5003: {
                    u8 data = getAccessData(pArgument);
                    pExShadow[reg & 0xff] = data;
                    mExSquare[0].Setup(reg, data, mEntity);
                    pExShadow[0x15] |= pExShadow[0x03] >> 7;
                    mChannelFlags = pExShadow[0x15];
                    return cResult_Success;
                }
                case 0x5004: {
                    u8 data = getAccessData(pArgument);
                    u8 control = ((data >> 3) & 0x10) | (data & 0xf) | 0x20;
                    pExShadow[reg & 0xff] = control;
                    mExSquare[1].Setup(reg - 4, control, mEntity);
                    u8 sweep = (getAccessData(pArgument) >> 4) & 7;
                    pExShadow[0x05] = sweep;
                    mExSquare[1].Setup(reg - 3, sweep, mEntity);
                    return cResult_Success;
                }
                case 0x5006:
                case 0x5007: {
                    u8 data = getAccessData(pArgument);
                    pExShadow[reg & 0xff] = data;
                    mExSquare[1].Setup(reg - 4, data, mEntity);
                    pExShadow[0x15] |= (pExShadow[0x15] >> 6) & 2;
                    mChannelFlags = pExShadow[0x15];
                    return cResult_Success;
                }
                case 0x5008:
                case 0x500a:
                case 0x500b: {
                    u8 data = getAccessData(pArgument);
                    pExShadow[reg & 0xff] = data;
                    mExWave.Setup(reg - 8, data, mEntity);
                    pExShadow[0x15] = ((pExShadow[0x0b] >> 5) & 4) | pExShadow[0x15];
                    mChannelFlags = pExShadow[0x15];
                    return cResult_Success;
                }
                default:
                    result = cResult_Unhandled;
                    break;
                }
            }
        }

        return result;
    }
    case cMapper_Disk: {
        u64 address = pArgument->mAccess.mAddress;
        s8* pData = static_cast<s8*>(pArgument->mAccess.mData);
        if (operation == cOperation_Write) {
            s8 data = *pData;
            switch (address) {
            case 0x4020:
            case 0x4021:
                pShadow[address & 0xff] = data;
                return cResult_Success;
            case 0x4022:
                pShadow[0x22] = data;
                mQdDisk.SetTimerStart(mEntity);
                return cResult_Success;
            case 0x4024:
                pShadow[0x24] = data;
                if ((pShadow[0x25] & 0x87) == 0x81) {
                    mEntity->mMemory[cMemory_DiskImage + mQdDisk.GetQdOffset()] = data;
                    mQdDisk.SetQdDirty(true);
                }

                return cResult_Success;
            case 0x4025:
                pShadow[0x25] = data;
                if (data < 0) {
                    mEntity->mOrderController._08 |= 1;
                } else if (pShadow[0x22] & 2) {
                    pShadow[0x32] &= ~4;
                    mQdDisk.SetQdOffset(mQdDisk.GetQdOffset() & 0xffff0000);
                }

                if (pShadow[0x25] & 8) {
                    BusMapSwitchVRAM(mEntity, 1);
                } else {
                    BusMapSwitchVRAM(mEntity, 0);
                }

                return cResult_Success;
            case 0x4026:
                pShadow[0x26] = data;
                return cResult_Success;
            case 0x4023:
                pShadow[0x23] = data;
                // fallthrough
            case 0x4080:
            case 0x4082:
            case 0x4083:
            case 0x4084:
            case 0x4085:
            case 0x4086:
            case 0x4087:
            case 0x4088:
            case 0x4089:
            case 0x408a:
                mQdMfm.Setup(address, data, mEntity);
                return cResult_Success;
            default:
                if ((address >> 6) == 0x101) {
                    mQdMfm.Setup(address, data, mEntity);
                    return cResult_Success;
                }

                return cResult_Unhandled;
            }
        }

        u8* pRegister = pMemory + cMemory_IoRegister;
        switch (address) {
        case 0x4030:
            *pData = pRegister[0x30];
            pRegister[0x30] = 0;
            return cResult_Success;
        case 0x4031:
            *pData = pRegister[0x31];
            mQdDisk.IncQdOffsetHalf();
            return cResult_Success;
        case 0x4032: {
            u16 programCounter = getProgramCounter(GetCHvc());
            u8 status = *pDiskStatus;
            *pData = status;
            if (programCounter >= 0xe000) {
                if (programCounter == 0xef36 || programCounter == 0xeee2) {
                    status &= mQdDisk.mStatusMask;
                } else {
                    status = 0x40;
                }

                *pData = status;
            } else {
                status |= 0x47;
                *pData = status;
                if (mQdDisk.mMediaCount <= 0xc2) {
                    status = (status & 0xf8) | (pShadow[0x4025] & 2);
                    *pData = status;
                }
            }

            *pDiskStatus = status;
            return cResult_Success;
        }
        case 0x4033:
            *pData = pRegister[0x33];
            return cResult_Success;
        case 0x4090:
            *pData = mQdMfm.mVolumeGain | 0x40;
            return cResult_Success;
        case 0x4091:
            *pData = mQdMfm.mFrequencyHigh;
            return cResult_Success;
        case 0x4092:
            *pData = mQdMfm.mModGain | 0x40;
            return cResult_Success;
        case 0x4093:
            *pData = mQdMfm.mModFrequencyHigh;
            return cResult_Success;
        case 0x4094:
        case 0x4095:
        case 0x4096:
        case 0x4097:
            return cResult_Success;
        default:
            if ((address >> 6) == 0x101) {
                if (mQdMfm.mIsWaveLocked) {
                    *pData = 0;
                } else {
                    *pData = mQdMfm.mWave[address - 0x4040] | 0x40;
                }

                return cResult_Success;
            }

            return cResult_Unhandled;
        }
    }
    default:
        return cResult_Success;
    }
}

/**
 * @brief Run the sound for a slice of CPU time, mixing one sample per sample clock.
 * @param pUnit The calling unit.
 * @param channel The bus channel.
 * @param operation The operation.
 * @param pArgument The events and the clock of the slice.
 * @return The result (always success).
 */
int CHvcSpu::Process(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) {
    _SEntity* pEntity = mEntity;
    u8* pMemory = pEntity->mMemory;
    bool isDisk;
    if (pEntity->mResetRequest & (1 << mDescriptor.mIndex)) {
        u8* pRegister = pMemory + cMemory_IoRegister;
        u8* pShadow = pMemory + cMemory_IoShadow;
        clearRegister(pRegister, pShadow, 0x15);
        clearRegister(pRegister, pShadow, 0x20);
        clearRegister(pRegister, pShadow, 0x21);
        clearRegister(pRegister, pShadow, 0x22);
        std::memset(pRegister, 0, 0x14);
        std::memset(pShadow, 0, 0x14);

        u8 mapper = mEntity->mMapper;
        if (mapper == cMapper_Vrc6 || mapper == cMapper_Mmc5) {
            u8* pExRegister = pMemory + cMemory_ExRegister;
            u8* pExShadow = pMemory + cMemory_ExShadow;
            clearRegister(pExRegister, pExShadow, 0x00);
            clearRegister(pExRegister, pExShadow, 0x02);
            clearRegister(pExRegister, pExShadow, 0x03);
            clearRegister(pExRegister, pExShadow, 0x04);
            clearRegister(pExRegister, pExShadow, 0x06);
            clearRegister(pExRegister, pExShadow, 0x07);
            clearRegister(pExRegister, pExShadow, 0x08);
            clearRegister(pExRegister, pExShadow, 0x10);
            clearRegister(pExRegister, pExShadow, 0x11);
            clearRegister(pExRegister, pExShadow, 0x15);
        }

        resetChannels();
        isDisk = isDiskSystem();
        if (isDisk) {
            mQdDisk.Reset(mEntity);
        }

        mEntity->mResetRequest &= ~(1 << mDescriptor.mIndex);
    } else {
        isDisk = isDiskSystem();
    }

    if (isDisk && (pArgument->mProcess.mEvents & 0x10) &&
        (mEntity->mScanlineClock & ~0xffull) == 0xf000) {
        mQdDisk.IncMediaCount();
    }

    mEvents |= pArgument->mProcess.mEvents;
    if (isDiskSystem()) {
        mQdDisk.Pulse(pArgument->mProcess.mClock >> 8, mEvents, mEntity);
    }

    mDelta.mFetchCount = 0;
    mClock += pArgument->mProcess.mClock;
    while (true) {
        u64 clock = mSampleClock + mEntity->mSampleClock;
        if (clock > mClock) {
            break;
        }

        s16 rate = clock >> 8;
        u8* pStatus = pMemory + cMemory_IoRegister + 0x15;
        *pStatus = (*pStatus & 0xe0) | mSquare[0].mStatus | mSquare[1].mStatus |
                   mTriangle.mStatus | mNoise.mStatus | mDelta.mStatus;
        mSquare[0].Pulse(rate, mEvents, mEntity);
        mSquare[1].Pulse(rate, mEvents, mEntity);
        mTriangle.Pulse(rate, mEvents, mEntity);
        mNoise.Pulse(rate, mEvents, mEntity);
        mDelta.Pulse(rate, mEvents, mEntity);
        mQdMfm.Pulse(rate, mEvents, mEntity);

        s64 consumed = rate << 8;
        mClock -= consumed;
        mSampleClock += mEntity->mSampleClock - consumed;

        const _SCustomParameter* pRender = mEntity->mTitleRender;
        s32 square0 = mSquare[0].mOutput * pRender->mVolume[0];
        s64 sample;
        if (pRender->_00[5] & 1) {
            s32 square1 = mSquare[1].mOutput * pRender->mVolume[1];
            s32 triangle = mTriangle.mOutput * pRender->mVolume[2];
            s32 noise = mNoise.mOutput * pRender->mVolume[3];
            s32 delta = mDelta.mOutput * pRender->mVolume[4];
            s32 mix = square0 / 4 + square1 / 4 - triangle / 4 - noise / 4 + delta / 32;
            sample = static_cast<s64>(mix) * 85 / 256;
        } else {
            s32 square1 = mSquare[1].mOutput * pRender->mVolume[1] / 32;
            s32 triangle = mTriangle.mOutput * pRender->mVolume[2] / 32;
            s32 noise = mNoise.mOutput * pRender->mVolume[3] / 32;
            s32 delta = mDelta.mOutput * pRender->mVolume[4] / 32;
            s32 disk = mQdMfm.mOutput * pRender->mVolume[5] / 32;
            s32 mix = square0 / 32 * 0x3f8 / 1024 + square1 * 0x3f8 / 1024 +
                      triangle * 0x477 / 1024 + noise * 0x27b / 1024 + delta * 0x1fc / 1024 +
                      disk * 4;
            sample = -mix;
        }

        mSample = sample;
        mLastSample = sample;
        mEntity->mAudioBuffer[mEntity->mAudioSampleNum] = sample;
        mEntity->mAudioSampleNum++;
        mEvents = 0;
    }

    if (pArgument->mProcess.mCycles != nullptr) {
        *pArgument->mProcess.mCycles = 0;
    }

    mDelta.mFetchCount = 0;
    return cResult_Success;
}

/**
 * @brief Save or load the state of the sound.
 * @param pBuffer The buffer, or nullptr to only get the size.
 * @param isImport Whether to load the state from the buffer.
 * @return The size of the state.
 */
u64 CHvcSpu::_SerializeCore(void* pBuffer, bool isImport) {
    Serializer serializer(pBuffer, isImport);
    serializer.Entry(mChannelFlags);
    serializer.Entry(mClock);
    serializer.Entry(mSampleClock);
    serializer.Entry(mEvents);
    serializer.Entry(mSample);
    serializer.Entry(mLastSample);
    if (isDiskSystem()) {
        u64 offset = serializer.GetOffset();
        u64 diskOffset = mQdDisk.GetQdOffset();
        serializer.Entry(diskOffset);
        if (isImport) {
            mQdDisk.SetQdOffset(diskOffset);
        }

        serializer.Entry(mQdDisk.mTimer);
        serializer.Entry(mQdDisk.mStatusMask);
        serializer.Entry(mQdDisk.mIsDirty);
        serializer.Entry(mQdDisk.mIsTimerActive);
        serializer.SetOffset(offset + cSerializeDiskSize);
    }

    mSquare[0].Serialize(serializer);
    mSquare[1].Serialize(serializer);
    mTriangle.Serialize(serializer);
    mNoise.Serialize(serializer);
    mDelta.Serialize(serializer);
    mQdMfm.Serialize(serializer);
    mExSquare[0].Serialize(serializer);
    mExSquare[1].Serialize(serializer);
    mExWave.Serialize(serializer);
    return serializer.GetOffset();
}

/**
 * @brief Save the state of the sound.
 * @param pBuffer The buffer.
 */
void CHvcSpu::_SerializeCoreExport(void* pBuffer) {
    _SerializeCore(pBuffer, false);
}

/**
 * @brief Load the state of the sound.
 * @param pBuffer The buffer.
 */
void CHvcSpu::_SerializeCoreImport(const void* pBuffer) {
    _SerializeCore(const_cast<void*>(pBuffer), true);
}

/**
 * @brief Get the size of the serialized state, including its tag.
 * @return The size.
 */
u64 CHvcSpu::_SerializeCoreInferSize() {
    return static_cast<u32>(_SerializeCore(nullptr, false) + cSerializeTagSize + 0x3f) & ~0xfu;
}

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
