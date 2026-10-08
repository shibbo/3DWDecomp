#pragma once
#include <basis/seadTypes.h>
#include "Boss/TentackBase.hpp"
#include "Library/LiveActor/LiveActor.hpp"

class TentackTentacle;

/** @brief Tentack boss of the first two battles: one head with its tentacles. */
class Tentack : public al::LiveActor, public TentackBase {
public:
    explicit Tentack(const char* pName);

    // Order of the new virtual slots follows the target vtable (getLevel at 0x1b8,
    // getHead at 0x1c8); the remaining slots are not confirmed yet.
    void receiveDamage(const TentackHead* pHead, bool isLast) override;
    s32 getLevel() const override;
    TentackAttachItemHolder* getAttachItemHolder() const override;
    TentackHead* getHead() const override;
    const sead::Vector3f& getTentackTrans() const override;
    TentackRockBase* tryGetDeadRock() const override;
    bool tryFindTransNearPlayer(sead::Vector3f* pPosition) override;

    TentackTentacle* getTentacle(s32 index) const;

private:
    unsigned char mUnknown150[0x20];

public:
    s32 mLevel;  // 0x170 battle level (1 or 2)

private:
    unsigned char mUnknown174[0x4c];
};
static_assert(sizeof(Tentack) == 0x1c0);
