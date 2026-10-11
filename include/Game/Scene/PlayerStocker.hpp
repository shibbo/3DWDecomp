#pragma once

#include <container/seadPtrArray.h>
#include "Library/Scene/ISceneObj.hpp"

namespace al { class LiveActor; class HitSensor; class Scene; }
class PlayerActor;
class PlayerRetargettingSelector;
class PlayerInvincibleBgmController;

class PlayerStocker : public al::ISceneObj {
public:
    explicit PlayerStocker(s32 maxDoubleMario);
    const char* getSceneObjName() const override { return "プレイヤー保持"; }
    bool tryCreateDoubleMario(const al::ActorInitInfo& rInfo,
                              PlayerRetargettingSelector* pSelector,
                              PlayerInvincibleBgmController* pBgmController, bool isNameplate);
    void createAndRegisterPlayer(const al::ActorInitInfo& rInfo, s32 characterType,
                                 const char* pCharacterName, PlayerRetargettingSelector* pSelector,
                                 PlayerInvincibleBgmController* pBgmController, bool isNameplate);
    PlayerActor* getUnusedPlayer(s32 characterType) const;
    void registerPlayer(PlayerActor* pPlayer, s32 characterType);
    void addDoubleItemAppearedNum(al::LiveActor* pItem);
    void decDoubleItemAppearedNum(al::LiveActor* pItem);
    bool isDoubleItemAppearedMax(const al::LiveActor* pActor) const;
    s32 getDoubleMarioItemAppearedNum() const;
    void killAllAppearDoubleItem();
    s32 getDoubleMarioCreateNum() const { return mDoubleMarioCreateNum; }

private:
    sead::PtrArray<sead::PtrArray<PlayerActor>> mPlayers;
    al::LiveActor** mAppearedItems = nullptr;
    s32 mMaxDoubleMario;
    s32 mDoubleMarioCreateNum = 0;
};
static_assert(sizeof(PlayerStocker) == 0x28);

namespace PlayerStockerFunction {
void createPlayerStocker(const al::Scene* pScene, bool isDoubleMario);
void createPlayerStockerSingleMode(const al::Scene* pScene, s32 maxDoubleMario);
void registerPlacementPlayer(const al::Scene* pScene, PlayerActor* pPlayer, s32 characterType);
void tryCreateDoubleMario(const al::Scene* pScene, const al::ActorInitInfo& rInfo,
                         PlayerRetargettingSelector* pSelector,
                         PlayerInvincibleBgmController* pBgmController, bool isNameplate);
void appearDoubleMario(const al::LiveActor* pActor, const al::HitSensor* pSensor);
void addDoubleItemAppearedNum(al::LiveActor* pActor);
void decDoubleItemAppearedNum(al::LiveActor* pActor);
bool isDoubleItemAppearedMax(const al::LiveActor* pActor);
s32 getDoubleMarioCreateNum(const al::LiveActor* pActor);
void killAllAppearDoubleItem(al::LiveActor* pActor);
}
