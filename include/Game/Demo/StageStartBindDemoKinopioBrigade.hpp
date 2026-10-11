#pragma once

#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Demo/StageStartEventBase.hpp"

namespace al {
class CameraInfo;
}  // namespace al

class WindowMessage;
class BindPuppeteerGroup;
class IUsePlayerPuppet;

/** @brief Binds the players while the Captain Toad stage introduction plays. */
class StageStartBindDemoKinopioBrigade : public StageStartEventBase {
public:
    /** @brief Steps of a single brigade member's entrance. */
    enum class IntroState : s32 {
        Delay,
        Appear,
        Rise,
        Fall,
        Walk,
        Turn,
        End,
    };

    /** @brief Placement and progress of one brigade member's entrance. */
    struct BrigadeIntro {
        sead::Matrix34f mBaseMtx;
        sead::Vector3f mStartTrans;
        sead::Vector3f mFrontDir;
        s32 mDelay = 0;
        s32 mTimer = 0;
        IntroState mState = IntroState::Delay;
    };

    static_assert(sizeof(BrigadeIntro) == 0x54);

    explicit StageStartBindDemoKinopioBrigade(const char* pName);
    /** @brief Destroys the opening event. */
    ~StageStartBindDemoKinopioBrigade() override = default;
    void init(const al::ActorInitInfo& rInfo) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    void warpDemoEndPos(bool isEndBind);
    bool updateBrigadeIntro(s32 index);
    void control() override;
    /** @brief Identifies a player-binding event. @return Event type 3. */
    s64 getEventType() const override { return 3; }
    /** @brief Activates the opening event. */
    void startDemo() override { appear(); }
    void endDemo() override;
    bool isEndDemo() const override;
    void exeBindWait();
    void exeCamera();
    void exeDokanAppear();
    void exeKinopioIntro();
    void exeWalk();
    void exeDokanDisappear();
    void exeMessage();
    void exeBindEnd();
    void exeGuide();

private:
    bool isPlayingDemo() const;
    IUsePlayerPuppet* findFirstBindPuppet() const;

    WindowMessage* mWindow = nullptr;
    BindPuppeteerGroup* mPuppeteers = nullptr;
    al::CameraInfo* mCamera = nullptr;
    sead::FixedSafeString<128> mCameraName;
    sead::Matrix34f mCameraMtx = sead::Matrix34f::ident;
    u8 _228[8];
    al::LiveActor* mDokan = nullptr;
    sead::Vector3f mWalkStartTrans = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mFirstPlayerTrans;
    sead::Vector3f mFrontDir;
    bool mIsTeresaStage = false;
    bool mIsStartAppearSe = false;
    BrigadeIntro* mBrigadeIntros = nullptr;
    BrigadeIntro* mGuideIntro = nullptr;
};

static_assert(sizeof(StageStartBindDemoKinopioBrigade) == 0x270);
