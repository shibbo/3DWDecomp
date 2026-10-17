#include "MapObj/ExplosionComboCounterHolder.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Math/MathUtil.hpp"

ExplosionComboCounterHolder::ExplosionComboCounterHolder() {
    mCounters.tryAllocBuffer(20, nullptr);
}

al::ComboCounter* ExplosionComboCounterHolder::getCounter() {
    al::ComboCounter* counter = mCounters.unsafeGet(mIndex);
    counter->mCounter = 0;
    return counter;
}

void ExplosionComboCounterHolder::setNextIndex() {
    mIndex = al::modi(mIndex + mCounters.size() + 1, mCounters.size());
}

const char* ExplosionComboCounterHolder::getSceneObjName() const {
    return "爆発コンボカウンター保持";
}

namespace rc {
void createExplosionComboCounter(const al::LiveActor* pActor) {
    al::createSceneObj(pActor, 6);
}

al::ComboCounter* getExplosionComboCounter(const al::LiveActor* pActor) {
    return al::getSceneObj<ExplosionComboCounterHolder>(pActor, 6)->getCounter();
}

void setExplosionComboCounterNextIndex(const al::LiveActor* pActor) {
    al::getSceneObj<ExplosionComboCounterHolder>(pActor, 6)->setNextIndex();
}

al::ComboCounter* getExplosionComboCounterAndNextIndex(const al::LiveActor* pActor) {
    al::ComboCounter* counter = getExplosionComboCounter(pActor);
    setExplosionComboCounterNextIndex(pActor);
    return counter;
}
}
