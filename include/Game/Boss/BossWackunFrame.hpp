#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class BossWackun;

class BossWackunFrame : public al::LiveActor {
public:
    explicit BossWackunFrame(BossWackun* pBoss);
    ~BossWackunFrame() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void exeWait();
    void exeDamage();
    void setDamage();
    void revival();

private:
    al::LiveActor* mBoss;
};
