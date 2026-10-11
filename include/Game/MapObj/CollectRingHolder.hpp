#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class CollectRing;
class CollectRingHolder : public al::LiveActor {
public:
    CollectRingHolder(const char* pName);
    ~CollectRingHolder() override;
    void init(const al::ActorInitInfo&) override;
    void noticeGet(CollectRing*);
    void appearItem(CollectRing*);
    void exeCountDown();
    void exeEnd();
private:
    al::DeriveActorGroup<CollectRing>* mRings = nullptr;
    int mCollectedCount = 0;
};
