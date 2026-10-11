#pragma once

#include <basis/seadTypes.h>

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
    unsigned char mUnknown[0x10];
};
static_assert(sizeof(TentackAttachItemHolder) == 0x10);
