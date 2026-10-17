#pragma once

namespace Common { namespace Serialize {

class LittleEndian {
public:
    /**
     * @brief Opens a little-endian serialization cursor.
     * @param pData Buffer to read or write.
     * @param reading Whether entries are read from the buffer.
     */
    LittleEndian(void* pData, bool reading)
        : mpData(static_cast<unsigned char*>(pData)), mReading(reading), mOffset(0) {}

    template <typename T> void Entry(T& rValue) { entryBits(rValue); }
    unsigned long GetOffset() const { return mOffset; }
    void SetOffset(unsigned long offset) { mOffset = offset; }
    /** @brief Returns the number of serialized bytes. @return Cursor offset. */
    unsigned long GetSize() const { return mOffset; }
    /** @brief Sets the serialized cursor after a fixed-size state block. */
    void SetSize(unsigned long size) { mOffset = size; }
    /** @brief Pads a serialized block to a multiple of sixteen bytes. */
    void AlignBlock(unsigned long start) {
        mOffset = start + ((static_cast<unsigned int>(mOffset - start) + 15) & ~15U);
    }

private:
    template <typename T> __attribute__((always_inline)) void entryBits(T& rValue) {
        unsigned char* data = mpData;
        if (!data) mOffset += sizeof(T);
        else if (mReading) {
            rValue = data[mOffset++];
            for (unsigned long i = 1; i < sizeof(T); ++i)
                rValue += static_cast<T>(data[mOffset++]) << (i * 8);
        } else {
            T value = rValue;
            for (unsigned long i = 0; i < sizeof(T); ++i)
                data[mOffset++] = static_cast<unsigned char>(value >> (i * 8));
        }
    }

    unsigned char* mpData;
    bool mReading;
    unsigned long mOffset;
};

/**
 * @brief Reads, writes, or sizes a single-byte entry.
 * @param rValue Byte to serialize.
 */
template <> inline void LittleEndian::Entry<unsigned char>(unsigned char& rValue) {
    if (mpData) {
        if (mReading) { rValue = mpData[mOffset++]; }
        else { mpData[mOffset++] = rValue; }
    } else { ++mOffset; }
}

/**
 * @brief Reads, writes, or sizes a little-endian 16-bit entry.
 * @param rValue Word to serialize.
 */
template <> inline void LittleEndian::Entry<unsigned short>(unsigned short& rValue) {
    unsigned char* pData = mpData;
    if (pData) {
        if (mReading) {
            rValue = pData[mOffset++];
            rValue |= static_cast<unsigned short>(pData[mOffset++]) << 8;
        } else {
            unsigned short value = rValue;
            pData[mOffset++] = value;
            pData[mOffset++] = value >> 8;
        }
    } else { mOffset += 2; }
}

/** @brief Transfers a signed little-endian 16-bit entry. */
template <> inline void LittleEndian::Entry<short>(short& rValue) {
    unsigned char* pData = mpData;
    if (pData) {
        if (mReading) {
            rValue = pData[mOffset++];
            rValue += static_cast<short>(pData[mOffset++]) << 8;
        } else {
            short value = rValue;
            pData[mOffset++] = value;
            pData[mOffset++] = value >> 8;
        }
    } else { mOffset += 2; }
}

/** @brief Reads, writes, or sizes a little-endian 32-bit entry.
 * @param rValue Word to serialize.
 */
template <> inline void LittleEndian::Entry<unsigned int>(unsigned int& rValue) {
    unsigned char* pData = mpData;
    if (pData) {
        if (mReading) {
            rValue = pData[mOffset++];
            rValue |= static_cast<unsigned int>(pData[mOffset++]) << 8;
            rValue |= static_cast<unsigned int>(pData[mOffset++]) << 16;
            rValue |= static_cast<unsigned int>(pData[mOffset++]) << 24;
        } else {
            unsigned int value = rValue;
            pData[mOffset++] = value;
            pData[mOffset++] = value >> 8;
            pData[mOffset++] = value >> 16;
            pData[mOffset++] = value >> 24;
        }
    } else { mOffset += 4; }
}

/** @brief Transfers the IEEE-754 bit representation of a double in little-endian order. */
template <> inline void LittleEndian::Entry<double>(double& rValue) {
    unsigned char* pData = mpData;
    unsigned long& bits = reinterpret_cast<unsigned long&>(rValue);
    if (pData) {
        if (mReading) {
            bits = pData[mOffset++];
            bits += static_cast<unsigned long>(pData[mOffset++]) << 8;
            bits += static_cast<unsigned long>(pData[mOffset++]) << 16;
            bits += static_cast<unsigned long>(pData[mOffset++]) << 24;
            bits += static_cast<unsigned long>(pData[mOffset++]) << 32;
            bits += static_cast<unsigned long>(pData[mOffset++]) << 40;
            bits += static_cast<unsigned long>(pData[mOffset++]) << 48;
            bits += static_cast<unsigned long>(pData[mOffset++]) << 56;
        } else {
            unsigned long value = bits;
            pData[mOffset++] = value;
            pData[mOffset++] = value >> 8;
            pData[mOffset++] = value >> 16;
            pData[mOffset++] = value >> 24;
            pData[mOffset++] = value >> 32;
            pData[mOffset++] = value >> 40;
            pData[mOffset++] = value >> 48;
            pData[mOffset++] = value >> 56;
        }
    } else { mOffset += 8; }
}

} }
