#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class SePlayObj : public al::LiveActor {
public:
    SePlayObj(const char* pName);
    ~SePlayObj() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void switchTrampleOff();
    void initWithAudioKeeper(const al::ActorInitInfo& rInfo, const char* pAudioKeeperName);
    void exeWait();
    void exeAttached();

    /**
     * @brief Initialize the sound object so that it follows the actor it is attached to.
     * @param rInfo Placement information.
     * @param pAudioKeeperName Name of the audio keeper to use.
     */
    void initAttached(const al::ActorInitInfo& rInfo, const char* pAudioKeeperName) {
        mIsAttached = true;
        mUpdatePose = false;
        mAudioKeeperName = pAudioKeeperName;
        init(rInfo);
    }

    /**
     * @brief Initialize the sound object with its default audio keeper so that it follows the
     * actor it is attached to.
     * @param rInfo Placement information.
     */
    void initAttached(const al::ActorInitInfo& rInfo) {
        mIsAttached = true;
        mUpdatePose = false;
        init(rInfo);
    }

    /**
     * @brief Initialize the sound object as an attached one whose pose is set by its owner.
     * @param rInfo Placement information.
     */
    void initAttachedUpdatePose(const al::ActorInitInfo& rInfo) {
        mIsAttached = true;
        mUpdatePose = true;
        init(rInfo);
    }

    /**
     * @brief Inline form of initWithAudioKeeper: an attached sound object whose pose is set by
     * its owner, playing through the given audio keeper.
     * @param rInfo Placement information.
     * @param pAudioKeeperName Name of the audio keeper to use.
     */
    void initAttachedUpdatePose(const al::ActorInitInfo& rInfo, const char* pAudioKeeperName) {
        mAudioKeeperName = pAudioKeeperName;
        mIsAttached = true;
        mUpdatePose = true;
        init(rInfo);
    }

private:
    const char* mSeName = nullptr;
    bool mIsValidClipping = false;
    float mClippingRadius = 0.0f;
    bool mIsStartSeBySwitch = false;
    bool mSwitchHandled = false;
    bool mIsAttached = false;
    bool mUpdatePose = false;
    const char* mAudioKeeperName = "SePlayObj";
};
