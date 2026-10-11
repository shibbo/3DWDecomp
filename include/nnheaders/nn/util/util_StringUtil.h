#pragma once

namespace nn {
namespace util {
template <typename T>
int Strnlen(const T* pStr, int count) {
    int length = 0;
    for (; length < count && pStr[length]; ++length) {
    }

    return length;
}

template <typename T>
int Strlcpy(T* pOutDst, const T* pSrc, int count) {
    auto pSrcEnd = pSrc;

    if (count > 0) {
        while (--count && *pSrcEnd) {
            *pOutDst++ = *pSrcEnd++;
        }
        *pOutDst = 0;
    }

    while (*pSrcEnd) {
        pSrcEnd++;
    }

    return static_cast<int>(pSrcEnd - pSrc);
}
};  // namespace util
};  // namespace nn