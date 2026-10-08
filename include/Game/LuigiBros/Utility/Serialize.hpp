#pragma once

#include <nn/types.h>

namespace Common::Serialize {

/**
 * @brief Reads or writes values from or to a byte buffer in little-endian order.
 *
 * Without a buffer, only the offset advances, which measures the size of the serialized data.
 */
class LittleEndian {
public:
    /**
     * @brief Construct a serializer.
     * @param pBuffer The buffer, or nullptr to only measure.
     * @param isImport Whether values are read from the buffer (true) or written to it (false).
     */
    LittleEndian(void* pBuffer, bool isImport)
        : mBuffer(static_cast<u8*>(pBuffer)), mIsImport(isImport), mOffset(0) {}

    /**
     * @brief Read or write one value.
     * @param rValue The value.
     */
    template <typename T>
    void Entry(T& rValue) {
        u8* pBuffer = mBuffer;
        if (pBuffer == nullptr) {
            mOffset += sizeof(T);
        } else if (mIsImport) {
            rValue = pBuffer[mOffset++];
            for (u64 i = 1; i < sizeof(T); i++) {
                rValue += static_cast<T>(pBuffer[mOffset++]) << (i * 8);
            }
        } else {
            T value = rValue;
            for (u64 i = 0; i < sizeof(T); i++) {
                pBuffer[mOffset++] = static_cast<u8>(value >> (i * 8));
            }
        }
    }

    /// Get the number of bytes read or written so far.
    u64 GetOffset() const { return mOffset; }

private:
    u8* mBuffer;
    bool mIsImport;
    u64 mOffset;
};

}  // namespace Common::Serialize
