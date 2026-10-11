#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/GreenStar.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "System/GameDataFunction.hpp"
#include <math/seadMathCalcCommon.h>
GreenStarKeeper::GreenStarKeeper() {}
void GreenStarKeeper::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    mSceneObjHolder = rInfo.getActorSceneInfo().sceneObjHolder;
    for (int i = 0; i < 30; ++i)
        mStars[i].isAcquired = GameDataFunction::isAcquireGreenStar(GameDataHolderAccessor(this), i);
}
void GreenStarKeeper::declareId(const al::LiveActor*, int id) {
    if (id >= 30 || mStarCount > 30) return;
    mStarCount = sead::Mathi::max(mStarCount, id + 1);
    mStars[id].isDeclared = true;
}
void GreenStarKeeper::acquireId(const al::LiveActor*, int id) {
    mStars[id].isAcquired = true;
    GameDataFunction::acquireGreenStar(GameDataHolderWriter(this), id);
}
int GreenStarKeeper::countAcquiredNum() const {
    int count = 0;
    for (int i = 0; i < mStarCount; ++i) count += mStars[i].isAcquired;
    return count;
}
namespace rc {
void declareGreenStarId(const al::LiveActor* pActor, int id) { al::getSceneObj<GreenStarKeeper>(pActor, 10)->declareId(pActor, id); }
void acquireGreenStarId(const al::LiveActor* pActor, int id) { al::getSceneObj<GreenStarKeeper>(pActor, 10)->acquireId(pActor, id); }
bool isAcquiredGreenStarInScene(const GreenStar* pStar) { return pStar->isAcquiredInScene(); }
al::HitSensor* getAcquirerSensor(const GreenStar* pStar) { return pStar->getAcquirerSensor(); }
}
const char* GreenStarKeeper::getSceneObjName() const { return "グリーンスター保持者"; }
al::SceneObjHolder* GreenStarKeeper::getSceneObjHolder() const { return mSceneObjHolder; }
