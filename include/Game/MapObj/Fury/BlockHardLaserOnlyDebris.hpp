#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BlockHardLaserOnly;
namespace al { class MtxConnector; }
class BlockHardLaserOnlyDebris : public al::LiveActor {
public:
    BlockHardLaserOnlyDebris(const char*, BlockHardLaserOnly*);
    ~BlockHardLaserOnlyDebris() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void tryAppear();
    void control() override;
private:
    al::MtxConnector* mConnector = nullptr;
    BlockHardLaserOnly* mParent;
};
static_assert(sizeof(BlockHardLaserOnlyDebris) == 0x158);
