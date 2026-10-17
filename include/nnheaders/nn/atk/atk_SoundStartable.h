#pragma once

#include <nn/atk/atk_SoundArchive.h>
#include <nn/types.h>

namespace nn::atk {
class OutputReceiver;
class SoundHandle;

namespace detail {
class IRegionInfoReadable;

namespace driver {
class StreamBufferPool;
}  // namespace driver
}  // namespace detail

enum WaveType {
    WaveType_Nwwav,
    WaveType_Dspadpcm,
    WaveType_Opus,
};

enum VoiceRendererType {
    VoiceRendererType_Sdk,
};

enum UpdateType {
    UpdateType_AudioFrame,
    UpdateType_GameFrame,
};

enum StreamRegionCallbackResult {
    StreamRegionCallbackResult_Finish,
    StreamRegionCallbackResult_Continue,
};

struct StreamRegionCallbackParam {
    int regionNo;
    char regionName[64];
    bool isRegionNameEnabled;
    int regionCount;
    detail::IRegionInfoReadable* pRegionInfoReader;
};

typedef StreamRegionCallbackResult (*StreamRegionCallback)(StreamRegionCallbackParam* pParam, void* pArg);
typedef void (*SoundStopCallback)();

class SoundStartable {
public:
    class StartResult {
    public:
        enum ResultCode {
            ResultCode_Success = 0,
            ResultCode_ErrorLowPriority = 1,
            ResultCode_ErrorInvalidLabelString = 2,
            ResultCode_ErrorInvalidSoundId = 3,
            ResultCode_CanceledForSinglePlay = 4,
            ResultCode_ErrorNotAvailable = 12,
            ResultCode_ErrorNotEnoughInstance = 14,
            ResultCode_ErrorInvalidParameter = 15,
            ResultCode_ErrorInvalidStreamFilePath = 21,
            ResultCode_ErrorUser = 128,
            ResultCode_ErrorUnknown = 255
        };

        StartResult() : m_Code(ResultCode_ErrorUnknown) {}
        explicit StartResult(ResultCode code) : m_Code(code) {}

        bool IsSuccess() const { return m_Code == ResultCode_Success; }
        ResultCode GetCode() const { return m_Code; }

    private:
        ResultCode m_Code;
    };

    struct StartInfo {
        enum EnableFlagBit {
            EnableFlagBit_StartOffset = 1 << 0,
            EnableFlagBit_PlayerId = 1 << 1,
            EnableFlagBit_PlayerPriority = 1 << 2,
            EnableFlagBit_ActorPlayerId = 1 << 3,
            EnableFlagBit_SequenceSoundInfo = 1 << 4,
            EnableFlagBit_StreamSoundInfo = 1 << 5,
            EnableFlagBit_WaveSoundInfo = 1 << 6,
            EnableFlagBit_VoiceRendererType = 1 << 7,
            EnableFlagBit_FadeFrame = 1 << 8,
            EnableFlagBit_SoundStopCallback = 1 << 9,
            EnableFlagBit_StreamSoundMetaInfo = 1 << 10,
            EnableFlagBit_StreamSoundMetaInfo2 = 1 << 11,
            EnableFlagBit_DelayTime = 1 << 12,
            EnableFlagBit_DelayCount = 1 << 13,
            EnableFlagBit_UpdateType = 1 << 14,
            EnableFlagBit_SubMixIndex = 1 << 15,
            EnableFlagBit_OutputReceiver = 1 << 16,
            EnableFlagBit_LoopInfo = 1 << 17,
            EnableFlagBit_IsAdditionalDecodingOnLoopEnabled = 1 << 18,
        };

        enum StartOffsetType {
            StartOffsetType_MilliSeconds,
            StartOffsetType_Tick,
            StartOffsetType_Sample,
        };

        struct SequenceSoundInfo {
            SequenceSoundInfo() : sequenceDataAddress(nullptr), startLocationLabel(nullptr) {
                for (int i = 0; i < 4; i++) {
                    bankIds[i] = SoundArchive::InvalidId;
                }
            }

            const void* sequenceDataAddress;
            const char* startLocationLabel;
            SoundArchive::ItemId bankIds[4];
        };

        struct StreamSoundInfo {
            StreamSoundInfo()
                : externalPath(nullptr), pExternalData(nullptr), externalDataSize(0), regionCallback(nullptr),
                  regionCallbackArg(nullptr), prefetchData(nullptr), forcePlayPrefetchFlag(false),
                  pStreamBufferPool(nullptr) {}

            const char* externalPath;
            const void* pExternalData;
            size_t externalDataSize;
            StreamRegionCallback regionCallback;
            void* regionCallbackArg;
            const void* prefetchData;
            bool forcePlayPrefetchFlag;
            detail::driver::StreamBufferPool* pStreamBufferPool;
        };

        struct WaveSoundInfo {
            WaveSoundInfo()
                : waveAddress(nullptr), waveType(WaveType_Nwwav), enableParameterFlag(0), release(127),
                  isContextCalculationSkipMode(false) {}

            const void* waveAddress;
            s8 waveType;
            u8 m_Padding[3];
            int enableParameterFlag;
            int release;
            bool isContextCalculationSkipMode;
        };

        struct LoopInfo {
            LoopInfo() : enableParameterFlag(0), isLoopEnabled(false) {}

            u32 enableParameterFlag;
            bool isLoopEnabled;
        };

        StartInfo()
            : enableFlag(0), voiceRendererType(VoiceRendererType_Sdk), soundStopCallback(nullptr), delayTime(0),
              delayCount(0), updateType(UpdateType_AudioFrame), subMixIndex(0), pOutputReceiver(nullptr),
              isAdditionalDecodingOnLoopEnabled(true) {}

        u32 enableFlag;
        StartOffsetType startOffsetType;
        int startOffset;
        SoundArchive::ItemId playerId;
        int playerPriority;
        int actorPlayerId;
        SequenceSoundInfo sequenceSoundInfo;
        StreamSoundInfo streamSoundInfo;
        SoundArchive::StreamSoundInfo streamSoundMetaInfo;
        SoundArchive::StreamSoundInfo2 streamSoundMetaInfo2;
        WaveSoundInfo waveSoundInfo;
        LoopInfo loopInfo;
        u8 voiceRendererType;
        int fadeFrame;
        SoundStopCallback soundStopCallback;
        int delayTime;
        int delayCount;
        UpdateType updateType;
        int subMixIndex;
        OutputReceiver* pOutputReceiver;
        bool isAdditionalDecodingOnLoopEnabled;
    };

    virtual ~SoundStartable() {}

    StartResult StartSound(SoundHandle* pHandle, u32 soundId, const StartInfo* pStartInfo = nullptr);
    StartResult StartSound(SoundHandle* pHandle, const char* pSoundName,
                           const StartInfo* pStartInfo = nullptr);
    StartResult PrepareSound(SoundHandle* pHandle, u32 soundId, const StartInfo* pStartInfo = nullptr);
    StartResult PrepareSound(SoundHandle* pHandle, const char* pSoundName,
                             const StartInfo* pStartInfo = nullptr);
    StartResult HoldSound(SoundHandle* pHandle, u32 soundId, const StartInfo* pStartInfo = nullptr);
    StartResult HoldSound(SoundHandle* pHandle, const char* pSoundName,
                          const StartInfo* pStartInfo = nullptr);

private:
    virtual StartResult detail_SetupSound(SoundHandle* pHandle, u32 soundId, bool holdFlag,
                                          const char* pSoundArchiveName,
                                          const StartInfo* pStartInfo) = 0;
    virtual u32 detail_GetItemId(const char* pString) = 0;
    virtual u32 detail_GetItemId(const char* pString, const char* pSoundArchiveName) = 0;
};
static_assert(sizeof(SoundStartable::StartInfo) == 0x168);
}  // namespace nn::atk
