#include "Library/Controller/PadRumbleDirector.hpp"

#include <algorithm>
#include <attributes.h>
#include <cmath>
#include <controller/seadControllerMgr.h>
#include <gfx/seadCamera.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <nerd/nerdMath.h>
#include <prim/seadSafeString.h>

#include "Library/Camera/CameraDirector.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/NpadController.hpp"
#include "Library/LiveActor/HitReactionKeeper.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/Action/Common/ActionPadAndCameraCtrl.hpp"
#include "Project/Action/Common/ActorActionKeeper.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Camera/Main/CameraDirector_RS.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Controller/WaveVibrationHolder.hpp"
#include "Project/Controller/WaveVibrationPlayer.hpp"

namespace al {
namespace {
typedef sead::PtrArray<PadRumbleData> PadRumbleDataArray;
typedef PadRumbleDirector::PlayerArray PlayerArray;

/**
 * Searches a pad rumble data list for an entry with the given name.
 * @param pData receives the entry
 * @param rDatas the data list
 * @param pName the name of the entry
 * @return true if an entry was found
 */
ALWAYS_INLINE bool tryFindData(const PadRumbleData** pData, const PadRumbleDataArray& rDatas,
                               const char* pName) {
    for (s32 i = 0; i < rDatas.size(); i++) {
        if (isEqualString(pName, rDatas.unsafeAt(i)->name)) {
            *pData = rDatas.at(i);
            return true;
        }
    }

    return false;
}

/**
 * Finds the wave vibration data of a pad rumble, applying the settings of its entry.
 * @param pVolume the volume, multiplied by the volume of the entry
 * @param pPriority receives the priority of the entry
 * @param pMinLength receives the minimum length of the entry
 * @param pHolder the wave vibration holder
 * @param pName the name of the pad rumble
 * @param pShowORDatas the one time entries shown in the option menu
 * @param pShowORLoopDatas the looping entries shown in the option menu
 * @param pDatas the other entries
 * @return the wave vibration data
 */
const WaveVibrationData* findWaveVibrationData(f32* pVolume, s32* pPriority, s32* pMinLength,
                                               const WaveVibrationHolder* pHolder,
                                               const char* pName,
                                               const PadRumbleDataArray* pShowORDatas,
                                               const PadRumbleDataArray* pShowORLoopDatas,
                                               const PadRumbleDataArray* pDatas) {
    const PadRumbleData* data = nullptr;

    if (!tryFindData(&data, *pShowORDatas, pName)) {
        if (!tryFindData(&data, *pShowORLoopDatas, pName)) {
            tryFindData(&data, *pDatas, pName);
        }
    }

    if (data == nullptr) {
        return pHolder->findWaveVibrationData(pName);
    }

    const WaveVibrationData* waveData = pHolder->findWaveVibrationData(data->dataName);
    *pVolume = data->volume * *pVolume;
    *pPriority = data->priority;
    *pMinLength = data->minLength;
    return waveData;
}

/**
 * Calculates the volume and the pan of a sound heard by an ear.
 * @param pVolume receives the volume
 * @param pPan receives the pan
 * @param pPitch the pitch, multiplied by the distance attenuation
 * @param rEarMtx the view matrix of the ear
 * @param x the x position of the sound
 * @param y the y position of the sound
 * @param z the z position of the sound
 * @param near the distance under which the sound is not attenuated
 * @param far the distance over which the sound is not heard
 * @param isLeft whether the ear is the left one
 * @param mode the attenuation mode
 */
void calcEarVolume(f32* pVolume, f32* pPan, f32* pPitch, const sead::Matrix34f& rEarMtx, f32 x,
                   f32 y, f32 z, f32 near, f32 far, bool isLeft, s32 mode) {
    sead::Vector3f local;
    local.setMul(rEarMtx, sead::Vector3f(x, y, z));

    f32 distance = nerd::sqrt(local.x * local.x + local.y * local.y + local.z * local.z);
    f32 distanceH = nerd::sqrt(local.x * local.x + local.z * local.z);
    f32 angle = 0.0f;
    f32 panRate = 0.0f;

    if (!isNearZero(distanceH, 0.001f)) {
        angle = sead::Mathf::rad2deg(acosf(sead::Mathf::clamp(-local.z / distanceH, -1.0f, 1.0f)));

        if (local.x < 0.0f) {
            angle = 360.0f - angle;
        }
    }

    f32 front = cosf(sead::Mathf::deg2rad(angle));
    f32 sideAngle = angle + 90.0f;

    if (sideAngle > 360.0f) {
        sideAngle -= 360.0f;
    }

    if (sideAngle > 180.0f) {
        sideAngle = 360.0f - sideAngle;
    }

    panRate = sead::Mathf::clamp(sideAngle / 180.0f, 0.0f, 1.0f);
    f32 panAngle = panRate * sead::Mathf::piHalf();

    f32 volume;
    if (mode == 0 || mode == 2) {
        f32 range = far - near;

        if (range <= 0.0f || distance <= near) {
            volume = 1.0f;
        } else {
            volume = powf(0.1f, (distance - near) / range);

            if (volume < 0.05f) {
                volume = 0.0f;
            }
        }
    } else if (mode == 3) {
        f32 range = far - near;

        if (range <= 0.0f || distance <= near) {
            volume = 1.0f;
        } else {
            volume = range / (range + (distance - near) * 9.0f);

            if (volume < 0.05f) {
                volume = 0.0f;
            }
        }
    } else {
        volume = std::max(1.0f - std::max(distance - near, 0.0f) / far - near, 0.0f);
    }

    f32 pan = isLeft ? cosf(panAngle) : sinf(panAngle);
    *pPan = sead::Mathf::abs(pan);
    *pVolume = volume;
    *pPitch = expf(front * (1.0f - volume) * 0.1f) * *pPitch;
}

/**
 * Moves a view matrix to the position of an ear.
 * @param pMtx the view matrix
 * @param rViewMtx the view matrix of the camera
 * @param rEarPos the position of the ear
 */
ALWAYS_INLINE void setEarTranslation(sead::Matrix34f* pMtx, const sead::Matrix34f& rViewMtx,
                                     const sead::Vector3f& rEarPos) {
    sead::Matrix34f rotate(rViewMtx);
    rotate.setTranslation(sead::Vector3f::zero);

    sead::Vector3f trans;
    trans.setMul(rotate, -rEarPos);
    pMtx->setTranslation(trans);
}

/**
 * Returns the scene camera info of a camera director.
 * @param pCameraDirector the camera director
 * @return the scene camera info
 */
ALWAYS_INLINE SceneCameraInfo* getCameraInfo(const CameraDirector* pCameraDirector) {
    return pCameraDirector->getSceneCameraInfo();
}

/**
 * Returns the scene camera info of a camera director.
 * @param pCameraDirector the camera director
 * @return the scene camera info
 */
ALWAYS_INLINE SceneCameraInfo* getCameraInfo(const CameraDirector_RS* pCameraDirector) {
    return pCameraDirector->getSceneCameraInfo();
}

/**
 * Calculates the position the rumbles of a player are felt from.
 * @param pPos receives the position of the listener
 * @param pCameraDirector the camera director
 * @param isCamera whether the rumbles are felt from the camera
 * @param pPlayerHolder the player holder
 * @param port the port of the player
 */
template <typename T>
ALWAYS_INLINE void calcListenerPos(sead::Vector3f* pPos, const T* pCameraDirector, bool isCamera,
                                   const PlayerHolder* pPlayerHolder, s32 port) {
    if (isCamera) {
        pPos->set(getCameraPos(getCameraInfo(pCameraDirector)));
        return;
    }

    LiveActor* player = tryFindAlivePlayerActorFromPort(pPlayerHolder, port);
    const sead::Vector3f& cameraPos = getCameraPos(getCameraInfo(pCameraDirector));

    if (player == nullptr) {
        pPos->set(cameraPos);
        return;
    }

    pPos->set(cameraPos * 0.05f + getTrans(player) * 0.95f);
}

/**
 * Calculates the volume and the pitch of a pad rumble from the ears of the listener.
 * @param pVolumeLeft receives the volume of the left side
 * @param pVolumeRight receives the volume of the right side
 * @param pPitchLeft the pitch of the left side
 * @param pPitchRight the pitch of the right side
 * @param near the distance under which the rumble is not attenuated
 * @param far the distance over which the rumble is not felt
 * @param rPos the position of the rumble
 * @param pCameraDirector the camera director
 * @param rListenerPos the position of the listener
 * @param mode the attenuation mode
 */
template <typename T>
ALWAYS_INLINE void calcVolumeImpl(f32* pVolumeLeft, f32* pVolumeRight, f32* pPitchLeft,
                                  f32* pPitchRight, f32 near, f32 far,
                                  const sead::Vector3f& rPos, const T* pCameraDirector,
                                  const sead::Vector3f& rListenerPos, s32 mode) {
    sead::Vector3f right;
    getCameraLookAtCamera(getCameraInfo(pCameraDirector))->getRightVectorByMatrix(&right);
    right *= 50.0f;

    const sead::Matrix34f& viewMtx = getCameraViewMtx(getCameraInfo(pCameraDirector));
    sead::Matrix34f mtxLeft(viewMtx);
    sead::Matrix34f mtxRight(viewMtx);
    setEarTranslation(&mtxLeft, viewMtx, rListenerPos - right);
    setEarTranslation(&mtxRight, viewMtx, rListenerPos + right);

    *pVolumeLeft = 1.0f;
    *pVolumeRight = 1.0f;

    f32 volumeLeft = 1.0f;
    f32 volumeRight = 1.0f;
    f32 panLeft = 1.0f;
    f32 panRight = 1.0f;
    calcEarVolume(&volumeLeft, &panLeft, pPitchLeft, mtxLeft, rPos.x, rPos.y, rPos.z, near, far,
                  true, mode);
    calcEarVolume(&volumeRight, &panRight, pPitchRight, mtxRight, rPos.x, rPos.y, rPos.z, near,
                  far, false, mode);

    f32 panSum = panLeft + panRight;
    f32 angle;
    if (isNearZero(panSum, 0.001f)) {
        angle = sead::Mathf::pi() / 4.0f;
    } else {
        angle = sead::Mathf::deg2rad(sead::Mathf::clamp(panRight / panSum, 0.2f, 0.8f) * 90.0f);
    }

    *pVolumeLeft = volumeLeft * cosf(angle);
    *pVolumeRight = volumeRight * sinf(angle);
}

/**
 * Calculates the volume and the pitch of a pad rumble from the ears of the listener.
 * @param pVolumeLeft receives the volume of the left side
 * @param pVolumeRight receives the volume of the right side
 * @param pPitchLeft the pitch of the left side
 * @param pPitchRight the pitch of the right side
 * @param near the distance under which the rumble is not attenuated
 * @param far the distance over which the rumble is not felt
 * @param rPos the position of the rumble
 * @param pCameraDirector the camera director
 * @param rListenerPos the position of the listener
 * @param mode the attenuation mode
 */
void calcVolume(f32* pVolumeLeft, f32* pVolumeRight, f32* pPitchLeft, f32* pPitchRight, f32 near,
                f32 far, const sead::Vector3f& rPos, const CameraDirector* pCameraDirector,
                const sead::Vector3f& rListenerPos, s32 mode) {
    calcVolumeImpl(pVolumeLeft, pVolumeRight, pPitchLeft, pPitchRight, near, far, rPos,
                   pCameraDirector, rListenerPos, mode);
}

/**
 * Calculates the volume and the pitch of a pad rumble from the ears of the listener.
 * @param pVolumeLeft receives the volume of the left side
 * @param pVolumeRight receives the volume of the right side
 * @param pPitchLeft the pitch of the left side
 * @param pPitchRight the pitch of the right side
 * @param near the distance under which the rumble is not attenuated
 * @param far the distance over which the rumble is not felt
 * @param rPos the position of the rumble
 * @param pCameraDirector the camera director
 * @param rListenerPos the position of the listener
 * @param mode the attenuation mode
 */
void calcVolume(f32* pVolumeLeft, f32* pVolumeRight, f32* pPitchLeft, f32* pPitchRight, f32 near,
                f32 far, const sead::Vector3f& rPos, const CameraDirector_RS* pCameraDirector,
                const sead::Vector3f& rListenerPos, s32 mode) {
    calcVolumeImpl(pVolumeLeft, pVolumeRight, pPitchLeft, pPitchRight, near, far, rPos,
                   pCameraDirector, rListenerPos, mode);
}

/**
 * Adjusts a rumble volume to the controller style of a port and to the power level.
 * @param volume the volume
 * @param powerLevel the power level (-1: weak, 0: normal, 1: strong)
 * @param isValid whether the pad rumbles are enabled
 * @param port the port of the controller
 * @return the adjusted volume
 */
NOINLINE f32 adjustVolume(f32 volume, s32 powerLevel, bool isValid, s32 port) {
    sead::Controller* controller = sead::ControllerMgr::instance()->getController(port);
    auto* npad = sead::DynamicCast<NpadController>(controller);
    sead::NinJoyNpadDevice::Style style =
        npad != nullptr ? npad->getStyle() : sead::NinJoyNpadDevice::cStyle_Invalid;

    f32 power = 0.0f;
    if (isValid) {
        power = powerLevel == 1 ? 1.8f : 1.0f;
        power = powerLevel == -1 ? 0.5f : power;
    }

    switch (style) {
    case sead::NinJoyNpadDevice::cStyle_FullKey:
        return sead::Mathf::min(power * volume * 1.4f, std::min(power, 1.0f));
    case sead::NinJoyNpadDevice::cStyle_Handheld:
        return sead::Mathf::min(power * volume * 1.3f,
                                sead::Mathf::clamp(power * 0.32f, 0.0f, 1.0f));
    case sead::NinJoyNpadDevice::cStyle_JoyDual:
        return sead::Mathf::min(power * volume, std::min(power, 1.0f));
    case sead::NinJoyNpadDevice::cStyle_JoyLeft:
    case sead::NinJoyNpadDevice::cStyle_JoyRight:
        return sead::Mathf::min(power * volume, power);
    default:
        return volume;
    }
}

/**
 * Checks if none of the players of a rumble is playing.
 * @param rPlayers the players of the rumble
 * @return true if none of the players is playing
 */
bool isDeadPlayers(const PlayerArray& rPlayers) {
    for (s32 i = 0; i < 5; i++) {
        if (rPlayers[i] != nullptr) {
            if (rPlayers[i]->isPlaying()) {
                return false;
            }

            if (rPlayers[i]->isPaused() && !rPlayers[i]->isPauseLocked()) {
                return false;
            }
        }
    }

    return true;
}
}  // namespace

/**
 * Sets the left and right volumes from a balance.
 * @param balance the balance (0: left, 1: right)
 */
void PadRumbleParam::setVolumeByBalance(f32 balance) {
    f32 angle = sead::Mathf::deg2rad(balance * 90.0f);
    volumeLeft = cosf(angle);
    volumeRight = sinf(angle);
}

/**
 * Starts playing a direct data.
 * @param pData the direct data
 */
void PadRumbleDirector::DirectPlayInfo::init(PadRumbleDirectData* pData) {
    data = pData;
    frame = 0;
    volumeLeft = 1.0f;
    isPaused = false;
    volumeRight = 1.0f;
}

/**
 * Constructs the pad rumble director of a scene using a CameraDirector.
 * @param pPlayerHolder the player holder
 * @param pCameraDirector the camera director
 */
PadRumbleDirector::PadRumbleDirector(const PlayerHolder* pPlayerHolder,
                                     const CameraDirector* pCameraDirector)
    : mWaveVibrationHolder(nullptr), mPlayerHolder(pPlayerHolder),
      mCameraDirector(pCameraDirector), mCameraDirectorRS(nullptr), mLoopInfos(nullptr),
      mListener(nullptr), mOneTimeInfos(nullptr), mDirectPlayInfos(nullptr) {
    sharedInit(false);
}

/**
 * Allocates the rumble lists and reads the pad rumble entries.
 * @param isUnused unused
 */
void PadRumbleDirector::sharedInit(bool isUnused) {
    mLoopInfos = new LoopInfo[80];
    mOneTimeInfos = new OneTimeInfo[100];
    mDirectPlayInfos = new DirectPlayInfo[16];

    for (s32 i = 0; i < 80; i++) {
        mLoopInfos[i] = LoopInfo();
        mLoopInfos[i].clearPlayers();
    }

    for (s32 i = 0; i < 100; i++) {
        mOneTimeInfos[i] = OneTimeInfo();
        mOneTimeInfos[i].clearPlayers();
    }

    mListener = new ListenerInfo[1];

    Resource* resource = findOrCreateResource("SystemData/VibrationWaves", nullptr);
    ByamlIter list(findResourceYaml(resource, "VibrationWaveList", nullptr));
    s32 size = list.getSize();

    s32 showORNum = 0;
    s32 showORLoopNum = 0;
    s32 dataNum = 0;
    s32 directNum = 0;
    for (s32 i = 0; i < size; i++) {
        ByamlIter iter;
        list.tryGetIterByIndex(&iter, i);

        if (tryGetByamlKeyBoolOrFalse(iter, "IsDirect")) {
            directNum++;
        } else if (!tryGetByamlKeyBoolOrFalse(iter, "IsShowOR")) {
            dataNum++;
        } else if (tryGetByamlKeyBoolOrFalse(iter, "IsLoop")) {
            showORLoopNum++;
        } else {
            showORNum++;
        }
    }

    if (showORNum > 0) {
        mShowORDatas.allocBuffer(showORNum, nullptr);
    }

    if (showORLoopNum > 0) {
        mShowORLoopDatas.allocBuffer(showORLoopNum, nullptr);
    }

    if (dataNum > 0) {
        mDatas.allocBuffer(dataNum, nullptr);
    }

    if (directNum > 0) {
        mDirectDatas.allocBuffer(directNum, nullptr);
    }

    for (s32 i = 0; i < size; i++) {
        ByamlIter iter;
        list.tryGetIterByIndex(&iter, i);

        if (tryGetByamlKeyBoolOrFalse(iter, "IsDirect")) {
            auto* data = new PadRumbleDirectData;
            data->name = tryGetByamlKeyStringOrNULL(iter, "Name");
            data->length = tryGetByamlKeyIntOrZero(iter, "Length");
            data->lowFreqStart = tryGetByamlKeyFloatOrZero(iter, "LowFreqStart");
            data->lowFreqEnd = tryGetByamlKeyFloatOrZero(iter, "LowFreqEnd");
            data->highFreqStart = tryGetByamlKeyFloatOrZero(iter, "HighFreqStart");
            data->highFreqEnd = tryGetByamlKeyFloatOrZero(iter, "HighFreqEnd");
            data->fadePercent = tryGetByamlKeyFloatOrZero(iter, "FadePercent");
            mDirectDatas.pushBack(data);
        } else {
            auto* data = new PadRumbleData;
            data->name = tryGetByamlKeyStringOrNULL(iter, "Name");
            data->dataName = tryGetByamlKeyStringOrNULL(iter, "DataName");
            data->volume = tryGetByamlKeyFloatOrZero(iter, "Volume");

            if (!iter.tryGetIntByKey(&data->priority, "Priority")) {
                data->priority = -1;
            }

            if (!iter.tryGetIntByKey(&data->minLength, "MinLength")) {
                data->minLength = -1;
            }

            data->isLoop = tryGetByamlKeyBoolOrFalse(iter, "IsLoop");
            data->isRecommend = tryGetByamlKeyBoolOrFalse(iter, "IsRecommend");
            data->isShowOR = tryGetByamlKeyBoolOrFalse(iter, "IsShowOR");

            if (data->isShowOR) {
                if (data->isLoop) {
                    mShowORLoopDatas.pushBack(data);
                } else {
                    mShowORDatas.pushBack(data);
                }
            } else {
                mDatas.pushBack(data);
            }
        }
    }
}

/**
 * Constructs the pad rumble director of a scene using a CameraDirector_RS.
 * @param pPlayerHolder the player holder
 * @param pCameraDirector the camera director
 */
PadRumbleDirector::PadRumbleDirector(const PlayerHolder* pPlayerHolder,
                                     const CameraDirector_RS* pCameraDirector)
    : mWaveVibrationHolder(nullptr), mPlayerHolder(pPlayerHolder), mCameraDirector(nullptr),
      mCameraDirectorRS(pCameraDirector), mLoopInfos(nullptr), mListener(nullptr),
      mOneTimeInfos(nullptr) {
    sharedInit(false);
}

/**
 * Sets the wave vibration holder and stops all of its vibrations.
 * @param pHolder the wave vibration holder
 */
void PadRumbleDirector::setWaveVibrationHolder(WaveVibrationHolder* pHolder) {
    mWaveVibrationHolder = pHolder;
    pHolder->stopAll();
}

/**
 * Updates the rumbles and the listener.
 */
void PadRumbleDirector::update() {
    if (mPauseCount >= 1) {
        if (mWaveVibrationHolder != nullptr) {
            mWaveVibrationHolder->update();
        }

        return;
    }

    updateInfoListAll();

    if (mWaveVibrationHolder != nullptr) {
        mWaveVibrationHolder->update();
    }

    mListener->prevPos.set(mListener->pos);

    sead::Vector3f pos;
    if (mIsListenerCamera) {
        if (mCameraDirector != nullptr) {
            pos.set(getCameraPos(mCameraDirector->getSceneCameraInfo()));
        } else {
            pos.set(getCameraPos(mCameraDirectorRS->getSceneCameraInfo()));
        }
    } else {
        LiveActor* player = tryGetPlayerActor(mPlayerHolder, 0);
        if (player == nullptr) {
            return;
        }

        sead::Vector3f cameraPos = getCameraPos(player) * mListener->cameraRate;
        pos.set(cameraPos + getPlayerPos(mPlayerHolder, 0) * (1.0f - mListener->cameraRate));
    }

    mListener->pos.set(pos);
}

/**
 * Updates all the rumble lists.
 */
void PadRumbleDirector::updateInfoListAll() {
    updateInfoListLoop();
    updateInfoListOneTime();
    updateDirectPlayInfoList();
}

/**
 * Starts a one time rumble at a position.
 * @param pName the name of the rumble
 * @param rPos the position of the rumble
 * @param rParam the rumble parameters
 * @param port the port of the controller, or a negative value for all players
 * @param isIgnorePriority whether the priority of the entry is ignored
 */
void PadRumbleDirector::startRumble(const char* pName, const sead::Vector3f& rPos,
                                    const PadRumbleParam& rParam, s32 port,
                                    bool isIgnorePriority) {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                startRumble(pName, rPos, rParam, getPlayerPort(mPlayerHolder, i), false);
            }
        }

        return;
    }

    f32 volume = 1.0f;
    s32 priority = -1;
    s32 minLength = -1;
    const WaveVibrationData* data =
        findWaveVibrationData(&volume, &priority, &minLength, mWaveVibrationHolder, pName,
                              &mShowORDatas, &mShowORLoopDatas, &mDatas);
    if (data == nullptr) {
        return;
    }

    if (isIgnorePriority) {
        priority = -1;
    }

    f32 rateRight = 1.0f;
    f32 rateLeft = 1.0f;
    f32 pitchLeft = 1.0f;
    f32 pitchRight = 1.0f;
    sead::Vector3f listenerPos;

    if (mCameraDirector != nullptr) {
        calcListenerPos(&listenerPos, mCameraDirector, mIsListenerCamera, mPlayerHolder, port);
        calcVolume(&rateLeft, &rateRight, &pitchLeft, &pitchRight, rParam.near, rParam.far, rPos,
                   mCameraDirector, listenerPos, rParam._18);
    } else {
        calcListenerPos(&listenerPos, mCameraDirectorRS, mIsListenerCamera, mPlayerHolder, port);
        calcVolume(&rateLeft, &rateRight, &pitchLeft, &pitchRight, rParam.near, rParam.far, rPos,
                   mCameraDirectorRS, listenerPos, rParam._18);
    }

    f32 volumeLeft =
        adjustVolume(volume * rateLeft * rParam.volumeLeft, mPowerLevel, mIsValid, port);
    f32 volumeRight =
        adjustVolume(volume * rateRight * rParam.volumeRight, mPowerLevel, mIsValid, port);

    OneTimeInfo* info = findDeadInfoOneTime();
    if (info != nullptr) {
        info->pos = rPos;
        info->volumeLeft = volumeLeft;
        info->volumeRight = volumeRight;
        info->port = port;
        info->name = pName;
        info->dataName = data->name;
        info->timer = 120;
        info->isNo3D = false;
    }

    if (rParam._1c) {
        s32 padNum = mWaveVibrationHolder->getUsePadNum();
        for (s32 i = 1; i <= padNum; i++) {
            WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(i);

            if (player != nullptr) {
                player->startOneTime(data, volumeLeft, volumeRight, rParam.pitchLeft * pitchLeft,
                                     rParam.pitchRight * pitchRight, priority, minLength,
                                     rParam._1d);

                if (info != nullptr) {
                    info->players[i] = player;
                }
            }
        }

        return;
    }

    WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(port);
    if (player == nullptr) {
        return;
    }

    player->startOneTime(data, volumeLeft, volumeRight, rParam.pitchLeft * pitchLeft,
                         rParam.pitchRight * pitchRight, priority, minLength, rParam._1d);

    if (info != nullptr) {
        info->players[port] = player;
    }
}

/**
 * Finds a one time rumble that is not playing anymore.
 * @return the rumble, or nullptr if all of them are playing
 */
PadRumbleDirector::OneTimeInfo* PadRumbleDirector::findDeadInfoOneTime() {
    for (s32 i = 0; i < 100; i++) {
        if (mOneTimeInfos[i].timer <= 0 && isDeadPlayers(mOneTimeInfos[i].players)) {
            return &mOneTimeInfos[i];
        }
    }

    return nullptr;
}

/**
 * Starts a one time rumble without position.
 * @param pName the name of the rumble
 * @param rParam the rumble parameters
 * @param port the port of the controller, or a negative value for all players
 * @param isIgnorePriority whether the priority of the entry is ignored
 */
void PadRumbleDirector::startRumbleNo3D(const char* pName, const PadRumbleParam& rParam, s32 port,
                                        bool isIgnorePriority) {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                getPlayerPort(mPlayerHolder, i);
                startRumbleNo3D(pName, rParam, getPlayerPort(mPlayerHolder, i), false);
            }
        }

        return;
    }

    f32 volume = 1.0f;
    s32 priority = -1;
    s32 minLength = -1;
    const WaveVibrationData* data =
        findWaveVibrationData(&volume, &priority, &minLength, mWaveVibrationHolder, pName,
                              &mShowORDatas, &mShowORLoopDatas, &mDatas);
    if (data == nullptr) {
        DirectPlayInfo* directInfo = findDirectPlayInfo(pName);

        if (directInfo != nullptr) {
            directInfo->volumeLeft = rParam.volumeLeft;
            directInfo->volumeRight = rParam.volumeRight;
            directInfo->port = port;
        }

        return;
    }

    if (isIgnorePriority) {
        priority = -1;
    }

    f32 volumeLeft =
        adjustVolume(volume * rParam.volumeLeft * 0.8f, mPowerLevel, mIsValid, port);
    f32 volumeRight =
        adjustVolume(volume * rParam.volumeRight * 0.8f, mPowerLevel, mIsValid, port);

    OneTimeInfo* info = findDeadInfoOneTime();
    if (info != nullptr) {
        info->pos = mListener->pos;
        info->volumeLeft = volumeLeft;
        info->volumeRight = volumeRight;
        info->port = port;
        info->name = pName;
        info->dataName = data->name;
        info->timer = 120;
        info->isNo3D = true;
    }

    if (rParam._1c) {
        s32 padNum = mWaveVibrationHolder->getUsePadNum();
        for (s32 i = 1; i <= padNum; i++) {
            WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(i);

            if (player != nullptr) {
                player->startOneTime(data, volumeLeft, volumeRight, rParam.pitchLeft,
                                     rParam.pitchRight, priority, minLength, rParam._1d);

                if (info != nullptr) {
                    info->players[i] = player;
                }
            }
        }

        return;
    }

    WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(port);
    if (player == nullptr) {
        return;
    }

    player->startOneTime(data, volumeLeft, volumeRight, rParam.pitchLeft, rParam.pitchRight,
                         priority, minLength, rParam._1d);

    if (info != nullptr) {
        info->players[port] = player;
    }
}

/**
 * Starts playing a direct data in a free direct play slot.
 * @param pName the name of the direct data
 * @return the direct play slot, or nullptr if none is available
 */
PadRumbleDirector::DirectPlayInfo* PadRumbleDirector::findDirectPlayInfo(const char* pName) {
    if (mDirectDatas.size() < 1) {
        return nullptr;
    }

    PadRumbleDirectData* data;
    for (s32 i = 0; i < mDirectDatas.size(); i++) {
        data = mDirectDatas.unsafeAt(i);

        if (isEqualString(data->name, pName)) {
            break;
        }
    }

    if (data == nullptr) {
        return nullptr;
    }

    for (s32 i = 0; i < 16; i++) {
        DirectPlayInfo* info = &mDirectPlayInfos[i];

        if (info->data == nullptr) {
            info->init(data);
            return info;
        }
    }

    return nullptr;
}

/**
 * Stops the direct data rumbles of a port.
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::stopPadRumbleDirect(s32 port) {
    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                stopPadRumbleDirect(getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    for (s32 i = 0; i < 16; i++) {
        if (mDirectPlayInfos[i].data != nullptr && mDirectPlayInfos[i].port == port) {
            mDirectPlayInfos[i].data = nullptr;
        }
    }

    stopRumbleDirectValue(port);
}

/**
 * Stops the direct value vibration of a port.
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::stopRumbleDirectValue(s32 port) {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                stopRumbleDirectValue(getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    mWaveVibrationHolder->stopVibrationDirectValue(port);
}

/**
 * Stops the one time rumbles with a name.
 * @param pName the name of the rumble
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::stopPadRumbleOneTime(const char* pName, s32 port) {
    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                stopPadRumbleOneTime(pName, getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    for (s32 i = 0; i < 100; i++) {
        if (mOneTimeInfos[i].port == port && isEqualString(mOneTimeInfos[i].name, pName)) {
            OneTimeInfo& info = mOneTimeInfos[i];

            for (s32 j = 0; j < 5; j++) {
                if (info.players[j] != nullptr) {
                    info.players[j]->stop();
                    info.players[j] = nullptr;
                }
            }

            mOneTimeInfos[i].timer = 0;
        }
    }
}

/**
 * Starts a looping rumble following a position.
 * @param pName the name of the rumble
 * @param pPos the position of the rumble
 * @param rParam the rumble parameters
 * @param port the port of the controller, or a negative value for all players
 * @param isIgnorePriority whether the priority of the entry is ignored
 */
void PadRumbleDirector::startRumbleLoop(const char* pName, const sead::Vector3f* pPos,
                                        const PadRumbleParam& rParam, s32 port,
                                        bool isIgnorePriority) {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                startRumbleLoop(pName, pPos, rParam, getPlayerPort(mPlayerHolder, i), false);
            }
        }

        return;
    }

    f32 volume = 1.0f;
    s32 priority = -1;
    s32 minLength = -1;
    const WaveVibrationData* data =
        findWaveVibrationData(&volume, &priority, &minLength, mWaveVibrationHolder, pName,
                              &mShowORDatas, &mShowORLoopDatas, &mDatas);
    if (data == nullptr) {
        return;
    }

    if (isIgnorePriority) {
        priority = -1;
    }

    LoopInfo* info = findDeadInfo();
    if (info == nullptr) {
        return;
    }

    info->name = pName;
    info->dataName = data->name;
    info->volume = volume;
    info->param = rParam;
    info->port = port;
    info->pos = pPos;
    info->prevPos.set(*pPos);
    info->isNo3D = false;

    f32 rateRight = 1.0f;
    f32 rateLeft = 1.0f;
    f32 pitchLeft = 1.0f;
    f32 pitchRight = 1.0f;
    sead::Vector3f listenerPos;

    if (mCameraDirector != nullptr) {
        calcListenerPos(&listenerPos, mCameraDirector, mIsListenerCamera, mPlayerHolder, port);
        calcVolume(&rateLeft, &rateRight, &pitchLeft, &pitchRight, rParam.near, rParam.far, *pPos,
                   mCameraDirector, listenerPos, rParam._18);
    } else {
        calcListenerPos(&listenerPos, mCameraDirectorRS, mIsListenerCamera, mPlayerHolder, port);
        calcVolume(&rateLeft, &rateRight, &pitchLeft, &pitchRight, rParam.near, rParam.far, *pPos,
                   mCameraDirectorRS, listenerPos, rParam._18);
    }

    f32 volumeLeft =
        adjustVolume(rateLeft * volume * rParam.volumeLeft, mPowerLevel, mIsValid, port);
    f32 volumeRight =
        adjustVolume(rateRight * volume * rParam.volumeRight, mPowerLevel, mIsValid, port);

    if (rParam._1c) {
        s32 padNum = mWaveVibrationHolder->getUsePadNum();
        for (s32 i = 1; i <= padNum; i++) {
            WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(i);

            if (player != nullptr) {
                info->players[i - 1] = player;
                player->startLoop(data, volumeLeft, volumeRight, rParam.pitchLeft * pitchLeft,
                                  rParam.pitchRight * pitchRight, priority, minLength,
                                  rParam._1d);
            }
        }

        return;
    }

    WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(port);
    if (player == nullptr) {
        return;
    }

    info->players(0) = player;
    player->startLoop(data, volumeLeft, volumeRight, rParam.pitchLeft * pitchLeft,
                      rParam.pitchRight * pitchRight, priority, minLength, rParam._1d);
}

/**
 * Finds a looping rumble that is not playing anymore.
 * @return the rumble, or nullptr if all of them are playing
 */
PadRumbleDirector::LoopInfo* PadRumbleDirector::findDeadInfo() {
    for (s32 i = 0; i < 80; i++) {
        if (!mLoopInfos[i].isPaused && isDeadPlayers(mLoopInfos[i].players)) {
            return &mLoopInfos[i];
        }
    }

    return nullptr;
}

/**
 * Starts a looping rumble without 3D attenuation.
 * @param pName the name of the rumble
 * @param pPos the position of the rumble
 * @param rParam the rumble parameters
 * @param port the port of the controller, or a negative value for all players
 * @param isIgnorePriority whether the priority of the entry is ignored
 */
void PadRumbleDirector::startRumbleLoopNo3D(const char* pName, const sead::Vector3f* pPos,
                                            const PadRumbleParam& rParam, s32 port,
                                            bool isIgnorePriority) {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                startRumbleLoopNo3D(pName, pPos, rParam, getPlayerPort(mPlayerHolder, i), false);
            }
        }

        return;
    }

    f32 volume = 1.0f;
    s32 priority = -1;
    s32 minLength = -1;
    const WaveVibrationData* data =
        findWaveVibrationData(&volume, &priority, &minLength, mWaveVibrationHolder, pName,
                              &mShowORDatas, &mShowORLoopDatas, &mDatas);
    if (data == nullptr) {
        return;
    }

    if (isIgnorePriority) {
        priority = -1;
    }

    LoopInfo* info = findDeadInfo();
    if (info == nullptr) {
        return;
    }

    info->name = pName;
    info->dataName = data->name;
    info->volume = volume;
    info->param = rParam;
    info->port = port;
    info->pos = pPos;
    info->prevPos.set(*pPos);
    info->isNo3D = true;

    f32 volumeLeft =
        adjustVolume(rParam.volumeLeft * volume * 0.8f, mPowerLevel, mIsValid, port);
    f32 volumeRight =
        adjustVolume(rParam.volumeRight * volume * 0.8f, mPowerLevel, mIsValid, port);

    if (rParam._1c) {
        s32 padNum = mWaveVibrationHolder->getUsePadNum();
        for (s32 i = 1; i <= padNum; i++) {
            WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(i);

            if (player != nullptr) {
                info->players[i - 1] = player;
                player->startLoop(data, volumeLeft, volumeRight, rParam.pitchLeft,
                                  rParam.pitchRight, priority, minLength, rParam._1d);
            }
        }

        return;
    }

    WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(port);
    if (player == nullptr) {
        return;
    }

    info->players(0) = player;
    player->startLoop(data, volumeLeft, volumeRight, rParam.pitchLeft, rParam.pitchRight,
                      priority, minLength, rParam._1d);
}

/**
 * Stops a looping rumble.
 * @param pName the name of the rumble
 * @param pPos the position of the rumble
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::stopRumbleLoop(const char* pName, const sead::Vector3f* pPos, s32 port) {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                stopRumbleLoop(pName, pPos, getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    LoopInfo* info = findInfo(pName, pPos, port);
    if (info == nullptr) {
        return;
    }

    for (s32 i = 0; i < 5; i++) {
        if (info->players[i] != nullptr) {
            info->players[i]->stop();
            info->players[i] = nullptr;
        }
    }
}

/**
 * Finds a looping rumble that is playing or paused.
 * @param pName the name of the rumble
 * @param pPos the position of the rumble
 * @param port the port of the controller
 * @return the rumble, or nullptr if it was not found
 */
PadRumbleDirector::LoopInfo* PadRumbleDirector::findInfo(const char* pName,
                                                         const sead::Vector3f* pPos, s32 port) {
    for (s32 i = 0; i < 80; i++) {
        LoopInfo& info = mLoopInfos[i];

        if (info.pos == pPos && info.port == port &&
            (!isDeadPlayers(info.players) || info.isPaused) && isEqualString(info.name, pName)) {
            return &mLoopInfos[i];
        }
    }

    return nullptr;
}

/**
 * Checks if a looping rumble is playing or paused.
 * @param pName the name of the rumble
 * @param pPos the position of the rumble
 * @param port the port of the controller, or a negative value for any player
 * @return true if the rumble is alive
 */
bool PadRumbleDirector::checkIsAliveRumbleLoop(const char* pName, const sead::Vector3f* pPos,
                                               s32 port) {
    if (mWaveVibrationHolder == nullptr) {
        return false;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i) &&
                checkIsAliveRumbleLoop(pName, pPos, getPlayerPort(mPlayerHolder, i))) {
                return true;
            }
        }

        return false;
    }

    return findInfo(pName, pPos, port) != nullptr;
}

/**
 * Stops all the rumbles.
 */
void PadRumbleDirector::stopAllRumble() {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    mPauseCount = 0;
    mWaveVibrationHolder->stopAll();
    clearAllInfoList();
}

/**
 * Clears all the rumble lists.
 */
void PadRumbleDirector::clearAllInfoList() {
    for (s32 i = 0; i < 80; i++) {
        LoopInfo& info = mLoopInfos[i];

        info.isPaused = false;
        info.clearPlayers();
        info.pos = nullptr;
    }

    clearAllOneTime();

    for (s32 i = 0; i < 16; i++) {
        if (mDirectPlayInfos[i].data != nullptr) {
            mDirectPlayInfos[i].data = nullptr;
        }
    }
}

/**
 * Pauses the vibrations.
 */
void PadRumbleDirector::pause() {
    if (mPauseCount <= 0) {
        mWaveVibrationHolder->pause();
    }

    mPauseCount++;
}

/**
 * Ends a pause of the vibrations.
 */
void PadRumbleDirector::endPause() {
    if (mPauseCount >= 1) {
        mPauseCount--;

        if (mPauseCount > 0) {
            return;
        }
    }

    mWaveVibrationHolder->endPause();
}

/**
 * Pauses the active looping rumbles and stops the other rumbles.
 */
void PadRumbleDirector::pauseActiveRumbles() {
    for (s32 i = 0; i < 80; i++) {
        LoopInfo& info = mLoopInfos[i];

        if (isDeadPlayers(info.players) || info.isPaused) {
            continue;
        }

        info.isPaused = true;

        for (s32 j = 0; j < 5; j++) {
            if (info.players[j] != nullptr) {
                info.players[j]->pause(true);
            }
        }
    }

    mWaveVibrationHolder->stopAllOneShot();
    clearAllOneTime();
    stopPadRumbleDirect(-1);
}

/**
 * Clears the one time rumble list.
 */
void PadRumbleDirector::clearAllOneTime() {
    for (s32 i = 0; i < 100; i++) {
        mOneTimeInfos[i].clear();
    }
}

/**
 * Resumes the looping rumbles paused by pauseActiveRumbles.
 */
void PadRumbleDirector::resumeActiveRumbles() {
    for (s32 i = 0; i < 80; i++) {
        LoopInfo& info = mLoopInfos[i];

        if (!info.isPaused) {
            continue;
        }

        info.isPaused = false;

        for (s32 j = 0; j < 5; j++) {
            if (info.players[j] != nullptr && info.players[j]->isPauseLocked()) {
                info.players[j]->endPause(true);
            }
        }
    }
}

/**
 * Changes the volume of a looping rumble.
 * @param pName the name of the rumble
 * @param pPos the position of the rumble
 * @param volumeLeft the volume of the left side
 * @param volumeRight the volume of the right side
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::changeRumbleLoopVolume(const char* pName, const sead::Vector3f* pPos,
                                               f32 volumeLeft, f32 volumeRight, s32 port) {
    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                changeRumbleLoopVolume(pName, pPos, volumeLeft, volumeRight,
                                       getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    LoopInfo* info = findInfo(pName, pPos, port);
    if (info == nullptr) {
        return;
    }

    info->param.volumeLeft = volumeLeft;
    info->param.volumeRight = volumeRight;
    updateInfoListLoop();
}

/**
 * Applies the doppler effect of a looping rumble to its pitches.
 * @param pPitchLeft the pitch of the left side
 * @param pPitchRight the pitch of the right side
 * @param pInfo the rumble
 * @param pCameraDirector the camera director
 * @param pListener the listener
 */
template <typename T>
ALWAYS_INLINE void applyDoppler(f32* pPitchLeft, f32* pPitchRight,
                                PadRumbleDirector::LoopInfo* pInfo, const T* pCameraDirector,
                                const PadRumbleDirector::ListenerInfo* pListener) {
    const sead::Vector3f* pos = pInfo->pos;

    sead::Vector3f right;
    getCameraLookAtCamera(getCameraInfo(pCameraDirector))->getRightVectorByMatrix(&right);
    right *= 150.0f;

    sead::Vector3f earLeft = pListener->pos - right;
    sead::Vector3f earRight = right + pListener->pos;
    getCameraViewMtx(getCameraInfo(pCameraDirector));

    sead::Vector3f dirLeft = *pos - earLeft;
    sead::Vector3f dirRight = *pos - earRight;

    if (tryNormalizeOrZero(&dirLeft) && tryNormalizeOrZero(&dirRight)) {
        sead::Vector3f velocity =
            (*pos - pInfo->prevPos) - (pListener->pos - pListener->prevPos);
        sead::Vector3f limited;
        limitLength(&limited, velocity, 240.0f);

        f32 speedLeft = limited.dot(dirLeft) * 2.6f;
        f32 speedRight = limited.dot(dirRight) * 2.6f;
        *pPitchLeft = *pPitchLeft * ((566.6667f - speedLeft) / 566.6667f);
        *pPitchRight = *pPitchRight * ((566.6667f - speedRight) / 566.6667f);
    }

    pInfo->prevPos.set(*pInfo->pos);
}

/**
 * Updates the volumes and pitches of the looping rumbles.
 */
void PadRumbleDirector::updateInfoListLoop() {
    for (s32 i = 0; i < 80; i++) {
        LoopInfo* info = &mLoopInfos[i];

        if (info->isPaused || isDeadPlayers(info->players)) {
            continue;
        }

        f32 rateRight = 1.0f;
        f32 rateLeft = 1.0f;
        f32 pitchRight = 1.0f;
        f32 pitchLeft = 1.0f;

        if (!info->isNo3D) {
            if (mCameraDirector != nullptr) {
                calcVolume(&rateLeft, &rateRight, &pitchLeft, &pitchRight, info->param.near,
                           info->param.far, *info->pos, mCameraDirector, mListener->pos,
                           info->param._18);
                applyDoppler(&pitchLeft, &pitchRight, info, mCameraDirector, mListener);
            } else {
                calcVolume(&rateLeft, &rateRight, &pitchLeft, &pitchRight, info->param.near,
                           info->param.far, *info->pos, mCameraDirectorRS, mListener->pos,
                           info->param._18);
                applyDoppler(&pitchLeft, &pitchRight, info, mCameraDirectorRS, mListener);
            }
        }

        f32 volumeLeft = adjustVolume(info->volume * info->param.volumeLeft * rateLeft,
                                      mPowerLevel, mIsValid, info->port);
        f32 volumeRight = adjustVolume(info->volume * info->param.volumeRight * rateRight,
                                       mPowerLevel, mIsValid, info->port);
        pitchLeft = pitchLeft * info->param.pitchLeft;
        pitchRight = pitchRight * info->param.pitchRight;

        for (s32 j = 0; j < 5; j++) {
            if (info->players[j] != nullptr) {
                info->players[j]->changeVolumeAndPitch(volumeLeft, volumeRight, pitchLeft,
                                                       pitchRight);
            }
        }
    }
}

/**
 * Changes the pitch of a looping rumble.
 * @param pName the name of the rumble
 * @param pPos the position of the rumble
 * @param pitchLeft the pitch of the left side
 * @param pitchRight the pitch of the right side
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::changeRumbleLoopPitch(const char* pName, const sead::Vector3f* pPos,
                                              f32 pitchLeft, f32 pitchRight, s32 port) {
    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                changeRumbleLoopPitch(pName, pPos, pitchLeft, pitchRight,
                                      getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    LoopInfo* info = findInfo(pName, pPos, port);
    if (info == nullptr) {
        return;
    }

    info->param.pitchLeft = pitchLeft;
    info->param.pitchRight = pitchRight;
    updateInfoListLoop();
}

/**
 * Starts a one time rumble with volumes.
 * @param pName the name of the wave vibration data
 * @param volumeLeft the volume of the left side
 * @param volumeRight the volume of the right side
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::startRumbleWithVolume(const char* pName, f32 volumeLeft, f32 volumeRight,
                                              s32 port) {
    if (mWaveVibrationHolder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                startRumbleWithVolume(pName, volumeLeft, volumeRight,
                                      getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    WaveVibrationPlayer* player = mWaveVibrationHolder->findPlayableVibrationPlayer(port);
    if (player == nullptr) {
        return;
    }

    const WaveVibrationData* data = mWaveVibrationHolder->findWaveVibrationData(pName);
    if (data == nullptr) {
        return;
    }

    f32 adjustedLeft = adjustVolume(volumeLeft, mPowerLevel, mIsValid, port);
    f32 adjustedRight = adjustVolume(volumeRight, mPowerLevel, mIsValid, port);
    player->startOneTime(data, adjustedLeft, adjustedRight, 1.0f, 1.0f, -1, -1, false);

    OneTimeInfo* info = findDeadInfoOneTime();
    if (info == nullptr) {
        return;
    }

    info->pos = mListener->pos;
    info->volumeLeft = volumeLeft;
    info->volumeRight = volumeRight;
    info->port = port;
    info->name = data->name;
    info->dataName = data->name;
    info->timer = 120;
    info->isNo3D = true;
}

/**
 * Starts a vibration from direct values.
 * @param freqLow the low frequency
 * @param freqHigh the high frequency
 * @param ampLow the amplitude of the low frequency
 * @param ampHigh the amplitude of the high frequency
 * @param volumeLeft the volume of the left side
 * @param volumeRight the volume of the right side
 * @param port the port of the controller, or a negative value for all players
 */
void PadRumbleDirector::startRumbleDirectValue(f32 freqLow, f32 freqHigh, f32 ampLow,
                                               f32 ampHigh, f32 volumeLeft, f32 volumeRight,
                                               s32 port) {
    WaveVibrationHolder* holder = mWaveVibrationHolder;
    if (holder == nullptr) {
        return;
    }

    if (port < 0) {
        s32 playerNum = getPlayerNumMax(mPlayerHolder);
        for (s32 i = 0; i < playerNum; i++) {
            if (!isPlayerDead(mPlayerHolder, i)) {
                startRumbleDirectValue(freqLow, freqHigh, ampLow, ampHigh, volumeLeft,
                                       volumeRight, getPlayerPort(mPlayerHolder, i));
            }
        }

        return;
    }

    f32 adjustedLeft = adjustVolume(volumeLeft, mPowerLevel, mIsValid, port);
    f32 adjustedRight = adjustVolume(volumeRight, mPowerLevel, mIsValid, port);
    holder->startVibrationDirectValue(port, freqLow, freqHigh, ampLow, ampHigh, adjustedLeft,
                                      adjustedRight);
}

/**
 * Updates the one time rumble list, releasing the players that ended.
 */
void PadRumbleDirector::updateInfoListOneTime() {
    for (s32 i = 0; i < 100; i++) {
        if (mOneTimeInfos[i].timer != 0) {
            mOneTimeInfos[i].timer--;
        }

        OneTimeInfo& info = mOneTimeInfos[i];
        for (s32 j = 0; j < 5; j++) {
            WaveVibrationPlayer* player = info.players[j];

            if (player != nullptr && (!player->isPaused() || player->isPauseLocked()) &&
                !player->isPlaying()) {
                info.players[j] = nullptr;
            }
        }
    }
}

/**
 * Updates the direct data rumbles.
 */
void PadRumbleDirector::updateDirectPlayInfoList() {
    for (s32 i = 0; i < 16; i++) {
        DirectPlayInfo& info = mDirectPlayInfos[i];

        if (info.isPaused || info.data == nullptr) {
            continue;
        }

        const PadRumbleDirectData* data = info.data;
        if (info.frame >= data->length) {
            info.data = nullptr;
            continue;
        }

        f32 rate = ((f32)info.frame / (f32)data->length - data->fadePercent) /
                   (1.0f - data->fadePercent);
        rate = sead::Mathf::clamp(rate, 0.0f, 1.0f);

        f32 amp = 1.0f - easeOut(rate);
        f32 freqHigh = lerpValue(rate, data->highFreqStart, data->highFreqEnd);
        f32 freqLow = lerpValue(rate, data->lowFreqStart, data->lowFreqEnd);
        startRumbleDirectValue(freqLow, freqHigh, amp, amp, info.volumeLeft, info.volumeRight,
                               info.port);
        info.frame++;
    }
}

/**
 * Starts a one time rumble on all players for testing.
 * @param pName the name of the wave vibration data
 * @param volumeLeft the volume of the left side
 * @param volumeRight the volume of the right side
 */
void PadRumbleDirector::testStartPadRumbleWithVolumeNoActor(const char* pName, f32 volumeLeft,
                                                            f32 volumeRight) {
    if (mIsListenerCamera) {
        return;
    }

    s32 playerNum = getPlayerNumMax(mPlayerHolder);
    for (s32 i = 0; i < playerNum; i++) {
        if (!isPlayerDead(mPlayerHolder, i)) {
            startRumbleWithVolume(pName, getPlayerPort(getPlayerActor(mPlayerHolder, i), i),
                                  volumeLeft, volumeRight);
        }
    }
}

/**
 * Constructs a pad rumble keeper.
 * @param port the port of the controller
 */
NOINLINE PadRumbleKeeper::PadRumbleKeeper(s32 port) : mPort(port) {}

/**
 * Creates a pad rumble keeper and sets it to an actor.
 * @param pActor the actor
 * @param port the port of the controller
 * @return the pad rumble keeper
 */
PadRumbleKeeper* createPadRumbleKeeper(const LiveActor* pActor, s32 port) {
    auto* keeper = new PadRumbleKeeper(port);
    setPadRumbleKeeper(pActor, keeper);
    return keeper;
}

/**
 * Sets a pad rumble keeper to the action and hit reaction of an actor.
 * @param pActor the actor
 * @param pKeeper the pad rumble keeper
 */
void setPadRumbleKeeper(const LiveActor* pActor, const PadRumbleKeeper* pKeeper) {
    ActorActionKeeper* actionKeeper = pActor->getActorActionKeeper();

    if (actionKeeper != nullptr && actionKeeper->getPadAndCameraCtrl() != nullptr) {
        actionKeeper->getPadAndCameraCtrl()->setPadRumbleKeeper(pKeeper);
    }

    HitReactionKeeper* hitReactionKeeper = pActor->getHitReactionKeeper();
    if (hitReactionKeeper != nullptr) {
        hitReactionKeeper->setPadRumbleKeeper(const_cast<PadRumbleKeeper*>(pKeeper));
    }
}

/**
 * Sets a pad rumble keeper to an actor if it has an action keeper.
 * @param pActor the actor
 * @param pKeeper the pad rumble keeper
 */
void trySetPadRumbleKeeper(const LiveActor* pActor, const PadRumbleKeeper* pKeeper) {
    if (pActor->getActorActionKeeper() == nullptr) {
        return;
    }

    setPadRumbleKeeper(pActor, pKeeper);
}

/**
 * Checks if an actor has a pad rumble keeper.
 * @param pActor the actor
 * @return true if the actor has a pad rumble keeper
 */
bool isExistPadRumbleKeeper(const LiveActor* pActor) {
    ActorActionKeeper* actionKeeper = pActor->getActorActionKeeper();

    if (actionKeeper == nullptr) {
        return false;
    }

    ActionPadAndCameraCtrl* padAndCameraCtrl = actionKeeper->getPadAndCameraCtrl();
    if (padAndCameraCtrl != nullptr) {
        return padAndCameraCtrl->getPadRumbleKeeper() != nullptr;
    }

    HitReactionKeeper* hitReactionKeeper = pActor->getHitReactionKeeper();
    if (hitReactionKeeper != nullptr) {
        return hitReactionKeeper->getPadRumblePort() != nullptr;
    }

    return false;
}

/**
 * Starts a weak rumble (unused).
 * @param pActor the actor
 * @param port the port of the controller
 */
void startPadRumbleWeak(const LiveActor* pActor, s32 port) {}

/**
 * Starts a normal rumble (unused).
 * @param pActor the actor
 * @param port the port of the controller
 */
void startPadRumbleNormal(const LiveActor* pActor, s32 port) {}

/**
 * Starts a strong rumble (unused).
 * @param pActor the actor
 * @param port the port of the controller
 */
void startPadRumbleStrong(const LiveActor* pActor, s32 port) {}

/**
 * Starts a continuous weak rumble (unused).
 * @param pActor the actor
 * @param port the port of the controller
 */
void startPadRumbleContinueWeak(const LiveActor* pActor, s32 port) {}

/**
 * Starts a continuous normal rumble (unused).
 * @param pActor the actor
 * @param port the port of the controller
 */
void startPadRumbleContinueNormal(const LiveActor* pActor, s32 port) {}

/**
 * Starts a continuous strong rumble (unused).
 * @param pActor the actor
 * @param port the port of the controller
 */
void startPadRumbleContinueStrong(const LiveActor* pActor, s32 port) {}

/**
 * Enables the pad rumbles of an actor's scene.
 * @param pActor the actor
 */
void validatePadRumbleAll(const LiveActor* pActor) {
    pActor->getSceneInfo()->padRumbleDirector->validate();
}

/**
 * Disables the pad rumbles of an actor's scene.
 * @param pActor the actor
 */
void invalidatePadRumbleAll(const LiveActor* pActor) {
    pActor->getSceneInfo()->padRumbleDirector->invalidate();
}

/**
 * Checks if the wave rumbles are enabled.
 * @param pDirector the pad rumble director
 * @return always true
 */
bool isEnablePadWaveRumble(const PadRumbleDirector* pDirector) {
    return true;
}

/**
 * Checks if the wave rumbles of an actor's scene are enabled.
 * @param pActor the actor
 * @return always true
 */
bool isEnablePadWaveRumble(const LiveActor* pActor) {
    return isEnablePadWaveRumble(pActor->getSceneInfo()->padRumbleDirector);
}
}  // namespace al
