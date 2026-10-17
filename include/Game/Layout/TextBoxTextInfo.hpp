#pragma once
namespace nn { namespace ui2d { class Layout; class TextBox; class Pane; } }
namespace fix {
class TextBoxTextInfo {
public:
    void setTextBox(nn::ui2d::Layout*, const char*, int);
    void applyFix();
private:
    nn::ui2d::TextBox* mTextBox;
    float mWidth;
    float mHeight;
    float mFontWidth;
    float mFontHeight;
    nn::ui2d::Pane* mRootPane;
};
static_assert(sizeof(TextBoxTextInfo) == 0x20);
}
