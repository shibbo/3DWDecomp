#include "Course/MapOceanShineParts.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include <math/seadMatrix.h>
#include "Project/Base/StringUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include <nn/ui2d/ui2d_Layout.h>

#include "MapObj/GoalItem.hpp"
#include "MapObj/GoalItemHolder.hpp"
#include "MapObj/LuckyIslandHolder.hpp"
#include "MapObj/LuckyIsland.hpp"
#include "System/GameDataHolder.hpp"
#include "System/Data/OceanScenarioList.hpp"
#include "System/IslandDataList.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"


/** @brief Scales both layers of every ocean-map icon. */
void MapOceanShineParts::updateScale(const sead::Vector2f& scale) {
    for (int i = 0; i < 52; ++i) {
        al::setPaneLocalScale(mIcons[i], "PicOceanIcons", scale);
        al::setPaneLocalScale(mIcons[i], "PicOceanBlue", scale);
    }
}
/** @brief Finds the visible icon nearest the origin, updating the squared-distance bound. */
int MapOceanShineParts::getNearestIconIdx(float& distance) {
    al::StringTmp<32> pane;
    int result = -1;
    for (int i = 0; i < 52; ++i) {
        pane.format("Par_mapDot_%02d", i);
        if (al::isHidePane(this, pane.cstr())) break;
        const auto& matrix = al::getPaneMtx(mIcons[i], "Target");
        sead::Vector3f position;
        matrix.getTranslation(position);
        float length = position.squaredLength();
        if (length < distance) { result = i; distance = length; }
    }
    return result;
}
/** @brief Returns the layout actor for an icon slot. */
al::LayoutActor* MapOceanShineParts::getIcon(int index) { return mIcons[index]; }
/** @brief Starts the selected icon's snap animation. */
void MapOceanShineParts::snapIcon(int index) { al::startAction(mIcons[index], "icon_SNAP", "Scale"); }
/** @brief Returns the scenario represented by an icon slot. */
ScenarioInfo MapOceanShineParts::getScenarioInfo(int index) { return *mScenarios[index]; }
/** @brief Records a temporary shine location keyed by its actor. */
void MapOceanShineParts::addSpecialShineLocation(al::LiveActor* actor, sead::Vector3f position, ScenarioInfo scenario) {
    GoalItemPosInfo info;
    info.scenario = scenario;
    info.position = position;
    info.complete = false;
    mSpecialShines.insert(actor, info);
}
/** @brief Removes a registered temporary shine location if present. */
void MapOceanShineParts::removeSpecialShineLocation(al::LiveActor* actor) { if (mSpecialShines.find(actor)) mSpecialShines.erase(actor); }
/** @brief Updates a registered temporary shine's completion state. */
void MapOceanShineParts::setSpecialShineIconComplete(al::LiveActor* actor, bool complete) {
    auto* node = mSpecialShines.find(actor);
    if (node) node->value().complete = complete;
}
/** @brief Blinks visible icons whose scenarios are still incomplete. */
void MapOceanShineParts::startIconBlink() {
    al::StringTmp<32> pane;
    for (int i = 0; i < 52; ++i) {
        pane.format("Par_mapDot_%02d", i);
        if (al::isHidePane(this, pane.cstr())) continue;
        ScenarioInfo info = *mScenarios[i];
        if (!SingleModeDataFunction::isScenarioComplete(this, info.mIslandId, info.mScenarioIndex))
            al::startAction(mIcons[i], "icon_BLINK", "Scale");
    }
}
/** @brief Freezes visible icons at the midpoint of their blink animation. */
void MapOceanShineParts::endIconBlink() {
    al::StringTmp<32> pane;
    for (int i = 0; i < 52; ++i) {
        pane.format("Par_mapDot_%02d", i);
        if (al::isHidePane(this, pane.cstr())) continue;
        auto* icon = mIcons[i];
        al::startFreezeAction(icon, "icon_BLINK", al::getActionFrameMax(icon, "icon_BLINK", "Scale") * 0.5f, "Scale");
    }
}
/** @brief Checks whether the first visible blinking icon is at its midpoint. */
bool MapOceanShineParts::isBlinkAnimDone() {
    al::StringTmp<32> pane;
    for (int i = 0; i < 52; ++i) {
        pane.format("Par_mapDot_%02d", i);
        if (al::isHidePane(this, pane.cstr())) continue;
        if (al::isActionPlaying(mIcons[i], "icon_BLINK", "Scale")) {
            float frame = al::getActionFrame(mIcons[i], "Scale");
            return frame == al::getActionFrameMax(mIcons[i], "icon_BLINK", "Scale") * 0.5f;
        }
    }
    return false;
}

/** @brief Allocates the temporary-shine map and the 52 ocean-map icon slots. */
MapOceanShineParts::MapOceanShineParts(const al::LayoutInitInfo& info, const char* name,
                                     const char* pane, al::LayoutActor* parent,
                                     const GameDataHolder* gameData)
    : al::LayoutActor(name), mGameData(gameData) {
    al::initLayoutPartsActor(this, parent, info, pane, nullptr);
    mSpecialShines.allocBuffer(40, nullptr);
    mScenarios.allocBuffer(52, nullptr);
    int count = getLayoutKeeper()->getLayout()->GetPartsList().size();
    if (count != 0) {
        initLayoutPartsActorKeeper(count);
        al::StringTmp<32> nameBuffer;
        mIcons = new al::LayoutActor*[52];
        for (int i = 0; i < 52; ++i) {
            nameBuffer.format("Par_mapDot_%02d", i);
            mIcons[i] = new al::LayoutActor("MapDotIconOceanParts");
            al::initLayoutPartsActor(mIcons[i], this, info, nameBuffer.cstr(), nullptr);
        }
    }
    for (int i = 0; i < 52; ++i) mScenarios.emplaceBack(ScenarioInfo{-1, -1});
}
/** @brief Releases the scenario-slot storage. */
MapOceanShineParts::~MapOceanShineParts() { mScenarios.freeBuffer(); }

/** @brief Combines ocean scenarios, temporary shines and Lucky Isle locations into map icons. */
void MapOceanShineParts::setPanePositions(sead::Vector2f scale, float offsetY) {
    auto* holder = al::tryGetSceneObj<GoalItemHolder>(this, 36);
    if (!holder) return;
    auto* scenarios = mGameData->getOceanScenarioList();
    const auto& items = holder->getGoalItems();
    int phase = SingleModeDataFunction::getUnlockedPhase(this);
    int icon = 0;
    for (int i = 0; i < items.size(); ++i) {
        int island = items.unsafeAt(i)->getIslandId();
        if (island > 0) continue;
        int scenario = items.unsafeAt(i)->getShineId();
        int quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(island);
        --scenario;
        if (scenario < 0) continue;
        if (scenarios->getScenarioListByQuadrant(quadrant)->getScenarioDataByIndex(scenario)->isSpecialScenario()) continue;
        al::StringTmp<32> pane("Par_mapDot_%02d", icon);
        if (SingleModeDataFunction::isScenarioComplete(this, island - 1, scenario)) {
            mScenarios[icon]->mIslandId = island - 1;
            mScenarios[icon]->mScenarioIndex = scenario;
            al::startAction(mIcons[icon], "WaterArea_COMPLETE");
        } else if (phase >= 8) {
            mScenarios[icon]->mIslandId = island - 1;
            mScenarios[icon]->mScenarioIndex = scenario;
            al::startAction(mIcons[icon], "WaterArea_UNCOLLECTED");
        } else continue;
        const auto& world = al::getTrans(items[i]);
        sead::Vector2f position(0.0f, 0.0f);
        position.x = scale.x * world.x;
        position.y = scale.y * world.z + offsetY;
        al::showPane(this, pane.cstr());
        al::setPaneLocalTrans(this, pane.cstr(), position);
        if (++icon >= 52) return;
    }
    auto* node = mSpecialShines.startIterating();
    while (node) {
        al::StringTmp<32> pane("Par_mapDot_%02d", icon);
        auto& info = node->value();
        ScenarioInfo scenario = info.scenario;
        if (info.complete) {
            *mScenarios[icon] = scenario;
            al::startAction(mIcons[icon], "WaterArea_COMPLETE");
        } else {
            if (phase < 8) { node = mSpecialShines.nextNode(node); continue; }
            *mScenarios[icon] = scenario;
            al::startAction(mIcons[icon], "WaterArea_UNCOLLECTED");
        }
        sead::Vector2f position(scale.x * info.position.x, scale.y * info.position.z + offsetY);
        al::showPane(this, pane.cstr());
        al::setPaneLocalTrans(this, pane.cstr(), position);
        ++icon;
        node = mSpecialShines.nextNode(node);
        if (icon >= 52) return;
    }
    if (phase >= 8) {
        auto* lucky = al::tryGetSceneObj<LuckyIslandHolder>(this, 38);
        if (lucky && lucky->getIslandCount() != 0) {
            auto* active = lucky->getCurrentIsland();
            al::StringTmp<32> pane;
            int i = 0;
            for (; i < lucky->getIslandCount(); ++i) {
                int index = icon;
                pane.format("Par_mapDot_%02d", index);
                const auto& world = al::getTrans(lucky->getIsland(i));
                sead::Vector2f position(scale.x * world.x, scale.y * world.z + offsetY);
                al::showPane(this, pane.cstr());
                mScenarios[index]->mScenarioIndex = i;
                if (SingleModeDataFunction::wasLuckyIslandPosCompleted(this, i)) {
                    al::startAction(mIcons[index], "LuckyIsle_COMPLETE");
                } else if (active == lucky->getIsland(i) && !al::isDead(active)) {
                    al::startAction(mIcons[index], "LuckyIsle_APPEARED");
                } else {
                    al::startAction(mIcons[index], "LuckyIsle_POSSIBLE");
                }
                al::setPaneLocalTrans(this, pane.cstr(), position);
                mScenarios[index]->mIslandId = -7;
                if (++icon >= 52) return;
            }

        }
    }
    for (; icon < 52; ++icon) {
        al::StringTmp<32> pane("Par_mapDot_%02d", icon);
        al::hidePane(this, pane.cstr());
    }
}
