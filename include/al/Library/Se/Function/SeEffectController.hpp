#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveExecutor.hpp"
#include "Project/Audio/AudioBusInfo.hpp"

namespace sead {
class AudioFxDelayNin;
class AudioFxReverbStdNin;
class AudioFxReverbHiNin;
class AudioFxI3DL2ReverbNin;
}  // namespace sead

namespace al {
class AudioEffectDataBase;
class DspLinearValueController;
class SeadAudioPlayer;
class SeadFxDelayParams;
class SeadFxReverbStdParams;
class SeadFxReverbHiParams;
class SeadFxReverbI3Dl2Params;

template <typename Fx, typename Params>
class SoundFxUnit;
template <typename Unit>
struct SoundFxUnitTable;

typedef SoundFxUnit<sead::AudioFxDelayNin, SeadFxDelayParams> SoundFxDelayUnit;
typedef SoundFxUnit<sead::AudioFxReverbStdNin, SeadFxReverbStdParams> SoundFxReverbStdUnit;
typedef SoundFxUnit<sead::AudioFxReverbHiNin, SeadFxReverbHiParams> SoundFxReverbHiUnit;
typedef SoundFxUnit<sead::AudioFxI3DL2ReverbNin, SeadFxReverbI3Dl2Params> SoundFxReverbI3Dl2Unit;

/** @brief Builds, swaps and fades the sound-effect bus effects (delay, reverbs, low-pass). */
class SeEffectController : public NerveExecutor {
public:
    SeEffectController();

    void init(SeadAudioPlayer* pSePlayer, SeadAudioPlayer* pBgmPlayer, const AudioEffectDataBase* pDataBase);
    void stopAllBusFxSend();
    void createEffectUnit(const char* pName);
    void fullDisableClearAllEffects();
    void disableAllEffects();
    bool isEffectSystemDisabled();
    void clearLpf();
    void finalize();
    void releaseAllWorkBuffer();
    void update();
    void changeEffect(const char* pName);
    void appendEffectUnit(const char* pBusName, const SeEffectProcInfo* pInfo);
    SeadAudioPlayer* getSeadAudioPlayer(const char* pCategoryName);
    void appendEffects(const SeEffectInfo* pInfo);
    void setFxSend();

    void exeWait();
    void exeWaitFadeOutBusSend();
    void exeWaitEffectUnitDisable();
    void exePrepareRun();
    void exePrepareWait();
    void exeRun();
    void exeWaitDisableAllAudioEffectUnit();
    void exeWaitClearAllEffectFromBus();
    void exeFinishedFinalize();

    /** @brief Gets the active effect name. @return Effect name, or nullptr when no effect runs. */
    const char* getCurEffectName() const {
        if (mCurEffectInfo == nullptr) {
            return nullptr;
        }

        return mCurEffectInfo->mName;
    }

private:
    const SeEffectInfo* tryFindEffectInfo(const char* pName) const;

    SeadAudioPlayer* mSePlayer = nullptr;
    SeadAudioPlayer* mBgmPlayer = nullptr;
    const AudioEffectDataBase* mDataBase = nullptr;
    AudioInfoList<SeEffectBusInfo>* mBusInfoList = nullptr;
    const SeEffectInfo* mCurEffectInfo = nullptr;
    SoundFxUnitTable<SoundFxDelayUnit>* mDelayUnits = nullptr;
    SoundFxUnitTable<SoundFxReverbStdUnit>* mReverbStdUnits = nullptr;
    SoundFxUnitTable<SoundFxReverbHiUnit>* mReverbHiUnits = nullptr;
    SoundFxUnitTable<SoundFxReverbI3Dl2Unit>* mReverbI3Dl2Units = nullptr;
    const Nerve* mNextNerve = nullptr;
    DspLinearValueController* mFadeController = nullptr;
};

static_assert(sizeof(SeEffectController) == 0x68);
}  // namespace al
