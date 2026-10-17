#pragma once

#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>

namespace al {
class ActorInitInfo;
class LiveActor;
}
class TentackAttachItem;

/** @brief Holds the items Tentack's tentacles can carry. */
class TentackAttachItemHolder {
public:
    TentackAttachItemHolder();
    void init(const al::ActorInitInfo& rInfo);
    TentackAttachItem* attachItem(al::LiveActor* pHost, s32 type);
    TentackAttachItem* tryAttachItem(al::LiveActor* pHost, s32 type);

private:
    sead::PtrArray<TentackAttachItem> mItems;
};
static_assert(sizeof(TentackAttachItemHolder) == 0x10);
