#include "Scene/PlayerStocker.hpp"

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Giga/PlayerActionGraphBuilder.hpp"
#include "Player/Normal/PlayerRetargettingSelector.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "MapObj/DoubleMario.hpp"
#include "Enemy/KuriboTower.hpp"
#include "Enemy/MarchGenerator.hpp"
#include "Enemy/BossBunretsu.hpp"
#include "MapObj/WoodBox.hpp"

namespace {
struct DoubleMarioEntry {
    const char* name;
    s32 (*calcNum)(const al::ActorInitInfo&);
};
const DoubleMarioEntry sDoubleMarioEntries[] = {
    {"DoubleMario", &DoubleMario::calcAppearDoubleMarioNum},
    {"KuriboTower", &KuriboTower::calcAppearDoubleMarioNum},
    {"MarchGenerator", &MarchGenerator::calcAppearDoubleMarioNum},
    {"BossBunretsu", &BossBunretsu::calcAppearDoubleMarioNum},
    {"WoodBox", &WoodBox::calcAppearDoubleMarioNum},
    {"BlockBrick", &rc::calcAppearDoubleMarioNumForBlock},
    {"BlockQuestion", &rc::calcAppearDoubleMarioNumForBlock},
    {"BlockTransparent", &rc::calcAppearDoubleMarioNumForBlock},
};
}

PlayerStocker::PlayerStocker(s32 maxDoubleMario) : mMaxDoubleMario(maxDoubleMario) {
    mPlayers.allocBuffer(rc::getPlayerCharacterNumMax(), nullptr);
    mAppearedItems = new al::LiveActor*[4];
    for (s32 i = 0; i < mPlayers.capacity(); ++i) {
        auto* players = new sead::PtrArray<PlayerActor>;
        players->allocBuffer(20, nullptr);
        mPlayers.pushBack(players);
    }
    for (s32 i = 0; i < 4; ++i)
        mAppearedItems[i] = nullptr;
}

bool PlayerStocker::tryCreateDoubleMario(const al::ActorInitInfo& rInfo,
                                        PlayerRetargettingSelector* pSelector,
                                        PlayerInvincibleBgmController* pBgmController,
                                        bool isNameplate) {
    const char* className = nullptr;
    al::getClassName(&className, rInfo);
    s32 index = 0;
    for (; index < 8; ++index) {
        if (al::isEqualString(className, sDoubleMarioEntries[index].name))
            break;
    }
    if (index == 8)
        return false;
    s32 count = sDoubleMarioEntries[index].calcNum(rInfo);
    if (count == 0 || mMaxDoubleMario <= mDoubleMarioCreateNum)
        return false;
    if (mMaxDoubleMario < mDoubleMarioCreateNum + count)
        count = mMaxDoubleMario - mDoubleMarioCreateNum;
    for (s32 i = 0; i < count; ++i) {
        createAndRegisterPlayer(rInfo, 0, "Mario", pSelector, pBgmController, isNameplate);
        createAndRegisterPlayer(rInfo, 1, "Luigi", pSelector, pBgmController, isNameplate);
        createAndRegisterPlayer(rInfo, 2, "Peach", pSelector, pBgmController, isNameplate);
        createAndRegisterPlayer(rInfo, 3, "Kinopio", pSelector, pBgmController, isNameplate);
        createAndRegisterPlayer(rInfo, 4, "Rosetta", pSelector, pBgmController, isNameplate);
    }
    mDoubleMarioCreateNum += count;
    return true;
}

void PlayerStocker::createAndRegisterPlayer(const al::ActorInitInfo& rInfo, s32 characterType,
                                           const char* pCharacterName,
                                           PlayerRetargettingSelector* pSelector,
                                           PlayerInvincibleBgmController* pBgmController,
                                           bool isNameplate) {
    auto* player = new PlayerActor(nullptr);
    PlayerActionGraphBuilder graphBuilder(false);
    player->initSpecial(rInfo, 0, pCharacterName, pSelector, pSelector, &graphBuilder,
                        "ダブルプレイヤー", 19, 0, nullptr);
    player->setInvincibleBgmController(pBgmController);
    rc::deactivatePlayer(player);
    alPlayerFunction::registerPlayer(player, player->getPadRumbleKeeper(), true);
    if (isNameplate)
        player->initNameplate(*rInfo.getLayoutInitInfo());
    registerPlayer(player, characterType);
}

PlayerActor* PlayerStocker::getUnusedPlayer(s32 characterType) const {
    for (s32 i = 0; i < mPlayers.unsafeAt(characterType)->size(); ++i) {
        PlayerActor* player = mPlayers.unsafeAt(characterType)->at(i);
        if (al::isDead(player))
            return player;
    }
    return nullptr;
}

void PlayerStocker::registerPlayer(PlayerActor* pPlayer, s32 characterType) {
    mPlayers.at(characterType)->pushBack(pPlayer);
}

void PlayerStocker::addDoubleItemAppearedNum(al::LiveActor* pItem) {
    for (s32 i = 0; i < 4; ++i) {
        if (!mAppearedItems[i]) {
            mAppearedItems[i] = pItem;
            return;
        }
    }
}

void PlayerStocker::decDoubleItemAppearedNum(al::LiveActor* pItem) {
    for (s32 i = 0; i < 4; ++i) {
        if (mAppearedItems[i] == pItem) {
            mAppearedItems[i] = nullptr;
            return;
        }
    }
}

bool PlayerStocker::isDoubleItemAppearedMax(const al::LiveActor* pActor) const {
    return mDoubleMarioCreateNum <= getDoubleMarioItemAppearedNum() + rc::calcDoubleMarioTotalNum(pActor);
}

s32 PlayerStocker::getDoubleMarioItemAppearedNum() const {
    s32 count = 0;
    for (s32 i = 0; i < 4; ++i) {
        if (mAppearedItems[i])
            ++count;
    }
    return count;
}

void PlayerStocker::killAllAppearDoubleItem() {
    for (s32 i = 0; i < 4; ++i) {
        if (mAppearedItems[i]) {
            mAppearedItems[i]->kill();
            mAppearedItems[i] = nullptr;
        }
    }
}

namespace PlayerStockerFunction {
void createPlayerStocker(const al::Scene* pScene, bool isDoubleMario) {
    s32 count = isDoubleMario ? GameDataFunction::getDoubleMarioNumMax(GameDataHolderAccessor(pScene)) : 0;
    al::setSceneObj(pScene, new PlayerStocker(count), SceneObjID_PlayerStocker);
}

void createPlayerStockerSingleMode(const al::Scene* pScene, s32 maxDoubleMario) {
    al::setSceneObj(pScene, new PlayerStocker(maxDoubleMario), SceneObjID_PlayerStocker);
}

void registerPlacementPlayer(const al::Scene* pScene, PlayerActor* pPlayer, s32 characterType) {
    al::getSceneObj<PlayerStocker>(pScene, SceneObjID_PlayerStocker)->registerPlayer(pPlayer, characterType);
}

void tryCreateDoubleMario(const al::Scene* pScene, const al::ActorInitInfo& rInfo,
                         PlayerRetargettingSelector* pSelector,
                         PlayerInvincibleBgmController* pBgmController, bool isNameplate) {
    if (!al::isExistSceneObj(pScene, SceneObjID_PlayerStocker))
        return;
    for (s32 stageIndex = 0; stageIndex < al::getStageInfoMapNum(pScene); ++stageIndex) {
        const al::StageInfo* stage = al::getStageInfoMap(pScene, stageIndex);
        al::PlacementInfo list;
        s32 count = 0;
        al::getPlacementInfoAndCount(&list, &count, stage, "ObjectList");
        for (s32 i = 0; i < count; ++i) {
            al::PlacementInfo placement;
            al::getPlacementInfoByIndex(&placement, list, i);
            al::ActorInitInfo info;
            info.initViewIdSelf(&placement, rInfo);
            al::getSceneObj<PlayerStocker>(pScene, SceneObjID_PlayerStocker)->tryCreateDoubleMario(
                info, pSelector, pBgmController, isNameplate);
        }
    }
}

void appearDoubleMario(const al::LiveActor* pActor, const al::HitSensor* pSensor) {
    s32 characterType = rc::getPlayerCharaType(pSensor);
    auto* stocker = al::getSceneObj<PlayerStocker>(pActor, SceneObjID_PlayerStocker);
    PlayerActor* player = stocker->getUnusedPlayer(characterType);
    s32 port = rc::getPlayerInputPort(pSensor);
    player->setViewMtx(rc::getPlayerViewMtx(pSensor));
    player->replaceInputPort(port);
    PlayerProperty* property = player->getProperty();
    property->mTrans = al::getTrans(pActor);
    al::onAreaTarget(player);
    player->copyNameplate(al::getSensorHost(pSensor));
    sead::Vector3f front;
    al::calcFrontDir(&front, pActor);
    player->getProperty()->setFrontVec(front);
    player->updatePosture();
    player->appear();
    player->getModelHolder()->appear();
    rc::invalidatePlayerDamage(player, 30);
    al::startHitReaction(player, "ダブルマリオ出現");
    al::LiveActor* host = al::getSensorHost(pSensor);
    s32 figureType = rc::getPlayerFigureType(host);
    if (rc::isPlayerNextFigureRequested(host))
        figureType = rc::getPlayerNextFigureType(host);
    switch (figureType) {
    case 0: rc::changeToSuperMarioForce(player); break;
    case 1: rc::changeToMiniMarioForce(player); break;
    case 2: rc::changeToFireMarioForce(player); break;
    case 3: rc::changeToClimbMarioForce(player); break;
    case 4: rc::changeToRaccoonDogMarioForce(player); break;
    case 5: rc::changeToBoomerangMarioForce(player); break;
    case 6: rc::changeToRaccoonDogWhiteMarioForce(player); break;
    case 7: rc::changeToClimbMarioSpecialForce(player); break;
    case 8: rc::changeToClimbWhiteMarioForce(player); break;
    }
    al::setSeSeqLocalVariableDefault(player, 1, figureType);
    al::startSe(player, "PgDoublePlayerAppear", nullptr);
}

void addDoubleItemAppearedNum(al::LiveActor* pActor) {
    al::getSceneObj<PlayerStocker>(pActor, SceneObjID_PlayerStocker)->addDoubleItemAppearedNum(pActor);
}

void decDoubleItemAppearedNum(al::LiveActor* pActor) {
    al::getSceneObj<PlayerStocker>(pActor, SceneObjID_PlayerStocker)->decDoubleItemAppearedNum(pActor);
}

bool isDoubleItemAppearedMax(const al::LiveActor* pActor) {
    return al::getSceneObj<PlayerStocker>(pActor, SceneObjID_PlayerStocker)->isDoubleItemAppearedMax(pActor);
}

s32 getDoubleMarioCreateNum(const al::LiveActor* pActor) {
    return al::getSceneObj<PlayerStocker>(pActor, SceneObjID_PlayerStocker)->getDoubleMarioCreateNum();
}

void killAllAppearDoubleItem(al::LiveActor* pActor) {
    al::getSceneObj<PlayerStocker>(pActor, SceneObjID_PlayerStocker)->killAllAppearDoubleItem();
}
}
