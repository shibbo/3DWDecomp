#pragma once
#include <prim/seadSafeString.h>
class GhostPlayerDisplayInfo;
class GhostPlayerLoaderBase {
public:
    GhostPlayerLoaderBase(const char*);
    virtual bool needAlloc() { return true; }
    virtual bool load(u8**, u32, int, int) = 0;
    virtual int getGhostPlayDataNum() const = 0;
    virtual void extractGhostDataUserName(sead::BufferedSafeString*, int) const = 0;
    virtual const GhostPlayerDisplayInfo* getDisplayInfo(int) const { return nullptr; }
    virtual void extractFileName(sead::BufferedSafeString*, int) const {}
private:
    const char* mName;
};
