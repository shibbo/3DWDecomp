#pragma once
#include <nn/atk/atk_Channel.h>
#include <nn/atk/atk_CurveLfo.h>
#include <nn/atk/atk_InstancePool.h>
#include <nn/atk/atk_OutputParam.h>

namespace nn::atk {
class OutputReceiver;
enum SequenceMute : int {
    SequenceMute_Off,
    SequenceMute_NoStop,
    SequenceMute_Release,
    SequenceMute_Stop,
};
}  // namespace nn::atk
namespace nn::atk::detail {
class OutputAdditionalParam;
}  // namespace nn::atk::detail
namespace nn::atk::detail::driver {
class SequenceTrack;
class Channel;
struct NoteOnInfo;
template <typename T> struct MmlMoveValue {
    T origin, target;
    s16 frame, counter;
    /** @brief Creates a zeroed, finished transition. */
    MmlMoveValue() : origin(0), target(0), frame(0), counter(0) {}
    /** @brief Creates a finished transition resting at a value. @param value Origin and target. */
    explicit MmlMoveValue(T value) : origin(value), target(value), frame(0), counter(0) {}
    /** @brief Jumps to a value with no transition. @param value Value used as origin and target. */
    void InitValue(T value) {
        origin = value;
        target = value;
        frame = 0;
        counter = 0;
    }
    /** @brief Advances the transition by one tick, saturating at its end. */
    void Update() {
        if (counter < frame) {
            ++counter;
        }
    }
    T GetValue() const {
        return counter >= frame ? target : static_cast<T>(origin + (target - origin) * counter / frame);
    }
    // value is the new target and frames is the interpolation duration.
    void Set(T value, int frames) {
        origin = GetValue(); target = value; frame = frames; counter = 0;
    }
};
class SequenceSoundPlayer;
class MmlSequenceTrack;
class MmlParser {
public:
    enum SeqArgType { SeqArgType_None, SeqArgType_Byte, SeqArgType_Short, SeqArgType_VariableLength,
                      SeqArgType_Random, SeqArgType_Variable };
    static bool mPrintVarEnabledFlag;
    virtual ~MmlParser();
    virtual void CommandProc(MmlSequenceTrack* track, u32 command, int first, int second) const;
    virtual Channel* NoteOnCommandProc(MmlSequenceTrack* track, int key, int velocity, int length, bool tie) const;
    int Parse(MmlSequenceTrack* track, bool playNotes) const;
    int ReadArg(const u8** position, SequenceSoundPlayer* player, SequenceTrack* track, SeqArgType type) const;
    u32 Read24(const u8** position) const;
    u16 Read16(const u8** position) const;
    u32 ReadVar(const u8** position) const;
    vs16* GetVariablePtr(SequenceSoundPlayer* player, SequenceTrack* track, int index) const;
    static u32 ParseAllocTrack(const void* data, u32 offset, u32* trackMask);
};
class SequenceTrack {
public:
    SequenceTrack();
    virtual ~SequenceTrack();
    virtual int Parse(bool playNotes) = 0;
    s16* GetVariablePtr(int index);
    Channel* NoteOn(int key, int velocity, int length, bool tie);
    void Close();
    void Open();
    void SetSeqData(const void* data, int offset);
    void ReleaseAllChannel(int release);
    void FreeAllChannel();
    void SetMute(SequenceMute mute);
    // player owns the track; null detaches it before returning its storage to the pool.
    void SetPlayer(SequenceSoundPlayer* player) { mPlayer = player; }
    void SetPlayerTrackNo(int playerTrackNo);
    void InitParam();
    void UpdateChannelLength();
    void UpdateChannelRelease(Channel* pChannel);
    int ParseNextTick(bool doNoteOn);
    void StopAllChannel();
    void UpdateChannelParam();
    void PauseAllChannel(bool isPause);
    void AddChannel(Channel* pChannel);
    int GetChannelCount() const;
    static void ChannelCallbackFunc(Channel* pDropChannel, Channel::ChannelCallbackStatus status,
                                    void* pUserData);
    void ForceMute();
    void SetSilence(bool isSilence, int fadeTimes);
    void SetBiquadFilter(int type, float value);
    void SetBankIndex(int bankIndex);
    void SetTranspose(s8 transpose);
    void SetVelocityRange(u8 velocityRange);
    void SetOutputLine(int outputLine);
    void SetTvMixParameter(u32 srcChannel, int dstChannel, float param);
    s16 GetTrackVariable(int index) const;
    void SetTrackVariable(int index, s16 value);

    /** @brief Tests whether the track is still playing its sequence. @return Whether the track is open. */
    bool IsOpened() const { return mOpenFlag; }
    /** @brief Sets the volume applied on top of the sequence's own. @param volume Linear gain. */
    void SetExtVolume(float volume) { mExtVolume = volume; }
    /** @brief Sets the pitch applied on top of the sequence's own. @param pitch Frequency ratio. */
    void SetExtPitch(float pitch) { mExtPitch = pitch; }
    /** @brief Sets the low-pass filter cutoff. @param lpfFreq Relative cutoff. */
    void SetLpfFreq(float lpfFreq) { mParamD8 = lpfFreq; }
    /** @brief Gets the TV output parameters. @return Mutable TV output parameters. */
    OutputParam& GetTvParam() { return mTvParam; }
    /** @brief Gets the start of the sequence data being parsed. @return Sequence data, or nullptr. */
    const u8* GetSequenceData() const { return mContext.mSequenceData; }
    /** @brief Gets the compare flag set by sequence conditionals. @return Compare flag. */
    bool GetCmpFlag() const { return mContext.mCondition; }
    /** @brief Sets the compare flag tested by sequence conditionals. @param flag Compare flag. */
    void SetCmpFlag(bool flag) { mContext.mCondition = flag; }
    /** @brief Gets the first channel the track is playing. @return Channel list head, or nullptr. */
    Channel* GetChannelList() const { return mChannelList; }

    static const int VariableCount = 16;
    static const int LfoCount = 4;
private:
    friend class MmlParser;
    u8 mPlayerTrackNo;
    bool mOpenFlag;
    bool mForceMute;
    u8 _b;
    float mExtVolume;
    float mExtPitch;
    float mPanRange;
    OutputParam mTvParam;
    struct SequenceContext {
        const u8* mSequenceData;
        const u8* mPosition;
        bool mCondition;
        bool mNoteWait;
        bool mTie;
        bool mMono;
        u8 _7c[4];
        struct StackEntry { bool isLoop; u8 count; const u8* position; };
        StackEntry mStack[10];
        u8 mStackDepth;
        bool mParamBF;
        bool mMuted;
        bool mSilence;
        int mWait;
        // Push the current cursor as a return address for a sequence subroutine.
        void PushCall() {
            const u8* position = mPosition;
            auto& entry = mStack[mStackDepth];
            entry.isLoop = false;
            entry.position = position;
            ++mStackDepth;
        }
    } mContext;
    bool mWaitForNote;
    bool mPortamento;
    bool mParamDF;
    u8 mParamB6;
    u32 mProgram;
    float mPitchSweep;
    MmlMoveValue<u8> mVolume;
    MmlMoveValue<u8> mVolume2;
    MmlMoveValue<s8> mPan;
    MmlMoveValue<s8> mSurroundPan;
    MmlMoveValue<s8> mPitchBend;
    u8 _152[2];
    struct LfoParam {
        float depth, speed;
        int delay;
        u8 type, target, range, reserved;

        /** @brief Creates the parameters with the curve LFO defaults. */
        LfoParam() { AsCurveParam().Initialize(); }
        /** @brief Views these parameters as the curve LFO block they mirror. @return Curve view. */
        CurveLfoParam& AsCurveParam() { return *reinterpret_cast<CurveLfoParam*>(this); }
        /** @brief Views these parameters as the curve LFO block they mirror. @return Curve view. */
        const CurveLfoParam& AsCurveParam() const {
            return *reinterpret_cast<const CurveLfoParam*>(this);
        }
    };
    LfoParam mLfo[4];
    u8 mLfoShape[4];
    u8 mParamB3;
    u8 mBendRange;
    s8 mParamDC;
    u8 _19b;
    s8 mTranspose;
    u8 mPriority;
    u8 mPortamentoKey;
    u8 mPortamentoTime;
    s8 mAttack, mDecay, mSustain, mRelease;
    s16 mHold;
    u8 mParamB4;
    u8 mParamDB;
    u8 mParamD9;
    u8 mParamDA;
    u8 mParamDE;
    u8 _1ab;
    float mParamD8;
    float mParamB5;
    int mOutputLine;
    s16 mTrackVariable[VariableCount];
    SequenceSoundPlayer* mPlayer;
    Channel* mChannelList;
};
class MmlSequenceTrack : public SequenceTrack {
public:
    MmlSequenceTrack();
    int Parse(bool playNotes) override;
    // parser interprets this track's sequence commands.
    void SetParser(const MmlParser* parser) { mParser = parser; }
private:
    const MmlParser* mParser;
};
class SequenceTrackAllocator {
public:
    virtual ~SequenceTrackAllocator() {}
    virtual SequenceTrack* AllocTrack(SequenceSoundPlayer* player) = 0;
    virtual void FreeTrack(SequenceTrack* track) = 0;
    virtual int GetAllocatableTrackCount() const = 0;
};
class MmlSequenceTrackAllocator : public SequenceTrackAllocator {
public:
    SequenceTrack* AllocTrack(SequenceSoundPlayer* player) override;
    void FreeTrack(SequenceTrack* track) override;
    int GetAllocatableTrackCount() const override;
    int Create(void* memory, size_t size);
    void Destroy();
private:
    const MmlParser* mParser;
    PoolImpl mPool;
};
static_assert(sizeof(SequenceTrack) == 0x1e8, "Sequence track size");
static_assert(sizeof(MmlSequenceTrack) == 0x1f0, "MML sequence track size");
static_assert(sizeof(MmlSequenceTrackAllocator) == 0x28, "MML track allocator size");
}

// SequenceSoundPlayer needs MmlMoveValue and SequenceTrackAllocator from this header.
#include <nn/atk/detail/seq/atk_SequenceSoundPlayer.h>
