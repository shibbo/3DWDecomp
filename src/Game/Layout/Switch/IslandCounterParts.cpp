#include "Layout/Switch/IslandCounterParts.hpp"

#include <nn/ui2d/ui2d_Layout.h>

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/Data/OceanScenarioList.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/IslandDataList.hpp"
#include "System/ScenarioList.hpp"

namespace {
/** Last scenario index that is always listed, collected or not. */
constexpr s32 cAlwaysShownScenarioMax = 2;
/** First scenario index that is only listed once collected or active. */
constexpr s32 cHiddenScenarioStart = 3;
/** Unlock phase from which the active scenario of the first island counts. */
constexpr s32 cFirstIslandActivePhase = 8;
/** Island identifier of the lucky island. */
constexpr s32 cLuckyIslandId = -7;
/** Quadrant index of the lucky island. */
constexpr s32 cLuckyIslandQuadrant = 3;

/** Ocean scenario types that list every scenario of the same type in the quadrant. */
enum OceanScenarioType {
    OceanScenarioType_Group3 = 3,
    OceanScenarioType_Group6 = 6,
    OceanScenarioType_Lucky = 9,
};

/**
 * @brief Creates the parts actor keeper of a layout actor with room for all of its parts panes.
 * @param pActor Layout actor whose layout was just initialized.
 */
inline void initPartsActorKeeper(al::LayoutActor* pActor) {
    const nn::util::IntrusiveListNode& partsList =
        pActor->getLayoutKeeper()->getLayout()->_48;

    if (partsList.GetNext() == &partsList) {
        return;
    }

    s32 partsNum = 0;

    for (const nn::util::IntrusiveListNode* node = partsList.GetNext(); node != &partsList;
         node = node->GetNext()) {
        partsNum++;
    }

    pActor->initLayoutPartsActorKeeper(partsNum);
}

/**
 * @brief Gets the localized name of an island scenario.
 * @param pHolder Scene object holder.
 * @param pMessageSystem Message system holding the system messages.
 * @param islandId One-based island identifier.
 * @param scenarioId One-based scenario number.
 * @return The scenario name.
 */
inline const char16_t* getScenarioName(al::IUseSceneObjHolder* pHolder,
                                       al::IUseMessageSystem* pMessageSystem, s32 islandId,
                                       s32 scenarioId) {
    return reinterpret_cast<const char16_t*>(
        IslandDataFunction::getIslandScenarioName(pHolder, pMessageSystem, islandId, scenarioId));
}

/**
 * @brief Gets the localized name of an ocean scenario.
 * @param pHolder Scene object holder providing the ocean scenario list.
 * @param pMessageSystem Message system holding the system messages.
 * @param scenarioId One-based scenario number in the quadrant.
 * @param quadrant Quadrant index.
 * @return The scenario name.
 */
inline const char16_t* getOceanScenarioName(al::IUseSceneObjHolder* pHolder,
                                            al::IUseMessageSystem* pMessageSystem,
                                            s32 scenarioId, s32 quadrant) {
    return reinterpret_cast<const char16_t*>(
        IslandDataFunction::getOceanScenarioName(pHolder, pMessageSystem, scenarioId, quadrant));
}
}  // namespace

/**
 * @brief Creates the counter and its scenario parts as parts of a parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPaneName Name of the parts pane in the parent layout.
 * @param pParent Parent layout actor.
 * @param pArchiveName Archive name suffix of the parts layout.
 */
IslandCounterParts::IslandCounterParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                       const char* pPaneName, al::LayoutActor* pParent,
                                       const char* pArchiveName)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPaneName, pArchiveName);
    initPartsActorKeeper(this);

    mScenarioParts = new al::LayoutActor*[cScenarioPartsNum];

    al::StringTmp<32> partsName;

    for (s32 i = 0; i < cScenarioPartsNum; i++) {
        partsName.format("ParScenario%d", i + 1);
        mScenarioParts[i] = new al::LayoutActor("ScenarioShineCounterParts");
        al::initLayoutPartsActor(mScenarioParts[i], this, rInfo, partsName.cstr());
    }
}

/** @brief Destroys the scenario parts. */
IslandCounterParts::~IslandCounterParts() {
    for (s32 i = 0; i < cScenarioPartsNum; i++) {
        delete mScenarioParts[i];
    }

    delete[] mScenarioParts;
}

/**
 * @brief Sets the displayed island name.
 * @param pName Island name.
 */
void IslandCounterParts::setIslandName(const char16_t* pName) {
    al::setPaneString(this, "TxtIslandName", pName);
}

/**
 * @brief Lists the scenarios of an island with their collected state.
 * @param islandId One-based island identifier.
 */
void IslandCounterParts::updateShineCount(s32 islandId) {
    s32 islandIndex = islandId - 1;
    u64 flags = SingleModeDataFunction::getScenarioFlag(GameDataHolderAccessor(this), islandIndex);
    s32 scenarioNum =
        SingleModeDataFunction::getScenarioNum(GameDataHolderAccessor(this), islandIndex);
    s32 activeIndex =
        SingleModeDataFunction::getCurActiveScenarioIndex(GameDataHolderAccessor(this), islandIndex);

    if (islandIndex == 0 &&
        SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(this)) <
            cFirstIslandActivePhase) {
        activeIndex = 0;
    }

    s32 partsIndex = 0;

    // The leading scenarios are listed up to the first one not collected yet.
    for (s32 i = 0; i <= sead::Mathi::min(activeIndex, cAlwaysShownScenarioMax); i++) {
        al::showPaneRootNoRecursive(mScenarioParts[i]);
        if ((flags & (1ull << i)) == 0) {
            if (activeIndex == i &&
                !SingleModeDataFunction::wasActiveScenarioNameSeen(GameDataHolderAccessor(this),
                                                                   islandIndex)) {
                al::setPaneString(mScenarioParts[i], "TxtScenarioMap", u"-");
            } else {
                al::setPaneString(mScenarioParts[i], "TxtScenarioMap",
                                  getScenarioName(this, this, islandId, i + 1));
            }

            partsIndex = i + 1;
            al::startAction(mScenarioParts[i], "Shine_OFF", nullptr);
            break;
        }

        al::setPaneString(mScenarioParts[i], "TxtScenarioMap",
                          getScenarioName(this, this, islandId, i + 1));
        al::startAction(mScenarioParts[i], "Shine_ON", nullptr);
        partsIndex = i + 1;
    }

    // Later scenarios only show up once collected or when they are the active one.
    for (s32 i = cHiddenScenarioStart; i < scenarioNum; i++) {
        if ((flags & (1ull << i)) != 0) {
            al::setPaneString(mScenarioParts[partsIndex], "TxtScenarioMap",
                              getScenarioName(this, this, islandId, i + 1));
            al::showPaneRootNoRecursive(mScenarioParts[partsIndex]);
            al::startAction(mScenarioParts[partsIndex], "Shine_ON", nullptr);
            partsIndex++;
        } else if (activeIndex == i) {
            al::setPaneString(mScenarioParts[partsIndex], "TxtScenarioMap",
                              getScenarioName(this, this, islandId, i + 1));
            al::showPaneRootNoRecursive(mScenarioParts[partsIndex]);
            al::startAction(mScenarioParts[partsIndex], "Shine_OFF", nullptr);
            partsIndex++;
        }
    }

    for (s32 i = partsIndex; i < cScenarioPartsNum; i++) {
        al::hidePaneRootNoRecursive(mScenarioParts[i]);
    }
}

/**
 * @brief Lists the scenarios of an ocean quadrant (or the lucky island) with their collected state.
 * @param pGameData Game data holder providing the ocean scenario list.
 * @param scenarioInfo Island and scenario index the player is at.
 */
void IslandCounterParts::updateShineCountOcean(const GameDataHolder* pGameData,
                                               ScenarioInfo scenarioInfo) {
    bool isLuckyIsland;
    ScenarioData* scenario;
    s32 quadrant;

    if (scenarioInfo.mIslandId == cLuckyIslandId) {
        IslandDataFunction::getIslandIDFromQuadrantIndex(cLuckyIslandQuadrant);
        al::setPaneString(this, "TxtIslandName",
                          al::getSystemMessageString(this, "IslandName", "LuckyIsland"));
        isLuckyIsland = true;
    } else {
        OceanScenarioList* oceanList = pGameData->getOceanScenarioList();
        al::setPaneString(this, "TxtIslandName",
                          al::getSystemMessageString(this, "IslandName", "OpenWater"));

        quadrant = IslandDataFunction::getQuadrantIndexFromIslandID(scenarioInfo.mIslandId + 1);
        scenario = oceanList->getScenarioListByQuadrant(quadrant)->getScenarioDataByIndex(
            scenarioInfo.mScenarioIndex);
        s32 type = scenario->mScenarioType;

        // Grouped scenarios list every scenario of the same type in the quadrant.
        if (type == OceanScenarioType_Group3 || type == OceanScenarioType_Group6) {
            ScenarioList* scenarioList = oceanList->getScenarioListByQuadrant(quadrant);
            s32 partsIndex = 0;

            for (s32 i = 0; i < scenarioList->mCount; i++) {
                ScenarioData* data = scenarioList->getScenarioDataByIndex(i);
                if (data->mScenarioType != type) {
                    continue;
                }

                al::showPaneRootNoRecursive(mScenarioParts[partsIndex]);
                if (SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this),
                                                               scenarioInfo.mIslandId,
                                                               data->mScenarioId - 1)) {
                    al::setPaneString(mScenarioParts[partsIndex], "TxtScenarioMap",
                                      getOceanScenarioName(this, this, data->mScenarioId,
                                                           quadrant));
                    al::startAction(mScenarioParts[partsIndex], "Shine_ON", nullptr);
                } else {
                    al::setPaneString(mScenarioParts[partsIndex], "TxtScenarioMap", u"-");
                    al::startAction(mScenarioParts[partsIndex], "Shine_OFF", nullptr);
                }

                partsIndex++;
            }

            for (s32 i = partsIndex; i < cScenarioPartsNum; i++) {
                al::hidePaneRootNoRecursive(mScenarioParts[i]);
            }

            return;
        }

        isLuckyIsland = type == OceanScenarioType_Lucky;
    }

    if (isLuckyIsland) {
        // A single lucky shine, named once it was collected.
        if (SingleModeDataFunction::wasLuckyIslandPosCompleted(GameDataHolderAccessor(this),
                                                               scenarioInfo.mScenarioIndex)) {
            s32 shineIndex = SingleModeDataFunction::getLuckyShineIdxByPosIdx(
                GameDataHolderAccessor(this), scenarioInfo.mScenarioIndex);
            al::StringTmp<32> label("LuckyIslandScenario%d", shineIndex + 1);
            al::setPaneSystemMessage(mScenarioParts[0], "TxtScenarioMap", "ScenarioName",
                                     label.cstr());
            al::startAction(mScenarioParts[0], "Shine_ON", nullptr);
        } else {
            al::setPaneString(mScenarioParts[0], "TxtScenarioMap", u"-");
            al::startAction(mScenarioParts[0], "Shine_OFF", nullptr);
        }
    } else {
        al::showPaneRootNoRecursive(mScenarioParts[0]);
        if (SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this),
                                                       scenarioInfo.mIslandId,
                                                       scenario->mScenarioId - 1)) {
            al::setPaneString(mScenarioParts[0], "TxtScenarioMap",
                              getOceanScenarioName(this, this, scenario->mScenarioId, quadrant));
            al::startAction(mScenarioParts[0], "Shine_ON", nullptr);
        } else {
            al::setPaneString(mScenarioParts[0], "TxtScenarioMap", u"-");
            al::startAction(mScenarioParts[0], "Shine_OFF", nullptr);
        }
    }

    for (s32 i = 1; i < cScenarioPartsNum; i++) {
        al::hidePaneRootNoRecursive(mScenarioParts[i]);
    }
}
