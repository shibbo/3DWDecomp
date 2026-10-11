#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class JumpFlipSweetsCream : public al::LiveActor {
public:
    JumpFlipSweetsCream(const char*);
    ~JumpFlipSweetsCream() override;
    void init(const al::ActorInitInfo&) override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void exeTouchAction();
private:
    bool mIsAppearCoin = false;
    int mCooldown = 0;
};
