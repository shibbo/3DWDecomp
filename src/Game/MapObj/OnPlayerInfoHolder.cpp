#include "MapObj/OnPlayerInfoHolder.hpp"

OnPlayerInfoHolder::OnPlayerInfoHolder(int count) : mCount(count) {
    mInfos = new OnPlayerInfo[count];
}

OnPlayerInfo* OnPlayerInfoHolder::getOnPlayerInfo(al::HitSensor* pSensor) {
    OnPlayerInfo* available = nullptr;
    for (int i = 0; i < mCount; ++i) {
        OnPlayerInfo* info = &mInfos[i];
        if (info->mSensor == pSensor)
            return info;
        if (!available && !info->isValid())
            available = info;
    }
    if (available)
        available->mSensor = pSensor;
    return available;
}

bool OnPlayerInfo::isValid() const { return mSensor != nullptr; }

void OnPlayerInfoHolder::update() {
    bool plus = false;
    bool minus = false;
    for (int i = 0; i < mCount; ++i) {
        mInfos[i].update();
        if (mInfos[i].mDirection > 0)
            plus = true;
        if (mInfos[i].mDirection < 0)
            minus = true;
    }
    mPrevState = mState;
    if (plus)
        mState = minus ? Both : Plus;
    else if (minus)
        mState = Minus;
    else
        mState = None;
}

void OnPlayerInfo::update() {
    mPrevDirection = mDirection;
    if (--mLife <= 0) {
        mSensor = nullptr;
        mLife = 0;
        mDirection = 0;
    }
}

void OnPlayerInfoHolder::reset() {
    for (int i = 0; i < mCount; ++i) {
        if (mInfos[i].isValid())
            mInfos[i].reset();
    }
    mPrevState = None;
    mState = None;
}

void OnPlayerInfo::reset() {
    mDirection = 0;
    mPrevDirection = 0;
    mLife = 0;
    mWeight = 1;
}

int OnPlayerInfoHolder::calcTotalOnPlayerDir() const {
    int total = 0;
    for (int i = 0; i < mCount; ++i) {
        if (mInfos[i].isValid())
            total += mInfos[i].mDirection * mInfos[i].mWeight;
    }
    return total;
}

int OnPlayerInfoHolder::calcTotalPrevOnPlayerDir() const {
    int total = 0;
    for (int i = 0; i < mCount; ++i) {
        if (mInfos[i].isValid())
            total += mInfos[i].mPrevDirection * mInfos[i].mWeight;
    }
    return total;
}

bool OnPlayerInfoHolder::isChangeOnState() const { return mPrevState != mState; }
bool OnPlayerInfoHolder::isOnStateNone() const { return mState == None; }
bool OnPlayerInfoHolder::isOnStatePlus() const { return mState == Plus; }
bool OnPlayerInfoHolder::isOnStateMinus() const { return mState == Minus; }
bool OnPlayerInfoHolder::isOnStateBoth() const { return mState == Both; }

OnPlayerInfo::OnPlayerInfo() {}
