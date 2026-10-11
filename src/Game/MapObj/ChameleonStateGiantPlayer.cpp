#include "MapObj/ChameleonStateGiantPlayer.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(ChameleonStateGiantPlayer, GiantPlayer);
    NERVES_MAKE_NOSTRUCT(ChameleonStateGiantPlayer, GiantPlayer)
}

ChameleonStateGiantPlayer::ChameleonStateGiantPlayer(al::LiveActor* pActor, RenderMaterialIndirectParam* pParam,
    const RenderMaterialIndirectParam* pNearParam, const RenderMaterialIndirectParam* pFarParam,
    float nearDistance, float farDistance)
    : al::ActorStateBase("カメレオン巨大プレイヤー状態", pActor), mParam(pParam), mNearParam(pNearParam),
      mFarParam(pFarParam), mNearDistance(nearDistance), mFarDistance(farDistance) {
    initNerve(&NrvChameleonStateGiantPlayerGiantPlayer, 0);
    mTouchParam = new RenderMaterialIndirectParam;
}

void ChameleonStateGiantPlayer::exeGiantPlayer() {
    sead::Vector3f touchPos = al::getSensorPos(mPlayerSensor);
    float distance = (touchPos - al::getTrans(mHostActor)).length();
    float distanceRange = mFarDistance - mNearDistance;
    float rate = sead::Mathf::clamp((distance - mNearDistance) / distanceRange,
                                  0.0f, 1.0f);
    float nearRate = 1.0f - rate;
    mTouchParam->mIntensity = nearRate * mNearParam->mIntensity + rate * mFarParam->mIntensity;
    mTouchParam->mVector0 = nearRate * mNearParam->mVector0 + rate * mFarParam->mVector0;
    mTouchParam->mVector1 = nearRate * mNearParam->mVector1 + rate * mFarParam->mVector1;
    mTouchParam->mVector2 = nearRate * mNearParam->mVector2 + rate * mFarParam->mVector2;
    ChameleonStateUtil::updateIndirectParam(mParam, mTouchParam);
    if (al::isGreaterStep(this, 15))
        kill();
}
