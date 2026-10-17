#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include "Util/ItemUtil.hpp"

namespace al {
class AreaObj;
}  // namespace al

class GoalItem;
class GuideBalloon;

/**
 * @brief Bowser Jr.'s graffiti in Bowser's Fury: painted over by Bowser Jr. when the player points
 * at it, it can hand out an item or a Cat Shine and is remembered per island phase.
 */
class KoopaGraffiti : public al::LiveActor {
public:
    KoopaGraffiti(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void createItem(const al::ActorInitInfo& rInfo);
    void setVandalizedAnim(bool isPainted);
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    bool receiveMsgScreenPointSM(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) override;
    void appear() override;
    void kill() override;
    void startClipped() override;
    void reset();
    void exeWait();
    void updateGuideMessage(bool isShow);
    void exeBroadcast();
    void exePainting();
    void exeFinish();
    void appearItem();
    void exeGoalItemAppear();

private:
    al::LiveActor* mItem = nullptr;                  // 0x148
    rc::ItemType mItemType = rc::ItemType(-1);       // 0x150
    GoalItem* mGoalItem = nullptr;                   // 0x158
    al::AreaObj* mCameraArea = nullptr;              // 0x160
    GuideBalloon* mGuideBalloon = nullptr;           // 0x168
    s32 mPaintCount = 0;                             // 0x170
    s32 mZoneId = -1;                                // 0x174
    s32 mType = 0;                                   // 0x178
    s32 mGraffitiId = 0;                             // 0x17C
    f32 mItemVelY = 11.0f;                           // 0x180
    f32 mItemVelZ = 5.0f;                            // 0x184
    bool mIsAppearItem = true;                       // 0x188
    bool mIsPlayerTouch = false;                     // 0x189
    bool mIsKoopaJrTouch = false;                    // 0x18A
    bool mIsShowGuideWindow = false;                 // 0x18B
    bool mIsShowKoopaJrGuide = false;                // 0x18C
    bool mIsBroadcasting = false;                    // 0x18D
    bool mIsMono = false;                            // 0x18E
    bool mIsPainted = false;                         // 0x18F
    bool mIsSwitchOnInstant = false;                 // 0x190
};

static_assert(sizeof(KoopaGraffiti) == 0x198);
