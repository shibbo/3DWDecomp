#include "MapObj/CoinRotater.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Util/AreaObjUtil.hpp"

CoinRotater::CoinRotater(al::ExecuteDirector* pDirector) {
    al::registerExecutorUser(this, pDirector, "コインローテータ");
}

void CoinRotater::execute() {
    mRotateY = al::wrapValue(mRotateY + 3.0f, 360.0f);
    mRotateYInWater = al::wrapValue(mRotateYInWater + 1.5f, 360.0f);
}

float CoinRotater::getRotateY() const {
    return mRotateY;
}

float CoinRotater::getRotateYInWater() const {
    return mRotateYInWater;
}

const char* CoinRotater::getSceneObjName() const {
    return "コインローテータ";
}

namespace rc {
float getCoinRotateY(const al::LiveActor* pActor) {
    CoinRotater* rotater = static_cast<CoinRotater*>(al::getSceneObj(pActor, 1));
    return isInWaterArea(pActor) ? rotater->getRotateYInWater() : rotater->getRotateY();
}

float getCoinRotateYByFrame(const al::LiveActor* pActor) {
    if (al::isAlive(pActor)) {
        if (isInWaterArea(pActor))
            return 1.5f;
    }
    return 3.0f;
}
}
