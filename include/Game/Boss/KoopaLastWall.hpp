#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class CollisionObj; }

class KoopaLastWall : public al::LiveActor {
public:
    explicit KoopaLastWall(const char* pName);
    ~KoopaLastWall() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void start();
    void exeWait();
    void exeSign();
    void exeBreak();

private:
    al::CollisionObj* mBreakCollision = nullptr;
};
static_assert(sizeof(KoopaLastWall) == 0x150);
