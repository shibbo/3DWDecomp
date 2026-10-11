#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include <math/seadVector.h>
namespace al { class LiveActor; class HitSensor; class ByamlIter; class IUseSceneObjHolder; }
class StageDataHolder;
class ScoreNumber;
class ScoreHolder : public al::ISceneObj {
public:
    ScoreHolder();
    const char* getSceneObjName() const override;
    void initAfterPlacementSceneObj(const al::ActorInitInfo&) override;
    void addScoreAndPopNumberFromIter(const al::LiveActor*, int, const al::ByamlIter&, int, const sead::Vector3f&, const sead::Vector3f*, bool, int);
    void popUpPlayerUp(const al::LiveActor*, int, const sead::Vector3f*, const sead::Vector3f&, int);
    void popUpPlayerUp(const al::LiveActor*, int, const sead::Vector3f&, int);
    void addScoreAndPopNumber(const al::LiveActor*, al::HitSensor*, const char*, int, const sead::Vector3f&, const sead::Vector3f*);
    int getScoreMaxLevel(const char*) const;
    void setStageDataHolder(StageDataHolder*);
    void popUpScore(const al::LiveActor*, al::HitSensor*, const char*, int, const sead::Vector3f&);
    void popUpScore(const al::LiveActor*, al::HitSensor*, const char*, int, const sead::Vector3f*, const sead::Vector3f&);
    void initScore();
    int getPlayerScore(int) const;
private:
    void advanceNumber() { if (++mNextNumber >= mNumberCount) mNextNumber = 0; }
    al::ByamlIter* mTable = nullptr;
    StageDataHolder* mStageData = nullptr;
    ScoreNumber** mNumbers = nullptr;
    int mNumberCount = 0;
    int mNextNumber = 0;
};
static_assert(sizeof(ScoreHolder) == 0x28);
namespace ScoreHolderUtil { ScoreHolder* initScoreHolder(const al::IUseSceneObjHolder*); }
