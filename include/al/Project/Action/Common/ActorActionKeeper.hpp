#pragma once

#include <basis/seadTypes.h>

namespace al {
class ActionAnimCtrl;
class ActionBgmCtrl;
class ActionEffectCtrl;
class ActionFlagCtrl;
class ActionOceanWaveCtrl;
class ActionPadAndCameraCtrl;
class ActionScreenEffectCtrl;
class ActionSeCtrl;
class LiveActor;
class NerveActionCtrl;

class ActorActionKeeper {
public:
    static ActorActionKeeper* tryCreate(LiveActor* pActor, const char* pArchiveName,
                                        const char* pSuffix);

    ActorActionKeeper(LiveActor* pActor, const char* pActorName, ActionAnimCtrl* pAnimCtrl,
                      NerveActionCtrl* pNerveActionCtrl, ActionFlagCtrl* pFlagCtrl,
                      ActionEffectCtrl* pEffectCtrl, ActionSeCtrl* pSeCtrl,
                      ActionBgmCtrl* pBgmCtrl, ActionOceanWaveCtrl* pOceanWaveCtrl,
                      ActionPadAndCameraCtrl* pPadAndCameraCtrl,
                      ActionScreenEffectCtrl* pScreenEffectCtrl);

    bool startAction(const char* pActionName);
    void tryStartActionNoAnim(const char* pActionName);
    void startBgmAction(const char* pActionName);
    void startEffectAction(const char* pActionName);
    void updatePrev();
    void updatePost();
    void updateSeActionCtrl();
    void tryUpdateSeEffect(f32 frameFrom, f32 frameTo);
    void init();

    ActionAnimCtrl* getAnimCtrl() const { return mAnimCtrl; }
    ActionEffectCtrl* getEffectCtrl() const { return mEffectCtrl; }
    ActionSeCtrl* getSeCtrl() const { return mSeCtrl; }
    ActionBgmCtrl* getBgmCtrl() const { return mBgmCtrl; }
    ActionPadAndCameraCtrl* getPadAndCameraCtrl() const { return mPadAndCameraCtrl; }

private:
    LiveActor* mActor;
    const char* mActorName;
    bool mIsActionStarted;
    ActionAnimCtrl* mAnimCtrl;
    NerveActionCtrl* mNerveActionCtrl;
    ActionFlagCtrl* mFlagCtrl;
    ActionEffectCtrl* mEffectCtrl;
    ActionSeCtrl* mSeCtrl;
    ActionBgmCtrl* mBgmCtrl;
    ActionOceanWaveCtrl* mOceanWaveCtrl;
    ActionPadAndCameraCtrl* mPadAndCameraCtrl;
    ActionScreenEffectCtrl* mScreenEffectCtrl;
};

static_assert(sizeof(ActorActionKeeper) == 0x60);
}  // namespace al
