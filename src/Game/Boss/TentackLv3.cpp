#include "Boss/TentackLv3.hpp"

#include <random/seadGlobalRandom.h>

#include "Boss/BossStateDemoStart.hpp"
#include "Boss/TentackAttachItemHolder.hpp"
#include "Boss/TentackHead.hpp"
#include "Boss/TentackMagmaBall.hpp"
#include "Boss/TentackResourceParamHolder.hpp"
#include "Boss/TentackRockFaller.hpp"
#include "Boss/TentackStateAttackShot.hpp"
#include "Boss/TentackTentacle.hpp"
#include "Boss/TentackTentacleGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Util/PlayerUtil.hpp"

// Nerve that shares the execute function of another nerve.
#define TENTACK_LV3_NERVE_SHARED_DECL(Action, ExeFunc)                                             \
    class TentackLv3Nrv##Action : public al::Nerve {                                               \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<TentackLv3>())->exe##ExeFunc();                                    \
        }                                                                                          \
    };

// Non-const nerve object: the nerves are merged into one data block, so neighbouring nerves are
// addressed relative to each other.
#define TENTACK_LV3_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(TentackLv3, DemoStart)

/** @brief Stands in for TentackLv3 as the host of its second head and that head's tentacles. */
class TentackLv3HeadLv2Host : public TentackBase {
public:
    /**
     * @brief Creates the host proxy.
     * @param pHost Boss forwarded to.
     * @param pHead Head reported as this host's head.
     */
    TentackLv3HeadLv2Host(TentackLv3* pHost, TentackHead* pHead) : mHost(pHost), mHead(pHead) {}

    /**
     * @brief Forwards a head's damage to the boss.
     * @param pHead Damaged head.
     * @param isLast Whether the hit was the head's last one.
     */
    void receiveDamage(const TentackHead* pHead, bool isLast) override {
        mHost->receiveDamage(pHead, isLast);
    }

    /** @brief Gets the second head. @return Head Lv2. */
    TentackHead* getHead() const override { return mHead; }

    /** @brief Gets the boss's item holder. @return Item holder. */
    TentackAttachItemHolder* getAttachItemHolder() const override {
        return mHost->getAttachItemHolder();
    }

    /** @brief Gets the boss's level. @return Battle level. */
    s32 getLevel() const override { return mHost->getLevel(); }

    /** @brief Gets the boss's position. @return Boss translation. */
    const sead::Vector3f& getTentackTrans() const override { return mHost->getTentackTrans(); }

    /** @brief Finds a lava ball ready to fall. @return Dead rock, or null. */
    TentackRockBase* tryGetDeadRock() const override { return mHost->tryGetDeadRock(); }

    /**
     * @brief Finds a fall position near a player.
     * @param pPosition Receives the position.
     * @return True if a position was found.
     */
    bool tryFindTransNearPlayer(sead::Vector3f* pPosition) override {
        return mHost->tryFindTransNearPlayer(pPosition);
    }

private:
    TentackLv3* mHost;
    TentackHead* mHead;
};

NERVE_DECL(TentackLv3, BackGroup)
NERVE_DECL(TentackLv3, DemoEnd)
TENTACK_LV3_NERVE_SHARED_DECL(CryStartLv1, CryStart)
TENTACK_LV3_NERVE_SHARED_DECL(CryStartLv2, CryStart)
TENTACK_LV3_NERVE_SHARED_DECL(CryRequestLv1, CryRequest)
TENTACK_LV3_NERVE_SHARED_DECL(CryRequestLv2, CryRequest)
NERVE_DECL(TentackLv3, Damage)
NERVE_DECL(TentackLv3, FallMagma)
NERVE_DECL(TentackLv3, AttackTentacleStart)
NERVE_DECL(TentackLv3, AttackTentacle)
TENTACK_LV3_NERVE_SHARED_DECL(CryLv1, Cry)
TENTACK_LV3_NERVE_SHARED_DECL(CryLv2, Cry)
TENTACK_LV3_NERVE_SHARED_DECL(CryEndLv1, CryEnd)
TENTACK_LV3_NERVE_SHARED_DECL(CryEndLv2, CryEnd)
TENTACK_LV3_NERVE_SHARED_DECL(BackGroupLv1, BackGroup)
FOR_EACH(TENTACK_LV3_NERVE_MAKE, TentackLv3, DemoStart, BackGroup, DemoEnd, CryStartLv1,
         CryStartLv2, CryRequestLv1, CryRequestLv2, Damage, FallMagma, AttackTentacleStart,
         CryLv1, CryLv2, CryEndLv1, CryEndLv2)
NERVES_MAKE_NOSTRUCT(TentackLv3, AttackTentacle, BackGroupLv1)

/**
 * @brief Picks the nerve variant of the first or second head.
 * @param isLv1 Whether to pick the first head's variant.
 * @param rLv1 Nerve of the first head.
 * @param rLv2 Nerve of the second head.
 * @return Selected nerve.
 */
inline const al::Nerve* selectNerve(bool isLv1, const al::Nerve& rLv1, const al::Nerve& rLv2) {
    return isLv1 ? &rLv1 : &rLv2;
}

/** @brief Tentacle groups pulled back before a head sinks, per damage stage. */
const s32 sHeadSinkInterval[] = {4, 3, 2};

/**
 * @brief Creates a tentacle at one of the boss's tentacle link points.
 * @param pName Tentacle name.
 * @param rInfo Boss initialization information.
 * @param pHost Host the tentacle reports to.
 * @param pPointLinkName Link name of the appear points.
 * @param pHeadLinkName Link name of the tentacle's own links.
 * @param index Appear point index.
 * @return Created tentacle.
 */
TentackTentacle* createTentacle(const char* pName, const al::ActorInitInfo& rInfo,
                                TentackBase* pHost, const char* pPointLinkName,
                                const char* pHeadLinkName, s32 index) {
    auto* tentacle = new TentackTentacle(pName, pHost);
    al::initLinksActor(tentacle, rInfo, pHeadLinkName, 0);

    al::PlacementInfo placementInfo;
    al::getLinksInfoByIndex(&placementInfo, al::getPlacementInfo(rInfo), pPointLinkName, index);
    al::getTrans(al::getTransPtr(tentacle), placementInfo);

    bool isSpare = false;
    if (al::tryGetArg(&isSpare, placementInfo, "IsSpare") && isSpare) {
        tentacle->mIsSpare = true;
    }

    return tentacle;
}

/**
 * @brief Counts the tentacles of a group that are not spares.
 * @param pGroup Tentacle group.
 * @return Number of attacking tentacles.
 */
inline s32 calcActiveTentacleNum(const TentackLv3::TentacleGroup* pGroup) {
    s32 num = 0;
    for (s32 i = 0; i < pGroup->getActorCount(); i++) {
        num += !pGroup->getDeriveActor(i)->isSpare();
    }

    return num;
}

/**
 * @brief Finds a non-spare tentacle of a group by its index among the non-spare ones.
 * @param pGroup Tentacle group.
 * @param index Index among the attacking tentacles.
 * @return Tentacle, or null.
 */
inline TentackTentacle* findActiveTentacle(const TentackLv3::TentacleGroup* pGroup, s32 index) {
    s32 activeIndex = -1;
    for (s32 i = 0; i < pGroup->getActorCount(); i++) {
        TentackTentacle* tentacle = pGroup->getDeriveActor(i);
        activeIndex += !tentacle->isSpare();
        if (activeIndex == index) {
            return tentacle;
        }
    }

    return nullptr;
}

/**
 * @brief Classifies which head is dead.
 * @param pHeadLv1 First head.
 * @param pHeadLv2 Second head.
 * @return 2 if the first head is dead, 1 if the second one is, 0 otherwise.
 */
inline s32 calcDeadHeadType(const TentackHead* pHeadLv1, const TentackHead* pHeadLv2) {
    return al::isDead(pHeadLv1) ? 2 : al::isDead(pHeadLv2);
}

/**
 * @brief Calculates the horizontal distance between a position and a rock fall point.
 * @param x Position X.
 * @param z Position Z.
 * @param pPoint Fall point, as X and Z.
 * @return Distance.
 */
inline f32 calcDistanceXZ(f32 x, f32 z, const sead::Vector2f* pPoint) {
    return sead::Mathf::sqrt(sead::Mathf::square(x - pPoint->x) +
                             sead::Mathf::square(z - pPoint->y));
}
}  // namespace

/**
 * @brief Creates the boss with its item holder and resource parameters.
 * @param pName Actor name.
 */
TentackLv3::TentackLv3(const char* pName)
    : al::LiveActor(pName), mAttachItemHolder(new TentackAttachItemHolder()),
      mResParamHolder(new TentackResourceParamHolder("Lv3")) {}

/**
 * @brief Creates both heads, their tentacles, the lava balls and the battle states.
 * @param rInfo Actor placement and scene initialization information.
 */
void TentackLv3::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "Tentack", "Lv3");
    al::initNerve(this, &NrvTentackLv3DemoStart, 2);
    al::tryGetMatrixTR(&mBaseMtx, rInfo);

    mHeadLv1 = new TentackHead("テンタックLv3：本体Lv1", this, true);
    mHeadLv2 = new TentackHead("テンタックLv3：本体Lv2", this, true);
    al::initLinksActor(mHeadLv1, rInfo, "TentackHeadLv1", 0);
    al::initLinksActor(mHeadLv2, rInfo, "TentackHeadLv2", 0);
    mHeadLv1->mRockFaller->setValidFollowForce();
    mHeadLv2->mRockFaller->setValidFollowForce();
    mHeadLv2Host = new TentackLv3HeadLv2Host(this, mHeadLv2);

    s32 tentacleNumLv1 = al::calcLinkChildNum(rInfo, "TentacleLv1AppearPoint");
    mTentaclesLv1 = new TentacleGroup("テンタックLv3：子蛇Lv1グループ", tentacleNumLv1);
    for (s32 i = 0; i < tentacleNumLv1; i++) {
        mTentaclesLv1->registerActor(createTentacle("テンタックLv3：子蛇Lv1", rInfo, this,
                                                    "TentacleLv1AppearPoint", "TentackHeadLv1",
                                                    i));
    }

    s32 tentacleNumLv2 = al::calcLinkChildNum(rInfo, "TentacleLv2AppearPoint");
    mTentaclesLv2 = new TentacleGroup("テンタックLv3：子蛇Lv2グループ", tentacleNumLv2);
    for (s32 i = 0; i < tentacleNumLv2; i++) {
        mTentaclesLv2->registerActor(createTentacle("テンタックLv3：子蛇Lv2", rInfo, mHeadLv2Host,
                                                    "TentacleLv2AppearPoint", "TentackHeadLv2",
                                                    i));
    }

    mTentacleGroups.allocBuffer(mResParamHolder->getTentacleGroupNumMax(3), nullptr);
    for (s32 i = 0; i < mTentacleGroups.capacity(); i++) {
        s32 tentacleNumMax = tentacleNumLv1 > tentacleNumLv2 ? tentacleNumLv1 : tentacleNumLv2;
        mTentacleGroups.pushBack(new TentackTentacleGroup(tentacleNumMax));
    }

    mAttackGroups.allocBuffer(2, nullptr);

    mMagmaBalls = new al::DeriveActorGroup<TentackMagmaBall>("溶岩弾グループ", 24);
    for (s32 i = 0; i < mMagmaBalls->getMaxActorCount(); i++) {
        auto* magmaBall = new TentackMagmaBall("テンタックLv3：溶岩弾", this);
        al::initCreateActorWithPlacementInfo(magmaBall, rInfo);
        mMagmaBalls->registerActor(magmaBall);
    }

    al::tryGetLinksTrans(&mStageCenterPos, rInfo, "StageCenterPos");

    mDemoStartState = new BossStateDemoStart(
        this, rInfo,
        new BossDemoStartInfo(al::initAnimCamera(mHeadLv1, rInfo), mHeadLv1, "DemoBattleStartLv3",
                              130, &mBaseMtx));
    al::initNerveState(this, mDemoStartState, &NrvTentackLv3DemoStart, "開始デモ");

    mAttackShotState = new TentackStateAttackShot(mHeadLv2, rInfo, this);
    al::initNerveState(this, mAttackShotState, &NrvTentackLv3BackGroup, "Lv2本体火吹き");
    mAttackShotState->_40 = 5.0f;

    s32 rockFallPointNum = al::calcLinkChildNum(rInfo, "RockFallPoint");
    mRockFallPoints.allocBuffer(rockFallPointNum, nullptr);
    for (s32 i = 0; i < mRockFallPoints.capacity(); i++) {
        sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, al::getPlacementInfo(rInfo), "RockFallPoint", i);
        al::getTrans(&trans, placementInfo);
        mRockFallPoints.pushBack(new sead::Vector2f(trans.x, trans.z));
    }

    mRockFallPoints.shuffle(sead::GlobalRandom::instance());
    mAttachItemHolder->init(rInfo);
    mAttackStartFlags = 0;
    al::trySyncStageSwitchAppear(this);
}

/** @brief Makes the boss appear without its appear actions and restarts the opening demo. */
void TentackLv3::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::setNerve(this, &NrvTentackLv3DemoStart);
}

/** @brief Appears the boss with its first head and starts the opening demo. */
void TentackLv3::appear() {
    al::LiveActor::appear();
    mHeadLv1->appear();
    al::setNerve(this, &NrvTentackLv3DemoStart);
}

/** @brief Kills the boss and turns on its dead switch. */
void TentackLv3::kill() {
    al::LiveActor::kill();
    al::tryOnSwitchDeadOn(this);
}

/** @brief Updates every tentacle group. */
void TentackLv3::control() {
    for (s32 i = 0; i < mTentacleGroups.size(); i++) {
        mTentacleGroups[i]->update();
    }
}

/**
 * @brief Reacts to a hit on one of the heads.
 * @param pHead Damaged head.
 * @param isLast Whether the hit was the head's last one.
 */
void TentackLv3::receiveDamage(const TentackHead* pHead, bool isLast) {
    mHeadSinkCursor = 0;
    for (s32 i = 0; i < mMagmaBalls->getActorCount(); i++) {
        if (al::isAlive(mMagmaBalls->getDeriveActor(i))) {
            mMagmaBalls->getDeriveActor(i)->kill();
        }
    }

    if (pHead == mHeadLv1) {
        for (s32 i = 0; i < mTentaclesLv1->getActorCount(); i++) {
            mTentaclesLv1->getDeriveActor(i)->receiveDamage(isLast, pHead->mDamage);
        }

        mAttackStartFlags &= ~AttackStartFlag_HeadLv1;
    } else if (pHead == mHeadLv2) {
        for (s32 i = 0; i < mTentaclesLv2->getActorCount(); i++) {
            mTentaclesLv2->getDeriveActor(i)->receiveDamage(isLast, pHead->mDamage);
        }

        mAttackStartFlags &= ~AttackStartFlag_HeadLv2;
    }

    mAttackShotState->receiveDamage();

    TentackHead* otherHead = pHead == mHeadLv1 ? mHeadLv2 : mHeadLv1;
    if (isLast && (al::isDead(otherHead) || otherHead->isDemoEnd())) {
        al::setNerve(this, &NrvTentackLv3DemoEnd);
        return;
    }

    if (al::isAlive(otherHead) && !otherHead->isDemoEnd()) {
        if (otherHead->isWaitAll()) {
            endSwingForceAllTentacle();
            al::setNerve(this, selectNerve(otherHead == mHeadLv1, NrvTentackLv3CryStartLv1,
                                           NrvTentackLv3CryStartLv2));
            return;
        }

        if (!otherHead->isDamageAction() && !otherHead->isBackOrPush()) {
            al::setNerve(this, selectNerve(otherHead == mHeadLv1, NrvTentackLv3CryRequestLv1,
                                           NrvTentackLv3CryRequestLv2));
            return;
        }
    }

    al::setNerve(this, &NrvTentackLv3Damage);
}

/** @brief Stops the swing of every appeared tentacle that is not being damaged. */
void TentackLv3::endSwingForceAllTentacle() {
    for (s32 i = 0; i < getTentacleNumMax(); i++) {
        if (getTentacle(i)->isAppear() && !getTentacle(i)->isDamage()) {
            getTentacle(i)->eatAttachItemIfAttached(false);
            getTentacle(i)->endSwingForce();
        }
    }
}

/** @brief Gets the boss position. @return Boss translation. */
const sead::Vector3f& TentackLv3::getTentackTrans() const {
    return al::getTrans(this);
}

/** @brief Finds a lava ball ready to fall. @return Dead lava ball, or null. */
TentackRockBase* TentackLv3::tryGetDeadRock() const {
    for (s32 i = 0; i < mMagmaBalls->getActorCount(); i++) {
        if (mMagmaBalls->getDeriveActor(i)->isDeadRock()) {
            return mMagmaBalls->getDeriveActor(i);
        }
    }

    return nullptr;
}

/**
 * @brief Picks the next living player and the rock fall point closest to where it is heading.
 * @param pPosition Receives the fall position.
 * @return Always true.
 */
bool TentackLv3::tryFindTransNearPlayer(sead::Vector3f* pPosition) {
    mPlayerIndex = al::wrapValue(mPlayerIndex, al::getPlayerNumMax(this));

    al::LiveActor* player = nullptr;
    for (s32 i = 0; i < al::getPlayerNumMax(this); i++) {
        mPlayerIndex = al::wrapValue(mPlayerIndex + 1, al::getPlayerNumMax(this));
        player = al::getPlayerActor(this, mPlayerIndex);
        if (player != nullptr && !rc::isPlayerDeadOrBubble(player)) {
            break;
        }
    }

    f32 targetX;
    f32 targetZ;
    s32 nearestIndex = -1;
    if (player != nullptr) {
        const sead::Vector3f& playerTrans = al::getTrans(player);
        const sead::Vector3f& playerVelocity = rc::getPlayerVelocity(player);
        sead::Vector3f target = playerVelocity * 10.0f + playerTrans;
        targetX = target.x;
        targetZ = target.z;

        f32 nearestDistance = sead::Mathf::maxNumber();
        for (s32 i = 0; i < mRockFallPoints.size(); i++) {
            if (calcDistanceXZ(targetX, targetZ, mRockFallPoints(i)) < nearestDistance) {
                nearestDistance = calcDistanceXZ(targetX, targetZ, mRockFallPoints(i));
                nearestIndex = i;
            }
        }
    } else {
        const sead::Vector2f* point = mRockFallPoints.popFront();
        targetX = point->x;
        targetZ = point->y;
        pPosition->x = targetX;
        pPosition->z = targetZ;
        pPosition->y = al::getTrans(this).y;
        mRockFallPoints.pushBack(point);
        return true;
    }

    pPosition->x = targetX;
    pPosition->z = targetZ;
    pPosition->y = al::getTrans(this).y;
    if (static_cast<u32>(mRockFallPoints.size()) > static_cast<u32>(nearestIndex)) {
        mRockFallPoints.swap(mRockFallPoints.size() - 1, nearestIndex);
    }

    return true;
}

/**
 * @brief Gives a rock fall point back to the pool.
 * @param pPoint Fall point.
 */
void TentackLv3::returnRockAppearPointPtr(const sead::Vector2f* pPoint) {
    mRockFallPoints.pushBack(pPoint);
}

/**
 * @brief Finds the index of a second-head tentacle.
 * @param pTentacle Tentacle.
 * @return Index of the tentacle, or the tentacle count if not found.
 */
s32 TentackLv3::calcSwingTentacleId(const TentackTentacle* pTentacle) const {
    s32 i = 0;
    for (; i < mTentaclesLv2->getActorCount(); i++) {
        if (mTentaclesLv2->getDeriveActor(i) == pTentacle) {
            break;
        }
    }

    return i;
}

/** @brief Plays the opening demo, starting the battle music and the second head. */
void TentackLv3::exeDemoStart() {
    if (al::isStep(this, 320)) {
        al::startBgm(this, "Boss2", -1, 0, -1, -1);
    }

    if (al::isStep(this, 120)) {
        mHeadLv2->appear();
    }

    s32 step = al::getNerveStep(this);
    if (al::updateNerveStateAndNextNerve(this, &NrvTentackLv3FallMagma)) {
        if (step < 320) {
            al::startBgm(this, "Boss2", -1, 0, -1, -1);
        }

        if (mDemoStartState->isSkipped()) {
            if (al::isDead(mHeadLv2)) {
                mHeadLv2->appear();
            }

            mHeadLv1->cancelDemoAppear();
            mHeadLv2->cancelDemoAppear();
        }
    }
}

/** @brief Drops lava balls while both heads look to the front, then starts a tentacle attack. */
void TentackLv3::exeFallMagma() {
    if (al::isFirstStep(this)) {
        if (!mHeadLv1->tryStartActionAttackRockIfWait()) {
            mHeadLv2->tryStartActionAttackRockIfWait();
        }
    }

    if (al::isGreaterEqualStep(this, 240)) {
        if (al::isStep(this, 240)) {
            if (mHeadLv1->isWaitAll()) {
                mHeadLv1->setWaitFixed();
                mHeadLv1->changeLookTarget();
            }

            if (mHeadLv2->isWaitAll()) {
                mHeadLv2->setWaitFixed();
                mHeadLv2->changeLookTarget();
            }
        }

        sead::Vector3f frontDir = {0.0f, 0.0f, 0.0f};
        al::calcFrontDir(&frontDir, this);
        if (mHeadLv1->isWaitAll()) {
            mHeadLv1->turnToDirectionGently(frontDir, 1.5f);
        }

        if (mHeadLv2->isWaitAll()) {
            mHeadLv2->turnToDirectionGently(frontDir, 1.5f);
        }
    } else {
        if (mHeadLv1->isWaitFixed()) {
            mHeadLv1->setWait();
        }

        if (mHeadLv2->isWaitFixed()) {
            mHeadLv2->setWait();
        }
    }

    if (al::getNerveStep(this) >= 1 && al::isIntervalStep(this, 30, 0)) {
        startFallMagmaBall((al::getNerveStep(this) / 30) % 2 == 0);
    }

    if (al::isGreaterEqualStep(this, 300)) {
        mAttackStartFlags = 0;
        mAttackGroups.clear();
        mGroupIndex = -1;
        al::setNerve(this, &NrvTentackLv3AttackTentacleStart);
    }
}

/**
 * @brief Drops a lava ball at the next rock fall point.
 * @param isNearPlayer Whether to aim at a point near a player.
 */
void TentackLv3::startFallMagmaBall(bool isNearPlayer) {
    auto* magmaBall = static_cast<TentackMagmaBall*>(tryGetDeadRock());
    if (magmaBall == nullptr) {
        return;
    }

    sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
    if (!isNearPlayer || tryFindTransNearPlayer(&trans)) {
        mRockFallPoints.pushBack(mRockFallPoints.popFront());
    }

    magmaBall->startFall(mRockFallPoints.back(), al::getTrans(this).y + 2500.0f);
    mRockFallPoints.popBack();
}

/** @brief Pulls back the oldest attacking tentacle group with its head. */
void TentackLv3::exeBackGroup() {
    if (al::isFirstStep(this)) {
        if (mAttackGroups.size() == 0) {
            al::setNerve(this, &NrvTentackLv3AttackTentacleStart);
            return;
        }

        if (al::isNerve(this, &NrvTentackLv3BackGroup)) {
            mAttackShotState->registerTentacleGroup(mAttackGroups.front());
        }
    }

    if (al::isNerve(this, &NrvTentackLv3BackGroup)) {
        if (al::updateNerveState(this)) {
            al::setNerve(this, &NrvTentackLv3AttackTentacleStart);
            mAttackGroups.popFront();
            if (updateHeadSinkCursor()) {
                mHeadLv2->setDisappear();
            }
        }

        return;
    }

    TentackTentacleGroup* group = mAttackGroups.front();
    TentackHead* head = group->getHead();
    if (head->isWaitAll() && head->turnToTargetGently(group->mTargetPos, 1.5f) &&
        group->requestEndSwingAll()) {
        group->eatAttachItemAll();
        mAttackGroups.popFront();
        if (updateHeadSinkCursor()) {
            head->tryStartActionEatAndDisappear();
        } else {
            head->tryStartActionEat();
        }

        al::setNerve(this, &NrvTentackLv3AttackTentacleStart);
    }
}

/**
 * @brief Advances the counter of attacks until a head sinks.
 * @return True if the head should sink now.
 */
bool TentackLv3::updateHeadSinkCursor() {
    s32 damage = getDamage();
    s32 index = al::isDead(mHeadLv1) || al::isDead(mHeadLv2) || damage > 2 ?
                    2 :
                    sead::Mathi::max(damage, 0);
    mHeadSinkCursor = al::wrapValue(mHeadSinkCursor + 1, sHeadSinkInterval[index]);
    return mHeadSinkCursor == 0;
}

/** @brief Prepares the next tentacle group and lets its head start the attack action. */
void TentackLv3::exeAttackTentacleStart() {
    if (al::isFirstStep(this)) {
        if (mGroupIndex <= 0) {
            makeTentacleGroupAndTurnNext(mGroupIndex == -1);
            if (mGroupIndex == -1) {
                mGroupIndex = 0;
            }
        }

        if (getCurrentTentacleGroup()->getHead() == mHeadLv1) {
            if (!(mAttackStartFlags & AttackStartFlag_HeadLv1)) {
                if (mHeadLv1->isWaitAll()) {
                    mHeadLv1->startActionAttackTentacle();
                }

                mAttackStartFlags |= AttackStartFlag_HeadLv1;
            }
        } else if (!(mAttackStartFlags & AttackStartFlag_HeadLv2)) {
            if (mHeadLv2->isWaitAll()) {
                mHeadLv2->startActionAttackTentacle();
            }

            mAttackStartFlags |= AttackStartFlag_HeadLv2;
        }
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvTentackLv3AttackTentacle);
    }
}

/**
 * @brief Sets up the tentacles for the next attack pattern and optionally rebuilds the groups.
 * @param isReset Whether to rebuild the tentacle groups.
 */
void TentackLv3::makeTentacleGroupAndTurnNext(bool isReset) {
    TentackResourceParamHolder* paramHolder = mResParamHolder;
    TentackResourceParam* param = paramHolder->getParamAndTurnNext(
        getDamage(), getLevel(), calcDeadHeadType(mHeadLv1, mHeadLv2));
    for (s32 i = 0; i < getTentacleNumMax(); i++) {
        TentackTentacle* tentacle = getTentacle(i);
        TentackTentacleInfo* info = tentacle->getInfo();
        info->reset();
        tentacle->setSwingYRate(*param->mSwingYRate[i]);
        param->setUpTentacleInfo(info, i);
    }

    if (!isReset) {
        return;
    }

    s32 tentacleIndex = 0;
    for (s32 i = 0; i < getResParamInfo()->mGroupNum; i++) {
        mTentacleGroups[i]->reset();
        for (s32 j = 0; j < getResParamInfo()->mGroupTentacleNum[i]; j++) {
            mTentacleGroups[i]->registerTentacle(getTentacle(tentacleIndex++));
        }
    }
}

/** @brief Gets the tentacle group that attacks next. @return Tentacle group. */
TentackTentacleGroup* TentackLv3::getCurrentTentacleGroup() const {
    return mTentacleGroups[getResParamInfo()->getGroupOrder(mGroupIndex)];
}

/** @brief Attacks with the current tentacle group until the attack queue is full. */
void TentackLv3::exeAttackTentacle() {
    if (al::isFirstStep(this)) {
        TentackTentacleGroup* group = getCurrentTentacleGroup();
        group->appearAll();
        mAttackGroups.pushBack(group);
        s32 groupIndex = mGroupIndex;
        mGroupIndex = al::wrapValue(groupIndex + 1, getResParamInfo()->mGroupNum);
    }

    TentackTentacleGroup* group = mAttackGroups.front();
    bool isTurning = false;
    if (mAttackGroups.size() >= mAttackGroups.capacity()) {
        TentackHead* head = group->getHead();
        if (al::isFirstStep(this)) {
            head->changeLookTarget();
        }

        if (head->isWaitAll()) {
            isTurning = !head->turnToTargetGently(mAttackGroups.front()->mTargetPos, 1.5f);
        }
    }

    if (!al::isGreaterEqualStep(this, 100)) {
        return;
    }

    if (!group->getHead()->isWaitAll() || isTurning) {
        return;
    }

    if (!tryStartBackGroup()) {
        al::setNerve(this, &NrvTentackLv3AttackTentacleStart);
    }
}

/** @brief Gets the tentacle grouping of the current damage stage. @return Parameter info. */
const TentackResourceParamInfo* TentackLv3::getResParamInfo() const {
    return mResParamHolder->getParamInfo(getDamage(), getLevel(),
                                         calcDeadHeadType(mHeadLv1, mHeadLv2));
}

/**
 * @brief Pulls back the oldest attacking group once the attack queue is full.
 * @return True if the back nerve was started.
 */
bool TentackLv3::tryStartBackGroup() {
    if (mAttackGroups.size() < mAttackGroups.capacity()) {
        return false;
    }

    if (mAttackGroups.front()->getHead() == mHeadLv1) {
        al::setNerve(this, &NrvTentackLv3BackGroupLv1);
    } else {
        al::setNerve(this, &NrvTentackLv3BackGroup);
    }

    return true;
}

/** @brief Waits for the crying head to be free before it turns towards the other head. */
void TentackLv3::exeCryRequest() {
    if (al::isFirstStep(this)) {
        endSwingForceAllTentacle();
    }

    TentackHead* head = al::isNerve(this, &NrvTentackLv3CryRequestLv1) ? mHeadLv1 : mHeadLv2;
    if (head->isWaitAll()) {
        head->setWaitFixed();
        head->changeLookTarget();
        bool isLv1 = al::isNerve(this, &NrvTentackLv3CryRequestLv1);
        al::setNerve(this, selectNerve(isLv1, NrvTentackLv3CryStartLv1, NrvTentackLv3CryStartLv2));
    }
}

/** @brief Turns the crying head towards the damaged head. */
void TentackLv3::exeCryStart() {
    TentackHead* head = al::isNerve(this, &NrvTentackLv3CryStartLv1) ? mHeadLv1 : mHeadLv2;
    TentackHead* otherHead = head == mHeadLv1 ? mHeadLv2 : mHeadLv1;

    sead::Vector3f target = al::getTrans(otherHead);
    target.setScaleAdd(1000.0f, otherHead->mFrontDir, target);
    if (!head->isWaitFixed()) {
        head->setWaitFixed();
    }

    if (head->turnToTargetGently(target, 5.0f) || al::isGreaterEqualStep(this, 90)) {
        bool isLv1 = al::isNerve(this, &NrvTentackLv3CryStartLv1);
        al::setNerve(this, selectNerve(isLv1, NrvTentackLv3CryLv1, NrvTentackLv3CryLv2));
    }
}

/** @brief Plays the cry action of the head. */
void TentackLv3::exeCry() {
    TentackHead* head = al::isNerve(this, &NrvTentackLv3CryLv1) ? mHeadLv1 : mHeadLv2;
    if (al::isFirstStep(this)) {
        head->tryStartActionCry();
    }

    if (head->isWaitAll()) {
        bool isLv1 = al::isNerve(this, &NrvTentackLv3CryLv1);
        al::setNerve(this, selectNerve(isLv1, NrvTentackLv3CryEndLv1, NrvTentackLv3CryEndLv2));
    }
}

/** @brief Turns the head back to its initial direction after crying. */
void TentackLv3::exeCryEnd() {
    TentackHead* head = al::isNerve(this, &NrvTentackLv3CryEndLv1) ? mHeadLv1 : mHeadLv2;
    if (head->turnToDirectionGently(head->mFrontDir, 5.0f)) {
        al::setNerve(this, &NrvTentackLv3FallMagma);
    }
}

/** @brief Waits after a hit before dropping lava balls again. */
void TentackLv3::exeDamage() {
    if (al::isStep(this, 60)) {
        endSwingForceAllTentacle();
    }

    s32 waitStep = al::isDead(mHeadLv1) || al::isDead(mHeadLv2) ? 360 : 180;
    if (al::isGreaterEqualStep(this, waitStep)) {
        al::setNerve(this, &NrvTentackLv3FallMagma);
    }
}

/** @brief Ends the battle once both heads are dead. */
void TentackLv3::exeDemoEnd() {
    if (al::isFirstStep(this)) {
        al::stopBgm(this, "Boss2", 20, -1);
    }

    if (al::isStep(this, 2)) {
        al::tryOnStageSwitch(this, "SwitchDeadEndOn");
    }

    if (al::isDead(mHeadLv1) && al::isDead(mHeadLv2)) {
        al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
        kill();
    }
}

/**
 * @brief Gets an attacking tentacle by index over both heads.
 * @param index Tentacle index.
 * @return Tentacle, or null.
 */
TentackTentacle* TentackLv3::getTentacle(s32 index) const {
    if (al::isDead(mHeadLv2)) {
        return mTentaclesLv1->getDeriveActor(index);
    }

    if (al::isDead(mHeadLv1)) {
        return mTentaclesLv2->getDeriveActor(index);
    }

    s32 activeNumLv1 = calcActiveTentacleNum(mTentaclesLv1);
    if (index < activeNumLv1) {
        return findActiveTentacle(mTentaclesLv1, index);
    }

    return findActiveTentacle(mTentaclesLv2, index - activeNumLv1);
}

/** @brief Gets the number of attacking tentacles over both heads. @return Tentacle count. */
s32 TentackLv3::getTentacleNumMax() const {
    if (al::isDead(mHeadLv2)) {
        return mTentaclesLv1->getActorCount();
    }

    if (al::isDead(mHeadLv1)) {
        return mTentaclesLv2->getActorCount();
    }

    return calcActiveTentacleNum(mTentaclesLv1) + calcActiveTentacleNum(mTentaclesLv2);
}

/** @brief Gets the damage stage of the battle. @return Highest damage of the living heads. */
s32 TentackLv3::getDamage() const {
    if (al::isDead(mHeadLv1)) {
        return mHeadLv2->mDamage;
    }

    if (al::isDead(mHeadLv2)) {
        return mHeadLv1->mDamage;
    }

    s32 damageLv1 = mHeadLv1->mDamage;
    s32 damageLv2 = mHeadLv2->mDamage;
    return al::clamp(damageLv1 < damageLv2 ? damageLv2 : damageLv1, 0, 3);
}
