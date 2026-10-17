#pragma once
#include <container/seadPtrArray.h>
#include "Library/HostIO/IUseHioNode.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"
namespace al { class ActorInitInfo; }
class PlayerEntryPlayer;
class PlayerModelHolder;
class PlayerRetargettingSelector;
/** @brief Owns the player actors and character models shown during player entry. */
class PlayerEntryPlayerHolder : public al::IUseSceneObjHolder, public al::IUseHioNode {
public:
    explicit PlayerEntryPlayerHolder(const al::ActorInitInfo& rInfo);
    PlayerRetargettingSelector* getSelector() const { return mSelector; }
    void hidePlayerAll();
    PlayerEntryPlayer* getPlayerApplyedCharacterModel(int playerIndex, int characterType, int figure);
    PlayerModelHolder* getModelHolder(int characterType) const;
    /** @brief Returns the scene objects used by the entry actors. */
    al::SceneObjHolder* getSceneObjHolder() const override { return mSceneObjHolder; }
private:
    al::SceneObjHolder* mSceneObjHolder = nullptr;
    PlayerRetargettingSelector* mSelector = nullptr;
    sead::PtrArray<PlayerModelHolder> mModels;
    sead::FixedPtrArray<PlayerEntryPlayer, 4> mPlayers;
};
static_assert(sizeof(PlayerEntryPlayerHolder) == 0x58);
