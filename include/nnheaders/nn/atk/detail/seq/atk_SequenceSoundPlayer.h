#pragma once

#include <nn/atk/atk_Bank.h>
#include <nn/atk/atk_BankFileReader.h>
#include <nn/atk/atk_BasicSound.h>
#include <nn/atk/atk_BasicSoundPlayer.h>
#include <nn/atk/atk_Global.h>
#include <nn/atk/atk_LoaderManager.h>
#include <nn/atk/atk_MmlSequenceTrack.h>
#include <nn/atk/atk_PlayerHeap.h>
#include <nn/atk/atk_PlayerHeapDataManager.h>
#include <nn/atk/atk_SoundDataManager.h>
#include <nn/atk/atk_SoundThread.h>
#include <nn/atk/atk_Task.h>
#include <nn/atk/atk_WaveArchiveFileReader.h>

namespace nn::atk {
class SoundPlayer;
class TaskProfileLogger;
struct SoundProfile;

/** @brief Variables handed to a sequence user procedure; it may change the compare flag. */
struct SequenceUserProcCallbackParam {
    vs16* localVariable;
    vs16* globalVariable;
    vs16* trackVariable;
    bool cmpFlag;
};
typedef void (*SequenceUserProcCallback)(u16 procId, SequenceUserProcCallbackParam* pParam,
                                         void* pArg);
}  // namespace nn::atk

namespace nn::atk::detail::driver {
class SequenceSoundPlayer;

/** @brief Turns a sequence note event into a playing channel. */
class NoteOnCallback {
public:
    /** @brief Destroys the callback. */
    virtual ~NoteOnCallback() = default;
    virtual Channel* NoteOn(SequenceSoundPlayer* pPlayer, u8 bankIndex,
                            const NoteOnInfo& rInfo) = 0;
};

/** @brief Loads a sequence, its banks and their wave archives into a player heap. */
class SequenceSoundLoader {
public:
    static const int BankCountMax = 4;

    /** @brief Archive items a sequence needs, referenced in place. */
    struct LoadInfo {
        LoadInfo(const SoundArchive* pArchive, const SoundDataManager* pDataManager,
                 LoadItemInfo* pSeqInfo, LoadItemInfo* pBankInfos, SoundPlayer* pSoundPlayer);

        const SoundArchive* pArchive;
        const SoundDataManager* pDataManager;
        LoadItemInfo* pSeqInfo;
        LoadItemInfo* pBankInfos[BankCountMax];
        SoundPlayer* pSoundPlayer;
    };

    /** @brief Archive context and the items to load. */
    struct Arg {
        /** @brief Creates an argument naming no archive or item. */
        Arg() : pArchive(nullptr), pDataManager(nullptr), pSoundPlayer(nullptr) {}

        const SoundArchive* pArchive;
        const SoundDataManager* pDataManager;
        SoundPlayer* pSoundPlayer;
        LoadItemInfo seqInfo;
        LoadItemInfo bankInfos[BankCountMax];
    };

    /** @brief Files resolved by a finished load. */
    struct Data {
        const void* seqFile;
        const void* bankFiles[BankCountMax];
        const void* warcFiles[BankCountMax];
        bool warcIsIndividuals[BankCountMax];
    };

    class DataLoadTask : public Task {
    public:
        /** @brief Releases the task's completion event through its base class. */
        ~DataLoadTask() override = default;
        void Initialize();
        bool TryAllocPlayerHeap();
        void Execute(TaskProfileLogger& rLogger) override;

        Arg arg;
        Data result;
        PlayerHeap* pHeap;
        PlayerHeapDataManager* pHeapDataManager;
        bool succeeded;
    };

    class FreePlayerHeapTask : public Task {
    public:
        /** @brief Releases the task's completion event through its base class. */
        ~FreePlayerHeapTask() override = default;
        void Initialize();
        void Execute(TaskProfileLogger& rLogger) override;

        Arg arg;
        PlayerHeap* pHeap;
        PlayerHeapDataManager* pHeapDataManager;
    };

    ~SequenceSoundLoader();
    void Initialize(const Arg& rArg);
    void Finalize();
    bool TryWait();
    bool IsInUse();

    /** @brief Tests whether the finished load found every file. @return Whether it succeeded. */
    bool IsLoadSuccess() const { return mLoadTask.succeeded; }
    /** @brief Gets the files resolved by the finished load. @return Loaded file addresses. */
    const Data& GetData() const { return mLoadTask.result; }

private:
    /** @brief Waits until both load and heap-release tasks finish using this loader. */
    void WaitTasks() {
        os::WaitEvent(&mLoadTask.mCompletionEvent);
        os::WaitEvent(&mFreeTask.mCompletionEvent);
    }

    DataLoadTask mLoadTask;
    FreePlayerHeapTask mFreeTask;
    PlayerHeapDataManager mHeapDataManager;

public:
    util::IntrusiveListNode mManagerLink;
};
static_assert(sizeof(SequenceSoundLoader::Arg) == 0x68, "SequenceSoundLoader argument size");
static_assert(sizeof(SequenceSoundLoader::DataLoadTask) == 0x118, "Sequence data load task size");
static_assert(sizeof(SequenceSoundLoader::FreePlayerHeapTask) == 0xc0,
              "Sequence heap release task size");
static_assert(sizeof(SequenceSoundLoader) == 0x4b0, "SequenceSoundLoader size");

typedef LoaderManager<SequenceSoundLoader> SequenceSoundLoaderManager;

/** @brief Driver-side player that parses a sequence and drives its tracks. */
class SequenceSoundPlayer : public BasicSoundPlayer,
                            public DisposeCallback,
                            public SoundThread::PlayerCallback {
public:
    static const int PlayerVariableCount = 16;
    static const int GlobalVariableCount = 16;
    static const int TrackCountPerPlayer = 16;
    static const int BankCountMax = SequenceSoundLoader::BankCountMax;
    static const int DefaultSkipIntervalTick = 16;

    enum StartOffsetType {
        StartOffsetType_Tick,
        StartOffsetType_MilliSeconds,
    };

    /** @brief Data-loading progress. */
    enum ResState : u8 {
        ResState_Invalid,
        ResState_ReceiveLoadRequest,
        ResState_AppendLoadTask,
        ResState_Assigned,
    };

    struct SetupArg {
        SequenceTrackAllocator* pTrackAllocator;
        u32 allocTracks;
        NoteOnCallback* pCallback;
    };

    /** @brief Files and start parameters of a loaded sequence. */
    struct PrepareArg {
        /** @brief Creates an argument with no files attached. */
        PrepareArg() : seqFile(nullptr), seqOffset(0), delayTime(0), delayCount(0) {
            for (int i = 0; i < BankCountMax; i++) {
                bankFiles[i] = nullptr;
                warcFiles[i] = nullptr;
                warcIsIndividuals[i] = false;
            }
        }

        const void* seqFile;
        const void* bankFiles[BankCountMax];
        const void* warcFiles[BankCountMax];
        bool warcIsIndividuals[BankCountMax];
        int seqOffset;
        u32 delayTime;
        int delayCount;
        UpdateType updateType;
    };

    /** @brief Start parameters kept while the sequence data loads. */
    struct StartInfo {
        int seqOffset;
        StartOffsetType startOffsetType;
        int startOffset;
        u32 delayTime;
        int delayCount;
        UpdateType updateType;
    };

    static void InitSequenceSoundPlayer();

    SequenceSoundPlayer();
    ~SequenceSoundPlayer() override;
    void Initialize(OutputReceiver* pReceiver) override;
    void Finalize() override;
    void Start() override;
    void Stop() override;
    void Pause(bool isPause) override;
    void InvalidateData(const void* pStart, const void* pEnd) override;
    /** @brief Hook run when one of the player's channels stops. @param pChannel Stopped channel. */
    virtual void ChannelCallback(Channel* pChannel) {}
    /** @brief Advances playback by one sound-thread frame. @param frame Elapsed time, in frames. */
    void OnUpdateFrameSoundThread(int frame) override { Update(frame); }
    /**
     * @brief Advances playback when the player runs at the audio-frame rate.
     * @param frame Elapsed time, in frames.
     */
    void OnUpdateFrameSoundThreadWithAudioFrameFrequency(int frame) override {
        if (mUpdateType == UpdateType_AudioFrame) {
            Update(frame);
        }
    }
    /** @brief Stops playback because the sound thread is going away. */
    void OnShutdownSoundThread() override { Stop(); }

    void FinishPlayer();
    void FreeLoader();
    void Setup(const SetupArg& rArg);
    void SetPlayerTrack(int trackNo, SequenceTrack* pTrack);
    void ForceTrackMute(u32 trackBitFlag);
    SequenceTrack* GetPlayerTrack(int trackNo);
    const SequenceTrack* GetPlayerTrack(int trackNo) const;
    void Skip(StartOffsetType offsetType, int offset);
    void SetTempoRatio(float tempoRatio);
    void SetPanRange(float panRange);
    void SetChannelPriority(int priority);
    void SetReleasePriorityFix(bool fix);
    void SetSequenceUserprocCallback(SequenceUserProcCallback callback, void* pArg);
    void CallSequenceUserprocCallback(u16 procId, SequenceTrack* pTrack);
    vs16* GetVariablePtr(int varNo);
    s16 GetLocalVariable(int varNo) const;
    static s16 GetGlobalVariable(int varNo);
    void SetLocalVariable(int varNo, s16 value);
    static void SetGlobalVariable(int varNo, s16 value);
    void SetTrackMute(u32 trackBitFlag, SequenceMute mute);
    void SetTrackSilence(unsigned long trackBitFlag, bool isSilence, int fadeTimes);
    void SetTrackVolume(u32 trackBitFlag, float volume);
    void SetTrackPitch(u32 trackBitFlag, float pitch);
    void SetTrackLpfFreq(u32 trackBitFlag, float lpfFreq);
    void SetTrackBiquadFilter(u32 trackBitFlag, int type, float value);
    bool SetTrackBankIndex(u32 trackBitFlag, int bankIndex);
    void SetTrackTranspose(u32 trackBitFlag, s8 transpose);
    void SetTrackVelocityRange(u32 trackBitFlag, u8 range);
    void SetTrackOutputLine(u32 trackBitFlag, u32 outputLine);
    void ResetTrackOutputLine(u32 trackBitFlag);
    void SetTrackTvVolume(u32 trackBitFlag, float volume);
    void SetTrackChannelTvMixParameter(u32 trackBitFlag, u32 srcChannel,
                                       const MixParameter& rParam);
    void SetTrackTvPan(u32 trackBitFlag, float pan);
    void SetTrackTvSurroundPan(u32 trackBitFlag, float span);
    void SetTrackTvMainSend(u32 trackBitFlag, float send);
    void SetTrackTvFxSend(u32 trackBitFlag, AuxBus bus, float send);
    void CloseTrack(int trackNo);
    void UpdateChannelParam();
    bool ParseNextTick(bool doNoteOn);
    void Update(int frame);
    bool TryAllocLoader();
    void PrepareForPlayerHeap(const PrepareArg& rArg);
    void SkipTick();
    void UpdateTick(int frame);
    Channel* NoteOn(u8 bankIndex, const NoteOnInfo& rInfo);
    void Prepare(const PrepareArg& rArg);
    void RequestLoad(const StartInfo& rInfo, const SequenceSoundLoader::Arg& rArg);
    u64 GetProcessTick(const SoundProfile& rProfile);
    void PrepareForMidi(const void** ppBankFiles, const void** ppWarcFiles,
                        bool* pWarcIsIndividuals);
    static void SetSkipIntervalTick(int tick);
    static int GetSkipIntervalTick();

    /**
     * @brief Sets the loader pool used for sequences whose data is not resident.
     * @param pManager Loader pool; must outlive the player's loads.
     */
    void SetLoaderManager(SequenceSoundLoaderManager* pManager) { mLoaderManager = pManager; }

private:
    friend class MmlParser;
    friend class SequenceTrack;

    /** @brief Gets the current tempo in ticks per millisecond. @return Ticks per millisecond. */
    float CalcTickPerMsec() const {
        return static_cast<float>(mParamB0 * mTempo) * mTempoRatio / 60000.0f;
    }

    static vs16 m_GlobalVariable[GlobalVariableCount];
    static int m_SkipIntervalTickPerFrame;

    bool mReleasePriorityFix;
    float mPanRange;
    float mTempoRatio;
    float mTickFraction;
    u32 mSkipTickCounter;
    float mSkipTimeCounter;
    int mDelayCount;
    u8 _10c[4];
    u8 mPriority;
    u8 mParamB0;  // Ticks per quarter note (timebase).
    u16 mTempo;
    MmlMoveValue<u8> mVolume;
    NoteOnCallback* mNoteOnCallback;
    SequenceTrackAllocator* mSequenceTrackAllocator;
    SequenceUserProcCallback mSequenceUserprocCallback;
    void* mSequenceUserprocCallbackArg;
    SequenceTrack* mTracks[TrackCountPerPlayer];
    vs16 mLocalVariable[PlayerVariableCount];
    u32 mTickCounter;
    WaveArchiveFileReader mWaveArchiveFileReader[BankCountMax];
    BankFileReader mBankFileReader[BankCountMax];
    ResState mResState;
    bool mIsInitialized;
    bool mIsRegisterPlayerCallback;
    StartInfo mStartInfo;
    SequenceSoundLoaderManager* mLoaderManager;
    SequenceSoundLoader* mLoader;
    SequenceSoundLoader::Arg mLoaderArg;
    UpdateType mUpdateType;
};
static_assert(sizeof(SequenceSoundPlayer) == 0x368, "SequenceSoundPlayer size");
}  // namespace nn::atk::detail::driver
