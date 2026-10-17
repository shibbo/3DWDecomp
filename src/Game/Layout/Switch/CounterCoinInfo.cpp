#include "Layout/Switch/CounterCoinInfo.hpp"

#include <prim/seadSafeString.h>

#include "Layout/CounterCoinParts.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/Data/SingleModeData.hpp"
#include "System/GameDataHolder.hpp"

namespace {
NERVE_DECL(CounterCoinInfo, Wait);
NERVE_DECL(CounterCoinInfo, Appear);
NERVE_DECL(CounterCoinInfo, Intro);
NERVE_DECL(CounterCoinInfo, ShowMiss);
NERVE_DECL(CounterCoinInfo, Outro);
NERVE_DECL(CounterCoinInfo, End);
NERVES_MAKE_NOSTRUCT(CounterCoinInfo, End, Wait, Intro, ShowMiss, Outro, Appear)

/** Most coins that are counted down one by one after a miss. */
constexpr s32 cCountDownCoinMax = 50;
/** Frames the counter stays visible before it hides. */
constexpr s32 cWaitStep = 45;
}  // namespace

/**
 * @brief Creates the coin counter layout and its coin parts.
 * @param rInfo Layout initialization context.
 * @param pHolder Game data holder used to read the coin count.
 */
CounterCoinInfo::CounterCoinInfo(const al::LayoutInitInfo& rInfo, const GameDataHolder* pHolder)
    : al::LayoutActor("CounterCoinInfo"), mGameDataHolder(pHolder) {
    al::initLayoutActor(this, rInfo, "CounterCoinInfo", nullptr);
    initNerve(&NrvCounterCoinInfoWait, 0);
    mCoinParts = new CounterCoinParts(rInfo, "CounterCoinParts", "ParCounterCoin", this);
    mCoinParts->waitMiss();
}

/** @brief Shows the counter and starts the appear animation. */
void CounterCoinInfo::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvCounterCoinInfoAppear);
}

/** @brief Shows the current coin count, then starts counting down if there are coins. */
void CounterCoinInfo::exeAppear() {
    if (al::isFirstStep(this)) {
        mCoinNum = mGameDataHolder->getSingleFile()->getCoinNum();
        mCoinParts->setCoinCount(mCoinNum);
        mCoinNum = mCoinNum < cCountDownCoinMax ? mCoinNum : cCountDownCoinMax;
        al::startAction(this, "Appear");
    }

    if (al::isActionEnd(this)) {
        if (mCoinNum > 0) {
            al::setNerve(this, &NrvCounterCoinInfoIntro);
        } else {
            al::setNerve(this, &NrvCounterCoinInfoWait);
        }
    }
}

/** @brief Removes one coin per step until the countdown is over. */
void CounterCoinInfo::exeShowMiss() {
    if (al::isFirstStep(this)) {
        SingleModeData* pFile = mGameDataHolder->getSingleFile();
        pFile->subtractCoin(1);
        mCoinParts->setCoinCount(mGameDataHolder->getSingleFile()->getCoinNum());
        mCoinNum--;
    }

    al::holdSe(this, "DecreaseStart");

    if (!al::isLessStep(this, 1)) {
        if (mCoinNum > 0) {
            al::setNerve(this, &NrvCounterCoinInfoShowMiss);
        } else {
            al::setNerve(this, &NrvCounterCoinInfoOutro);
        }
    }
}

/** @brief Keeps the counter on screen for a while before hiding it. */
void CounterCoinInfo::exeWait() {
    if (!al::isLessStep(this, cWaitStep)) {
        al::setNerve(this, &NrvCounterCoinInfoEnd);
    }
}

/** @brief Plays the intro animation before the countdown. */
void CounterCoinInfo::exeIntro() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Intro");
    }

    if (al::isActionEnd(this)) {
        if (mCoinNum > 0) {
            al::setNerve(this, &NrvCounterCoinInfoShowMiss);
        } else {
            al::setNerve(this, &NrvCounterCoinInfoWait);
        }
    }
}

/** @brief Plays the outro animation once the countdown is over. */
void CounterCoinInfo::exeOutro() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Outro");
        al::startSe(this, "DecreaseEnd");
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvCounterCoinInfoEnd);
    }
}

/** @brief Plays the end animation and kills the counter afterwards. */
void CounterCoinInfo::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End");
    } else if (al::isActionEnd(this)) {
        kill();
    }
}
