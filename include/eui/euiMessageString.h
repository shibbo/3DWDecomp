#pragma once

#include <prim/seadSafeString.h>

namespace eui {

class MessageString {
public:
    struct Iterator {
        const char16_t* pText;
        u32 index;
    };

    MessageString();
    MessageString(int length, const char16_t* pText);
    MessageString(const char16_t* pBegin, const char16_t* pEnd);
    MessageString(const sead::WSafeString& rText);
    MessageString(const MessageString& rOther);
    MessageString& operator=(const MessageString& rOther);
    const char16_t& operator[](int index) const;
    Iterator begin() const;
    Iterator end() const;
    Iterator toIterator(int index) const;
    bool tryMakeTagStrippedString(sead::WBufferedSafeString* pOutput) const;
    int countPrintableStringLength() const;

    const char16_t* getText() const { return m_pText; }
    u32 getLength() const { return mLength; }

    /** @brief Reads a control tag and returns the character following it. */
    static const char16_t* readTag_(const char16_t* pCurrent, const char16_t** ppTag) {
        if (*pCurrent == 0xe) {
            *ppTag = pCurrent;
            return reinterpret_cast<const char16_t*>(
                reinterpret_cast<const char*>(pCurrent) + pCurrent[3] + 8);
        }

        if (*pCurrent == 0xf) {
            *ppTag = pCurrent;
            return pCurrent + 3;
        }

        *ppTag = nullptr;
        return pCurrent;
    }

private:

    const char16_t* m_pText;
    u32 mLength;
};

}  // namespace eui
