#include "Layout/PlayerEntryPlayerHolder.hpp"
#include "Layout/PlayerEntryPlayer.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerModelHolderCreator.hpp"
#include "Player/Normal/PlayerRetargettingSelector.hpp"
#include "System/GameDataConst.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
}
/** @brief Creates four entry actors and all five playable character model sets.
 * @param rInfo Actor initialization context. */
PlayerEntryPlayerHolder::PlayerEntryPlayerHolder(const al::ActorInitInfo& rInfo) {
    mSceneObjHolder = rInfo.getActorSceneInfo().sceneObjHolder;
    for (int i = 0; i < mPlayers.capacity(); i++) {
        PlayerEntryPlayer* pPlayer = new PlayerEntryPlayer("プレイヤー選択画面プレイヤー", this);
        pPlayer->init(rInfo);
        mPlayers.pushBack(pPlayer);
    }
    mSelector = rc::createPlayerRetargettingSelector(this);
    mModels.allocBuffer(5, nullptr);
    for (int i = 0; i < mModels.capacity(); i++) {
        PlayerModelHolder* pModel = PlayerModelHolderCreator::createModelHolderAll(
            rInfo, GameDataConst::getPlayerCharacterName(i), mSelector, this, false, 0, nullptr, 0);
        for (int figure = 0; figure < 10; figure++)
            al::createPadRumbleKeeper(pModel->getModel(figure), -1);
        pModel->kill();
        mModels.pushBack(pModel);
        PlayerModel* pCurrent = pModel->getCurrentModel();
        mSelector->applyDefaultRetargettingInfo(pCurrent, pCurrent->getRetargettingIndex());
    }
}
/** @brief Hides every loaded character model set. */
void PlayerEntryPlayerHolder::hidePlayerAll() {
    for (int i = 0; i < mModels.size(); i++)
        mModels.at(i)->kill();
}
/** @brief Assigns a character and figure to an entry actor.
 * @param playerIndex Entry actor index.
 * @param characterType Character model index.
 * @param figure Initial figure index.
 * @return Entry actor with the selected model. */
PlayerEntryPlayer* PlayerEntryPlayerHolder::getPlayerApplyedCharacterModel(
    int playerIndex, int characterType, int figure) {
    PlayerEntryPlayer* pPlayer = mPlayers.at(playerIndex);
    PlayerModelHolder* pModel = mModels.at(characterType);
    pModel->initCurrentModel(figure);
    pPlayer->setPlayerModel(pModel);
    return pPlayer;
}
/** @brief Gets a loaded character model set.
 * @param characterType Character model index.
 * @return Model holder, or nullptr when the index is outside the array. */
PlayerModelHolder* PlayerEntryPlayerHolder::getModelHolder(int characterType) const {
    return mModels.at(characterType);
}
