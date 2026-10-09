#pragma once

#include "prim/seadDelegate.h"
#include "prim/seadNamable.h"

namespace sead
{
class Job : public INamable
{
public:
    virtual ~Job();
    virtual void invoke() = 0;
};

template <typename T>
class Job0 : public Job
{
public:
    Job0(const Delegate<T>& delegate) : mDelegate(delegate) {}
    void invoke() override { mDelegate.invoke(); }

protected:
    Delegate<T> mDelegate;
};

template <typename T, typename A1>
class Job1 : public Job
{
public:
    Job1(T* instance, void (T::*fn)(A1), A1 arg) : mDelegate(instance, fn), mArg(arg) {}
    void invoke() override
    {
        IDelegate1<A1>& delegate = mDelegate;
        delegate(mArg);
    }

protected:
    Delegate1<T, A1> mDelegate;
    A1 mArg;
};
}  // namespace sead
