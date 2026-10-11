#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>
class ShardsWatcher;
class ShardsWatcherHolder : public al::ISceneObj {
public:
    ShardsWatcherHolder(int);
    void registerShardWatcher(ShardsWatcher*);
    const ShardsWatcher* getShardWatcher(int) const;
    ShardsWatcher* getShardWatcherPtr(int);
    const char* getSceneObjName() const override;
private:
    sead::PtrArray<ShardsWatcher> mWatchers;
};
