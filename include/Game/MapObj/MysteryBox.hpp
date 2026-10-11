#pragma once

#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class MtxConnector;
class PlacementId;
}  // namespace al

class BindPuppeteerGroup;
class CounterMysteryBox;
class GreenStar;
class PlayerBindEndParam;
class WarpCubeBindPuppeteer;

/**
 * @brief A mystery box: players that enter it are carried to a timed challenge room and are
 * brought back when they get its green star, hit its end switch or run out of time.
 */
class MysteryBox : public al::LiveActor {
public:
    /// The "PlayerAppearPos" placement argument of the destination box.
    enum PlayerAppearPos : s32 {
        PlayerAppearPos_Center = 0,  ///< Players are lined up around the box center.
        PlayerAppearPos_Side = 1,    ///< Players are lined up by their user id, from one side.
    };

    using BindPuppeteerArray = sead::PtrArray<WarpCubeBindPuppeteer>;

    explicit MysteryBox(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void endSwitchOn();
    void setDestMysteryBox(const MysteryBox* pDestBox);
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool tryCancelCountDown(bool isKill);
    bool isCountDown() const;
    bool isCountDownEnd() const;
    void warpPlayerPuppet(WarpCubeBindPuppeteer* pPuppeteer, const sead::Vector3f& rTrans,
                          const sead::Vector3f& rFront, const sead::Vector3f& rLocalVelocity,
                          const PlayerBindEndParam* pEndParam);
    bool tryCancelGreenStarGetOrSwitchOn();
    void updateShadowScalePlayerIn();
    void exeWait();
    void exeAppearWait();
    void exeAppear();
    void exePlayerIn();
    void exeWaitWarpBegin();
    void exeAllBindForceInit();
    void exeAllBindForce();
    void exeAllBindForceIn();
    void exeWarpBegin();
    void exeCountDownIntro();
    void exeCountDown();
    void exeWaitWarpReturn();
    void exeWarpReturn();
    void exePlayerOut();

    void setAutoCountDownCancel(bool enabled) { mAutoCountDownCancel = enabled; }

private:
    /**
     * @brief Hide the counter and stop the count down music.
     */
    inline void endCountDown();

    BindPuppeteerGroup* mPuppeteerGroup = nullptr;        // 0x148
    BindPuppeteerArray mBindPuppeteers;                   // 0x150
    sead::Matrix34f* mDestMtx = nullptr;                  // 0x160
    sead::Matrix34f* mEndMtx = nullptr;                   // 0x168
    s32 mWarpStep = 0;                                    // 0x170
    al::PlacementId* mPlacementId;                        // 0x178
    CounterMysteryBox* mCounter = nullptr;                // 0x180
    bool mAutoCountDownCancel = true;                     // 0x188
    al::MtxConnector* mMtxConnector = nullptr;            // 0x190
    GreenStar* mGreenStar = nullptr;                      // 0x198
    PlayerAppearPos mPlayerAppearPos = PlayerAppearPos_Center;  // 0x1a0
    s32 mBindForceNum = 0;                                // 0x1a4
    bool mIsDest = false;                                 // 0x1a8
    sead::Vector3f mShadowMaskSize = {125.0f, 500.0f, 125.0f};  // 0x1ac
    s32 mSeType = 0;                                      // 0x1b8
};

static_assert(sizeof(MysteryBox) == 0x1c0);
