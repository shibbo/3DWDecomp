#pragma once
#include "Project/Block/BlockRailRider.hpp"

class IUsePlayerPuppet;
class PuppetStickRouteSelecter : public al::BlockRailRouteSelecter {
public:
    PuppetStickRouteSelecter(int capacity);
    void clearPuppetAll();
    void addPuppet(IUsePlayerPuppet* pPuppet);
    void clearPuppet(IUsePlayerPuppet* pPuppet);
    bool compareBlockRailRoute(const al::BlockRailRider* pRider, const al::BlockRailLink* pLinkA,
                              const al::BlockRailLink* pLinkB) const override;
private:
    IUsePlayerPuppet** mPuppets;
    int mCount = 0;
    int mCapacity;
};
