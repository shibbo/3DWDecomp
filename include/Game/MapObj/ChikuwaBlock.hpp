#pragma once

#include "Library/MapObj/FallMapParts.hpp"

class ChikuwaBlock : public al::FallMapParts {
public:
    explicit ChikuwaBlock(const char* pName);
    ~ChikuwaBlock() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void switchAppear() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
};
