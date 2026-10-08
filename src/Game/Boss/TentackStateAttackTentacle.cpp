#include "Boss/TentackStateAttackTentacle.hpp"

#include "Boss/Tentack.hpp"
#include "Boss/TentackHead.hpp"
#include "Boss/TentackResourceParamHolder.hpp"
#include "Boss/TentackStateAttackShot.hpp"
#include "Boss/TentackTentacle.hpp"
#include "Boss/TentackTentacleGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

// Nerve that runs the execute function of another nerve.
#define TENTACK_ATTACK_TENTACLE_NERVE_SHARED_DECL(Action, ExeFunc)                                 \
    class TentackStateAttackTentacleNrv##Action : public al::Nerve {                               \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<TentackStateAttackTentacle>())->exe##ExeFunc();                    \
        }                                                                                          \
    };

// Non-const nerve object: these nerves are merged into one data block.
#define TENTACK_ATTACK_TENTACLE_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(TentackStateAttackTentacle, AttackEndWait)
NERVE_DECL(TentackStateAttackTentacle, Start)
NERVE_DECL(TentackStateAttackTentacle, Shot)
TENTACK_ATTACK_TENTACLE_NERVE_SHARED_DECL(ShotAndHide, Shot)
NERVE_DECL(TentackStateAttackTentacle, AttackEndShot)
NERVE_DECL(TentackStateAttackTentacle, GroupAppear)
NERVE_DECL(TentackStateAttackTentacle, OldGroupBack)
TENTACK_ATTACK_TENTACLE_NERVE_SHARED_DECL(EatAndDisappear, Eat)
NERVE_DECL(TentackStateAttackTentacle, Eat)
TENTACK_ATTACK_TENTACLE_NERVE_SHARED_DECL(GroupAppearAfterEat, GroupAppear)
NERVE_DECL(TentackStateAttackTentacle, AttackEndInit)
NERVE_DECL(TentackStateAttackTentacle, GroupWait)
NERVE_DECL(TentackStateAttackTentacle, AttackEnd)
FOR_EACH(TENTACK_ATTACK_TENTACLE_NERVE_MAKE, TentackStateAttackTentacle, AttackEndWait, Start,
         Shot, ShotAndHide, AttackEndShot, GroupAppear, OldGroupBack, EatAndDisappear, Eat,
         GroupAppearAfterEat, AttackEndInit, GroupWait, AttackEnd)

constexpr TentackStateAttackTentacleParam sDefaultParam(0, 90, 900.0f, 0);

/** @brief Number of tentacles a tentacle group can hold. */
constexpr s32 cGroupTentacleNumMax = 5;
}  // namespace

/** @brief Initializes the default tentacle attack parameters. */
TentackStateAttackTentacleParam::TentackStateAttackTentacleParam()
    : mDamageStage(0), mEndWaitStep(90), mPlacementRadius(900.0f), mShotInterval(0) {}

/**
 * @brief Creates the tentacle attack state, its tentacle groups and, in the shooting battle, the
 * fire shot state.
 * @param pTentack Boss that owns the state.
 * @param rInfo Actor initialization information.
 * @param pParam Attack parameters, or null to use the defaults.
 */
TentackStateAttackTentacle::TentackStateAttackTentacle(Tentack* pTentack,
                                                       const al::ActorInitInfo& rInfo,
                                                       const TentackStateAttackTentacleParam* pParam)
    : al::HostStateBase<Tentack>("テンタックの触手攻撃ステート", pTentack), mParam(pParam),
      mResParamHolder(new TentackResourceParamHolder(nullptr)) {
    if (pTentack->mLevel == cShotLevel) {
        initNerve(&NrvTentackStateAttackTentacleStart, 3);
        mShotState = new TentackStateAttackShot(pTentack->getHead(), rInfo, this);
        al::initNerveState(this, mShotState, &NrvTentackStateAttackTentacleShot, "攻撃弾発射");
        al::initNerveState(this, mShotState, &NrvTentackStateAttackTentacleShotAndHide,
                           "攻撃弾発射[+引っ込み]");
        al::initNerveState(this, mShotState, &NrvTentackStateAttackTentacleAttackEndShot,
                           "攻撃終了時攻撃弾発射");
    } else {
        initNerve(&NrvTentackStateAttackTentacleStart, 0);
    }

    if (mParam == nullptr) {
        mParam = &sDefaultParam;
    }

    mSwingTentacles.allocBuffer(mResParamHolder->getSwingTentacleNumMax(getHost()->getLevel()),
                                nullptr);
    mAttackTentacles.allocBuffer(4, nullptr);
    mGroupNum = mResParamHolder->getTentacleGroupNumMax(getHost()->getLevel());
    mGroups = new TentackTentacleGroup*[mGroupNum];
    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i] = new TentackTentacleGroup(cGroupTentacleNumMax);
    }

    mAttackGroups.allocBuffer(2, nullptr);
}

/** @brief Checks whether this is the battle where the head also shoots. @return True if so. */
inline bool TentackStateAttackTentacle::isShotLevel() const {
    return getHost()->mLevel == cShotLevel;
}

/**
 * @brief Checks whether the oldest attacking group has to retreat before the next one appears.
 * @return True if the attack queue is full or the last round is running.
 */
inline bool TentackStateAttackTentacle::isOldGroupBack() const {
    return mAttackGroups.size() >= mAttackGroups.capacity() ||
           (mAttackGroups.size() != 0 && isLastPeriod());
}

/** @brief Checks whether the head shoots this time. @return True if the interval is complete. */
inline bool TentackStateAttackTentacle::isShotTurn() const {
    return mParam->mShotInterval >= 1 && mShotCount == mParam->mShotInterval;
}

/** @brief Advances the position in the shot interval. */
inline void TentackStateAttackTentacle::updateShotCount() {
    s32 interval = mParam->mShotInterval;
    if (interval > 0) {
        mShotCount = al::modi(mShotCount + interval, interval) + 1;
    }
}

/** @brief Starts the attack, sorting the tentacles into swinging ones and attack-only ones. */
void TentackStateAttackTentacle::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvTentackStateAttackTentacleStart);
    mAttackGroups.clear();
    mGroupIndex = 0;
    mPeriod = 0;
    mShotCount = mParam->mShotInterval > 0 ? 1 : 0;

    s32 swingTentacleNum = getCurrentParamInfo()->mSwingTentacleNum;
    mSwingTentacles.clear();
    mAttackTentacles.clear();
    for (s32 i = 0; i < swingTentacleNum + mAttackTentacles.capacity(); i++) {
        TentackTentacle* tentacle = getHost()->getTentacle(i);
        tentacle->getInfo()->reset();
        if (i < swingTentacleNum) {
            mSwingTentacles.pushBack(tentacle);
        } else {
            mAttackTentacles.pushBack(tentacle);
            tentacle->getInfo()->mIsAttack = true;
        }
    }

    s32 tentacleIndex = 0;
    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->reset();
        for (s32 j = 0; j < getCurrentParamInfo()->mGroupTentacleNum[i]; j++) {
            mGroups[i]->registerTentacle(mSwingTentacles.at(tentacleIndex++));
        }
    }
}

/** @brief Gets the tentacle grouping of the current damage stage. @return Parameter info. */
const TentackResourceParamInfo* TentackStateAttackTentacle::getCurrentParamInfo() const {
    return mResParamHolder->getParamInfo(mParam->mDamageStage, getHost()->getLevel(), 0);
}

/** @brief Ends the state and forgets the targets of the fire shot. */
void TentackStateAttackTentacle::kill() {
    al::NerveStateBase::kill();
    if (mShotState != nullptr) {
        mShotState->clearTarget();
    }
}

/** @brief Updates every tentacle group. */
void TentackStateAttackTentacle::control() {
    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->update();
    }
}

/**
 * @brief Lets the swinging tentacles and the fire shot react to a hit on the head.
 * @param damage Number of hits taken.
 * @param isLast Whether the hit defeats Tentack.
 */
void TentackStateAttackTentacle::receiveDamage(s32 damage, bool isLast) {
    for (s32 i = 0; i < mSwingTentacles.size(); i++) {
        mSwingTentacles.at(i)->receiveDamage(isLast, damage);
    }

    if (mShotState != nullptr) {
        mShotState->receiveDamage();
    }
}

/** @brief Lines up the tentacles in front of the head and prepares the first attack pattern. */
void TentackStateAttackTentacle::exeStart() {
    if (al::isFirstStep(this)) {
        getHost()->getHead()->startActionAttackTentacle();
        placementTentacleHalfCircle();
    }

    if (al::isGreaterEqualStep(this, 60)) {
        prepareTentacleGroups();
        al::setNerve(this, &NrvTentackStateAttackTentacleGroupAppear);
    }
}

/** @brief Places the swinging tentacles on a half circle in front of Tentack. */
void TentackStateAttackTentacle::placementTentacleHalfCircle() {
    s32 tentacleNum = mSwingTentacles.size();
    f32 stepDegree = tentacleNum > 1 ? 210.0f / (tentacleNum - 1) : 105.0f;

    sead::Vector3f center = al::getTrans(getHost()) + sead::Vector3f(0.0f, 0.0f, 220.0f);
    sead::Vector3f dir = sead::Vector3f::ez;
    al::rotateVectorDegreeY(&dir, -105.0f);
    for (s32 i = 0; i < mSwingTentacles.size(); i++) {
        sead::Vector3f trans = center;
        trans += dir * mParam->mPlacementRadius;
        al::setTrans(mSwingTentacles.at(i), trans);
        al::rotateVectorDegreeY(&dir, stepDegree);
    }
}

/** @brief Sets up the swinging tentacles for the next attack pattern. */
void TentackStateAttackTentacle::prepareTentacleGroups() {
    TentackResourceParam* param = mResParamHolder->getParamAndTurnNext(
        mParam->mDamageStage, getHost()->getLevel(), 0);
    for (s32 i = 0; i < mSwingTentacles.size(); i++) {
        TentackTentacleInfo* info = mSwingTentacles.at(i)->getInfo();
        info->reset();
        mSwingTentacles.at(i)->setSwingYRate(*param->mSwingYRate[i]);
        param->setUpTentacleInfo(info, i);
    }
}

/** @brief Waits while a group attacks, turning the head towards the group or the shot target. */
void TentackStateAttackTentacle::exeGroupWait() {
    bool isBack = (isLastPeriod() && mAttackGroups.size() != 0) ||
                  mAttackGroups.size() >= mAttackGroups.capacity();
    if (al::isFirstStep(this) && isShotLevel() && isBack) {
        mShotState->registerTentacleGroup(mAttackGroups.front());
    }

    TentackTentacleGroup* oldGroup = isOldGroupBack() ? mAttackGroups.front() : nullptr;
    s32 lookStep = isShotLevel() ? 0 : 60;
    if (al::isStep(this, lookStep)) {
        getHost()->getHead()->changeLookTarget();
    }

    if (!getHost()->getHead()->isWaitAll()) {
        return;
    }

    if (oldGroup != nullptr && al::isGreaterEqualStep(this, isShotLevel() ? 0 : 60)) {
        if (isShotLevel()) {
            if (mShotState->isExistTarget()) {
                getHost()->getHead()->turnToTargetGently(mShotState->getFirstTarget(), 1.5f);
            }
        } else {
            getHost()->getHead()->turnToTargetGently(oldGroup->mTargetPos, 1.5f);
        }
    }

    if (!al::isGreaterEqualStep(this, 200)) {
        return;
    }

    if (isBack) {
        if (!isShotLevel()) {
            al::setNerve(this, &NrvTentackStateAttackTentacleOldGroupBack);
        } else if (isShotTurn()) {
            al::setNerve(this, &NrvTentackStateAttackTentacleShotAndHide);
        } else {
            al::setNerve(this, &NrvTentackStateAttackTentacleShot);
        }
    } else {
        al::setNerve(this, &NrvTentackStateAttackTentacleGroupAppear);
    }
}

/** @brief Checks whether the last round through the groups is running. @return True if so. */
bool TentackStateAttackTentacle::isLastPeriod() const {
    return getCurrentParamInfo()->mPeriodNum <= mPeriod + 1;
}

/** @brief Pulls back the oldest attacking group once the head faces it. */
void TentackStateAttackTentacle::exeOldGroupBack() {
    TentackTentacleGroup* group = mAttackGroups.front();
    if (getHost()->getHead()->isWaitAll() &&
        getHost()->getHead()->turnToTargetGently(group->mTargetPos, 1.5f) &&
        group->requestEndSwingAll()) {
        if (mParam->mShotInterval < 1 || mShotCount != mParam->mShotInterval) {
            al::setNerve(this, &NrvTentackStateAttackTentacleEat);
        } else {
            al::setNerve(this, &NrvTentackStateAttackTentacleEatAndDisappear);
        }
    }
}

/** @brief Lets the head eat the items of the retreated group, optionally sinking afterwards. */
void TentackStateAttackTentacle::exeEat() {
    if (al::isFirstStep(this)) {
        updateShotCount();
    }

    bool isEat = al::isNerve(this, &NrvTentackStateAttackTentacleEat);
    TentackHead* head = getHost()->getHead();
    if (isEat ? head->tryStartActionEat() : head->tryStartActionEatAndDisappear()) {
        al::setNerve(this, &NrvTentackStateAttackTentacleGroupAppearAfterEat);
    }
}

/** @brief Fires at the retreating group, optionally sinking the head afterwards. */
void TentackStateAttackTentacle::exeShot() {
    if (al::isFirstStep(this)) {
        updateShotCount();
    }

    if (al::updateNerveState(this)) {
        if (al::isNerve(this, &NrvTentackStateAttackTentacleShotAndHide)) {
            getHost()->getHead()->setDisappear();
        }

        al::setNerve(this, &NrvTentackStateAttackTentacleGroupAppear);
    }
}

/** @brief Lets the next tentacle group appear and queues it as attacking. */
void TentackStateAttackTentacle::exeGroupAppear() {
    if (al::isStep(this, 15) && al::isNerve(this, &NrvTentackStateAttackTentacleGroupAppearAfterEat)) {
        mAttackGroups.front()->eatAttachItemAll();
    }

    if (!al::isGreaterEqualStep(this, 30)) {
        return;
    }

    TentackTentacleGroup* group = getTentacleGroup(mGroupIndex);
    for (s32 i = 0; i < mAttackTentacles.size(); i++) {
        if (group->isFull()) {
            break;
        }

        if (al::isDead(mAttackTentacles.at(i))) {
            group->registerTentacle(mAttackTentacles.at(i));
        }
    }

    group->appearAll();
    if (isOldGroupBack()) {
        mAttackGroups.popFront();
    }

    if (!isLastPeriod()) {
        mAttackGroups.pushBack(group);
    }

    mGroupIndex = al::modi(mGroupIndex + mGroupNum + 1, mGroupNum);
    if (mGroupIndex == 0) {
        mPeriod++;
    }

    if (isEnd()) {
        al::setNerve(this, &NrvTentackStateAttackTentacleAttackEndInit);
        return;
    }

    if (mGroupIndex == 0) {
        prepareTentacleGroups();
    }

    al::setNerve(this, &NrvTentackStateAttackTentacleGroupWait);
}

/**
 * @brief Gets the tentacle group at a position of the attack order.
 * @param index Position in the attack order.
 * @return Tentacle group.
 */
TentackTentacleGroup* TentackStateAttackTentacle::getTentacleGroup(s32 index) const {
    return mGroups[getCurrentParamInfo()->getGroupOrder(index)];
}

/** @brief Checks whether all rounds through the groups are done. @return True if so. */
bool TentackStateAttackTentacle::isEnd() const {
    return getCurrentParamInfo()->mPeriodNum <= mPeriod;
}

/** @brief Turns the head back to its front before ending the attack. */
void TentackStateAttackTentacle::exeAttackEndInit() {
    if (al::isFirstStep(this)) {
        getHost()->getHead()->changeLookTarget();
    }

    TentackHead* head = getHost()->getHead();
    if (head->turnToDirectionGently(getHost()->getHead()->mFrontDir, 1.5f) &&
        al::isGreaterEqualStep(this, isShotLevel() ? 0 : 360)) {
        if (isShotLevel()) {
            al::setNerve(this, &NrvTentackStateAttackTentacleAttackEndShot);
        } else {
            al::setNerve(this, &NrvTentackStateAttackTentacleAttackEnd);
        }
    }
}

/** @brief Ends the swing of every group once all of them can stop and the head eats. */
void TentackStateAttackTentacle::exeAttackEnd() {
    bool isEnableEndSwing = true;
    for (s32 i = 0; i < mGroupNum; i++) {
        isEnableEndSwing &= mGroups[i]->isEnableEndSwing();
    }

    if (!isEnableEndSwing || !getHost()->getHead()->tryStartActionEat()) {
        return;
    }

    for (s32 i = 0; i < mGroupNum; i++) {
        mGroups[i]->eatAttachItemAll();
        mGroups[i]->endSwingAllForce();
    }

    al::setNerve(this, &NrvTentackStateAttackTentacleAttackEndWait);
}

/** @brief Fires at all swinging tentacles when the attack ends. */
void TentackStateAttackTentacle::exeAttackEndShot() {
    if (al::isFirstStep(this)) {
        mShotState->clearTarget();
        for (s32 i = 0; i < mSwingTentacles.size(); i++) {
            mShotState->registerTentacle(mSwingTentacles.at(i));
        }
    }

    al::updateNerveStateAndNextNerve(this, &NrvTentackStateAttackTentacleAttackEndWait);
}

/** @brief Waits for the head to face front, or for the end wait, then ends the state. */
void TentackStateAttackTentacle::exeAttackEndWait() {
    if ((isShotLevel() &&
         getHost()->getHead()->turnToDirectionGently(getHost()->getHead()->mFrontDir, 1.5f)) ||
        al::isGreaterEqualStep(this, mParam->mEndWaitStep)) {
        kill();
    }
}

/**
 * @brief Finds the index of a swinging tentacle.
 * @param pTentacle Tentacle.
 * @return Index of the tentacle, or the tentacle count if not found.
 */
s32 TentackStateAttackTentacle::calcSwingTentacleId(const TentackTentacle* pTentacle) const {
    s32 i = 0;
    for (; i < mSwingTentacles.size(); i++) {
        if (mSwingTentacles.at(i) == pTentacle) {
            break;
        }
    }

    return i;
}
