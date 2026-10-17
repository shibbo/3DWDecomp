#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class BreakModel; }

class BoxKillerBlockBrick : public al::LiveActor {
public:
    BoxKillerBlockBrick(const char* pName, int type);
    ~BoxKillerBlockBrick() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) override;
    bool isBreakable(const al::SensorMsg* pMsg, al::HitSensor* pOther) const;
    void exeWait();
    void exeReaction();

private:
    al::BreakModel* mBreakModel = nullptr;
    int mType;
};
