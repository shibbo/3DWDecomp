#include "MapObj/Fury/GoalPedestal.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

GoalPedestal::GoalPedestal(const char* pName) : al::FixMapParts(pName) {}

GoalPedestal::~GoalPedestal() {}

void GoalPedestal::init(const al::ActorInitInfo& rInfo) {
    bool isCollisionInvalid = false;
    al::tryGetArg(&isCollisionInvalid, rInfo, "IsCollisionInvalid");
    initWithSuffix(rInfo, isCollisionInvalid ? "NoCollision" : nullptr);
    int color = 0;
    al::tryGetArg(&color, rInfo, "GoalFrameColor");
    if (al::isMtpAnimExist(this, "GoalPedestal"))
        al::startMtpAnimAndSetFrameAndStop(this, "GoalPedestal", color);
}
