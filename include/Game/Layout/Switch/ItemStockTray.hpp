#pragma once

#include <math/seadVector.h>

#include "Layout/ItemStockLayoutBase.hpp"
#include "Library/Layout/LayoutActor.hpp"
#include "Library/Nerve/IUseNerve.hpp"

namespace al {
class HitSensor;
class LayoutInitInfo;
class LiveActor;
class NerveKeeper;
}  // namespace al

class GameDataHolder;
class ItemStockCoinSpawner;
class ProjectItemDirector;
class StockBlurEffect;

/**
 * @brief Bowser's Fury item tray: shows the stocked items and lets the player pick one to use.
 */
class ItemStockTray : public al::LayoutActor, public ItemStockLayoutBase {
public:
    ItemStockTray(const al::LayoutInitInfo& rInfo, const char* pName, const char* pArchiveName,
                  al::LayoutActor* pParent, const GameDataHolder* pHolder,
                  ProjectItemDirector* pDirector);

    void setCount(s32 slot, s32 count);
    void control() override;
    void appear() override;
    void updateCountAndSelection(s32 mode);
    void startAppear(s32 port);
    void spawnCoins(u8 count, al::HitSensor* pSensor);
    void startDemo();
    void endDemo();
    void stockItem(const al::LiveActor* pItem, const al::LiveActor* pActor, s32 itemType,
                   bool isPlayEffect);
    bool canSpawnItem() const;
    bool isStartClose();
    void updateSelection(s32 slot);

    void exeAppear();
    void exeWaitActive();
    void exeWaitIdle();
    void exeTriggerMenu();
    void exeWaitUseItem();
    void exeWaitStockEffect();
    void exeStockEffect();
    void exeStock();
    void exeTrySpawnItem();
    void exeWaitReleaseButton();

    /**
     * @brief Get the controller port that opened the tray.
     * @return The port, or -1 if none.
     */
    s32 getPort() const { return mPort; }

    /**
     * @brief Check whether the tray is idle (closed and waiting).
     * @return True if idle.
     */
    bool isIdle() const { return mIsIdle; }
    void exeEnd();

private:
    bool isHoldItemButton() const;
    u32 getStockItemCount(s32 slot) const;
    StockBlurEffect* takeBlurEffect();
    void startStockEffect(const sead::Vector3f& rStartPos);
    void startSlotAction(const char* pFormat);

    const GameDataHolder* mGameDataHolder;           // 0x130
    ProjectItemDirector* mItemDirector;              // 0x138
    s32 mPort = -1;                                  // 0x140
    s32 mSelectedSlot = 0;                           // 0x144
    bool mIsIdle = true;                             // 0x148
    bool mIsEmpty = true;                            // 0x149
    bool mIsDemo = false;                            // 0x14a
    StockBlurEffect** mBlurEffects = nullptr;        // 0x150
    sead::Vector3f mStockEffectStartPos;             // 0x158
    al::LayoutActor** mSlotParts;                    // 0x168
    ItemStockCoinSpawner* mCoinSpawner;              // 0x170
    const al::LiveActor* mStockActor = nullptr;      // 0x178
};

static_assert(sizeof(ItemStockTray) == 0x180);

/**
 * @brief Spawns the coins given for picking up an item while the tray is full, one per step.
 */
class ItemStockCoinSpawner : public al::IUseNerve {
public:
    ItemStockCoinSpawner(ItemStockTray* pTray, ProjectItemDirector* pDirector);

    void spawnCoins(u8 count, al::HitSensor* pSensor);

    void exeWait();
    void exeAddCoins();

    al::NerveKeeper* getNerveKeeper() const override { return mNerveKeeper; }

private:
    void* _8 = nullptr;                              // 0x08
    ProjectItemDirector* mItemDirector;              // 0x10
    al::NerveKeeper* mNerveKeeper;                   // 0x18
    s32 mCoinNum = 0;                                // 0x20
    al::HitSensor* mSensor = nullptr;                // 0x28
    f32 mAngle = 0.0f;                               // 0x30
};

static_assert(sizeof(ItemStockCoinSpawner) == 0x38);
