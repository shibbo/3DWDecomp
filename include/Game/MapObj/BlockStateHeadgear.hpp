#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class ActorInitInfo; class SensorMsg; class HitSensor; }
class BoxPropeller;
class BoxKiller;
class BlockStateHeadgear : public al::ActorStateBase {
public:
    BlockStateHeadgear(al::LiveActor*, const al::ActorInitInfo&, int);
    void init() override;
    void appear() override;
    bool reset();
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*);
    bool isAppearHeadgear();
    void exeWait();
    void exeAppearHeadgear();
private:
    BoxPropeller* mPropeller = nullptr;
    BoxKiller* mKiller = nullptr;
    bool mSingleMode = false;
};
