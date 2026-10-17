#pragma once

#include <basis/seadTypes.h>
#include <container/seadBuffer.h>
#include <container/seadSafeArray.h>
#include <prim/seadSafeString.h>

namespace nn::hid {
struct ControllerSupportArg;
struct ControllerSupportResultInfo;
}  // namespace nn::hid

namespace al {
class AudioSystem;
class IUseCancel;

class GamePadSystemDelegate {
public:
    virtual void setAppletDisabled(bool isDisabled) = 0;
    virtual void setDisabled(bool isDisabled) = 0;
    virtual bool disconnect() = 0;
    virtual void setCancelUser(IUseCancel* pCancelUser) = 0;
};

class GamePadSystem {
public:
    using DisconnectCallback = bool (*)(const GamePadSystem* pSystem, void* pUserData);

    GamePadSystem(bool isSinglePlay);

    void changeSinglePlayMode(bool isAnyController);
    void changeMultiPlayMode(s32 maxPlayerNum, s32 minPlayerNum);
    void disableControllerApplet(bool isDisable);
    void disableControllerConnectChecker(bool isDisable);
    void setIsAllowHandheld(bool isAllow);
    void setMaxNpadNum(s32 num);
    void initSingleJoycon();
    bool isDisconnectPlayable() const;
    const sead::WSafeString& getPadName(u8 index) const;
    s32 getPadPlayStyle(u8 index) const;
    void update();
    bool isDisconnectPlayableImpl() const;
    void setDisconnectFrame(s32 frame);
    void forceImmediateDisconnect(bool isForce);
    void setInvalidateDisconnectFrame(s32 frame);
    void setPadName(u8 index, const sead::WSafeString& rName);
    void callDisconnectController();
    void changeTopMenuPlayMode();
    void setAssistMode(bool isAssist, bool isForceDisconnect);
    void setSoftwareKeyboard(IUseCancel* pCancel);

    void setAudioSystem(AudioSystem* pAudioSystem) { mAudioSystem = pAudioSystem; }

    AudioSystem* getAudioSystem() const {
        return mAudioSystem;
    }

    void setDelegate(GamePadSystemDelegate* pDelegate) { mDelegate = pDelegate; }

    s32 getMaxPlayerNum() const { return mMaxPlayerNum; }

    s32 getMinPlayerNum() const { return mMinPlayerNum; }

    bool isForceImmediateDisconnect() const { return mIsForceImmediateDisconnect; }

    void resetForceImmediateDisconnect() { mIsForceImmediateDisconnect = false; }

    bool isAllowHandheld() const { return mIsAllowHandheld; }

    bool is2PAssistMode() const { return _46; }

    bool isEnableAutoHandheld() const { return mIsEnableAutoHandheld; }

    bool isChangedPadState() const { return mIsChangedPadState; }

    void requestForceImmediateDisconnect() { mIsForceImmediateDisconnect = true; }

    void set2PAssistMode(bool isAssist) { _46 = isAssist; }

    void setIsEnableAutoHandheld(bool isEnable) { mIsEnableAutoHandheld = isEnable; }

    /**
     * @brief Sets the flag at 0x40 (cleared by the title scene, meaning unknown).
     * @param isOn The new value.
     */
    void set40(bool isOn) { _40 = isOn; }

private:
    s32 mMaxPlayerNum = 1;
    s32 mMinPlayerNum = 1;
    s32 mDisconnectFrame = 0;
    s32 mDisconnectFrameMax = 60;
    s32 mInvalidateDisconnectFrame = 0;
    sead::Buffer<sead::WFixedSafeString<256>> mPadNames;
    AudioSystem* mAudioSystem = nullptr;
    DisconnectCallback mDisconnectCallback = nullptr;
    void* mDisconnectCallbackUserData = nullptr;
    bool _40 = false;
    bool mIsChangedPadState = false;
    bool _42 = false;
    bool _43 = false;
    bool mIsForceImmediateDisconnect = false;
    bool mIsAllowHandheld = true;
    bool _46 = false;
    bool mIsEnableAutoHandheld = true;
    GamePadSystemDelegate* mDelegate = nullptr;
    sead::SafeArray<s32, 9> mPadStyles;
    sead::SafeArray<s32, 9> mPadConnectStates;
};

static_assert(sizeof(GamePadSystem) == 0x98);

bool tryCallControllerApplet(const GamePadSystem* pSystem, s32 minPlayerNum, s32 maxPlayerNum);
bool tryCallControllerApplet(const GamePadSystem* pSystem, nn::hid::ControllerSupportArg* pArg,
                             nn::hid::ControllerSupportResultInfo* pResultInfo,
                             bool isUseControllerNames);
}  // namespace al
