#pragma once
#include <eui/euiTextBoxEx.h>
namespace nn::font { class ScalableFont; }
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
    void reserveRegisterGlyphs_(ScalableFontMgr* pMgr);
    const nn::font::ScalableFont* getScalableFont_() const;
    const char16_t* getGlyphText_() const;
    void markGlyphsReady_();
    /** @brief Destroys the text box and its inherited pane resources. */
    ~ScalableFontTextBoxEx() override = default;
    NN_RUNTIME_TYPEINFO(TextBoxEx);
    void Calculate(nn::ui2d::DrawInfo&, CalculateContext&, bool) override;
    void DrawSelf(nn::ui2d::DrawInfo&, nn::gfx::CommandBuffer&) override;
    u16 setStringNoPreproces(const char16_t*, u16) override;
    void doPreprocess_(sead::WBufferedSafeString*, u32*, u32*, const char16_t*, u32, int, bool, void*) override;
    ScalableFontMgr* mFontMgr;
    u32 mGlyphState;
    u32 mGlyphUpdateCount;
    ScalableFontTextBoxEx* mNextPendingTextBox;
};

static_assert(sizeof(ScalableFontTextBoxEx) == 0x178, "ScalableFontTextBoxEx size");
}
