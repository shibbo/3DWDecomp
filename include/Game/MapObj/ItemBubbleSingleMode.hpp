#pragma once
#include "MapObj/ItemBubble.hpp"
class ItemBubbleSingleMode : public ItemBubble {
public:
    explicit ItemBubbleSingleMode(const char*);
    ~ItemBubbleSingleMode() override;
    void init(const al::ActorInitInfo&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void control() override;
    sead::Vector3f getItemActorOffset(int) const override;
};
