#pragma once

#include <new>
#include <utility>

#include <attributes.h>

#include "basis/seadTypes.h"

namespace sead
{
/// Binary min-heap over a caller supplied buffer. Elements are ordered with operator>.
template <typename T>
class PriorityQueue
{
public:
    PriorityQueue() = default;

    void setBuffer(s32 capacity, T* buffer)
    {
        mBuffer = buffer;
        mCapacity = capacity;
    }

    s32 size() const { return mSize; }
    s32 capacity() const { return mCapacity; }
    bool isEmpty() const { return mSize == 0; }
    bool isFull() const { return mSize == mCapacity; }
    const T& top() const { return mBuffer[0]; }
    void clear() { mSize = 0; }

    /// Constructs a new element at the end of the heap and moves it up to its place.
    /// Returns nullptr when the queue is full.
    template <typename... Args>
    NOINLINE T* tryBirth(Args&&... args);

protected:
    T* upHeap_(T* node)
    {
        s64 index = node - mBuffer;
        if (index <= 0 || index >= mSize)
            return node;

        T value = *node;
        s64 parent = (index + 1) / 2 - 1;
        if (value > mBuffer[parent])
            return node;

        do
        {
            mBuffer[index] = mBuffer[parent];
            index = parent;
            if (index == 0)
                break;
            parent = (index + 1) / 2 - 1;
        } while (!(value > mBuffer[parent]));

        mBuffer[index] = value;
        return &mBuffer[index];
    }

    T* mBuffer = nullptr;
    s32 mCapacity = 0;
    s32 mSize = 0;
};

template <typename T>
template <typename... Args>
T* PriorityQueue<T>::tryBirth(Args&&... args)
{
    if (mSize == mCapacity)
        return nullptr;

    T* node = new (&mBuffer[mSize]) T(std::forward<Args>(args)...);
    mSize++;
    return upHeap_(node);
}

/// PriorityQueue with an inline buffer of N elements.
template <typename T, s32 N>
class FixedPriorityQueue : public PriorityQueue<T>
{
public:
    FixedPriorityQueue() { this->setBuffer(N, mWork); }

private:
    T mWork[N];
};
}  // namespace sead
