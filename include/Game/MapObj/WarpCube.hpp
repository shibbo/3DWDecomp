#pragma once

#include <container/seadPtrArray.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class CameraInfo;
class CameraTicket;
class MtxConnector;
class PlacementId;
}  // namespace al

class ActorStateDemoCamera;
class ActorStateDemoCameraParam;
class BindPuppeteerGroup;
class CounterWarpCube;
class WarpCubeBindPuppeteer;

/**
 * @brief A warp box: players that enter it are carried to its linked destination box.
 */
class WarpCube : public al::LiveActor {
public:
    /// The "Type" placement argument.
    enum Type : s32 {
        Type_Normal = 0,   ///< Can be entered and exited.
        Type_OutOnly = 1,  ///< Exit only: only appears while players come out of it.
        Type_OneWay = 2,   ///< Disappears after one warp.
    };

    using BindPuppeteerArray = sead::PtrArray<WarpCubeBindPuppeteer>;

    explicit WarpCube(const char* pName);
    ~WarpCube() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void appearBySwitch();
    void initAfterPlacement() override;
    void appear() override;
    void appearByKoopaChase();
    void control() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    void startOutEnd(bool isKill);
    void endGetPiece(al::HitSensor* pSensor);
    bool isTypeOutOnly() const;
    void startControl(const char* pActionName);
    bool isControlled() const;
    void startOutSign();
    void setPlayerTransToDest();
    void endWarp();
    void endBindAllPuppet();
    void updateShadowScalePlayerIn(bool isDest);
    void exeAppear();
    void exeAppearWithCamera();
    void exeWait();
    void exePlayerIn();
    void exeWaitStartWarp();
    void exeAllBindForceInit();
    void exeAllBindForce();
    void exeAllBindForceIn();
    void exeOutSign();
    void exeOutSignWithCamera();
    void exeOutSignWithCameraWait();
    void exeOut();
    void exeWaitPiece();
    void exeEndPieceDemo();
    void exeControlled();
    void exeDisappear();

private:
    /**
     * @brief Check whether the destination has an object camera for the exit.
     * @return Whether the out camera of the destination exists.
     */
    bool isUseCameraTicket() const {
        return mIsSingleMode && mDestCube->mObjectCameraTicket != nullptr;
    }

    /**
     * @brief Stop the object camera of the destination started by the exit.
     */
    inline void endOutCamera();

    BindPuppeteerGroup* mPuppeteerGroup = nullptr;             // 0x148
    BindPuppeteerArray mBindPuppeteers;                        // 0x150
    al::LiveActor* mLockedModel = nullptr;                     // 0x160
    WarpCube* mDestCube = nullptr;                             // 0x168
    CounterWarpCube* mCounter = nullptr;                       // 0x170
    Type mType = Type_Normal;                                  // 0x178
    al::PlacementId* mPlacementId;                             // 0x180
    ActorStateDemoCamera* mEndPieceDemo = nullptr;             // 0x188
    ActorStateDemoCameraParam* mEndPieceDemoParam = nullptr;   // 0x190
    s32 mWarpStep = 0;                                         // 0x198
    al::CameraTicket* mObjectCameraTicket = nullptr;           // 0x1a0
    al::CameraInfo* mObjectCamera = nullptr;                   // 0x1a8
    s32 mOutCameraPlayStep = 0;                                // 0x1b0
    ActorStateDemoCamera* mSwitchAppearDemo = nullptr;         // 0x1b8
    ActorStateDemoCameraParam* mSwitchAppearDemoParam = nullptr;  // 0x1c0
    al::MtxConnector* mMtxConnector = nullptr;                 // 0x1c8
    al::HitSensor* mPieceSensor = nullptr;                     // 0x1d0
    bool mIsOutSpeedZero = false;                              // 0x1d8
    bool mIsBgmChangeWhenPieceComplete = false;                // 0x1d9
    s32 mBindForceNum = 0;                                     // 0x1dc
    sead::Vector3f mShadowMaskSize = {125.0f, 500.0f, 125.0f};  // 0x1e0
    bool mIsSingleMode = false;                                // 0x1ec
};

static_assert(sizeof(WarpCube) == 0x1f0);
