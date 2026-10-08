#pragma once

#include <container/seadBuffer.h>
#include <math/seadVector.h>

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
    class ActorInitInfo;
    class ComboCounter;
    class HitSensor;
    class SensorMsg;
    class ScreenPointer;
};  // namespace al

class SnowCover;
class BlockEmpty;
class BlockStateHeadgear;
class BlockStateCoinTen;

/**
 * @brief Item state shared by item blocks (? blocks, bricks, transparent and assist blocks).
 *
 * Handles the punch reaction of the host block, the item that pops out of it (a normal item,
 * one per player, ten coins or a headgear box) and the empty block left behind.
 */
class BlockStateItem : public al::ActorStateBase {
public:
    BlockStateItem(al::LiveActor*, const al::ActorInitInfo&, bool, bool, bool, bool, bool);

    virtual ~BlockStateItem();

    bool isUseStateHeadgear() const;
    bool isItemType(int) const;
    bool isUseStateCoinTen() const;
    void appearCoinRandom10();
    void appearCoin10();
    bool reset();
    void attackSensor(al::HitSensor*, al::HitSensor*);
    bool trySendMsgToUpperLowerObj(al::HitSensor*, al::HitSensor*);
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    bool receiveMsgAndAppearItem(const al::SensorMsg*, al::HitSensor*);
    void validateHitSensorsForBlockLowerPunch();
    void validateHitSensorsForBlockUpperPunch();
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*);
    void validateHitSensorsForBlockDrcPunch();
    bool tryAppearItem(const al::SensorMsg*, al::HitSensor*, bool);
    bool isValidAppearItem() const;
    const char* collectAppearItemTiming(al::HitSensor*);
    void tryExecAppearItemLong(const al::HitSensor*);
    BlockEmpty* getBlockEmpty() const;
    BlockEmpty* tryGetBlockEmpty() const;
    BlockStateCoinTen* tryGetBlockStateCoinTen() const;
    void setNerveEnd();
    void exeWait();
    void exeAppearItem();
    void exeCoin10();
    void exeHeadgear();
    void exeEnd();

    bool isLong() const { return mIsLong; }
    void setConnectedRailBlock() { mIsConnectedRailBlock = true; }

    s32 mItemType = 0;                                // 0x20
    BlockEmpty* mBlockEmpty = nullptr;                // 0x28
    SnowCover* mSnowCover = nullptr;                  // 0x30
    BlockStateCoinTen* mStateCoinTen = nullptr;       // 0x38
    BlockStateHeadgear* mStateHeadgear = nullptr;     // 0x40
    sead::Vector3f mAppearDir = sead::Vector3f::ez;   // 0x48
    bool mIsPopUp = false;                            // 0x54
    bool mIsItemPerPlayer = false;                    // 0x55
    sead::Buffer<bool> mIsUserItemAppeared;           // 0x58
    bool mIsAppearEnd = false;                        // 0x68
    s32 mPlayerNum = 0;                               // 0x6C
    s32 mAppearCount = 0;                             // 0x70
    bool mIsLong;                                     // 0x74
    bool mIsTransparent;                              // 0x75
    bool mIsAssist;                                   // 0x76
    bool mIsConnectedRailBlock = false;               // 0x77
    s32 mReactionStep = 0;                            // 0x78
    al::HitSensor* mAppearSensor = nullptr;           // 0x80
    s32 mControlUserId = -1;                          // 0x88
    bool mIsForceChangeItem = false;                  // 0x8C
    bool mIsWallSide = false;                         // 0x8D
    bool mIsDrcPunch = false;                         // 0x8E
    al::ComboCounter* mComboCounter = nullptr;        // 0x90
};
