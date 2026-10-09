#include "Boss/Tentack.hpp"

#include "Boss/BossStateDemoStart.hpp"
#include "Boss/TentackAttachItemHolder.hpp"
#include "Boss/TentackHead.hpp"
#include "Boss/TentackMagmaBall.hpp"
#include "Boss/TentackRock.hpp"
#include "Boss/TentackStateAttackTentacle.hpp"
#include "Boss/TentackStateFallRock.hpp"
#include "Boss/TentackTentacle.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/PlayerUtil.hpp"

// Non-const nerve object: the nerves are merged into one data block, so neighbouring nerves are
// addressed relative to each other.
#define TENTACK_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(Tentack, DemoStart)
NERVE_DECL(Tentack, FallRock)
NERVE_DECL(Tentack, AttackTentacle)
NERVE_DECL(Tentack, DemoEnd)
NERVE_DECL(Tentack, Damage)
FOR_EACH(TENTACK_NERVE_MAKE, Tentack, DemoStart, FallRock, AttackTentacle, DemoEnd, Damage)

/** @brief Attack parameters of every damage stage. */
struct TentackParamTable {
    /** @brief Sets up the parameters that differ from the defaults. */
    TentackParamTable() {
        mAttackTentacle[1].mDamageStage = 1;
        mAttackTentacle[1].mShotInterval = 3;
        mAttackTentacle[2].mDamageStage = 2;
        mAttackTentacle[2].mShotInterval = 2;
        mFallRockStart.mStartWait = 0;
        mFallRockStart.mIsPlayAction = false;
        mFallRock.mStartWait = 0;
    }

    TentackStateAttackTentacleParam mAttackTentacle[3];  // per damage stage
    TentackStateFallRockParam mFallRockStart;           // first rain after a hit
    TentackStateFallRockParam mFallRock;                // later rains
};

TentackParamTable sParamTable;

/**
 * @brief Checks the placement object name of the actor.
 * @param rInfo Actor initialization information.
 * @param pName Object name to compare with.
 * @return True if the object name matches.
 */
inline bool isObjectNameInline(const al::ActorInitInfo& rInfo, const char* pName) {
    const char* objectName = nullptr;
    return al::tryGetObjectName(&objectName, rInfo) &&
           al::isEqualString(pName, objectName != nullptr ? objectName : "");
}

/**
 * @brief Calculates the horizontal distance between two positions.
 * @param rA First position.
 * @param rB Second position.
 * @return Distance on the XZ plane.
 */
inline f32 calcDistanceXZ(const sead::Vector3f& rA, const sead::Vector3f& rB) {
    f32 dx = rA.x - rB.x;
    f32 dz = rA.z - rB.z;
    return sead::Mathf::sqrt(dx * dx + dz * dz);
}
}  // namespace

/**
 * @brief Creates the boss with its item holder.
 * @param pName Actor name.
 */
Tentack::Tentack(const char* pName)
    : al::LiveActor(pName), mAttachItemHolder(new TentackAttachItemHolder()) {}

/**
 * @brief Creates the head, the tentacles, the rocks or lava balls and the battle states.
 * @param rInfo Actor placement and scene initialization information.
 */
void Tentack::init(const al::ActorInitInfo& rInfo) {
    if (isObjectNameInline(rInfo, "TentackLv2")) {
        mLevel = cMagmaLevel;
    } else {
        mLevel = 1;
    }

    al::initActorWithArchiveName(this, rInfo, "Tentack", mLevel == cMagmaLevel ? "Lv2" : nullptr);
    al::initNerve(this, &NrvTentackDemoStart, 3);

    mHead = new TentackHead("テンタック本体", this, false);
    al::initCreateActorWithPlacementInfo(mHead, rInfo);

    mTentacles = new TentacleGroup("テンタック子蛇", 15);
    for (s32 i = 0; i < mTentacles->getMaxActorCount(); i++) {
        auto* tentacle = new TentackTentacle("テンタック子蛇", this);
        al::initCreateActorWithPlacementInfo(tentacle, rInfo);
        mTentacles->registerActor(tentacle);
    }

    if (mLevel == cMagmaLevel) {
        mMagmaBalls = new MagmaBallGroup("テンタック溶岩弾グループ", 12);
        for (s32 i = 0; i < mMagmaBalls->getMaxActorCount(); i++) {
            auto* magmaBall = new TentackMagmaBall("テンタック溶岩弾", this);
            al::initCreateActorWithPlacementInfo(magmaBall, rInfo);
            mMagmaBalls->registerActor(magmaBall);
        }
    } else {
        mRocks = new RockGroup("テンタック岩", 12);
        for (s32 i = 0; i < mRocks->getMaxActorCount(); i++) {
            auto* rock = new TentackRock("テンタック岩");
            al::initCreateActorWithPlacementInfo(rock, rInfo);
            mRocks->registerActor(rock);
        }
    }

    mPlayerList = new const al::LiveActor*[cPlayerListSize];
    for (s32 i = 0; i < cPlayerListSize; i++) {
        mPlayerList[i] = nullptr;
    }

    al::tryGetLinksTrans(&mStageCenterPos, rInfo, "StageCenterPos");
    mAttachItemHolder->init(rInfo);

    mDemoStartInfo = new BossDemoStartInfo(al::initAnimCamera(mHead, rInfo), mHead,
                                           "DemoBattleStart", 60, nullptr);
    mFallRockState = new TentackStateFallRock(mHead, &sParamTable.mFallRockStart);
    mAttackTentacleState =
        new TentackStateAttackTentacle(this, rInfo, &sParamTable.mAttackTentacle[0]);
    mDemoStartState = new BossStateDemoStart(this, rInfo, mDemoStartInfo);
    al::initNerveState(this, mFallRockState, &NrvTentackFallRock, "岩降らし");
    al::initNerveState(this, mAttackTentacleState, &NrvTentackAttackTentacle, "触手攻撃");
    al::initNerveState(this, mDemoStartState, &NrvTentackDemoStart, "開始デモ");

    if (mLevel == cMagmaLevel) {
        mFallRockState->mIsDisableFollow = true;
    }

    al::trySyncStageSwitchAppear(this);
}

/** @brief Makes the boss and its head appear without their appear actions. */
void Tentack::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    mHead->makeActorAppeared();
    al::setNerve(this, &NrvTentackDemoStart);
}

/** @brief Appears the boss with its head and starts the opening demo. */
void Tentack::appear() {
    al::LiveActor::appear();
    mHead->appear();
    al::setNerve(this, &NrvTentackDemoStart);
}

/** @brief Kills the boss and turns on its dead switch. */
void Tentack::kill() {
    al::LiveActor::kill();
    al::tryOnSwitchDeadOn(this);
}

/**
 * @brief Reacts to a hit on the head.
 * @param pHead Damaged head.
 * @param isLast Whether the hit was the last one.
 */
void Tentack::receiveDamage(const TentackHead* pHead, bool isLast) {
    s32 damage = pHead->mDamage;
    breakAllRocks();
    mAttackTentacleState->receiveDamage(damage, isLast);
    mFallRockState->mParam = &sParamTable.mFallRockStart;

    if (isLast) {
        al::setNerve(this, &NrvTentackDemoEnd);
        return;
    }

    mAttackTentacleState->setParam(damage == 1 ? &sParamTable.mAttackTentacle[1] :
                                                 &sParamTable.mAttackTentacle[2]);
    al::setNerve(this, &NrvTentackDamage);
}

/** @brief Kills every falling rock or lava ball. */
void Tentack::breakAllRocks() {
    if (mLevel == cMagmaLevel) {
        for (s32 i = 0; i < mMagmaBalls->getActorCount(); i++) {
            if (al::isAlive(mMagmaBalls->getDeriveActor(i))) {
                mMagmaBalls->getDeriveActor(i)->kill();
            }
        }
    } else {
        for (s32 i = 0; i < mRocks->getActorCount(); i++) {
            if (al::isAlive(mRocks->getDeriveActor(i))) {
                mRocks->getDeriveActor(i)->kill();
            }
        }
    }
}

/** @brief Gets the boss position. @return Boss translation. */
const sead::Vector3f& Tentack::getTentackTrans() const {
    return al::getTrans(this);
}

/** @brief Finds a rock or lava ball ready to fall. @return Dead rock, or null. */
TentackRockBase* Tentack::tryGetDeadRock() const {
    if (mLevel == cMagmaLevel) {
        for (s32 i = 0; i < mMagmaBalls->getActorCount(); i++) {
            if (mMagmaBalls->getDeriveActor(i)->isDeadRock()) {
                return mMagmaBalls->getDeriveActor(i);
            }
        }
    } else {
        for (s32 i = 0; i < mRocks->getActorCount(); i++) {
            if (mRocks->getDeriveActor(i)->isDeadRock()) {
                return mRocks->getDeriveActor(i);
            }
        }
    }

    return nullptr;
}

/** @brief Plays the opening demo and starts the battle music. */
void Tentack::exeDemoStart() {
    if (al::isStep(this, 225)) {
        al::startBgm(this, "Boss2", -1, 0, -1, -1);
    }

    s32 step = al::getNerveStep(this);
    if (al::updateNerveState(this)) {
        if (step < 225) {
            al::startBgm(this, "Boss2", -1, 0, -1, -1);
        }

        if (mDemoStartState->isSkipped()) {
            mHead->cancelDemoAppear();
        }

        al::setNerve(this, &NrvTentackFallRock);
    }
}

/** @brief Drops rocks around the players, then starts the tentacle attack. */
void Tentack::exeFallRock() {
    if (al::isFirstStep(this)) {
        mPlayerNum = rc::calcPlayerListOrderByDistance(this, mPlayerList, cPlayerListSize);
        mPlayerIndex = al::getRandom(mPlayerNum);
    }

    if (al::updateNerveState(this)) {
        mFallRockState->mParam = &sParamTable.mFallRock;
        al::setNerve(this, &NrvTentackAttackTentacle);
    }
}

/** @brief Attacks with the tentacles until the attack state ends. */
void Tentack::exeAttackTentacle() {
    al::updateNerveStateAndNextNerve(this, &NrvTentackFallRock);
}

/** @brief Waits after a hit before dropping rocks again. */
void Tentack::exeDamage() {
    if (!mHead->isDamageAction() && al::isGreaterEqualStep(this, 340)) {
        al::setNerve(this, &NrvTentackFallRock);
    }
}

/** @brief Ends the battle once the head is dead. */
void Tentack::exeDemoEnd() {
    if (al::isFirstStep(this)) {
        al::stopBgm(this, "Boss2", 20, -1);
    }

    if (al::isStep(this, 2)) {
        al::tryOnStageSwitch(this, "SwitchDemoEndOn");
    }

    if (al::isDead(mHead)) {
        al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
        kill();
    }
}

/**
 * @brief Gets a tentacle by index.
 * @param index Tentacle index.
 * @return Tentacle.
 */
TentackTentacle* Tentack::getTentacle(s32 index) const {
    return mTentacles->getDeriveActor(index);
}

/** @brief Gets the number of registered tentacles. @return Tentacle count. */
s32 Tentack::getTentacleNum() const {
    return mTentacles->getActorCount();
}

/** @brief Gets the capacity of the tentacle group. @return Maximum tentacle count. */
s32 Tentack::getTentacleNumMax() const {
    return mTentacles->getMaxActorCount();
}

/**
 * @brief Finds a fall position at or around the next living player.
 * @param pPosition Receives the position.
 * @return True if a position was found.
 */
bool Tentack::tryFindTransNearPlayer(sead::Vector3f* pPosition) {
    s32 playerIndex = mPlayerIndex;
    const al::LiveActor* firstPlayer = nullptr;
    for (s32 i = 0; i < mPlayerNum; i++) {
        playerIndex = al::wrapValue(playerIndex + 1, mPlayerNum);
        const al::LiveActor* player = mPlayerList[playerIndex];
        if (player == nullptr || rc::isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (firstPlayer == nullptr) {
            firstPlayer = player;
        }

        if (isEnablePlacementPos(al::getTrans(player))) {
            mPlayerIndex = playerIndex;
            pPosition->set(al::getTrans(player));
            return true;
        }
    }

    if (firstPlayer == nullptr) {
        return false;
    }

    sead::Vector3f playerTrans = al::getTrans(firstPlayer);
    sead::Vector3f dir = {0.0f, 0.0f, 1.0f};
    al::rotateVectorDegreeY(&dir, al::getRandomDegree());

    for (s32 i = 0; i < 16; i++) {
        al::rotateVectorDegreeY(&dir, 22.5f);
        sead::Vector3f pos = dir;
        pos *= 400.0f;
        pos = playerTrans + pos;
        if (isEnablePlacementPos(pos)) {
            pPosition->set(pos);
            return true;
        }
    }

    for (s32 i = 0; i < 16; i++) {
        al::rotateVectorDegreeY(&dir, 22.5f);
        sead::Vector3f pos = dir;
        pos *= 800.0f;
        pos = playerTrans + pos;
        if (isEnablePlacementPos(pos)) {
            pPosition->set(pos);
            return true;
        }
    }

    return false;
}

/**
 * @brief Checks whether a position is on the floor, away from the head and inside the stage.
 * @param rPos Position to check.
 * @return True if something can be placed there.
 */
bool Tentack::isEnablePlacementPos(const sead::Vector3f& rPos) const {
    sead::Vector3f start = rPos + sead::Vector3f(0.0f, 50.0f, 0.0f);
    if (!alCollisionUtil::getFirstPolyOnArrow(mHead, nullptr, nullptr, start,
                                              sead::Vector3f(0.0f, -100.0f, 0.0f), nullptr,
                                              nullptr)) {
        return false;
    }

    if (calcDistanceXZ(rPos, al::getTrans(mHead)) <
        TentackHead::getHeadRadius() + TentackTentacle::getTentacleRadius()) {
        return false;
    }

    return !(calcDistanceXZ(rPos, mStageCenterPos) > 1350.0f);
}

/** @brief Checks whether the tentacle attack is disabled. @return Always true. */
bool Tentack::isInvalidAttackTentacle() {
    return true;
}

/**
 * @brief Checks whether a position can hold a tentacle without touching the others or the head.
 * @param rPos Position to check.
 * @param pTentacle Tentacle to ignore.
 * @return True if the position is free.
 */
bool Tentack::isEmptyTransOtherTentacleOrHead(const sead::Vector3f& rPos,
                                              const TentackTentacle* pTentacle) const {
    if (!isEnablePlacementPos(rPos)) {
        return false;
    }

    for (s32 i = 0; i < mTentacles->getActorCount(); i++) {
        if (pTentacle != nullptr && mTentacles->getDeriveActor(i) == pTentacle) {
            continue;
        }

        if (!mTentacles->getDeriveActor(i)->isDecidedTrans()) {
            continue;
        }

        if (calcDistanceXZ(rPos, al::getTrans(mTentacles->getDeriveActor(i))) <
            TentackTentacle::getTentacleRadius() * 2.0f) {
            return false;
        }
    }

    return true;
}
