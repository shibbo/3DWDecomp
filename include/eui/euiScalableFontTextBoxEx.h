#pragma once
#include <eui/euiTextBoxEx.h>
namespace eui {
class ScalableFontMgr;
class ScalableFontTextBoxEx : public TextBoxEx {
public:
    explicit ScalableFontTextBoxEx(ScalableFontMgr* pMgr);
    ScalableFontTextBoxEx(const nn::ui2d::ResTextBox* pResource,
                          const nn::ui2d::ResTextBox* pOverride,
                          const nn::ui2d::BuildArgSet& rArgs, InitializeStringParam* pParam);
    ScalableFontTextBoxEx(const ScalableFontTextBoxEx& rOther, LayoutEx* pLayout);
    ScalableFontTextBoxEx* registerGlyphsAndGetNext(ScalableFontMgr* pMgr);
    ~ScalableFontTextBoxEx() override = default;
    NN_RUNTIME_TYPEINFO(TextBoxEx);
    void Calculate(nn::ui2d::DrawInfo&, CalculateContext&, bool) override;
    void DrawSelf(nn::ui2d::DrawInfo&, nn::gfx::CommandBuffer&) override;
    u16 setStringNoPreproces(const char16_t*, u16) override;
    void doPreprocess_(sead::WBufferedSafeString*, u32*, u32*, const char16_t*, u32, int, bool, void*) override;
    ScalableFontMgr* mFontMgr;
    u32 mGlyphState;
    u32 _16c;
    void* _170;
};

static_assert(sizeof(ScalableFontTextBoxEx) == 0x178, "ScalableFontTextBoxEx size");
}
