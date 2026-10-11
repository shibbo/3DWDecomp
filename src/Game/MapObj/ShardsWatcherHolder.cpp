#include "MapObj/ShardsWatcherHolder.hpp"
#include "MapObj/ShardsWatcher.hpp"
ShardsWatcherHolder::ShardsWatcherHolder(int capacity) { mWatchers.allocBuffer(capacity, nullptr); }
void ShardsWatcherHolder::registerShardWatcher(ShardsWatcher* pWatcher) { mWatchers.pushBack(pWatcher); }
const ShardsWatcher* ShardsWatcherHolder::getShardWatcher(int id) const {
    for (int i = 0; i < mWatchers.size(); ++i) {
        if (mWatchers[i]->getId() == id) return mWatchers[i];
    }
    return nullptr;
}
ShardsWatcher* ShardsWatcherHolder::getShardWatcherPtr(int id) {
    for (int i = 0; i < mWatchers.size(); ++i) {
        if (mWatchers[i]->getId() == id) return mWatchers[i];
    }
    return nullptr;
}
const char* ShardsWatcherHolder::getSceneObjName() const { return "Shard Watcher Holder"; }
