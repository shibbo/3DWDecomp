#pragma once

#include "MapObj/BindPuppeteer.hpp"
#include <math/seadVector.h>

class PlayerActor;
class GoalItem;

class GoalItemBindPuppeteer : public BindPuppeteer {
public:
    GoalItemBindPuppeteer(const char* pName, GoalItem* pItem);
    void setActionName(const char* pName);
    void startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor);
    void endBind(const PlayerBindEndParam* pParam) override;
    bool isEndAnimate();
    void update();
    void exeBegin();
    void exeEnd();
    void exeAnimate();

private:
    GoalItem* mGoalItem;
    const char* mActionName = nullptr;
    bool mHasAction = true;
    sead::Vector3f mBindDirection = sead::Vector3f::ez;
    PlayerActor* mPlayer = nullptr;
};

static_assert(sizeof(GoalItemBindPuppeteer) == 0x48);
