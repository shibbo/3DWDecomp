#include "Scene/SceneObjFactory.hpp"
#include "Library/Scene/SceneObjHolder.hpp"
#include "MapObj/ExplosionComboCounterHolder.hpp"
#include "MapObj/GreenStarKeeper.hpp"
#include "MapObj/IllustItemKeeper.hpp"
#include "MapObj/MysteryBoxConnector.hpp"
#include "Player/PlayerFireBallAppearWatcher.hpp"
#include "Player/Normal/PlayerGroupSceneObj.hpp"
#include "Project/Block/BlockRailDirector.hpp"
#include "MapObj/ScoreHolder.hpp"
#include "MapObj/SpotLightPatrollerHolder.hpp"
#include "Library/MapObj/SupportFreezeSyncGroupHolder.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "CourseSelect/CourseSelectDokanHolder.hpp"
#include "MapObj/ChikaChikaBlockSynchronizer.hpp"
#include "Util/ControllerEventWatcher.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/CatGullHolder.hpp"
#include "MapObj/LuckyIslandHolder.hpp"
#include "MapObj/Fury/CloudBonusWatcher.hpp"
#include "MapObj/SuperbViewArea.hpp"
#include "Layout/GuideObjHolder.hpp"
#include "MapObj/TimerManager.hpp"

namespace {
al::ISceneObj* createSceneObj(s32 id) {
    switch (id) {
    case 6:
        return new ExplosionComboCounterHolder;
    case 10:
        return new GreenStarKeeper;
    case 11:
        return new IllustItemKeeper;
    case 12:
        return new MysteryBoxConnector;
    case 15:
        return new PlayerFireBallAppearWatcher;
    case 16:
        return new PlayerGroupSceneObj;
    case 19:
        return new al::BlockRailDirector;
    case 20:
        return new ScoreHolder;
    case 21:
        return new SpotLightPatrollerHolder;
    case 22:
        return new al::SupportFreezeSyncGroupHolder;
    case 25:
        return new PlayerRetargettingSelectorSceneObj;
    case 26:
        return new CourseSelectDokanHolder;
    case 29:
        return new ChikaChikaBlockSynchronizer;
    case 34:
        return new ControllerEventWatcher;
    case 36:
        return new GoalItemHolder;
    case 37:
        return new CatGullHolder;
    case 38:
        return new LuckyIslandHolder;
    case 52:
        return new CloudBonusWatcher;
    case 54:
        return new SuperbViewAreaHolder;
    case 55:
        return new GuideObjHolder;
    case 57:
        return new TimerManager;
    default:
        return nullptr;
    }
}
} // namespace

al::SceneObjHolder* SceneObjFactory::createSceneObjHolder() {
    return new al::SceneObjHolder(createSceneObj, 62);
}
