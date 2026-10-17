#include "Project/Controller/WaveVibrationHolder.hpp"

#include "Library/Controller/GamePadSystem.hpp"
#include "Library/Controller/NpadController.hpp"
#include <controller/seadControllerMgr.h>
#include "Library/File/FileUtil.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include <nn/hid.h>

#include <attributes.h>

/** @brief Sorts vibration resources by name using the library shaker sort. */
template <>
inline NOINLINE void sead::PtrArray<al::WaveVibrationData>::sort() {
    if (mPtrNum < 2) {
        return;
    }

    al::WaveVibrationData** pPtrs = reinterpret_cast<al::WaveVibrationData**>(mPtrs);
    s32 lo = 0;
    s32 hi = mPtrNum - 1;

    while (lo < hi) {
        s32 last = lo;

        for (s64 i = lo; i < hi; i++) {
            if (compareT(pPtrs[i], pPtrs[i + 1]) > 0) {
                al::WaveVibrationData* pTemp = pPtrs[i + 1];
                pPtrs[i + 1] = pPtrs[i];
                pPtrs[i] = pTemp;
                last = i;
            }
        }

        hi = last;
        if (hi <= lo) {
            break;
        }

        for (s64 i = hi; i > lo; i--) {
            if (compareT(pPtrs[i], pPtrs[i - 1]) < 0) {
                al::WaveVibrationData* pTemp = pPtrs[i - 1];
                pPtrs[i - 1] = pPtrs[i];
                pPtrs[i] = pTemp;
                last = i;
            }
        }

        if (last == hi) {
            break;
        }

        lo = last;
    }
}

namespace {
/** @brief Gets a port's controller when it is an Npad controller.
 * @param port Controller manager index.
 * @return Npad controller, or nullptr for a missing or different controller type.
 */
inline al::NpadController* getNpad(s32 port) {
    return sead::DynamicCast<al::NpadController>(sead::ControllerMgr::instance()->getController(port));
}

/** @brief Changes the amplitude scaling of one direct vibration output.
 * @param pConnection Connection to the output mixer.
 * @param pWriter Source vibration writer.
 * @param volume Amplitude multiplier for both frequency bands.
 */
void setDirectVolume(nn::hid::VibrationNodeConnection* pConnection,
                     nn::hid::VibrationWriter* pWriter, f32 volume) {
    nn::hid::VibrationModulation modulation;
    modulation.frequencyLow = 1.0f;
    modulation.frequencyHigh = 1.0f;
    modulation.amplitudeLow = volume;
    modulation.amplitudeHigh = volume;
    pWriter->SetModulationTo(pConnection->GetDestination(), modulation);
}
}

namespace al {
/** @brief Creates vibration outputs and playback slots for the controller ports.
 * @param pGamePadSystem Optional source of the active player count.
 */
WaveVibrationHolder::WaveVibrationHolder(const GamePadSystem* pGamePadSystem)
    : mGamePadSystem(pGamePadSystem) {
    for (s32 port = 0; port < mPortCount; port++) {
        auto* pOutput = new OutputInfo;
        s32 deviceNum = -1;
        if (getNpad(port)) {
            deviceNum = getNpad(port)->getVibrationDeviceNum();
            if (deviceNum >= 1) {
                pOutput->pLeftHandle = &getNpad(port)->getVibrationDeviceHandle(0);
            }
            if (deviceNum >= 2) {
                pOutput->pRightHandle = &getNpad(port)->getVibrationDeviceHandle(1);
            }
        }
        pOutput->pLeftTarget = new nn::hid::VibrationTarget;
        pOutput->pRightTarget = new nn::hid::VibrationTarget;
        if (pOutput->pLeftHandle) {
            pOutput->pLeftTarget->SetVibrationDeviceHandle(*pOutput->pLeftHandle);
        }
        if (pOutput->pRightHandle) {
            pOutput->pRightTarget->SetVibrationDeviceHandle(*pOutput->pRightHandle);
        }
        pOutput->pLeftMixer = new nn::hid::VibrationMixer;
        pOutput->pRightMixer = new nn::hid::VibrationMixer;
        pOutput->pMonoMixer = new nn::hid::VibrationMixer;
        pOutput->pLeftOutput = new nn::hid::VibrationNodeConnection;
        pOutput->pRightOutput = new nn::hid::VibrationNodeConnection;
        pOutput->pLeftToMono = new nn::hid::VibrationNodeConnection(pOutput->pLeftMixer, pOutput->pMonoMixer);
        pOutput->pRightToMono = new nn::hid::VibrationNodeConnection(pOutput->pRightMixer, pOutput->pMonoMixer);
        pOutput->pMonoOutput = new nn::hid::VibrationNodeConnection;
        if (deviceNum == 1) {
            pOutput->pMonoOutput->Connect(pOutput->pMonoMixer, pOutput->pLeftTarget);
        } else if (deviceNum >= 2) {
            pOutput->pLeftOutput->Connect(pOutput->pLeftMixer, pOutput->pLeftTarget);
            pOutput->pRightOutput->Connect(pOutput->pRightMixer, pOutput->pRightTarget);
        }
        pOutput->deviceNum = deviceNum;
        pOutput->pLeftMixer->SetMixMode(nn::hid::VibrationMixMode_Max);
        pOutput->pRightMixer->SetMixMode(nn::hid::VibrationMixMode_Max);
        mOutputs.pushBack(pOutput);
        auto* pDirect = new DirectPlayer{};
        pDirect->pWriter = new nn::hid::VibrationWriter;
        pDirect->pLeft = new nn::hid::VibrationNodeConnection(pDirect->pWriter, pOutput->pLeftMixer);
        pDirect->pRight = new nn::hid::VibrationNodeConnection(pDirect->pWriter, pOutput->pRightMixer);
        mDirectPlayers.pushBack(pDirect);
    }
    mPlayers = new WaveVibrationPlayer**[mPortCount];
    mStyles = new s32[mPortCount];
    for (s32 port = 0; port < mPortCount; port++) {
        mStyles[port] = 5;
        mPlayers[port] = new WaveVibrationPlayer*[8];
        for (s32 i = 0; i < 8; i++) {
            mPlayers[port][i] = new WaveVibrationPlayer(mOutputs(port)->pLeftMixer,
                                                       mOutputs(port)->pRightMixer);
        }
    }
    loadData();
}

/** @brief Updates playing effects and reconnects outputs after controller changes. */
void WaveVibrationHolder::update() {
    for (s32 port = 0; port < mPortCount; port++) {
        for (s32 i = 0; i < 8; i++) {
            if (mPlayers[port][i]->isPlaying()) {
                mPlayers[port][i]->update();
            }
        }
    }
    if (mGamePadSystem) {
        mMaxPort = mGamePadSystem->getMaxPlayerNum();
    }
    nn::hid::VibrationValue value;
    value.mAmplitudeLow = 0.0f;
    value.mFrequencyLow = 160.0f;
    value.mAmplitudeHigh = 0.0f;
    value.mFrequencyHigh = 320.0f;
    for (s32 port = 0; port < mPortCount; port++) {
        auto* pOutput = mOutputs[port];
        if (port > mMaxPort) {
            auto* pOldLeft = pOutput->pLeftHandle;
            auto* pOldRight = pOutput->pRightHandle;
            if (pOutput->pLeftTarget->IsVibrationDeviceHandleSet()) {
                pOutput->pLeftTarget->UnsetVibrationDeviceHandle();
                pOutput->pLeftHandle = nullptr;
            }
            if (pOutput->pRightTarget->IsVibrationDeviceHandleSet()) {
                pOutput->pRightTarget->UnsetVibrationDeviceHandle();
                pOutput->pRightHandle = nullptr;
            }
            pOutput->deviceNum = 0;
            if (pOldLeft) {
                nn::hid::SendVibrationValue(*pOldLeft, value);
            }
            if (pOldRight) {
                nn::hid::SendVibrationValue(*pOldRight, value);
            }
            continue;
        }
        if (!getNpad(port)) {
            continue;
        }
        s32 deviceNum = getNpad(port)->getVibrationDeviceNum();
        if (getNpad(port)->getStyle() == mStyles[port] && deviceNum == pOutput->deviceNum) {
            if (deviceNum == 0) {
                continue;
            }
            if (deviceNum == 1 && pOutput->pLeftHandle == &getNpad(port)->getVibrationDeviceHandle(0)) {
                continue;
            }
            if (deviceNum == 2 && pOutput->pLeftHandle == &getNpad(port)->getVibrationDeviceHandle(0) &&
                pOutput->pRightHandle == &getNpad(port)->getVibrationDeviceHandle(1)) {
                continue;
            }
        }
        mStyles[port] = getNpad(port)->getStyle();
        if (deviceNum == 1) {
            auto* pOldLeft = pOutput->pLeftHandle;
            auto* pOldRight = pOutput->pRightHandle;
            pOutput->pLeftHandle = &getNpad(port)->getVibrationDeviceHandle(0);
            pOutput->pLeftTarget->SetVibrationDeviceHandle(*pOutput->pLeftHandle);
            if (pOutput->pRightTarget && pOutput->pRightTarget->IsVibrationDeviceHandleSet()) {
                pOutput->pRightTarget->UnsetVibrationDeviceHandle();
            }
            pOutput->pRightHandle = nullptr;
            pOutput->pLeftOutput->Disconnect();
            pOutput->pRightOutput->Disconnect();
            pOutput->pMonoOutput->Connect(pOutput->pMonoMixer, pOutput->pLeftTarget);
            if (pOldLeft) {
                nn::hid::SendVibrationValue(*pOldLeft, value);
            }
            if (pOldRight) {
                nn::hid::SendVibrationValue(*pOldRight, value);
            }
        } else if (deviceNum >= 2) {
            auto* pOldLeft = pOutput->pLeftHandle;
            auto* pOldRight = pOutput->pRightHandle;
            pOutput->pLeftHandle = &getNpad(port)->getVibrationDeviceHandle(0);
            pOutput->pRightHandle = &getNpad(port)->getVibrationDeviceHandle(1);
            pOutput->pLeftTarget->SetVibrationDeviceHandle(*pOutput->pLeftHandle);
            pOutput->pRightTarget->SetVibrationDeviceHandle(*pOutput->pRightHandle);
            pOutput->pMonoOutput->Disconnect();
            pOutput->pLeftOutput->Connect(pOutput->pLeftMixer, pOutput->pLeftTarget);
            pOutput->pRightOutput->Connect(pOutput->pRightMixer, pOutput->pRightTarget);
            if (pOldLeft) {
                nn::hid::SendVibrationValue(*pOldLeft, value);
            }
            if (pOldRight) {
                nn::hid::SendVibrationValue(*pOldRight, value);
            }
        }
        pOutput->deviceNum = deviceNum;
    }
}

/** @brief Loads and sorts the wave resources in the system vibration archive. */
void WaveVibrationHolder::loadData() {
    if (!isExistArchive("SystemData/VibrationWaves")) {
        return;
    }
    auto* pResource = findOrCreateResource("SystemData/VibrationWaves", nullptr);
    s32 count = pResource->getEntryNum("/");
    if (count > 0) {
        mData.allocBuffer(count, nullptr);
        for (s32 i = 0; i < mData.capacity(); i++) {
            mData.pushBack(new WaveVibrationData);
            StringTmp<128> name;
            pResource->getEntryName(&name, "/", i);
            mData(i)->data = pResource->getOtherFile(name, nullptr);
            mData(i)->size = pResource->getFileSize(name);
            name.removeSuffix(".bnvib");
            mData(i)->name = createStringIfInStack(name.cstr());
        }
    }
    mData.sort();
}

/** @brief Finds a loaded vibration resource by its name.
 * @param pName Exact resource name without the file extension.
 * @return Matching resource, or nullptr if no resource has that name.
 */
const WaveVibrationData* WaveVibrationHolder::findWaveVibrationData(const char* pName) const {
    for (s32 i = 0; i < mData.size(); i++) {
        if (isEqualString(pName, mData(i)->name)) {
            return mData[i];
        }
    }
    return nullptr;
}

/** @brief Chooses a free playback slot, falling back to the oldest playing slot.
 * @param port Controller index; a negative value has no playable slot.
 * @return Available player, or nullptr for a negative controller index.
 */
WaveVibrationPlayer* WaveVibrationHolder::findPlayableVibrationPlayer(s32 port) const {
    if (port < 0) {
        return nullptr;
    }
    for (s32 i = 0; i < 8; i++) {
        if (!mPlayers[port][i]->isPlaying() &&
            !(mPlayers[port][i]->mPaused && !mPlayers[port][i]->mPauseLocked)) {
            return mPlayers[port][i];
        }
    }
    s32 oldestFrame = 0;
    s32 oldest = 0;
    for (s32 i = 0; i < 8; i++) {
        s32 frame = mPlayers[port][i]->mFrame;
        bool isOlder = frame > oldestFrame;
        oldest = isOlder ? i : oldest;
        oldestFrame = isOlder ? frame : oldestFrame;
    }
    return mPlayers[port][oldest];
}

/** @brief Maps a controller port to its vibration index.
 * @param port Controller port, passed through unchanged.
 * @return The supplied controller port.
 */
s32 WaveVibrationHolder::findId(s32 port) const {
    return port;
}

/** @brief Reads the priority of the first playing slot with a resource.
 * @param port Controller index; negative indices have no active priority.
 * @param pPriority Receives the priority only when a playing resource is found.
 * @return Whether a playing resource was found.
 */
bool WaveVibrationHolder::getPlayingPriority(s32 port, s32* pPriority) const {
    if (port < 0) {
        return false;
    }
    for (s32 i = 0; i < 8; i++) {
        if (mPlayers[port][i]->isPlaying() && mPlayers[port][i]->mData) {
            *pPriority = mPlayers[port][i]->mPriority;
            return true;
        }
    }
    return false;
}

/** @brief Stops direct output and all playing or paused wave effects. */
void WaveVibrationHolder::stopAll() {
    nn::hid::VibrationValue value;
    value.mAmplitudeLow = 0.0f;
    value.mFrequencyLow = 160.0f;
    value.mAmplitudeHigh = 0.0f;
    value.mFrequencyHigh = 320.0f;
    for (s32 port = 0; port <= mMaxPort; port++) {
        mDirectPlayers(port)->pWriter->Write(value);
        for (s32 i = 0; i < 8; i++) {
            auto* pPlayer = mPlayers[port][i];
            if (pPlayer->mPaused && !pPlayer->mPauseLocked) {
                pPlayer->endPause(false);
            } else if (pPlayer->mPauseLocked) {
                pPlayer->endPause(true);
            }
            if (pPlayer->isPlaying()) {
                pPlayer->stop();
            }
        }
    }
}

/** @brief Stops non-looping wave effects, releasing any pause they hold. */
void WaveVibrationHolder::stopAllOneShot() {
    for (s32 port = 0; port <= mMaxPort; port++) {
        for (s32 i = 0; i < 8; i++) {
            auto* pPlayer = mPlayers[port][i];
            if (pPlayer->isLoop()) {
                continue;
            }
            if (pPlayer->mPaused && !pPlayer->mPauseLocked) {
                pPlayer->endPause(false);
            } else if (pPlayer->mPauseLocked) {
                pPlayer->endPause(true);
            }
            if (pPlayer->isPlaying()) {
                pPlayer->stop();
            }
        }
    }
}

/** @brief Gets the number of configured controller ports.
 * @return The game pad system's player limit, or one without a game pad system.
 */
s32 WaveVibrationHolder::getUsePadNum() const {
    return mGamePadSystem ? mGamePadSystem->getMaxPlayerNum() : 1;
}

/** @brief Pauses playing wave effects and silences all direct output. */
void WaveVibrationHolder::pause() {
    for (s32 port = 0; port <= mMaxPort; port++) {
        for (s32 i = 0; i < 8; i++) {
            auto* pPlayer = mPlayers[port][i];
            if (pPlayer->isPlaying()) {
                pPlayer->pause(false);
            }
        }
    }
    nn::hid::VibrationValue value;
    value.mAmplitudeLow = 0.0f;
    value.mFrequencyLow = 160.0f;
    value.mAmplitudeHigh = 0.0f;
    value.mFrequencyHigh = 320.0f;
    for (s32 i = 0; i < mDirectPlayers.size(); i++) {
        mDirectPlayers(i)->pWriter->Write(value);
    }
}

/** @brief Resumes effects held by an ordinary, unlocked pause. */
void WaveVibrationHolder::endPause() {
    for (s32 port = 0; port <= mMaxPort; port++) {
        for (s32 i = 0; i < 8; i++) {
            auto* pPlayer = mPlayers[port][i];
            if (pPlayer->mPaused && !pPlayer->mPauseLocked) {
                pPlayer->endPause(false);
            }
        }
    }
}

/** @brief Sends a vibration value through independently scaled left and right outputs.
 * @param port Controller index; negative indices are ignored.
 * @param lowFrequency Frequency of the low band.
 * @param highFrequency Frequency of the high band.
 * @param lowAmplitude Amplitude of the low band.
 * @param highAmplitude Amplitude of the high band.
 * @param leftVolume Amplitude multiplier for the left output.
 * @param rightVolume Amplitude multiplier for the right output.
 */
void WaveVibrationHolder::startVibrationDirectValue(s32 port, f32 lowFrequency,
    f32 highFrequency, f32 lowAmplitude, f32 highAmplitude, f32 leftVolume, f32 rightVolume) {
    if (port < 0) {
        return;
    }
    auto* pDirect = mDirectPlayers(port);
    setDirectVolume(pDirect->pLeft, pDirect->pWriter, leftVolume);
    setDirectVolume(pDirect->pRight, pDirect->pWriter, rightVolume);
    nn::hid::VibrationValue value;
    value.mFrequencyLow = lowFrequency;
    value.mFrequencyHigh = highFrequency;
    value.mAmplitudeLow = lowAmplitude;
    value.mAmplitudeHigh = highAmplitude;
    pDirect->pWriter->Write(value);
}

/** @brief Silences the direct vibration writer for a controller.
 * @param port Controller index; negative indices are ignored.
 */
void WaveVibrationHolder::stopVibrationDirectValue(s32 port) {
    if (port < 0) {
        return;
    }
    auto* pDirect = mDirectPlayers[port];
    nn::hid::VibrationValue value;
    value.mAmplitudeLow = 0.0f;
    value.mFrequencyLow = 160.0f;
    value.mAmplitudeHigh = 0.0f;
    value.mFrequencyHigh = 320.0f;
    pDirect->pWriter->Write(value);
}
/** @brief Processes the vibration graph once per audio frame. */
void WaveVibrationHolder::audioFrameProcess_() {
    nn::hid::VibrationNode::Update();
}
} // namespace al
