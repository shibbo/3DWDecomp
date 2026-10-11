#pragma once
#include <nn/types.h>

namespace nn::atk::detail {
template <typename T>
class ValueArray {
public:
    ValueArray() : mValues(nullptr), mCount(0) {}
    // memory holds count entries; the array borrows this storage and clears its values.
    void Initialize(void* memory, int count) {
        mValues = static_cast<T*>(memory);
        mCount = count;
        Reset();
    }
    void Finalize() { mValues = nullptr; mCount = 0; }
    void Reset() {
        for (int i = 0; i < mCount; ++i) mValues[i] = T();
    }
    // other supplies entries up to this array's capacity; remaining entries are cleared.
    ValueArray& operator=(const ValueArray& other) {
        int count = other.mCount < mCount ? other.mCount : mCount;
        for (int i = 0; i < count; ++i) mValues[i] = other.mValues[i];
        for (int i = count; i < mCount; ++i) mValues[i] = T();
        return *this;
    }

    /**
     * @brief Reads an entry without a range check.
     * @param index Entry index.
     * @return The entry.
     */
    T GetValue(int index) const { return mValues[index]; }
    /**
     * @brief Writes an entry without a range check.
     * @param index Entry index.
     * @param value New value.
     */
    void SetValue(int index, T value) { mValues[index] = value; }

    /** @brief Gets the number of entries. @return Entry count. */
    int GetCount() const { return mCount; }
    /**
     * @brief Reads an entry if it exists.
     * @param index Entry index.
     * @return The entry, or a value-initialized T if index is out of range.
     */
    T TryGetValue(int index) const { return index < mCount ? mValues[index] : T(); }
    /**
     * @brief Writes an entry if it exists.
     * @param index Entry index; out-of-range writes are ignored.
     * @param value New value.
     */
    void TrySetValue(int index, T value) {
        if (index < mCount) {
            mValues[index] = value;
        }
    }

private:
    friend class OutputAdditionalParam;
    T* mValues;
    int mCount;
};
}
