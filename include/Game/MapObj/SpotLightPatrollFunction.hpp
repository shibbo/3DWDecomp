#pragma once
namespace al { class LiveActor; }
class SpotLightPatrollerHolder;
namespace SpotLightPatrollFunction {
SpotLightPatrollerHolder* tryCreateAndGetHolder(const al::LiveActor*);
}
