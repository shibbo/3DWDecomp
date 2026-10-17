#pragma once

#include <basis/seadTypes.h>

/** @brief A view of PCM samples with a wrapping read position. */
template <typename T>
class PcmReader {
public:
    /** @brief Reads a sample relative to the current position.
     * @param offset Sample offset, requiring at most one wrap.
     * @return Sample at the relative position.
     */
    T at(u32 offset) const {
        u32 index = offset + mReadPosition;
        if (index >= mCapacity) {
            index -= mCapacity;
        }
        return mSamples[index];
    }

    /** @brief Gets a sample reference relative to the read position.
     * @param offset Sample offset, requiring at most one wrap.
     * @return Reference to the sample at the relative position.
     */
    const T& operator[](u32 offset) const {
        return mSamples[offset + mReadPosition >= mCapacity ?
                            offset + mReadPosition - mCapacity : offset + mReadPosition];
    }

    /** @brief Advances the read position.
     * @param count Samples to skip, requiring at most one wrap.
     */
    void advance(u32 count) {
        mReadPosition += count;
        if (mReadPosition >= mCapacity) {
            mReadPosition -= mCapacity;
        }
    }

    /** @brief Reads and advances by one sample.
     * @return Sample at the previous read position.
     */
    T read() {
        T value = mSamples[mReadPosition];
        mReadPosition++;
        if (mReadPosition >= mCapacity) {
            mReadPosition = 0;
        }
        return value;
    }

    T* mSamples;
    u32 mCapacity;
    u32 mReadPosition;
};

/** @brief PCM ring buffer with separate read and write positions. */
template <typename T>
class PcmBuffer : public PcmReader<T> {
public:
    /** @brief Appends a sample and wraps the write position.
     * @param value Sample to append.
     */
    void write(T value) {
        this->mSamples[mWritePosition] = value;
        mWritePosition++;
        if (mWritePosition >= this->mCapacity) {
            mWritePosition = 0;
        }
    }

    /** @brief Gets the number of samples between the read and write positions.
     * @return Available sample count.
     */
    u32 size() const {
        if (mWritePosition >= this->mReadPosition) {
            return mWritePosition - this->mReadPosition;
        }
        return this->mCapacity + (mWritePosition - this->mReadPosition);
    }

    u32 mWritePosition;
};

class PcmTempoChanger {
public:
    PcmTempoChanger();
    void initialize(f32 tempo, u32 correlationSampleNum, u32 minPeriod, u32 maxPeriod);
    void CopyPcmOnTempoUp(PcmBuffer<s16>& rOutput, PcmReader<s16>& rInput, u32 period);
    void CopyPcmOnTempoDown(PcmBuffer<s16>& rOutput, PcmReader<s16>& rInput, u32 period);
    void changeTempo(PcmBuffer<s16>& rOutput, PcmReader<s16>& rInput, u32 sampleNum);
    void changeTempo(PcmBuffer<s16>& rOutput, PcmBuffer<s16>& rInput);

private:
    u32 mCorrelationSampleNum = 0;
    u32 mMinPeriod = 0;
    u32 mMaxPeriod = 0;
    u32 mRequiredSampleNum = 0;
    u32 mSkipCount = 0;
    f32 mTempo = 1.0f;
};

static_assert(sizeof(PcmReader<s16>) == 0x10);
static_assert(sizeof(PcmBuffer<s16>) == 0x18);
static_assert(sizeof(PcmTempoChanger) == 0x18);
