#include "MapObj/ChameleonStateHipDrop.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
    NERVE_DECL(ChameleonStateHipDrop, Wait);
    NERVE_DECL(ChameleonStateHipDrop, Appear);
    NERVE_DECL(ChameleonStateHipDrop, AppearWait);
    NERVE_DECL(ChameleonStateHipDrop, End);
    NERVES_MAKE_NOSTRUCT(ChameleonStateHipDrop, Wait, Appear, AppearWait, End)
}

ChameleonStateHipDrop::ChameleonStateHipDrop(al::LiveActor* pActor,
    RenderMaterialIndirectParam* pParam, const RenderMaterialIndirectParam* pAppearParam,
    const RenderMaterialIndirectParam* pEndParam)
    : al::ActorStateBase("カメレオンヒップドロップ状態", pActor), mParam(pParam),
      mAppearParam(pAppearParam), mEndParam(pEndParam) {
    initNerve(&NrvChameleonStateHipDropWait, 0);
}

void ChameleonStateHipDrop::exeWait() {}

void ChameleonStateHipDrop::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvChameleonStateHipDropAppear);
}

void ChameleonStateHipDrop::exeAppear() {
    if (al::isGreaterStep(this, mAppearDelay - 5)) {
        float rate = al::calcNerveRate(this, mAppearDelay - 5, mAppearDelay);
        ChameleonStateUtil::updateIndirectParam(mParam, rate, mEndParam, mAppearParam);
    }
    if (al::isGreaterStep(this, mAppearDelay)) {
        al::startSe(mHostActor, "TouchTrg");
        mParam->mIntensity = mAppearParam->mIntensity;
        mParam->mVector0.w = mAppearParam->mVector0.w;
        al::setNerve(this, &NrvChameleonStateHipDropAppearWait);
    }
}

void ChameleonStateHipDrop::exeAppearWait() {
    if (al::isGreaterStep(this, 20))
        al::setNerve(this, &NrvChameleonStateHipDropEnd);
}

void ChameleonStateHipDrop::exeEnd() {
    ChameleonStateUtil::updateIndirectParam(mParam, mEndParam->mBlendRate, mEndParam->mIntensity,
        mEndParam->mVector0, mEndParam->mVector1, mEndParam->mVector2);
    if (al::isGreaterStep(this, 60))
        kill();
}
