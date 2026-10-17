#include "MapObj/ChameleonStateTouch.hpp"
#include "MapObj/ChameleonStateUtil.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include <math/seadMathCalcCommon.h>

namespace {
    NERVE_DECL(ChameleonStateTouch, DRCTouch);
    NERVES_MAKE_NOSTRUCT(ChameleonStateTouch, DRCTouch)
}

ChameleonStateTouch::ChameleonStateTouch(al::LiveActor* pActor, RenderMaterialIndirectParam* pParam,
    const RenderMaterialIndirectParam* pNearParam, const RenderMaterialIndirectParam* pFarParam,
    float nearDistance, float farDistance)
    : al::ActorStateBase("カメレオンタッチ状態", pActor), mParam(pParam), mNearParam(pNearParam),
      mFarParam(pFarParam), mNearDistance(nearDistance), mFarDistance(farDistance) {
    initNerve(&NrvChameleonStateTouchDRCTouch, 0);
    mTouchParam = new RenderMaterialIndirectParam;
}

void ChameleonStateTouch::exeDRCTouch() {
    if (!rc::isEnableTouchPointer(mHostActor)) {
        al::stopSeByName(mHostActor, "Touch");
        kill();
        return;
    }
    sead::Vector3f touchPos = rc::getTouchPointerPosition(mHostActor);
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
    ChameleonStateUtil::isVisible(mParam, mNearParam, mFarParam);
    if (ChameleonStateUtil::isVisible(mTouchParam, mNearParam, mFarParam))
        al::holdSeWithParam(mHostActor, "Touch", nearRate);
    else
        al::stopSeByName(mHostActor, "Touch");
    mIsVisible = ChameleonStateUtil::isVisible(mTouchParam, mNearParam, mFarParam);
}
