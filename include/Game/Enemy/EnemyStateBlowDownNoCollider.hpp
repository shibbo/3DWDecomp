#pragma once

#include "Enemy/EnemyStateBlowDown.hpp"

class EnemyStateBlowDownNoCollider : public EnemyStateBlowDown {
public:
    EnemyStateBlowDownNoCollider(al::LiveActor* pHost,
        const EnemyStateBlowDownParam* pParam, bool* pIsOnGround);
    /** @brief Destroys the knockback state. */
    ~EnemyStateBlowDownNoCollider() override = default;
    void appear() override;
    void exeDown();

private:
    bool* mIsOnGround;
};

static_assert(sizeof(EnemyStateBlowDownNoCollider) == 0x40);
