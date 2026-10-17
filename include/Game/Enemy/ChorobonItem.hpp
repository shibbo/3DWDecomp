#pragma once
#include "Enemy/ChorobonNode.hpp"

class ChorobonItem : public ChorobonNode {
public:
    ChorobonItem(const char* pName, const f32* pSpeed);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void updateItemPosture();
    void exeRailMove();
    /** @brief Gets the carried item. @return Item actor. */
    al::LiveActor* getItem() const { return mItem; }
private:
    al::LiveActor* mItem = nullptr;
    s32 mItemType = -1;
    f32 mItemOffsetY = -50.0f;
    bool mIsPhysicalItem = true;
    sead::Vector3f mClippingCenter = sead::Vector3f(0.0f, 0.0f, 0.0f);
};
static_assert(sizeof(ChorobonItem) == 0x170);
