#pragma once

#include <basis/seadTypes.h>

#include "Layout/ItemStockLayoutBase.hpp"
#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
class LiveActor;
}  // namespace al

class ButtonTouch;
class ProjectItemDirector;
class StockBlurEffect;

/**
 * @brief Item-stock button parts layout: the box showing the stocked items (in a stage or on the
 * course select screen). Stocked items fly into it as blur effects, and touching it or pressing
 * up / X uses the stocked item.
 */
class ButtonItemStockParts : public al::LayoutActor, public ItemStockLayoutBase {
public:
    ButtonItemStockParts(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPaneName,
                         al::LayoutActor* pParent, ProjectItemDirector* pItemDirector,
                         bool isCourseSelect);

    void control() override;
    void updateIcon();
    bool isDisable() const;
    void setDisable();
    void stockItem(const al::LiveActor* pItem, const al::LiveActor* pActor, s32 itemType);
    void startDemo();
    void endDemo();
    bool useItem(s32 port);

    void exeDisable();
    void exeDemoWait();
    void exeWait();
    void exeWaitCourseSelect();
    void exeNoItemHide();
    void exeNoItemWait();
    void exeStockEffect();
    void exeStock();
    void exeUse();

private:
    void updateItemAction();
    bool isExistStockItem() const;
    const al::Nerve* getStockWaitNerve() const;
    const al::Nerve* getWaitNerve() const;
    void updateBox();
    StockBlurEffect* takeBlurEffect();
    bool tryUseItemByPad(s32 port);

    ButtonTouch* mTouch = nullptr;                // 0x130
    ProjectItemDirector* mItemDirector;           // 0x138
    StockBlurEffect** mBlurEffects = nullptr;     // 0x140
    s32 mActiveUserNum = -1;                      // 0x148
    bool mIsDemo = false;                         // 0x14c
    bool mIsCourseSelect;                         // 0x14d
};

static_assert(sizeof(ButtonItemStockParts) == 0x150);
