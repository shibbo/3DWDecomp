#pragma once

#include <basis/seadTypes.h>

#include "Library/LiveActor/LiveActor.hpp"

class Koura;
class NokonokoNormal;
class NokonokoNaked;

/** @brief Koopa Troopa: a NokonokoNaked body that wears a Koura shell. */
class Nokonoko : public al::LiveActor {
public:
    explicit Nokonoko(const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void killBySwitch();
    void startEject();
    void startEjectUpper();
    void exeWait();
    bool tryStartWear();
    void attachKoura();

    Koura* getKoura() const { return mKoura; }

private:
    NokonokoNormal* mNormal = nullptr;
    NokonokoNaked* mNaked = nullptr;
    Koura* mKoura = nullptr;
};

static_assert(sizeof(Nokonoko) == 0x160);
