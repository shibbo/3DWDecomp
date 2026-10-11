#include "MapObj/ChameleonStateMic.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "Library/Audio/System/AudioVolumeCtrl.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(ChameleonStateMic, Mic);
    NERVES_MAKE_NOSTRUCT(ChameleonStateMic, Mic)
}

ChameleonStateMic::ChameleonStateMic(al::LiveActor* pActor, RenderMaterialIndirectParam* pParam,
                                    const RenderMaterialIndirectParam* pMicParam)
    : al::ActorStateBase("カメレオンマイク状態", pActor), mParam(pParam), mMicParam(pMicParam) {
    initNerve(&NrvChameleonStateMicMic, 0);
}

void ChameleonStateMic::appear() {
    al::ActorStateBase::appear();
    al::setNerve(this, &NrvChameleonStateMicMic);
}

void ChameleonStateMic::exeMic() {
    if (al::isFirstStep(this))
        al::startSe(mHostActor, "TouchTrg");
    float power = sead::Mathf::clamp(al::getMicInputPowerOld(mHostActor), 0.0f, 32000.0f);
    float rate = power / 32000.0f;
    ChameleonStateUtil::updateIndirectParam(mParam, mMicParam->mBlendRate,
        rate * mMicParam->mIntensity,
        mMicParam->mVector0, mMicParam->mVector1, mMicParam->mVector2);
    if (!al::isMicInputOn(mHostActor))
        kill();
}
