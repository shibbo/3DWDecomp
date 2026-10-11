#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadQuat.h>

namespace al { class MtxConnector; }

class ItemAssistRotateParam;

class ItemStateAssistRotate : public al::ActorStateBase {
public:
    ItemStateAssistRotate(al::LiveActor* pActor, const ItemAssistRotateParam* pParam);
    void init() override;
    void appear() override;
    void setRotateDegree(float degree);
    void setRotateDegreePtr(float* degree) { mRotateDegree = degree; }
    void setConnector(al::MtxConnector* pConnector, const sead::Quatf& rQuat) {
        mConnector = pConnector;
        mBaseQuat = rQuat;
    }
    void exeSpin();
    void rotate(float speed);
    void exeLerp();

private:
    const ItemAssistRotateParam* mParam;
    al::MtxConnector* mConnector = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    float* mRotateDegree = nullptr;
    float mLocalRotateDegree = 0.0f;
};
