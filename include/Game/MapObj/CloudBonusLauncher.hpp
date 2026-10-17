#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Play/Placement/PlacementId.hpp"

namespace al {
class AreaObj;
class AreaObjGroup;
class CameraInfo;
class CameraTicket;
class WipeSimple;
}  // namespace al

class ActorStateSupportStroke;
class BindPuppeteerGroup;
class CloudBonusLauncherBindPuppeteer;
class GameSkyProjection;

/**
 * @brief The cannon that shoots the players into a cloud bonus area and brings them back.
 */
class CloudBonusLauncher : public al::LiveActor {
public:
    typedef sead::PtrArray<CloudBonusLauncherBindPuppeteer> PuppeteerArray;

    CloudBonusLauncher(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;

    virtual bool receiveMsgCloudBonusLauncher(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                              al::HitSensor* pSelf);

    bool receiveMsgCloudBonusStart(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                   al::HitSensor* pSelf);
    bool receiveMsgCloudBonusEnd(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                 al::HitSensor* pSelf);
    void endBindAllPuppet();
    void startCloudCamera(const al::LiveActor* pActor, al::CameraTicket* pTicket,
                          sead::Vector3f* pCameraPos, sead::Vector3f* pCameraLookAt);
    void restartLauncher(bool isRestartWait);

    void exeWait();
    /// Does nothing (folded with CloudBonusLauncherBindPuppeteer::exeNothing in the game).
    void exeNothing();
    void exeReaction();
    void exeReactionSlide();
    void exeIn();
    void exeInWait();
    void exeInForce();
    void exeInForceIn();
    void exeWaitAllBind();
    void exeLaunchSign();
    void exeLaunchStart();
    void exeLaunchEnd();
    void exeBonusStartWarpStart();
    void exeBonusStartWarpEnd();
    void exeEnd();
    void exeBonus();
    void exeWaitAllBindBonusEnd();
    void exeBonusEndWarpStart();
    void exeBonusEndWarpEnd();
    void exeBonusEndCameraEnd();

private:
    bool mIsInBonus = false;                                   // 0x144
    sead::Matrix34f mDestMtx = sead::Matrix34f::ident;         // 0x148
    sead::Matrix34f mEndMtx = sead::Matrix34f::ident;          // 0x178
    al::CameraInfo* mInWaitCamera = nullptr;                   // 0x1a8
    al::CameraInfo* mInWaitObjCamera = nullptr;                // 0x1b0
    al::CameraTicket* mInWaitCameraRS = nullptr;               // 0x1b8
    sead::Vector3f mInWaitCameraPos = sead::Vector3f::zero;    // 0x1c0
    sead::Vector3f mInWaitCameraLookAt = sead::Vector3f::zero; // 0x1cc
    al::CameraInfo* mLaunchCamera = nullptr;                   // 0x1d8
    al::CameraInfo* mLaunchObjCamera = nullptr;                // 0x1e0
    al::CameraTicket* mLaunchCameraRS = nullptr;               // 0x1e8
    sead::Vector3f mLaunchCameraPos = sead::Vector3f::zero;    // 0x1f0
    sead::Vector3f mLaunchCameraLookAt = sead::Vector3f::zero; // 0x1fc
    al::CameraInfo* mEndObjCamera = nullptr;                   // 0x208
    al::CameraTicket* mEndCameraRS = nullptr;                  // 0x210
    al::CameraTicket* mActiveCameraRS = nullptr;               // 0x218
    bool mIsStarted = false;                                   // 0x220
    bool mIsSingleMode = false;                                // 0x221
    s32 mForceBindNum = 0;                                     // 0x224
    s32 mLaunchIndex = 0;                                      // 0x228
    al::WipeSimple* mWipe = nullptr;                           // 0x230
    const char* mBgmName = nullptr;                            // 0x238
    al::AreaObjGroup* mEndAreaGroup = nullptr;                 // 0x240
    al::AreaObj* mEndCameraArea = nullptr;                     // 0x248
    BindPuppeteerGroup* mPuppeteerGroup = nullptr;             // 0x250
    PuppeteerArray mBindPuppeteers;                            // 0x258
    ActorStateSupportStroke* mSupportStroke = nullptr;         // 0x268
    al::PlacementId* mDestPlacementId = new al::PlacementId();  // 0x270
    al::PlacementId* mEndPlacementId = new al::PlacementId();   // 0x278
    GameSkyProjection* mSky = nullptr;                         // 0x280
    s32 mQuadrant = 0;                                         // 0x288
    bool mIsAllowRestart = false;                              // 0x28c
    al::HitSensor* mLastBindPlayerSensor = nullptr;            // 0x290
};

static_assert(sizeof(CloudBonusLauncher) == 0x298);
