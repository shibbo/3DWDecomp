#pragma once

namespace Vessel { namespace Utility {

class CTickWatcher {
public:
    CTickWatcher();
    virtual ~CTickWatcher();
    void Reset();
    void Enter();
    void Leave();
    void Report(unsigned long count, const char* pLabel);
    unsigned long GetAverage() const;
    unsigned long GetMaximum() const;
    unsigned long GetMinimum() const;

private:
    unsigned long mStart;
    unsigned long mElapsed;
    unsigned long mCount;
    unsigned long mTotal;
    unsigned long mMaximum;
    unsigned long mMinimum;
};

class CTickChecker {
public:
    CTickChecker();
    virtual ~CTickChecker();
    void Reset();
    void Check();
    void Report(const char* pLabel);

private:
    unsigned long mPrevious;
    unsigned long mElapsed;
    unsigned long mCount;
};

} }
