#include "MapObj/GoalBonusGameBlockSlot.hpp"
#include "MapObj/BlockSlot.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWait.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
NERVE_ACTION_IMPL(GoalBonusGameBlockSlot, Wait)
NERVE_ACTION_IMPL(GoalBonusGameBlockSlot, Rolling)
NERVE_ACTION_IMPL(GoalBonusGameBlockSlot, Result)
NERVE_ACTION_IMPL(GoalBonusGameBlockSlot, SplashCoin)
NERVE_ACTION_IMPL(GoalBonusGameBlockSlot, SplashCoinEnd)
NERVE_ACTION_IMPL(GoalBonusGameBlockSlot, Head)
NERVE_ACTION_IMPL(GoalBonusGameBlockSlot, End)
NERVE_ACTIONS_MAKE_STRUCT(GoalBonusGameBlockSlot, Wait, Rolling, Result, SplashCoin, SplashCoinEnd, Head, End)
}
GoalBonusGameBlockSlot::GoalBonusGameBlockSlot(const char* name) : al::LiveActor(name) {}
GoalBonusGameBlockSlot::~GoalBonusGameBlockSlot() = default;
void GoalBonusGameBlockSlot::init(const al::ActorInitInfo& info) {
    al::initNerveAction(this, "Wait", &NrvGoalBonusGameBlockSlot.collector, 0);
    al::initActor(this, info);
    mSlotCount = al::calcLinkChildNum(info, "BlockSlotList");
    if (mSlotCount == 0) { makeActorDead(); return; }
    mSlots = new BlockSlot*[mSlotCount];
    for (int i = 0; i < mSlotCount; ++i) {
        mSlots[i] = new BlockSlot("ブロックスロット");
        al::initLinksActor(mSlots[i], info, "BlockSlotList", i);
    }
    mCounter = new al::SimpleLayoutAppearWaitEnd("報酬コインカウンター", "CounterSlot", al::getLayoutInitInfo(info), nullptr, false);
    mResult = new al::SimpleLayoutAppearWait("結果発表レイアウト", "HeadSlotResult", al::getLayoutInitInfo(info), nullptr);
    makeActorAppeared();
}
void GoalBonusGameBlockSlot::exeWait() {
    if (al::isGreaterEqualStep(this, 60)) al::startNerveAction(this, "Rolling");
}
void GoalBonusGameBlockSlot::startSlot() {
    for (int i = 0; i < mSlotCount; ++i) if (mSlots[i]) mSlots[i]->startRolling();
}
bool GoalBonusGameBlockSlot::updateSameId() {
    bool changed = false;
    for (int i = 0; i < mSlotCount; ++i) {
        if (!mSlots[i]->isStop()) continue;
        for (int j = 0; j < mSlotCount; ++j) {
            if (i == j || !mSlots[j]->isStop() || mSlots[i]->getResultNumber() != mSlots[j]->getResultNumber()) continue;
            if (mFirstMatch == -1) mFirstMatch = mSlots[i]->getResultNumber();
            int match = mFirstMatch != mSlots[i]->getResultNumber();
            if (mSlots[i]->getMatchIndex() < 0 || mSlots[j]->getMatchIndex() < 0) changed = true;
            mSlots[i]->setMatchIndex(match);
            mSlots[j]->setMatchIndex(match);
        }
    }
    return changed;
}
bool GoalBonusGameBlockSlot::isStopSlotAll() const {
    for (int i = 0; i < mSlotCount; ++i) if (mSlots[i] && !mSlots[i]->isStop()) return false;
    return true;
}
void GoalBonusGameBlockSlot::exeRolling() {
    if (al::isFirstStep(this)) startSlot();
    if (updateSameId()) for (int i = 0; i < mSlotCount; ++i) mSlots[i]->tryStartMatch();
    if (isStopSlotAll()) {
        bool perfect = true;
        for (int i = 1; i < mSlotCount; ++i) {
            if (mSlots[0]->getResultNumber() != mSlots[i]->getResultNumber()) { perfect = false; break; }
        }
        if (perfect) for (int i = 0; i < mSlotCount; ++i) mSlots[i]->startPerfect();
        al::startNerveAction(this, "Result");
    }
}
int GoalBonusGameBlockSlot::calcResultCount() const {
    int reward = 0;
    for (int i = 0; i < mSlotCount; ++i) {
        if (!mSlots[i]) continue;
        int count = 0;
        for (int j = 0; j < mSlotCount; ++j) if (mSlots[j] && mSlots[j]->getMatchIndex() == i) ++count;
        if (count == 2) reward += 100;
        if (count == 3) reward += 300;
        if (count > 3) reward += 777;
    }
    return reward == 0 ? 10 : reward;
}
void GoalBonusGameBlockSlot::exeResult() {
    if (al::isFirstStep(this)) {
        mReward = calcResultCount();
        mExtraLives = (GameDataFunction::getCoinNum(GameDataHolderAccessor(this)) + mReward) / 100;
        al::setPaneNumberDigit3(mCounter, "TxtNum", mReward, 0);
        al::setPaneNumberDigit3(mCounter, "TxtNum_ds", mReward, 0);
        mCounter->appear();
    }
    if (al::isGreaterEqualStep(this, 120)) al::startNerveAction(this, "SplashCoin");
}
void GoalBonusGameBlockSlot::exeSplashCoin() {
    if (al::isFirstStep(this)) {
        if (mReward > 10) al::tryEmitEffect(this, "CoinShower", nullptr);
        else al::tryEmitEffect(this, "CoinShower10", nullptr);
        mCoinsRemaining = mReward;
    }
    if (!al::isFirstStep(this) && al::isIntervalStep(this, 5, 0)) {
        if (mCoinsRemaining >= 10 && mReward > 10) {
            al::acquirerItem(this, nullptr, "コインx10[自動取得]");
            al::startSe(this, "PgCoin", nullptr);
            mCoinsRemaining -= 10;
        } else if (mCoinsRemaining >= 5) {
            al::acquirerItem(this, nullptr, "コインx5[自動取得]");
            al::startSe(this, "PgCoin", nullptr);
            mCoinsRemaining -= 5;
        } else if (mCoinsRemaining > 0) {
            al::acquirerItem(this, nullptr, "コインx1[自動取得]");
            al::startSe(this, "PgCoin", nullptr);
            --mCoinsRemaining;
        }
    }
    al::setPaneNumberDigit3(mCounter, "TxtNum", mCoinsRemaining, 0);
    al::setPaneNumberDigit3(mCounter, "TxtNum_ds", mCoinsRemaining, 0);
    al::holdSe(this, "PgCoinFall", nullptr);
    if (mCoinsRemaining <= 0) {
        mCounter->end();
        al::startNerveAction(this, "SplashCoinEnd");
        al::startSe(this, "PgCoinFallEnd", nullptr);
    }
}
void GoalBonusGameBlockSlot::exeSplashCoinEnd() {
    if (al::isGreaterEqualStep(this, 15)) al::startNerveAction(this, "Head");
}
void GoalBonusGameBlockSlot::exeHead() {
    if (al::isFirstStep(this)) {
        al::stopAllBgm(this, -1);
        al::changeAudioEffect(this, nullptr);
        al::startSequenceBgm(this, "RouletteClear", -1, 0);
    }
    if (al::isStep(this, 94) && mExtraLives > 0) {
        rc::replacePaneMsgNumber(mResult, mResult, "TxtSlotResult", "HeadSlotResult", "HeadSlotResult_SlotResult", mExtraLives);
        rc::replacePaneMsgNumber(mResult, mResult, "TxtSlotResult_ds", "HeadSlotResult", "HeadSlotResult_SlotResult", mExtraLives);
        mResult->appear();
    }
    if (al::isGreaterEqualStep(this, 210)) al::startNerveAction(this, "End");
}
void GoalBonusGameBlockSlot::exeEnd() {}
bool GoalBonusGameBlockSlot::isGoal() const { return al::isNerve(this, NrvGoalBonusGameBlockSlot.End.data()); }
bool GoalBonusGameBlockSlot::isEndGoalDemo() const { return al::isNerve(this, NrvGoalBonusGameBlockSlot.End.data()); }
bool GoalBonusGameBlockSlot::isUseResult() const { return false; }
