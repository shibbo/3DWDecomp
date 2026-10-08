#pragma once
#include "Library/Nerve/NerveStateBase.hpp"
namespace al { class ActorInitInfo; class SensorMsg; class FunctorBase; }
class BoxCoin;
class BlockStateCoinTen : public al::ActorStateBase {
public:
    BlockStateCoinTen(al::LiveActor*, const al::ActorInitInfo&, bool);
    void appear() override;
    bool receiveMsg(const al::SensorMsg*);
    bool isValidAppearCoin() const;
    void appearCoin();
    void setAppearCoinCallBack(const al::FunctorBase&);
    bool isValidUpperPunch(int) const;
    bool isAppearBoxCoin() const;
    bool isAppearCoinMax() const;
    BoxCoin* getBoxCoin() const;
    bool isTimerEndOrCoinMax() const;
    void setCoinMax(int coinMax) { mMaxCoins = coinMax; }
    void exeWait();
    void exeAppearCoin();
    void exeAppearCoinWait();
private:
    int mElapsedFrames = 0;
    int mCoinCount = 0;
    int mMaxCoins = 10;
    int mReactionFrames = 6;
    al::FunctorBase* mAppearCoinCallback = nullptr;
    BoxCoin* mBoxCoin = nullptr;
    bool mStatueDrop = false;
};
