#pragma once

#include <math/seadVector.h>

#include "Demo/DemoObjBase.hpp"

namespace al {
class ActorInitInfo;
class AreaObj;
class HitSensor;
class SensorMsg;
class WipeSimple;
}  // namespace al

namespace alSeFunction {
enum DemoType : s32;
}  // namespace alSeFunction

class BindPuppeteerGroup;

/**
 * @brief Cutscene demo actor placed in a stage.
 *
 * Optionally binds every player (through a group of bind puppeteers) while the cutscene plays,
 * fades the screen in and out around it, shows a guide window afterwards and records the
 * cutscene as seen in the save data.
 */
class DemoCutscene : public DemoObjBase {
public:
    DemoCutscene(const char* pName, alSeFunction::DemoType type);

    void init(const al::ActorInitInfo& rInfo) override;
    void initPuppets(const al::ActorInitInfo& rInfo);
    void createWipeFade(const al::ActorInitInfo& rInfo, bool isUseWipeFade, s32 frames);
    void setEndAtCutscenePos(const sead::Vector3f& rTrans, const sead::Vector3f& rFront);
    void setEndSceneFlag() override;

    /** @brief Do not end the current phase once the cutscene is done. */
    void unsetEndSceneFlag() override { mIsEndPhaseWhenDone = false; }

    bool requestStartDemo();
    void tryStartDemo();
    void startDemo() override;
    void exeWaitCutsceneStart();
    void exeWaitOnGround();
    void exeBindWait();
    void exeBindEndFade();
    void restorePlayerPosition();
    void exeBindEnd();
    void exeFinalEndDemo();
    void exeBindEndWait();
    void exeFadeToBlackStart();
    void tryHidePlayers();
    void exeAllBindDone();
    void exePlay() override;
    void exeEndDemo();
    void setGuideWindowState();
    void endDemoCutscene();
    void exeGuideWindow();
    void exeFadeOutGuideWindow();
    bool isFadeDone() const;
    bool isFadedOut();
    bool isWipeCloseEnd() const;
    void cancelSE();
    void endDemo(bool setWaitState) override;
    void setPuppetsToWait();
    void forceSwitchOff();
    virtual void trueStartDemo();
    void overrideBaseMtx(const sead::Matrix34f* pMtx) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void endAndResetDemo();

    /** @brief Keep the player where it stands when the cutscene ends. */
    void setKeepPlayerPos() { mIsKeepPlayerPos = true; }

    /** @brief Hide the player during the cutscene. */
    void setHidePlayer() { mIsHidePlayer = true; }

    /** @brief Skip the wipe at the end of the cutscene. */
    void setSkipEndWipe() { mIsSkipEndWipe = true; }

    /** @brief Place the cutscene at the matrix given with overrideBaseMtx(). */
    void setUseBaseMtx() { mIsUseBaseMtx = true; }

    /**
     * @brief Marks the cutscene as directly followed by another one.
     * @param isFollowed Whether another cutscene plays right after this one.
     */
    void setFollowedByDemo(bool isFollowed) { mIsFollowedByDemo = isFollowed; }

    /**
     * @brief Set the id of the cutscene saved as seen once it played.
     * @param cutsceneId The cutscene id.
     */
    void setCutsceneId(s32 cutsceneId) { mCutsceneId = cutsceneId; }

    s32 getCutsceneId() const { return mCutsceneId; }

    /**
     * @brief Set whether the cutscene waits for every player to be on the ground (0x300).
     * @param isSet The flag.
     */
    void setUnk300(bool isSet) { mIsRequireOnGround = isSet; }

    /**
     * @brief Set whether the players are bound while the cutscene plays (0x302).
     * @param isSet The flag.
     */
    void setUnk302(bool isSet) { mIsBindPlayer = isSet; }

    /**
     * @brief Set the flag at 0x32a (skips ending the player cutscene demo).
     * @param isSet The flag.
     */
    void setUnk32a(bool isSet) { _32a = isSet; }

    /**
     * @brief Set the flag at 0x32b (skips requesting the cutscene demo).
     * @param isSet The flag.
     */
    void setUnk32b(bool isSet) { _32b = isSet; }

private:
    /** @brief End the current phase if requested, otherwise end the cutscene. */
    void endPhaseOrCutscene();

    bool mIsRequireOnGround = false;  // 0x300
    bool mIsInvalidatePlayerInput = false;  // 0x301
    bool mIsBindPlayer;  // 0x302
    bool mIsEndOnGround = false;  // 0x303
    bool mIsKeepPlayerPos = false;  // 0x304
    bool mIsEndAtCutscenePos;  // 0x305
    bool mIsHidePlayer = false;  // 0x306
    bool mIsSkipEndWipe = false;  // 0x307
    bool mIsUseBaseMtx = false;  // 0x308
    s32 mHidePlayerStep = 0;  // 0x30C
    s32 mWipeFrames = -1;  // 0x310
    al::ActorInitInfo* mPlayerRestartInfo = nullptr;  // 0x318
    BindPuppeteerGroup* mPuppeteerGroup;  // 0x320
    bool mIsEndPhaseWhenDone = false;  // 0x328
    bool mIsUseWipeFade = false;  // 0x329
    bool _32a = false;
    bool _32b = false;
    bool mIsFollowedByDemo;  // 0x32C
    bool mIsFadingOut = false;  // 0x32D
    bool mIsFadeOutEnd = false;  // 0x32E
    s32 mCutsceneId = -1;  // 0x330
    al::AreaObj* mCameraArea = nullptr;  // 0x338
    al::WipeSimple* mWipeFade = nullptr;  // 0x340
    sead::Vector3f mEndTrans;  // 0x348
    sead::Vector3f mEndFront;  // 0x354
    alSeFunction::DemoType mDemoType;  // 0x360
};

static_assert(sizeof(DemoCutscene) == 0x368);
