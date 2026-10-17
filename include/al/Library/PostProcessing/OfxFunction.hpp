#pragma once
namespace al { class LiveActor; class OccludedEffectDirector; }
namespace OfxFunction {
al::OccludedEffectDirector* getOccludedEffectDirector(const al::LiveActor*);
}
