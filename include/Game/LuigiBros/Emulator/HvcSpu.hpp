#pragma once

#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"
#include <attributes.h>

namespace Common::Serialize {
class LittleEndian;
}  // namespace Common::Serialize

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Sound processing unit of the emulated Famicom.
 *
 * Emulates the APU channels ($4000-$4015), the Famicom Disk System sound and disk drive
 * ($4020-$409x) and the expansion sound of MMC5 and VRC6, and mixes them into the audio buffer.
 */
class CHvcSpu : public CPlatformHvcUnit {
public:
    using Serializer = Common::Serialize::LittleEndian;

    /// Register written to enable or disable the channels ($4015, relative to a channel).
    static constexpr u8 cRegister_Enable = 0x10;

    /**
     * @brief State shared by every APU style channel: timer, length counter and output.
     */
    struct _SWave {
        void clockLength();
        s64 stepTimer(s16 rate);
        void approachTarget(const _SEntity* pEntity);
        void serializeWave(Serializer& rSerializer);

        u8 mKind;
        u8 mIndex;
        u8 mIsEnabled;
        u8 mStatus;
        s16 mOutput;
        s16 mTarget;
        s16 mLevel;
        u8 mIsTimerActive;
        u8 mIsTimerReload;
        s16 _0c;
        s16 mPhase;

        union {
            s16 mPeriod;

            struct {
                u8 mPeriodLow;
                u8 mPeriodHigh;
            };
        };

        u8 mIsLengthEnabled;
        u8 mIsLengthLoaded;
        s16 mLength;
        s16 mLengthReload;
    };

    static_assert(sizeof(_SWave) == 0x18, "_SWave size");

    /**
     * @brief A channel with a volume envelope (square and noise).
     */
    struct _SWaveEnvelope : _SWave {
        void clockEnvelope();
        void setEnvelope(u8 data);
        s64 getEnvelopeVolume() const;
        void serializeEnvelope(Serializer& rSerializer);

        u8 mIsEnvelopeEnabled;
        u8 mIsEnvelopeStart;
        s8 mEnvelopeVolume;
        u8 mIsEnvelopeLoop;
        s16 mEnvelopeCounter;
        s16 mEnvelopePeriod;
    };

    static_assert(sizeof(_SWaveEnvelope) == 0x20, "_SWaveEnvelope size");

    /**
     * @brief Square wave channel (APU and MMC5).
     */
    struct _SWavwR : _SWaveEnvelope {
        void Reset(u8 kind, u8 index);
        void Setup(u8 reg, u8 data, _SEntity* pEntity);
        void Pulse(s16 rate, u64 events, _SEntity* pEntity);
        void Serialize(Serializer& rSerializer);

        u8 mIsSweepEnabled;
        u8 mIsSweepReload;
        s8 mSweepShift;
        s8 mSweepDirection;
        s16 mSweepCounter;
        u16 mSweepPeriod;
        u64 mDuty;
        u8 mDutyPosition;
    };

    static_assert(sizeof(_SWavwR) == 0x38, "_SWavwR size");

    /**
     * @brief Triangle wave channel.
     */
    struct _SWavwT : _SWave {
        void Reset(u8 kind, u8 index);
        void Setup(u8 reg, u8 data, _SEntity* pEntity);
        void Pulse(s16 rate, u64 events, _SEntity* pEntity);
        void Serialize(Serializer& rSerializer);

        u8 mIsLinearEnabled;
        u8 mIsLinearReload;
        s16 mLinearCounter;
        u16 mLinearReload;
        u64 _20;
        u8 mStep;
    };

    static_assert(sizeof(_SWavwT) == 0x30, "_SWavwT size");

    /**
     * @brief Noise channel.
     */
    struct _SNoise : _SWaveEnvelope {
        void Reset(u8 kind, u8 index);
        void Setup(u8 reg, u8 data, _SEntity* pEntity);
        void Pulse(s16 rate, u64 events, _SEntity* pEntity);
        void Serialize(Serializer& rSerializer);

        u64 _20;
        u8 _28;
        u64 mShift;
        u8 mTapA;
        u8 mTapB;
    };

    static_assert(sizeof(_SNoise) == 0x40, "_SNoise size");

    /**
     * @brief Delta modulation channel.
     */
    struct _SDelta : _SWave {
        void Reset(u8 kind, u8 index);
        void Setup(u8 reg, u8 data, _SEntity* pEntity);
        void Pulse(s16 rate, u64 events, _SEntity* pEntity);
        void Serialize(Serializer& rSerializer);

        s16 mCounter;
        s16 mCounterReload;
        u16 mAddress;
        u16 mAddressReload;
        u8 mBuffer;
        u8 mBit;
        u8 mIsLoop;
        u8 mIsIrqEnabled;
        u8 mIsIrq;
        u8 _25;
        u8 mFetchCount;
        u8 _27;
    };

    static_assert(sizeof(_SDelta) == 0x28, "_SDelta size");

    /**
     * @brief Famicom Disk System wave channel with its modulator.
     */
    struct _SQdMfm {
        void Reset(u8 kind, u8 index);
        void Setup(u8 reg, u8 data, _SEntity* pEntity);
        void Pulse(s16 rate, u64 events, _SEntity* pEntity);
        void Serialize(Serializer& rSerializer);

        u8 mKind;
        u8 mIndex;
        s16 mOutput;
        u8 mIsSoundEnabled;
        u8 mIsEnvelopeDisabled;
        u8 mEnvelopeSpeed;
        u8 _07;
        u8 mIsPlaying;
        u8 mIsWaveLocked;
        u8 _0a[6];

        union {
            u16 mFrequency;

            struct {
                u8 mFrequencyLow;
                u8 mFrequencyHigh;
            };
        };

        f64 mWavePhase;
        s8 mWave[64];
        u8 mVolumeGain;
        u8 mVolume;
        u8 mVolumeSpeed;
        u64 mVolumeCounter;
        u8 mIsVolumeEnvelope;
        u8 mIsVolumeIncrease;
        u64 mVolumePeriod;
        u8 mMasterVolume;
        u8 _81[7];

        union {
            u16 mModFrequency;

            struct {
                u8 mModFrequencyLow;
                u8 mModFrequencyHigh;
            };
        };

        f64 mModPhase;
        s8 mModCounter;
        u8 mModWriteIndex;
        u8 mIsModHalted;
        s8 mModTable[64];
        s8 mModPosition;
        u8 _dc[4];
        u8 mModGain;
        u8 mModSpeed;
        u64 mModCounterClock;
        u8 mIsModEnvelope;
        u8 mIsModIncrease;
        u64 mModPeriod;
    };

    static_assert(__builtin_offsetof(_SQdMfm, mIsPlaying) == 0x8, "_SQdMfm::mIsPlaying");
    static_assert(__builtin_offsetof(_SQdMfm, mWave) == 0x20, "_SQdMfm::mWave");
    static_assert(__builtin_offsetof(_SQdMfm, mModFrequency) == 0x88, "_SQdMfm::mModFrequency");
    static_assert(__builtin_offsetof(_SQdMfm, mModGain) == 0xe0, "_SQdMfm::mModGain");
    static_assert(sizeof(_SQdMfm) == 0x100, "_SQdMfm size");

    /**
     * @brief Famicom Disk System drive: disk head position and timer IRQ.
     */
    struct _SQdDisk {
        void Reset(_SEntity* pEntity);
        void Pulse(s16 rate, u64 events, _SEntity* pEntity);
        void SetTimer(_SEntity* pEntity);
        u64 GetQdOffset();
        void SetQdOffset(u64 offset);
        void SetQdOffsetHalf(u64 offset);
        void IncQdOffsetHalf();
        void SetTimerStart(_SEntity* pEntity);
        void SetQdDirty(bool isDirty);
        void IncMediaCount();

        u64 mOffset;
        u8 mIsOffsetChanged;
        u16 mTimer;
        u8 mStatusMask;
        u8 mMediaCount;
        u8 mIsDirty;
        u8 mIsTimerActive;
    };

    static_assert(sizeof(_SQdDisk) == 0x10, "_SQdDisk size");

    CHvcSpu();
    ~CHvcSpu() override;
    int _Prologue() override;
    int _Epilogue() override;
    int Access(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    int Process(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    u64 _SerializeCore(void* pBuffer, bool isImport);
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;

    /// Content tag of the serialized state.
    u8 _SerializeTagContent() override { return 5; }

    /// Species tag of the serialized state.
    u8 _SerializeTagSpecies() override { return 0; }

    /// Version of the serialized state.
    u16 _SerializeTagVersion() override { return 0x100; }

    /// Comment stored with the serialized state.
    const char* _SerializeTagComment() override { return "SPU    "; }

private:
    ALWAYS_INLINE void resetChannels();

public:
    u16 mChannelFlags;
    u64 mClock;
    u64 mSampleClock;
    u64 mEvents;
    s64 mSample;
    s64 mLastSample;
    _SWavwR mSquare[2];
    _SWavwT mTriangle;
    _SNoise mNoise;
    _SDelta mDelta;
    _SQdMfm mQdMfm;
    _SWavwR mExSquare[2];
    _SWavwT mExWave;
    _SQdDisk mQdDisk;
};

static_assert(__builtin_offsetof(CHvcSpu, mSquare) == 0x78, "CHvcSpu::mSquare");
static_assert(__builtin_offsetof(CHvcSpu, mQdMfm) == 0x180, "CHvcSpu::mQdMfm");
static_assert(__builtin_offsetof(CHvcSpu, mQdDisk) == 0x320, "CHvcSpu::mQdDisk");
static_assert(sizeof(CHvcSpu) == 0x330, "CHvcSpu size");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
