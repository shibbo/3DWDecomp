#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { class FunctorBase; class WipeSimple; class EffectSystem; }
class DemoSceneActorHolder;
class DemoSkipLayout;

/** @brief Shared cutscene actor with skip handling and frame callbacks. */
class DemoObjBase : public al::LiveActor {
public:
    struct FrameHook { al::FunctorBase* callback; unsigned int frame; };
    explicit DemoObjBase(const char* pName);
    /** @brief Destroys the demo actor. */
    ~DemoObjBase() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    virtual void setBgmRequest(const char* pName, unsigned int frame, al::AudioKeeper* pKeeper);
    virtual void startDemo();
    virtual void endDemo(bool setWaitState);
    /** @brief Base demos do not change the scene end flag. */
    virtual void setEndSceneFlag() {}
    /** @brief Base demos do not change the scene end flag. */
    virtual void unsetEndSceneFlag() {}
    virtual void overrideBaseMtx(const sead::Matrix34f* pMtx);
    virtual void exePlay();
    virtual void exeCancelDemo();
    virtual void initDemoPlacement(const al::ActorInitInfo& rInfo);
    virtual void startAction();
    virtual void updateDemo();
    virtual bool checkDemoEnd() const;
    void setWait();
    bool isCancelWipeActive() const;
    void setDemoName(const char* pName);
    bool isFullyStarted() const;
    void callEndDemoHook();
    bool isEndDemo() const;
    void forceWait();
    void setCameraInterpolateFrame(int frames);
    int getMaxFrame();
    void exeWait();
    void displayFrameCount();
    static void sharedDisplayFrameCount(int frame, int maxFrame);
    void callFrameHooks(unsigned int frame);
    void setForceIgnoreCharId();
    FrameHook* registerFrameHook(const al::FunctorBase& rCallback, unsigned int frame);
    void registerEndDemoHook(const al::FunctorBase& rCallback);
    void registerCancelDemoHook(const al::FunctorBase& rCallback);

    /**
     * @brief Set the base matrix the demo is placed at.
     * @param pMtx The matrix.
     */
    void setPlacementBaseMtx(const sead::Matrix34f* pMtx) { mPlacementBaseMtx = pMtx; }
    /** @brief Sets the actor hidden while the demo plays. */
    void setHideActor(al::LiveActor* pActor) { mHideActor = pActor; }
    /** @brief Sets whether the demo audio is cancelled when the demo ends. */
    void setCancelAudioFlag(bool isCancel) { mCancelAudioFlag = isCancel; }
    /** @brief Sets whether all effects are killed when the demo starts. */
    void setKillAllEffects(bool isKill) { mKillAllEffects = isKill; }
    /** @brief Sets whether Bowser Jr. is hidden while the demo plays. */
    void setHideKoopaJr(bool isHide) { mHideKoopaJr = isHide; }

    /**
     * @brief Allow or forbid skipping the demo.
     * @param isAllow True to allow skipping.
     */
    void setAllowSkip(bool isAllow) { mAllowSkip = isAllow; }

    /**
     * @brief Disable or enable screen captures during the demo.
     * @param isDisable True to disable captures.
     */
    void setDisableCapture(bool isDisable) { mDisableCapture = isDisable; }

protected:
    const char* mDemoName = nullptr;
    DemoSceneActorHolder* mDemo = nullptr;
    DemoSkipLayout* mSkipLayout = nullptr;
    al::WipeSimple* mWipe;
    al::LiveActor* mHideActor = nullptr;
    int mEndFrameWindow = -1;
    const sead::Matrix34f* mPlacementBaseMtx = nullptr;
    void* _180 = nullptr;
    int _188 = 0;
    void* _190 = nullptr;
    bool mAllowSkip = true;
    bool mCancelAudioFlag = false;
    bool mKillAllEffects = false;
    bool mDisableCapture = false;
    bool mHideKoopaJr = false;
    al::EffectSystem* mEffectSystem = nullptr;
    FrameHook mFrameHooks[20];
    unsigned int mFrameHookCount = 0;
    al::FunctorBase* mEndHook = nullptr;
    al::FunctorBase* mCancelHook = nullptr;
};
