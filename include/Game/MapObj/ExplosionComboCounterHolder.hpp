#pragma once

#include "Library/Actor/ComboCounter.hpp"
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadBuffer.h>

namespace al { class LiveActor; }

class ExplosionComboCounterHolder : public al::ISceneObj {
public:
    ExplosionComboCounterHolder();
    al::ComboCounter* getCounter();
    void setNextIndex();
    const char* getSceneObjName() const override;

private:
    sead::Buffer<al::ComboCounter> mCounters;
    int mIndex = 0;
};

namespace rc {
void createExplosionComboCounter(const al::LiveActor* pActor);
al::ComboCounter* getExplosionComboCounter(const al::LiveActor* pActor);
void setExplosionComboCounterNextIndex(const al::LiveActor* pActor);
al::ComboCounter* getExplosionComboCounterAndNextIndex(const al::LiveActor* pActor);
}
