#include "Game/Bgm/PcmTempoChanger.hpp"

namespace {
/** @brief Clamps a mixed sample to the signed 16-bit PCM range.
 * @param sample Mixed floating-point sample.
 * @return Clamped PCM sample.
 */
inline s16 clampSample(f32 sample) {
    if (sample > 32767.0f) {
        sample = 32767.0f;
    } else if (sample < -32768.0f) {
        sample = -32768.0f;
    }
    return static_cast<s16>(sample);
}
}  // namespace

/** @brief Creates a tempo changer with the original playback tempo. */
PcmTempoChanger::PcmTempoChanger() = default;

/** @brief Configures the tempo and the period search bounds.
 * @param tempo Playback tempo multiplier.
 * @param correlationSampleNum Samples used to compare candidate periods.
 * @param minPeriod First period to test.
 * @param maxPeriod Exclusive upper bound for the period search.
 */
void PcmTempoChanger::initialize(f32 tempo, u32 correlationSampleNum, u32 minPeriod,
                                 u32 maxPeriod) {
    mCorrelationSampleNum = correlationSampleNum;
    mMinPeriod = minPeriod;
    mMaxPeriod = maxPeriod;
    mTempo = tempo;
    if (mTempo > 1.0f) {
        u32 skipCount = 0;
        while (tempo > 2.0f) {
            tempo *= 0.5f;
            skipCount += 2;
        }
        mTempo = tempo;
        u32 required = static_cast<u32>(maxPeriod / (mTempo - 1.0f) + 0.5f) + maxPeriod;
        mRequiredSampleNum = required > 2 * maxPeriod ? required : 2 * maxPeriod;
        mSkipCount = skipCount;
    } else if (mTempo < 1.0f) {
        u32 required = static_cast<u32>(maxPeriod / (1.0f - mTempo) + 0.5f);
        mRequiredSampleNum = required > 2 * maxPeriod ? required : 2 * maxPeriod;
    } else {
        mRequiredSampleNum = maxPeriod;
    }
}

/** @brief Crossfades adjacent periods and skips samples to increase the tempo.
 * @param rOutput Destination PCM buffer.
 * @param rInput Source PCM reader.
 * @param period Selected period length in samples.
 */
void PcmTempoChanger::CopyPcmOnTempoUp(PcmBuffer<s16>& rOutput, PcmReader<s16>& rInput,
                                      u32 period) {
    f32 step = 1.0f / period;
    f32 firstWeight = 1.0f;
    f32 secondWeight = 0.0f;
    for (u32 i = 0; i < period; i++) {
        firstWeight -= step;
        secondWeight += step;
        rOutput.write(clampSample(firstWeight * rInput.at(i) + secondWeight * rInput.at(period + i)));
    }
    rInput.advance(period);
    u32 copyNum = static_cast<u32>(period / (mTempo - 1.0f) + 0.5f);
    for (u32 i = period; i < copyNum; i++) {
        rOutput.write(rInput.at(i));
    }
    rInput.advance(copyNum);
    rInput.advance(mSkipCount * (period + copyNum));
}

/** @brief Repeats a period with a crossfade to decrease the tempo.
 * @param rOutput Destination PCM buffer.
 * @param rInput Source PCM reader.
 * @param period Selected period length in samples.
 */
void PcmTempoChanger::CopyPcmOnTempoDown(PcmBuffer<s16>& rOutput, PcmReader<s16>& rInput,
                                        u32 period) {
    for (u32 i = 0; i < period; i++) {
        rOutput.write(rInput.at(i));
    }
    f32 step = 1.0f / period;
    f32 firstWeight = 1.0f;
    f32 secondWeight = 0.0f;
    for (u32 i = 0; i < period; i++) {
        firstWeight -= step;
        secondWeight += step;
        rOutput.write(clampSample(firstWeight * rInput.at(period + i) + secondWeight * rInput.at(i)));
    }
    u32 copyNum = static_cast<u32>(mTempo * period / (1.0f - mTempo) + 0.5f);
    for (u32 i = period; i < copyNum; i++) {
        rOutput.write(rInput.at(i));
    }
    rInput.advance(copyNum);
}

/** @brief Produces PCM samples using the best-correlated period in the configured range.
 * @param rOutput Destination PCM buffer.
 * @param rInput Source PCM reader.
 * @param sampleNum Requested output sample count.
 */
void PcmTempoChanger::changeTempo(PcmBuffer<s16>& rOutput, PcmReader<s16>& rInput,
                                  u32 sampleNum) {
    if (static_cast<u32>(mTempo * 1000.0f) == 1000) {
        for (u32 i = 0; i < sampleNum; i++) {
            rOutput.write(rInput.read());
        }
        return;
    }
    while (rOutput.size() < sampleNum) {
        s32 bestCorrelation = (-2147483647 - 1);
        u32 bestPeriod = 0;
        for (u32 period = mMinPeriod; period < mMaxPeriod; period++) {
            s32 correlation = 0;
            for (u32 i = 0; i < mCorrelationSampleNum; i++) {
                correlation += (rInput[i] * rInput[period + i]) >> 7;
            }
            if (bestCorrelation < correlation) {
                bestCorrelation = correlation;
                bestPeriod = period;
            }
        }
        if (mTempo > 1.0f) {
            CopyPcmOnTempoUp(rOutput, rInput, bestPeriod);
        } else {
            CopyPcmOnTempoDown(rOutput, rInput, bestPeriod);
        }
    }
}

/** @brief Unused buffer-to-buffer overload; performs no operation.
 * @param rOutput Unused destination buffer.
 * @param rInput Unused source buffer.
 */
void PcmTempoChanger::changeTempo(PcmBuffer<s16>& rOutput, PcmBuffer<s16>& rInput) {}
