#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

#include "NPC/GhostPlayerDisplayInfo.hpp"

/** @brief Source of recorded ghost play data (ROM data or network data store). */
class GhostPlayerLoaderBase {
public:
    GhostPlayerLoaderBase(const char* pName);

    virtual bool load(u8** ppBuffer, u32 size) = 0;
    virtual bool load(u8** ppBuffer, u32 size, s32 index, s32 dataIndex) = 0;
    virtual s32 getGhostPlayDataNum() const = 0;
    virtual void calcDataSize(u32* pSize, s32 index) const = 0;
    virtual const GhostPlayerDisplayInfo* getDisplayInfo(s32 index) const;
};
