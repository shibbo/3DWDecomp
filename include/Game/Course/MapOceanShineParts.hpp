#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include "System/ScenarioInfo.hpp"
#include <container/seadTreeMap.h>
#include <container/seadObjArray.h>
#include <math/seadVector.h>
namespace al { class LayoutInitInfo; class LiveActor; }
class GameDataHolder;
class MapOceanShineParts : public al::LayoutActor {
public:
    struct GoalItemPosInfo {
        ScenarioInfo scenario;
        sead::Vector3f position;
        bool complete;
    };
    MapOceanShineParts(const al::LayoutInitInfo&, const char*, const char*, al::LayoutActor*, const GameDataHolder*);
    ~MapOceanShineParts();
    void setPanePositions(sead::Vector2f, float);
    void updateScale(const sead::Vector2f&);
    int getNearestIconIdx(float&);
    al::LayoutActor* getIcon(int);
    void snapIcon(int);
    ScenarioInfo getScenarioInfo(int);
    void addSpecialShineLocation(al::LiveActor*, sead::Vector3f, ScenarioInfo);
    void removeSpecialShineLocation(al::LiveActor*);
    void setSpecialShineIconComplete(al::LiveActor*, bool);
    void startIconBlink();
    void endIconBlink();
    bool isBlinkAnimDone();
private:
    const GameDataHolder* mGameData;
    sead::TreeMap<al::LiveActor*, GoalItemPosInfo> mSpecialShines;
    sead::ObjArray<ScenarioInfo> mScenarios;
    al::LayoutActor** mIcons = nullptr;
};

static_assert(sizeof(MapOceanShineParts) == 0x178);
