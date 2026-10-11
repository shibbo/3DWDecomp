#pragma once

#include <basis/seadTypes.h>

namespace nn::ui2d {
class Layout;
}  // namespace nn::ui2d

namespace fix {

/** @brief Keeps a layout text box whose text has to be re-laid out every frame (font fix-up). */
class TextBoxTextInfo {
public:
    void setTextBox(nn::ui2d::Layout* pLayout, const char* pPaneName, s32 type);
    void applyFix();

private:
    void* _0;
    void* _8;
    void* _10;
    void* _18;
};

static_assert(sizeof(TextBoxTextInfo) == 0x20);

}  // namespace fix
