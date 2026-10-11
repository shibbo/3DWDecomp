#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"
#include <container/seadSafeArray.h>
namespace al { class LiveActor; class HitSensor; }
class GreenStar;
class GreenStarKeeper : public al::ISceneObj, public al::IUseSceneObjHolder {
public:
    GreenStarKeeper();
    void initAfterPlacementSceneObj(const al::ActorInitInfo&) override;
    const char* getSceneObjName() const override;
    al::SceneObjHolder* getSceneObjHolder() const override;
    void declareId(const al::LiveActor*, int);
    void acquireId(const al::LiveActor*, int);
    int countAcquiredNum() const;
private:
    struct StarInfo { bool isDeclared = false; bool isAcquired = false; };
    al::SceneObjHolder* mSceneObjHolder = nullptr;
    sead::SafeArray<StarInfo, 30> mStars;
    int mStarCount = 0;
};
namespace rc {
void declareGreenStarId(const al::LiveActor*, int);
void acquireGreenStarId(const al::LiveActor*, int);
bool isAcquiredGreenStarInScene(const GreenStar*);
al::HitSensor* getAcquirerSensor(const GreenStar*);
}
