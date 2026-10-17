#pragma once
#include "Enemy/TuccondorBase.hpp"
namespace al { class MtxConnector; }
class TuccondorGlass;
class TuccondorGlassBlower;
class ActorStateSupportFreeze;
/** @brief Conkdor trap that strikes when a player approaches. */
class TuccondorTrap : public TuccondorBase {
public:
    TuccondorTrap(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void exeWait();
    void exeSupportFreeze();
    void exeAttack();
    void exeAttackWait();
    void exeAttackEnd();
    void exeAttackSign();
private:
    al::MtxConnector* mConnector = nullptr;
    TuccondorGlass* mGlass = nullptr;
    TuccondorGlassBlower* mGlassBlower = nullptr;
    ActorStateSupportFreeze* mFreeze = nullptr;
    bool mShortAttack = false;
};
static_assert(sizeof(TuccondorTrap) == 0x180);
