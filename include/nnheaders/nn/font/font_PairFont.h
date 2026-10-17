#pragma once

#include <nn/font/font_Font.h>

namespace nn {
namespace font {

class PairFont : public Font {
public:
    NN_RUNTIME_TYPEINFO(Font);

    PairFont();
    PairFont(Font* pFirst, Font* pSecond);
    ~PairFont() override;

    void SetFont(Font* pFirst, Font* pSecond);

    int GetWidth() const override;
    int GetHeight() const override;
    int GetAscent() const override;
    int GetDescent() const override;
    int GetBaselinePos() const override;
    int GetCellHeight() const override;
    int GetCellWidth() const override;
    int GetMaxCharWidth() const override;
    FontType GetType() const override;
    TexFmt GetTextureFormat() const override;
    int GetLineFeed() const override;
    const CharWidths GetDefaultCharWidths() const override;
    void SetDefaultCharWidths(const CharWidths& rWidths) override;
    bool SetAlternateChar(uint32_t c) override;
    void SetLineFeed(int linefeed) override;
    int GetCharWidth(uint32_t c) const override;
    const CharWidths GetCharWidths(uint32_t c) const override;
    int GetGlyph(Glyph* pGlyph, uint32_t c) const override;
    bool HasGlyph(uint32_t c) const override;
    bool IsGlyphExistInFont(uint32_t c) const override;
    int GetKerning(uint32_t c0, uint32_t c1) const override;
    CharacterCode GetCharacterCode() const override;
    void SetLinearFilterEnabled(bool atSmall, bool atLarge) override;
    bool IsLinearFilterEnabledAtSmall() const override;
    bool IsLinearFilterEnabledAtLarge() const override;
    uint32_t GetTextureWrapFilterValue() const override;
    bool IsColorBlackWhiteInterpolationEnabled() const override;
    void SetColorBlackWhiteInterpolationEnabled(bool isEnabled) override;

    /**
     * Checks whether both fonts have border glyphs.
     * @return whether both fonts have border glyphs
     */
    bool IsBorderAvailable() const override {
        return m_pFirstFont->IsBorderAvailable() && m_pSecondFont->IsBorderAvailable();
    }

    bool IsBorderEffectEnabled() const override;
    void GetAlternateCharGlyph(Glyph* pGlyph, uint32_t c) const override;

    /** @return The font searched first. */
    Font* GetFirstFont() const { return m_pFirstFont; }

    /** @return The font searched when the first one has no glyph. */
    Font* GetSecondFont() const { return m_pSecondFont; }

private:
    Font* m_pFirstFont;
    Font* m_pSecondFont;
    bool m_IsAlternateCharInFirstFont;
};
static_assert(sizeof(PairFont) == 0x28);

}  // namespace font
}  // namespace nn
