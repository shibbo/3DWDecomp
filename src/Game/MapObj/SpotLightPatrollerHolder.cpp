#include "MapObj/SpotLightPatrollerHolder.hpp"
#include "MapObj/SpotLightPatroller.hpp"
#include "Library/ActorUtil.hpp"
SpotLightPatrollerHolder::SpotLightPatrollerHolder() : al::LiveActor(getSceneObjName()) {}
void SpotLightPatrollerHolder::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    al::initExecutorWatchObj(this, rInfo);
    mAlertPatrollers.allocBuffer(mPatrollers.size(), nullptr);
    al::initActorSceneInfo(this, rInfo);
    makeActorAppeared();
}
void SpotLightPatrollerHolder::movement() {
    mAlertPatrollers.clear();
    int count = mPatrollers.size();
    for (int i = 0; i < count; ++i) {
        auto* patroller = mPatrollers[i];
        bool switchHandled = [&]() {
            int alertCount = mAlertPatrollers.size();
            for (int j = 0; j < alertCount; ++j) {
                auto* alertPatroller = mAlertPatrollers[j];
                if (al::isSameStageSwitch(patroller, alertPatroller, "SwitchAreaOn"))
                    return true;
            }
            return false;
        }();
        if (switchHandled)
            continue;
        if (patroller->isAlert()) {
            al::tryOnStageSwitch(patroller, "SwitchAreaOn");
            mAlertPatrollers.pushBack(patroller);
        } else {
            al::tryOffStageSwitch(patroller, "SwitchAreaOn");
        }
    }
}
