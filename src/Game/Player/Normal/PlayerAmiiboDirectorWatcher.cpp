#include "Player/Normal/PlayerAmiiboDirectorWatcher.hpp"

#include <controller/seadControllerMgr.h>
#include <cstdio>
#include <random/seadRandom.h>
#include <time/seadTickTime.h>

#include "Layout/AmiiboLayout.hpp"
#include "Layout/GuideGameWindow.hpp"
#include "Layout/IslandMap.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/NpadController.hpp"
#include "Library/Item/ItemDirectorBase.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Nfp/NfpDirector.hpp"
#include "Library/Nfp/NfpFunction.hpp"
#include "Library/Nfp/NfpTypes.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Sequence/Sequence.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAmiiboDirector.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/GameSystem.hpp"
#include "Util/DemoUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/SceneUtil.hpp"

/// Declares a nerve named Action that runs PlayerAmiiboDirector::exe##Exe (several nerves share an exe).
#define PLAYER_AMIIBO_DIRECTOR_NERVE(Action, Exe)                                                 \
    class PlayerAmiiboDirectorNrv##Action : public al::Nerve {                                     \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<PlayerAmiiboDirector>()->exe##Exe();                                \
        }                                                                                          \
    };

namespace {
NERVE_DECL(PlayerAmiiboDirector, WaitForInput)
NERVE_DECL(PlayerAmiiboDirector, Scan)
NERVE_DECL(PlayerAmiiboDirector, Hold)
NERVE_DECL(PlayerAmiiboDirector, ScanFailed)
PLAYER_AMIIBO_DIRECTOR_NERVE(ScanInvalid, Scan)
PLAYER_AMIIBO_DIRECTOR_NERVE(PostScanWhiteBell, PostScan)
PLAYER_AMIIBO_DIRECTOR_NERVE(PostScanRandomItem, PostScan)
PLAYER_AMIIBO_DIRECTOR_NERVE(PostScanDisaster, PostScan)
PLAYER_AMIIBO_DIRECTOR_NERVE(PostScanSuperStar, PostScan)
PLAYER_AMIIBO_DIRECTOR_NERVE(PostScanKoopaJr, PostScan)
PLAYER_AMIIBO_DIRECTOR_NERVE(PostScanSuperKinoko, PostScan)
PLAYER_AMIIBO_DIRECTOR_NERVE(PostScanOneUpKinoko, PostScan)

NERVES_MAKE_NOSTRUCT(PlayerAmiiboDirector, Hold)

/// Items a cat Peach amiibo can give while the player rides Plessie (thrown at the player).
const char* const cRandomItemNamesHoming[] = {
    "WhiteBell[Homing]",
    "スーパーキノコ[アイテムストック]",
    "スーパーベル[アイテムストック]",
    "ファイアフラワー[アイテムストック]",
    "スーパーこのは[アイテムストック]",
    "ブーメランフラワー[アイテムストック]",
    "まねきネコベル[アイテムストック]",
    "SuperStar[Homing]",
};

/// Items a cat Peach amiibo can give.
const char* const cRandomItemNames[] = {
    "WhiteBell",      "スーパーキノコ",     "スーパーベル",   "ファイアフラワー",
    "スーパーこのは", "ブーメランフラワー", "まねきネコベル", "スーパースター",
};

NERVES_MAKE_STRUCT(PlayerAmiiboDirector, WaitForInput, Scan, ScanFailed, ScanInvalid,
                   PostScanWhiteBell, PostScanRandomItem, PostScanDisaster, PostScanSuperStar,
                   PostScanKoopaJr, PostScanSuperKinoko, PostScanOneUpKinoko)

/// Compact form of an al::NfpCharacterId.
struct CharacterIdBase {
    s8 gameId;
    s8 characterId;
    s8 characterVariant;
};

/// Characters of the Super Mario series whose amiibo give a Super Star.
const CharacterIdBase cSuperStarCharacterIds[] = {
    {0, 0, 0},  {0, 1, 0},  {0, 2, 0},  {0, 3, 0},  {0, 4, 0},  {0, 7, 0},  {0, 8, 0},  {0, 9, 0},
    {0, 10, 0}, {0, 19, 0}, {0, 20, 0}, {0, 21, 0}, {0, 23, 0}, {0, 35, 0}, {0, 36, 0},
};

constexpr s32 cSuperStarCharacterIdNum = 15;

/**
 * Gets the npad id the NFP director uses for a player's controller.
 * @param pPlayer player whose controller is looked up
 * @return the npad id (the handheld controller maps to its own npad id)
 */
s32 getPlayerNpadId(const al::LiveActor* pPlayer) {
    al::NpadController* controller = sead::DynamicCast<al::NpadController>(
        sead::ControllerMgr::instance()->getController(rc::getPlayerInputPort(pPlayer)));
    s32 npadId = controller->getNpadId();
    return npadId == 8 ? 0x20 : npadId;
}

/**
 * Compares the unique ids of two tags.
 * @param rTagIdA first tag
 * @param rTagIdB second tag
 * @return true if both tags have the same unique id
 */
bool isEqualTagId(const nn::nfp::TagId& rTagIdA, const nn::nfp::TagId& rTagIdB) {
    for (s32 i = 0; i < 10; i++) {
        if (rTagIdA.uuid[i] != rTagIdB.uuid[i]) {
            return false;
        }
    }

    return true;
}

/**
 * Spawns an amiibo reward item in front of a player.
 * @param pPlayer player receiving the item
 * @param pName item name
 * @param rTrans spawn position
 * @param rFront spawn direction
 * @param pSensor sensor the item homes in on, or nullptr
 * @param isTakeOut whether the item pops out instead of being placed
 */
void appearAmiiboItem(const PlayerActor* pPlayer, const char* pName, const sead::Vector3f& rTrans,
                      const sead::Vector3f& rFront, const al::HitSensor* pSensor,
                      bool isTakeOut) {
    al::ItemDirectorBase* director = pPlayer->getSceneInfo()->itemDirectorBase;
    if (director != nullptr) {
        director->appearItem(pName, rTrans, rFront, pSensor, isTakeOut, true);
    }
}
/// Picks the random item of a cat Peach amiibo (seeded by PlayerAmiiboDirector::initRandomSeed).
sead::Random sRandom(0);
}  // namespace

/**
 * Seeds the random generator of the amiibo rewards with the system tick.
 */
void PlayerAmiiboDirector::initRandomSeed() {
    sead::TickTime tick;
    tick.setNow();
    sRandom.init(tick.toTicks());
}

/**
 * Creates the director of a player and registers it with the watcher.
 * @param pPlayer player the director belongs to
 * @param pWatcher watcher shared by every player's director
 * @param rInfo init info of the player
 */
PlayerAmiiboDirector::PlayerAmiiboDirector(const PlayerActor* pPlayer,
                                           PlayerAmiiboDirectorWatcher* pWatcher,
                                           const al::ActorInitInfo& rInfo)
    : mLayout(nullptr), mPlayer(pPlayer), mInitInfo(nullptr), mIsDuplicate(false),
      mWatcher(pWatcher), mIsBusy(false), mIsPause(false), mIsSingleMode(false),
      mIsScanQueued(false) {
    mIsSingleMode = al::isSingleMode(rInfo);
    initRandomSeed();
    mNerveKeeper = new al::NerveKeeper(this, &NrvPlayerAmiiboDirector.WaitForInput, 0);
    if (!mWatcher->tryRegisterDirector(this)) {
        mIsDuplicate = true;
    }

    mLayout = new AmiiboLayout(*rInfo.getLayoutInitInfo(), pPlayer);
    mInitInfo = new al::ActorInitInfo(rInfo);
}

/**
 * Registers a player's director unless another one already serves the same character.
 * @param pDirector director to register
 * @return true if the director was registered
 */
bool PlayerAmiiboDirectorWatcher::tryRegisterDirector(PlayerAmiiboDirector* pDirector) {
    EPlayerChara chara = pDirector->getPlayerChara();
    for (s32 i = 0; i < mDirectors.size(); i++) {
        if (static_cast<s32>(chara) == static_cast<s32>(mDirectors[i]->getPlayerChara())) {
            return false;
        }
    }

    mDirectors.pushBack(pDirector);
    return true;
}

/**
 * Updates the busy state from the player's situation and runs the nerve.
 */
void PlayerAmiiboDirector::update() {
    if (mIsDuplicate || mPlayer == nullptr) {
        return;
    }

    mIsBusy = mIsPause;
    mIsBusy |= rc::isPlayerBinded(mPlayer) && !rc::isPlayerOnRaidon(mPlayer);
    if (mPlayer->getSceneInfo()->demoDirector != nullptr) {
        mIsBusy |= rc::isActiveDemo(mPlayer);
    }

    GuideGameWindow* window =
        al::tryGetSceneObj<GuideGameWindow>(mPlayer, SceneObjID_GuideGameWindow);
    if (window != nullptr) {
        mIsBusy |= window->isWaitConfirm();
    }

    mNerveKeeper->update();
}

/**
 * Closes the layout and goes back to waiting for input.
 * @param isDemo whether a demo is running (closes the layout immediately)
 */
void PlayerAmiiboDirector::reset(bool isDemo) {
    if (mLayout->isAlive()) {
        mLayout->end(isDemo);
    }

    al::setNerve(this, &NrvPlayerAmiiboDirector.WaitForInput);
}

/**
 * Forgets the tags used so far.
 */
void PlayerAmiiboDirector::clear() {
    if (mWatcher != nullptr) {
        mWatcher->clear();
    }
}

/**
 * Forgets the tags used so far.
 */
void PlayerAmiiboDirectorWatcher::clear() {
    mUsedTags.clear();
}

/**
 * Gets the character of the player.
 * @return the character, or -1 without a player
 */
EPlayerChara PlayerAmiiboDirector::getPlayerChara() {
    if (mPlayer == nullptr) {
        return -1;
    }

    return mPlayer->getChara();
}

/**
 * Pauses the director and cancels a running scan.
 * @param isDemo whether a demo is starting
 */
void PlayerAmiiboDirector::startPause(bool isDemo) {
    mIsPause = true;
    if (al::isNerve(this, &NrvPlayerAmiiboDirector.Scan)) {
        mWatcher->requestStop(false, mPlayer);
        reset(isDemo);
    }
}

/**
 * Stops a player's scan request, or every scan when forced.
 * @param isForce whether to reset every director and stop the NFP director
 * @param pPlayer player whose request is withdrawn
 */
void PlayerAmiiboDirectorWatcher::requestStop(bool isForce, const al::LiveActor* pPlayer) {
    if (isForce) {
        for (s32 i = 0; i < mDirectors.size(); i++) {
            mDirectors[i]->reset(false);
        }

        mScanRequestCount = 0;
        mNfpDirector->stop(true);
        return;
    }

    s32 npadId = getPlayerNpadId(pPlayer);
    if (mScanRequestCount > 0 && mNfpDirector->getRequestNpadId() == npadId) {
        mScanRequestCount--;
        if (mScanRequestCount == 0) {
            mNfpDirector->stop(true);
        }
    }
}

/**
 * Resumes the director.
 */
void PlayerAmiiboDirector::endPause() {
    mIsPause = false;
}

/**
 * Sets the player the director (and its layout) belongs to.
 * @param pPlayer the player
 */
void PlayerAmiiboDirector::setPlayerActor(const PlayerActor* pPlayer) {
    mPlayer = pPlayer;
    mLayout->setPlayerActor(pPlayer);
}

/**
 * Checks whether a scan was requested and has not started yet.
 * @return true if a scan is queued
 */
bool PlayerAmiiboDirector::isScanQueued() const {
    return mIsScanQueued;
}

/**
 * Waits for the player to hold the left button.
 */
void PlayerAmiiboDirector::exeWaitForInput() {
    if (!mIsBusy && al::isPadHoldLeft(rc::getPlayerInputPort(mPlayer))) {
        al::setNerve(this, &NrvPlayerAmiiboDirectorHold);
    }
}

/**
 * Starts a scan once the left button was held long enough and the player may get a reward.
 */
void PlayerAmiiboDirector::exeHold() {
    if (mIsBusy || !al::isPadHoldLeft(rc::getPlayerInputPort(mPlayer))) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.WaitForInput);
    }

    if (!al::isStep(this, 25) || rc::isPlayerDamageTrigOn(mPlayer) || !mWatcher->isEnable()) {
        return;
    }

    bool isGiga = mPlayer->mActorSceneInfo->isSingleMode && rc::isPlayerGiga(mPlayer);
    bool isSingleMode = GameDataFunction::isSingleMode(mPlayer);
    bool isOnGround = rc::isPlayerOnGroundOrWater(mPlayer);
    bool isFailed;
    if (isSingleMode) {
        bool isInAir = !isOnGround && !rc::isPlayerOnRaidon(mPlayer);
        isFailed = isInAir | isGiga;
        isFailed |= SingleModeDataFunction::getUnlockedPhase(mLayout) <= 1 &&
                    !SingleModeDataFunction::hasSeenCutscene(mLayout, 1);

        GameSystem* gameSystem = GameSystemFunction::getGameSystem();
        if (gameSystem != nullptr) {
            al::Sequence* sequence = gameSystem->getSequence();
            if (sequence != nullptr && sequence->getCurrentScene() != nullptr) {
                isFailed |= rc::isSingleModeBossBattleScene(sequence->getCurrentScene());
            }
        }
    } else {
        isFailed = !isOnGround | isGiga;
    }

    if (isFailed || mWatcher->isScanRequested() || mWatcher->isOtherScanQueued(this)) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.ScanFailed);
        return;
    }

    mIsScanQueued = true;
    al::setNerve(this, &NrvPlayerAmiiboDirector.Scan);
}

/**
 * Checks whether a director other than the given one has a scan queued.
 * @param pDirector director to ignore
 * @return true if another director has a scan queued
 */
bool PlayerAmiiboDirectorWatcher::isOtherScanQueued(const PlayerAmiiboDirector* pDirector) const {
    for (s32 i = 0; i < mDirectors.size(); i++) {
        if (mDirectors[i] != pDirector && mDirectors[i]->isScanQueued()) {
            return true;
        }
    }

    return false;
}

/**
 * Scans the amiibo, reports it and picks the reward nerve.
 */
void PlayerAmiiboDirector::exeScan() {
    if (al::isFirstStep(this)) {
        mIsScanQueued = false;
        if (mIsBusy) {
            al::setNerve(this, &NrvPlayerAmiiboDirector.WaitForInput);
            return;
        }

        mLayout->appear();
        mWatcher->requestScan(mPlayer);
    }

    bool hadError = mWatcher->hadNfpError();
    bool isHold = al::isPadHoldLeft(rc::getPlayerInputPort(mPlayer));
    if (hadError || !isHold || mIsBusy) {
        mWatcher->requestStop(hadError, mPlayer);
        mLayout->end(rc::isActiveDemo(mPlayer));
        al::setNerve(this, &NrvPlayerAmiiboDirector.WaitForInput);
        return;
    }

    al::NfpInfo* info = mWatcher->getNfpInfo(mPlayer);
    if (info == nullptr || !info->_9e) {
        return;
    }

    if (al::isNerve(this, &NrvPlayerAmiiboDirector.ScanInvalid) || !isAmiiboTypeAllowed(info) ||
        !mWatcher->isValidTag(info->tagInfo.id)) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.ScanFailed);
        return;
    }

    mWatcher->requestStop(true, nullptr);
    al::startSe(mLayout, "Success");

    if (mPlayer != nullptr) {
        al::NfpCharacterId characterId = {};
        s32 numberingId;
        s32 seriesId;
        s32 nfpType;
        al::tryGetCharacterId(&characterId, *info);
        al::tryGetNumberingId(&numberingId, *info);
        al::tryGetSeriesID(&seriesId, *info);
        al::tryGetNfpType(&nfpType, *info);

        sprintf(mCharacterIdBuffer, "%03u %03u %03u", characterId.gameId, characterId.characterId,
                characterId.characterVariant);
        mCharacterIdBuffer[12] = '\0';
        mCharacterIdString = sead::SafeString(mCharacterIdBuffer);

        const nn::nfp::TagId& tagId = info->tagInfo.id;
        char* tagIdText = mTagIdBuffer;
        for (u8 i = 0; i < tagId.uuidLength; i++) {
            sprintf(tagIdText, " %02X", tagId.uuid[i]);
            if (i + 1 >= sizeof(tagId.uuid)) {
                break;
            }

            tagIdText += 3;
        }

        mTagIdString = sead::SafeString(mTagIdBuffer);

        if (!SingleModeDataFunction::beginPlayReport(mPlayer, mPlayer,
                                                     preport::KeyEventType(22), 6, 0)) {
            SingleModeDataFunction::setPlayReportData(mPlayer, preport::Key(45),
                                                      GameDataFunction::isSingleMode(mPlayer));
            SingleModeDataFunction::setPlayReportData(mPlayer, preport::Key(53), mTagIdString);
            SingleModeDataFunction::setPlayReportData(mPlayer, preport::Key(54),
                                                      mCharacterIdString);
            SingleModeDataFunction::setPlayReportData(mPlayer, preport::Key(57), numberingId);
            SingleModeDataFunction::setPlayReportData(mPlayer, preport::Key(58), seriesId);
            SingleModeDataFunction::setPlayReportData(mPlayer, preport::Key(59), nfpType);
            SingleModeDataFunction::endPlayReport(mPlayer);
        }
    }

    if (al::isCharacterIdBaseMario(*info) && al::isEqualNumberingId(*info, 934)) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanWhiteBell);
        return;
    }

    if (al::isCharacterIdBasePeach(*info) && al::isEqualNumberingId(*info, 935)) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanRandomItem);
        return;
    }

    if (al::isCharacterIdBaseKoopa(*info)) {
        if (GameDataFunction::isSingleMode(mPlayer)) {
            al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanDisaster);
        } else {
            al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanSuperStar);
        }

        return;
    }

    if (al::isCharacterIdBaseKoopaJr(*info)) {
        if (GameDataFunction::isSingleMode(mPlayer)) {
            al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanKoopaJr);
        } else {
            al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanSuperStar);
        }

        return;
    }

    if (al::isEqualSeriesID(*info, 1) || al::isEqualSeriesID(*info, 6)) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanSuperStar);
        return;
    }

    for (s32 i = 0; i < cSuperStarCharacterIdNum; i++) {
        const CharacterIdBase& base = cSuperStarCharacterIds[i];
        al::NfpCharacterId id = {base.gameId, base.characterId, base.characterVariant};
        if (al::isEqualCharacterIdBase(*info, id)) {
            al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanSuperStar);
            return;
        }
    }

    if (GameDataFunction::isSingleMode(mPlayer)) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanSuperKinoko);
    } else {
        al::setNerve(this, &NrvPlayerAmiiboDirector.PostScanOneUpKinoko);
    }
}

/**
 * Requests a scan on a player's controller unless one is already running.
 * @param pPlayer player whose controller scans
 */
void PlayerAmiiboDirectorWatcher::requestScan(const al::LiveActor* pPlayer) {
    if (mScanRequestCount != 0) {
        return;
    }

    mNfpDirector->startDelayed(10);
    mScanRequestCount++;
    s32 npadId = getPlayerNpadId(pPlayer);
    mNfpDirector->setRequestNpadId(npadId);
}

/**
 * Checks whether the NFP director ran into an error.
 * @return true on an error
 */
bool PlayerAmiiboDirectorWatcher::hadNfpError() const {
    return mNfpDirector->hadError();
}

/**
 * Gets the scan result of a player's controller.
 * @param pPlayer player whose controller scanned
 * @return the scan result, or nullptr
 */
al::NfpInfo* PlayerAmiiboDirectorWatcher::getNfpInfo(const al::LiveActor* pPlayer) {
    al::NfpDirector* nfpDirector = mNfpDirector;
    s32 index = nfpDirector->findDeviceIndex(getPlayerNpadId(pPlayer));
    return mNfpDirector->getNfpInfo(index);
}

/**
 * Checks whether the scanned amiibo may be used by the player right now.
 * @param pInfo scan result
 * @return true if the amiibo is allowed
 */
bool PlayerAmiiboDirector::isAmiiboTypeAllowed(const al::NfpInfo* pInfo) const {
    if (al::isCharacterIdBaseKoopa(*pInfo)) {
        return isKoopaAllowed();
    }

    if (al::isCharacterIdBaseKoopaJr(*pInfo)) {
        return isKoopaJrAllowed();
    }

    return true;
}

/**
 * Checks whether a tag was not used yet and remembers it.
 * @param rTagId tag of the scanned amiibo
 * @return true if the tag was not used before
 */
bool PlayerAmiiboDirectorWatcher::isValidTag(const nn::nfp::TagId& rTagId) {
    for (s32 i = 0; i < mUsedTags.size(); i++) {
        if (isEqualTagId(rTagId, *mUsedTags.unsafeAt(i))) {
            return false;
        }
    }

    mUsedTags.emplaceBack(rTagId);
    return true;
}

/**
 * Hands out the reward of the scanned amiibo, then goes back to waiting.
 */
void PlayerAmiiboDirector::exePostScan() {
    bool isSingleMode = mIsSingleMode;
    sead::Vector3f trans = al::getTrans(mPlayer);
    sead::Vector3f front;

    bool isOnRaidon = false;
    if (mIsSingleMode) {
        isOnRaidon = rc::isPlayerOnRaidon(mPlayer);
    }

    const al::HitSensor* sensor = nullptr;
    if (mIsSingleMode) {
        bool isInWater = rc::isPlayerInWater(mPlayer);
        if (isOnRaidon || isInWater) {
            trans.y += 80.0f;
        }

        if (isOnRaidon) {
            al::HitSensor* bodySensor = al::getHitSensor(mPlayer, "Body");
            front = al::getSensorPos(bodySensor) - trans;
            al::setAppearItemAttackerSensor(mPlayer, bodySensor);
            sensor = bodySensor;
        } else {
            al::calcFrontDir(&front, mPlayer);
        }
    } else {
        al::calcFrontDir(&front, mPlayer);
    }

    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvPlayerAmiiboDirector.PostScanWhiteBell)) {
            appearAmiiboItem(mPlayer, isOnRaidon ? "WhiteBell[Homing]" : "WhiteBell", trans,
                             front, sensor, !isSingleMode);
        } else if (al::isNerve(this, &NrvPlayerAmiiboDirector.PostScanRandomItem)) {
            al::ItemDirectorBase* director = mPlayer->getSceneInfo()->itemDirectorBase;
            if (director != nullptr) {
                u32 index = sRandom.getU32(8);
                const char* name =
                    isOnRaidon ? cRandomItemNamesHoming[index] : cRandomItemNames[index];
                director->appearItem(name, trans, front, sensor, !isSingleMode, true);
            }
        } else if (al::isNerve(this, &NrvPlayerAmiiboDirector.PostScanDisaster)) {
            DisasterModeController* controller = DisasterModeController::tryGetController(mPlayer);
            if (controller != nullptr) {
                if (controller->needToPlayFirstDisasterModeCutscene()) {
                    controller->begin(true);
                } else {
                    controller->beginImmediate(true);
                }
            }
        } else if (al::isNerve(this, &NrvPlayerAmiiboDirector.PostScanSuperStar)) {
            appearAmiiboItem(mPlayer, isOnRaidon ? "SuperStar[Homing]" : "スーパースター", trans,
                             front, sensor, !isSingleMode);
        } else if (al::isNerve(this, &NrvPlayerAmiiboDirector.PostScanSuperKinoko)) {
            appearAmiiboItem(mPlayer,
                             isOnRaidon ? "スーパーキノコ[アイテムストック]" : "スーパーキノコ",
                             trans, front, sensor, !isSingleMode);
        } else if (al::isNerve(this, &NrvPlayerAmiiboDirector.PostScanOneUpKinoko)) {
            appearAmiiboItem(mPlayer, "1UPキノコ", trans, front, sensor, !isSingleMode);
        } else if (al::isNerve(this, &NrvPlayerAmiiboDirector.PostScanKoopaJr)) {
            PlayerKoopaJr* koopaJr = PlayerKoopaJr::tryGetPlayerKoopaJr(mPlayer);
            if (koopaJr != nullptr) {
                koopaJr->tryStartAmiiboAttack();
            }
        }
    }

    if (al::isStep(this, 120) || mIsBusy) {
        mLayout->end(rc::isActiveDemo(mPlayer));
        al::setNerve(this, &NrvPlayerAmiiboDirector.WaitForInput);
    }
}

/**
 * Plays the failure feedback and waits for the button to be released.
 */
void PlayerAmiiboDirector::exeScanFailed() {
    if (al::isFirstStep(this)) {
        al::startSe(mLayout, "Fail");
        mWatcher->requestStop(false, mPlayer);
        mLayout->end(rc::isActiveDemo(mPlayer));
    }

    if (al::isGreaterEqualStep(this, 15) && !al::isPadHoldLeft(rc::getPlayerInputPort(mPlayer))) {
        al::setNerve(this, &NrvPlayerAmiiboDirector.WaitForInput);
    }
}

/**
 * Checks whether a Bowser amiibo may be used (not during a disaster or in a bonus area).
 * @return true if it is allowed
 */
bool PlayerAmiiboDirector::isKoopaAllowed() const {
    if (GameDataFunction::isSingleMode(mPlayer)) {
        DisasterModeController* controller = DisasterModeController::tryGetController(mPlayer);
        if (controller != nullptr && controller->isDisasterMode()) {
            return false;
        }

        if (isPlayerInKoopaRestrictedArea()) {
            return false;
        }
    }

    return true;
}

/**
 * Checks whether a Bowser Jr. amiibo may be used (only once Bowser Jr. joined).
 * @return true if it is allowed
 */
bool PlayerAmiiboDirector::isKoopaJrAllowed() const {
    return !GameDataFunction::isSingleMode(mPlayer) ||
           SingleModeDataFunction::hasSeenCutscene(mPlayer, 1);
}

/**
 * Checks whether the player is in an area where Fury Bowser cannot be summoned.
 * @return true inside a cloud bonus area
 */
bool PlayerAmiiboDirector::isPlayerInKoopaRestrictedArea() const {
    IslandMap* islandMap = al::tryGetSceneObj<IslandMap>(mPlayer, SceneObjID_IslandMap);
    return islandMap != nullptr && islandMap->isPlayerInBonusArea();
}

/**
 * Checks whether the director belongs to a player.
 * @param pActor player to check
 * @return true if it is the director's player
 */
bool PlayerAmiiboDirector::isCurrentPlayerActor(const PlayerActor* pActor) {
    return mPlayer == pActor;
}

/**
 * Creates the watcher and the storage for the directors and the used tags.
 * @param isDisable whether amiibo scanning is disabled
 */
PlayerAmiiboDirectorWatcher::PlayerAmiiboDirectorWatcher(bool isDisable)
    : mNfpDirector(nullptr), mScanRequestCount(0), mIsEnable(!isDisable) {
    mDirectors.allocBuffer(rc::getPlayerCharacterNumMax(), nullptr);
    mUsedTags.allocBuffer(cUsedTagNumMax, nullptr);
    mNfpDirector = GameSystemFunction::getGameSystem()->getGameSystemInfo()->getNfpDirector();
}

/**
 * Updates the NFP director.
 */
void PlayerAmiiboDirectorWatcher::update() {
    if (mNfpDirector != nullptr) {
        mNfpDirector->update();
    }
}
