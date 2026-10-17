#pragma once

#include <prim/seadSafeString.h>
#include "NPC/GhostPlayerDisplayInfo.hpp"

class GhostPlayerDisplayInfo;

class GhostPlayerLoaderBase {
public:
    explicit GhostPlayerLoaderBase(const char* pName);
    virtual bool needAlloc();
    virtual bool load(u8** pData, u32 bufferSize, int index, int playerIndex) = 0;
    virtual int getGhostPlayDataNum() const = 0;
    virtual void extractGhostDataUserName(sead::BufferedSafeString* pName, int index) const = 0;

    /**
     * @brief Supplies the default display record for a loader.
     * @param index Recording index; unused by the base implementation.
     * @return nullptr because the base loader has no display records.
     */
    virtual const GhostPlayerDisplayInfo* getDisplayInfo(int index) const { return nullptr; }
    virtual void extractFileName(sead::BufferedSafeString* pFileName, int index) const;

private:
    const char* mName;
};
