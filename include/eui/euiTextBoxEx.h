#pragma once
#include <nn/ui2d/ui2d_TextBox.h>
#include <eui/euiControlCreator.h>
#include <prim/seadSafeString.h>
namespace eui {
class LayoutEx;
class MessageString;
class LetterAnimControl;
class TextBoxEx : public nn::ui2d::TextBox {
public:
    TextBoxEx();
    TextBoxEx(const nn::ui2d::ResTextBox* pResource, const nn::ui2d::ResTextBox* pOverride,
              const nn::ui2d::BuildArgSet& rArgs, InitializeStringParam* pParam);
    TextBoxEx(const TextBoxEx& rOther, LayoutEx* pLayout);
    ~TextBoxEx() override = default;
    NN_RUNTIME_TYPEINFO(nn::ui2d::TextBox);
    void InitializeString(nn::ui2d::BuildResultInformation*, nn::gfx::Device*, const nn::ui2d::BuildArgSet&, const InitializeStringParam&) override;
    u16 SetString(const u16*, u16) override;
    u16 SetString(const u16*, u16, u16) override;
    bool InitializeStringWithTextSearcherInfo(nn::gfx::Device*, const nn::ui2d::BuildArgSet&, const nn::ui2d::TextSearcher::TextInfo&) override;
    virtual u16 setStringNoPreproces(const char16_t*, u16);
    virtual u16 doSetString_(const char16_t*, u16, bool*, int, bool, void*);
    virtual void doPreprocess_(sead::WBufferedSafeString*, u32*, u32*, const char16_t*, u32, int, bool, void*);
    virtual void adjustText_(LayoutEx*);
    virtual bool getTextAdjustMinScale_(float*);
    virtual bool isWordwrapOn_();
    virtual bool isTextChangeOn_() const;
    virtual bool getLetterAnimSpeed_(float*);
    virtual void createLetterAnimControl_(ControlList*, LayoutEx*);
    u16 setMessageString(const MessageString& rText, void* pUserData);
    u16 setMessageStringWithPage(const MessageString& rText, bool* pHasNext, u32 page, bool flag, void* pUserData);
    u16 setStringWithPage(const char16_t* pText, u16 length, bool* pHasNext, u32 page, bool flag, void* pUserData);
    float calcStringWidth_();
    LetterAnimControl* mLetterAnimControl;
};

static_assert(sizeof(TextBoxEx) == 0x160, "TextBoxEx size");
}
