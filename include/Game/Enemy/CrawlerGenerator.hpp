#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; template <class T> class DeriveActorGroup; }
class Crawler;
class CrawlerLaunchState;

/** @brief Repeatedly launches a reusable pool of crawlers. */
class CrawlerGenerator : public al::LiveActor {
public:
    explicit CrawlerGenerator(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    void stopBySwitch();
    void exeStop();
    void exeLaunchInterval();
    void exeLaunchSign();
    void exeLaunch();
private:
    bool mIsLauncher = false;
    s32 mWaitFrame = 180;
    al::DeriveActorGroup<Crawler>* mGroup = nullptr;
    al::MtxConnector* mConnector = nullptr;
    CrawlerLaunchState* mLaunch = nullptr;
};
static_assert(sizeof(CrawlerGenerator) == 0x168);
