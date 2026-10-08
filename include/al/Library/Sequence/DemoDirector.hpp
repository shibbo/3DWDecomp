#pragma once

#include <basis/seadTypes.h>

namespace alSeFunction {
enum DemoType : s32;
}

namespace al {
class ActorInitInfo;
class AudioDirector;
class EffectSystem;
class LiveActor;

class DemoDirector {
public:
    DemoDirector(s32 maxActors);

    virtual void endInit(const ActorInitInfo& rInfo) {}
    virtual void registerActorWithGroup(LiveActor* pActor, const char* pName) {}
    virtual bool startDemo(const LiveActor* pActor, const char* pName);
    virtual void endDemo(const LiveActor* pActor, const char* pName);

    bool isActiveDemo() const;
    bool isActiveDemo(const LiveActor* pActor) const;
    bool isAnyActiveDemo() const;
    bool isOtherDemoRunning();
    void changeActiveAudioDemoType(alSeFunction::DemoType type);
    void setIsOtherDemoRunning(LiveActor* pActor, bool isRunning);
    const char* getActiveDemoName() const;
    bool requestStartDemo(const LiveActor* pActor, const char* pName);
    bool tryRequestStartDemo(const LiveActor* pActor, const char* pName);
    void requestEndDemo(const LiveActor* pActor, const char* pName);
    void addDemoActor(LiveActor* pActor);
    void removeDemoActor(LiveActor* pActor);
    LiveActor** getDemoActorList() const;
    s32 getDemoActorNum() const;
    void updateDemoActor(EffectSystem* pEffectSystem);

    bool isImmediateDemoSwitch() const { return mIsImmediateDemoSwitch; }
    void resetImmediateDemoSwitch() { mIsImmediateDemoSwitch = false; }

    /**
     * @brief Sets the audio director the demos change the audio demo type of.
     * @param pAudioDirector The audio director.
     */
    void setAudioDirector(AudioDirector* pAudioDirector) { mAudioDirector = pAudioDirector; }

    /**
     * @brief Gets the audio demo type of the active demo.
     * @return The audio demo type.
     */
    s32 getAudioDemoType() const { return mAudioDemoType; }

    /**
     * @brief Checks whether the audio demo type changed since it was last read.
     * @return Whether the audio demo type changed.
     */
    bool isChangedAudioDemoType() const { return mIsChangedAudioDemoType; }

    /**
     * @brief Clears the audio demo type change flag.
     */
    void resetChangedAudioDemoType() { mIsChangedAudioDemoType = false; }

    /**
     * @brief Sets the unknown flag at 0xd4 (set while the Bowser's Fury intro plays).
     * @param isSet The flag value.
     */
    void setUnknownD4(bool isSet) { _d4 = isSet; }

    /** @brief Gets the unknown flag at 0xd4. @return The flag value. */
    bool isUnknownD4() const { return _d4; }

    /** @brief Gets the unknown flag at 0xd5. @return The flag value. */
    bool isUnknownD5() const { return _d5; }

    /** @brief Gets the unknown flag at 0xe1. @return The flag value. */
    bool isUnknownE1() const { return _e1; }

    /** @brief Sets the unknown flag at 0xe1. @param isSet The flag value. */
    void setUnknownE1(bool isSet) { _e1 = isSet; }

    /** @brief Sets the unknown flag at 0xe2. @param isSet The flag value. */
    void setUnknownE2(bool isSet) { _e2 = isSet; }

    /** @brief Gets the unknown flag at 0xe2. @return The flag value. */
    bool isUnknownE2() const { return _e2; }

    /** @brief Gets the unknown flag at 0xe3. @return The flag value. */
    bool isUnknownE3() const { return _e3; }

    /** @brief Sets the unknown flag at 0xe3. @param isSet The flag value. */
    void setUnknownE3(bool isSet) { _e3 = isSet; }

    const char* mActiveDemoName = nullptr;
    LiveActor** mDemoActors = nullptr;
    s32 mDemoActorNum = 0;
    s32 mDemoActorMax;
    LiveActor** mAddDemoActors = nullptr;
    s32 mAddDemoActorNum = 0;
    LiveActor* mOtherDemoActors[20];
    s32 mAudioDemoType = 0;
    bool _d4 = false;
    bool _d5 = false;
    bool mIsImmediateDemoSwitch = false;
    bool mIsUpdatingDemoActor = false;
    AudioDirector* mAudioDirector;
    bool mIsChangedAudioDemoType;
    bool _e1 = false;
    bool _e2 = false;
    bool _e3 = false;
    const LiveActor* mActiveDemoActor = nullptr;
};

static_assert(sizeof(DemoDirector) == 0xf0);
}  // namespace al
