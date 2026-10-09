#pragma once
#include <nn/types.h>
namespace nn::ui2d {
class Layout;
class TextBox;
class TextSearcher {
public:
    struct TextInfo {
        const char16_t* pText;
        u32 length;
        int bufferLength;
        int bufferLengthOverride;
    };
    struct TextInfoUtf8 {
        const char* pText;
        u32 length;
        int bufferLength;
        int bufferLengthOverride;
    };
    virtual ~TextSearcher() {}
    virtual void SearchText(TextInfo* pInfo, const char* pId, Layout* pLayout,
                            TextBox* pTextBox, Layout* pRootLayout) = 0;
    // The default searcher supplies no UTF-8 text for any identifier or layout.
    virtual void SearchTextUtf8(TextInfoUtf8* pInfo, const char* pId, Layout* pLayout,
                                TextBox* pTextBox, Layout* pRootLayout) {}
};
}
