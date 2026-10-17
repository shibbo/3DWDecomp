#pragma once

#include "MapObj/BindPuppeteer.hpp"

class PlayerActor;

class GigaBellBindPuppeteer : public BindPuppeteer {
public:
    GigaBellBindPuppeteer(const char* pName);
    void setActionName(const char* pName);
    void startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor);
    void endBind(const PlayerBindEndParam* pParam) override;
    bool isEndAnimate();
    void update();
    void exeWait();
    void exeAnimate();

private:
    const char* mActionName = nullptr;
    bool mHasAction = false;
    PlayerActor* mPlayer = nullptr;
};

static_assert(sizeof(GigaBellBindPuppeteer) == 0x38);
