#include "MapObj/SpotLightPatrollFunction.hpp"
#include "MapObj/SpotLightPatrollerHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
SpotLightPatrollerHolder* SpotLightPatrollFunction::tryCreateAndGetHolder(const al::LiveActor* pActor) {
    if (al::isExistSceneObj(pActor, 21))
        return static_cast<SpotLightPatrollerHolder*>(al::getSceneObj(pActor, 21));
    return static_cast<SpotLightPatrollerHolder*>(al::createSceneObj(pActor, 21));
}
SpotLightPatrollerHolder::~SpotLightPatrollerHolder() {}
