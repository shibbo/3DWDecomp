#pragma once

namespace al { class HitSensor; }

class OnPlayerInfo {
public:
    OnPlayerInfo();
    bool isValid() const;
    void update();
    void reset();

    al::HitSensor* mSensor = nullptr;
    int mDirection = 0;
    int mPrevDirection = 0;
    int mLife = 0;
    int mWeight = 1;
};

class OnPlayerInfoHolder {
public:
    explicit OnPlayerInfoHolder(int count);
    OnPlayerInfo* getOnPlayerInfo(al::HitSensor* pSensor);
    void update();
    void reset();
    int calcTotalOnPlayerDir() const;
    int calcTotalPrevOnPlayerDir() const;
    bool isChangeOnState() const;
    bool isOnStateNone() const;
    bool isOnStatePlus() const;
    bool isOnStateMinus() const;
    bool isOnStateBoth() const;

private:
    enum State { None, Plus, Minus, Both };
    OnPlayerInfo* mInfos = nullptr;
    int mCount;
    State mPrevState = None;
    State mState = None;
};
