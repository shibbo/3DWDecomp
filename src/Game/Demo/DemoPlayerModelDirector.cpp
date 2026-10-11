#include "Demo/DemoPlayerModelDirector.hpp"
#include "Demo/DemoScenePlayerModel.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Scene/SceneObjHolderAdapter.hpp"
#include "System/GameDataConst.hpp"
#include "Util/PlayerUtil.hpp"

namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
}

/** @brief Allocates empty model and figure request slots for every character. */
DemoPlayerModelDirector::DemoPlayerModelDirector()
    : mModels(nullptr), mRequests(nullptr), mSuffix(nullptr) {
    mModels = new DemoScenePlayerModel*[rc::getPlayerCharacterNumMax()];
    mRequests = new Request[rc::getPlayerCharacterNumMax()];
    for (int i = 0; i < rc::getPlayerCharacterNumMax(); ++i) {
        mModels[i] = nullptr;
        mRequests[i] = None;
    }
}

/** @brief Requests all figures for every character. @param pSuffix Model resource suffix. */
void DemoPlayerModelDirector::requestCreateAllPlayer(const char* pSuffix) {
    for (int i = 0; i < rc::getPlayerCharacterNumMax(); ++i) {
        mRequests[i] = All;
    }
    mSuffix = pSuffix;
}

/** @brief Sets the shared model suffix. @param pSuffix Model resource suffix. */
void DemoPlayerModelDirector::setPlayerSuffix(const char* pSuffix) {
    mSuffix = pSuffix;
}

/**
 * @brief Requests every figure for a character.
 * @param character Valid character index.
 * @param pSuffix Model resource suffix.
 * @param keepSuffix Whether to preserve the existing shared suffix.
 */
void DemoPlayerModelDirector::requestCreateAllFigure(int character, const char* pSuffix, bool keepSuffix) {
    mRequests[character] = All;
    if (!keepSuffix) {
        mSuffix = pSuffix;
    }
}

/**
 * @brief Requests the Super figure unless all figures were already requested.
 * @param character Valid character index.
 * @param pSuffix Model resource suffix.
 */
void DemoPlayerModelDirector::requestCreateFigureSuper(int character, const char* pSuffix) {
    if (mRequests[character] != All) {
        mRequests[character] = Super;
    }
    mSuffix = pSuffix;
}

/**
 * @brief Requests the climbing figure unless all figures were already requested.
 * @param character Valid character index.
 * @param pSuffix Model resource suffix.
 */
void DemoPlayerModelDirector::requestCreateFigureClimb(int character, const char* pSuffix) {
    if (mRequests[character] != All) {
        mRequests[character] = Climb;
    }
    mSuffix = pSuffix;
}

/**
 * @brief Records a Giga climbing figure request unless all figures were requested.
 * @param character Valid character index.
 * @param pSuffix Model resource suffix.
 */
void DemoPlayerModelDirector::requestCreateFigureClimbGiga(int character, const char* pSuffix) {
    if (mRequests[character] != All) {
        mRequests[character] = ClimbGiga;
    }
    mSuffix = pSuffix;
}

/**
 * @brief Creates and deactivates models for the supported figure requests.
 * @param rInfo Scene and actor initialization data for the new models.
 */
void DemoPlayerModelDirector::endInit(const al::ActorInitInfo& rInfo) {
    al::SceneObjHolderAdapter adapter(rInfo.getActorSceneInfo().sceneObjHolder);
    auto* pSelector = rc::createPlayerRetargettingSelector(&adapter);
    for (int i = 0; i < rc::getPlayerCharacterNumMax(); ++i) {
        const char* pName = GameDataConst::getPlayerCharacterName(i);
        switch (mRequests[i]) {
        case All:
            mModels[i] = DemoScenePlayerModel::createAll(rInfo, pName, pSelector, this, mSuffix);
            break;
        case Super:
            mModels[i] = DemoScenePlayerModel::createSingle(rInfo, pName, 0, pSelector, this, mSuffix);
            break;
        case Climb:
            mModels[i] = DemoScenePlayerModel::createSingle(rInfo, pName, 3, pSelector, this, mSuffix);
            break;
        default:
            continue;
        }
        mModels[i]->kill();
    }
}

/**
 * @brief Gets the created model for a character.
 * @param character Valid character index.
 * @return Model pointer, or nullptr if the character has no created model.
 */
DemoScenePlayerModel* DemoPlayerModelDirector::getDemoPlayerModel(int character) const {
    return mModels[character];
}
