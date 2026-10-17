#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class DemoScenePlayerModel;
namespace al {
class MtxConnector;
}
class PlayerNpc : public al::LiveActor {
public:
    PlayerNpc(const char* pName);
    ~PlayerNpc() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*,
                               al::ScreenPointTarget*) override;
    void exeWait();
    void exeHappy();

private:
    DemoScenePlayerModel* mModel;
    al::MtxConnector* mConnector;
    int mCharacter;
    int mZone;
    int mScenario;
};
static_assert(sizeof(PlayerNpc) == 0x168);
