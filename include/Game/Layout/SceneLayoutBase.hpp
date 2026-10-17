#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;
}  // namespace al

/**
 * @brief Interface of the scene HUD layouts that receive items put into the item stock.
 */
class SceneLayoutBase {
public:
    virtual void stockItem(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                           s32 itemType) = 0;
    virtual void stockItemSilent(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                                 s32 itemType) = 0;
};
