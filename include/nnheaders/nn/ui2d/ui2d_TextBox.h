#pragma once
#include <nn/font/font_DispStringBuffer.h>
#include <nn/font/font_TagProcessorBase.h>
#include <nn/ui2d/ui2d_Pane.h>
#include <nn/ui2d/ui2d_Resources.h>
#include <nn/ui2d/ui2d_TextSearcher.h>
namespace nn::font {
class Font;
}
namespace nn::ui2d {
struct BuildResultInformation;

/** @brief Text box block of a layout resource. */
struct ResTextBox : public ResPane {
    u16 textBufBytes;
    u16 textStrBytes;
    u16 materialIdx;
    u16 fontIdx;
    u8 textPosition;
    u8 textAlignment;
    u16 textBoxFlag;
    float italicRatio;
    u32 textStrOffset;
    nn::util::Unorm8x4 textCols[2];
    nn::util::Float2 fontSize;
    float charSpace;
    float lineSpace;
    u32 textIdOffset;
    nn::util::Float2 shadowOffset;
    nn::util::Float2 shadowScale;
    nn::util::Unorm8x4 shadowCols[2];
    float shadowItalicRatio;
    u32 lineWidthOffsetOffset;
    u32 perCharacterTransformOffset;
};
static_assert(sizeof(ResTextBox) == 0xa8, "ResTextBox size");
struct ResAnimationInfo;
struct ResAnimationTarget;

/** @brief Per-line offset and width tables used when a text box lays out each line separately. */
struct LineWidthOffset {
    float* pLineWidth;
    float* pLineOffset;
};

/** @brief Animation curve bound to one per-character transform channel. */
struct PerCharacterTransformCurveInfo {
    const ResAnimationTarget* pCurve;
    int curveType;
};

/** @brief Runtime state of the per-character transform animation of a text box. */
struct PerCharacterTransform {
    float offset;
    float width;
    nn::font::PerCharacterTransformInfo* pPerCharacterTransformInfos;
    u8 loopType;
    u8 originV;
    u8 curveCount;
    u8 padding;
    float originVOffset;
    float fixSpaceWidth;
    int fixSpaceOrigin;
    PerCharacterTransformCurveInfo curveInfos[1];
};

class TextBox : public Pane {
public:
    /** @brief Text source data handed from the constructor to InitializeString. */
    struct InitializeStringParam {
        u32 textBoxFlag;
        Layout* pRootLayout;
        union {
            const u16* pText;
            const char* pTextUtf8;
        };
        size_t allocStringLength;
        int resStringLength;
    };

    /** @brief Flag bits stored in mBits. */
    struct Bits {
        u16 textAlignment : 2;
        u16 isDirty : 1;
        u16 isShadowEnabled : 1;
        u16 isInvisibleBorderEnabled : 1;
        u16 isDoubleDrawnBorderEnabled : 1;
        u16 isWidthLimitEnabled : 1;
        u16 isPerCharacterTransformEnabled : 1;
        u16 isCenterCeilingEnabled : 1;
        u16 isPerCharacterTransformSplitByCharWidth : 1;
        u16 isPerCharacterTransformAutoShadowAlpha : 1;
        u16 isDrawFromRightToLeft : 1;
        u16 isPerCharacterTransformOriginToCenter : 1;
        u16 isPerCharacterTransformFixSpace : 1;
        u16 isLinefeedByCharacterHeightEnabled : 1;
        u16 isPerCharacterTransformSplitByCharWidthInsertSpace : 1;
    };

    TextBox();
    explicit TextBox(bool isUtf8);
    TextBox(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
            InitializeStringParam* pParam, const ResTextBox* pBaseBlock,
            const ResTextBox* pOverrideBlock, const BuildArgSet& rArgs);
    ~TextBox() override;
    NN_RUNTIME_TYPEINFO(Pane);
    void Finalize(nn::gfx::Device*) override;
    nn::util::Unorm8x4 GetVertexColor(int) const override;
    void SetVertexColor(int, const nn::util::Unorm8x4&) override;
    u8 GetVertexColorElement(int) const override;
    void SetVertexColorElement(int, u8) override;
    u32 GetMaterialCount() const override;
    Material* GetMaterial(int) const override;
    using Pane::GetMaterial;
    void GetSizeWithCaptureEffect(Size*) const override;
    void GetVertexPosWithCaptureEffect(nn::util::Float2*) const override;
    float GetItalicSize() const override;
    void Calculate(DrawInfo&, CalculateContext&, bool) override;
    void DrawSelf(DrawInfo&, nn::gfx::CommandBuffer&) override;
    void SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer&) const override;
    void LoadMtx(DrawInfo&) override;
    virtual void InitializeString(BuildResultInformation*, nn::gfx::Device*, const BuildArgSet&, const InitializeStringParam&);
    virtual void AllocateStringBuffer(nn::gfx::Device*, u16);
    virtual void AllocateStringBuffer(nn::gfx::Device*, u16, u16);
    virtual void FreeStringBuffer(nn::gfx::Device*);
    virtual u16 SetString(const u16*, u16);
    virtual u16 SetStringUtf8(const char*, u16);
    virtual u16 SetString(const u16*, u16, u16);
    virtual u16 SetStringUtf8(const char*, u16, u16);
    virtual nn::font::Rectangle GetTextDrawRect() const;
    virtual void SetupTextWriter(nn::font::TextWriterBase<u16>*) const;
    virtual void SetupTextWriterUtf8(nn::font::TextWriterBase<char>*) const;
    virtual bool InitializeStringWithTextSearcherInfo(nn::gfx::Device*, const BuildArgSet&, const TextSearcher::TextInfo&);
    virtual bool InitializeStringWithTextSearcherInfoUtf8(nn::gfx::Device*, const BuildArgSet&, const TextSearcher::TextInfoUtf8&);

    void Initialize();
    void InitializeMaterial();
    void InitializePerCharacterTransformCurves(const ResAnimationInfo* pInfo);
    void CopyLineWidthOffset(const TextBox& rOther);
    void CopyPerCharacterTransform(const TextBox& rOther);
    void CopyCommonImpl(const TextBox& rOther);
    void CopyImpl(const TextBox& rOther, nn::gfx::Device* pDevice);
    void CopyImpl(const TextBox& rOther, nn::gfx::Device* pDevice, u16 bufferLength);
    int GetDrawStringBufferLength() const;
    void SetMaterial(Material* pMaterial);
    void UnsetMaterial();
    bool IsSpreadPaneSizeOfItalicEnabled() const;
    void CalculateStringRect(nn::font::Rectangle* pRect) const;
    float GetAngleFromItalicRatio() const;
    void SetAngleToItalicRatio(float angle);
    float GetAngleFromShadowItalicRatio() const;
    void SetAngleToShadowItalicRatio(float angle);
    void SetFontInfoUtf8(nn::font::TextWriterBase<char>* pWriter) const;
    void SetFontInfo(nn::font::TextWriterBase<u16>* pWriter) const;
    nn::util::Float2 AdjustTextPos(const Size& rSize, bool isCeil) const;
    void SetupConstantBufferAdditionalContent(nn::font::ConstantBufferAdditionalContent& rContent) const;
    void BuildConstantBuffer(DrawInfo& rDrawInfo) const;
    void GetTextGlobalMtx(nn::util::MatrixT4x3fType* pMtx, const DrawInfo& rDrawInfo) const;
    template <typename CharType>
    void UpdatePerCharacterTransform(nn::font::TagProcessorBase<CharType>* pTagProcessor);
    bool CompareCopiedInstanceTest(const TextBox& rOther) const;
    u16 SetStringImpl(const u16* pStr, u16 dstIdx, int strLen);
    u16 SetStringImplUtf8(const char* pStr, u16 dstIdx, int strLen);
    void SetTextPos(nn::font::TextWriterBase<u16>* pWriter) const;
    void SetTextPosUtf8(nn::font::TextWriterBase<char>* pWriter) const;
    void SetPerCharacterTransform(int index, float value);
    float GetPerCharacterTransform(int index) const;
    float CalculateBeginTime(float stepTime, float totalWidth);
    bool ValidateNextPrintableChar(const char* current, const char* next);
    bool ValidateNextPrintableChar(const u16* current, const u16* next);
    u32 GetCharFromPointer(const char* text);
    u32 GetCharFromPointer(const u16* text);
    void ApplyPerCharacterTransformCurve(nn::font::PerCharacterTransformInfo* pInfo, float frame);
    template <typename CharType>
    float CalculateStepTime(float* pTotalWidth, nn::font::TagProcessorBase<CharType>* pTagProcessor);

    const u16* GetStringBuffer() const { return mTextBufferUtf16; }
    u16* GetStringBuffer() { return mTextBufferUtf16; }
    const char* GetStringBufferUtf8() const { return mTextBufferUtf8; }
    char* GetStringBufferUtf8() { return mTextBufferUtf8; }
    const nn::font::Font* GetFont() const;
    void SetFont(const nn::font::Font* pFont);
    void SetFontSize(const Size& rSize);
    u16 GetStringBufferLength() const;

    const Size& GetFontSize() const { return mFontSize; }
    u16 GetStringLength() const { return mTextLength; }
    const char* GetTextId() const { return mTextId; }
    float GetLineSpace() const { return mLineSpace; }
    float GetCharSpace() const { return mCharSpace; }
    float GetItalicRatio() const { return mItalicRatio; }
    float GetShadowItalicRatio() const { return mShadowItalicRatio; }
    u8 GetTextPositionH() const { return mTextPosition & 3; }
    u8 GetTextPositionV() const { return (mTextPosition >> 2) & 3; }
    int GetTextAlignment() const { return mBits.textAlignment; }
    bool IsUtf8() const { return mIsUtf8; }
    bool IsTextFlag12() const { return mBits.isPerCharacterTransformOriginToCenter; }
    bool IsShadowEnabled() const { return mBits.isShadowEnabled; }
    bool IsPerCharacterTransformEnabled() const { return mBits.isPerCharacterTransformEnabled; }
    bool IsWidthLimitEnabled() const { return mBits.isWidthLimitEnabled; }
    nn::font::TagProcessorBase<u16>* GetTagProcessor() const { return mTagProcessor; }

    /** @return Whether characters are spaced by their width with inserted spaces. */
    bool IsFixSpaceInsertSpaceMode() const {
        return mBits.isPerCharacterTransformOriginToCenter &&
               mBits.isPerCharacterTransformSplitByCharWidthInsertSpace;
    }

    /** @return Whether characters are spaced by the fixed space width. */
    bool IsFixSpaceMode() const {
        return mBits.isPerCharacterTransformOriginToCenter && mBits.isPerCharacterTransformFixSpace;
    }

    /** @return Whether the curve time is centered on each character's width. */
    bool IsSplitByCharWidthCenterMode() const {
        return mBits.isPerCharacterTransformSplitByCharWidth &&
               mBits.isPerCharacterTransformOriginToCenter;
    }

    /** @brief Marks the text box for a rebuild of its string buffer when isChanged is set. */
    void SetDirtyFlag(bool isChanged) { mBits.isDirty = mBits.isDirty || isChanged; }

    void SetTagProcessor(nn::font::TagProcessorBase<u16>* pTagProcessor) {
        bool isChanged = mTagProcessor != pTagProcessor;
        mBits.isDirty = mBits.isDirty || isChanged;

        if (isChanged) {
            mTagProcessor = pTagProcessor;
        }
    }

protected:
    u8 _d2[6];
    union {
        void* mTextBuffer;
        u16* mTextBufferUtf16;
        char* mTextBufferUtf8;
    };
    const char* mTextId;
    nn::util::Unorm8x4 mTextColors[2];
    const nn::font::Font* mFont;
    Size mFontSize;
    float mLineSpace;
    float mCharSpace;
    nn::font::TagProcessorBase<u16>* mTagProcessor;
    u16 mTextBufferLength;
    u16 mTextLength;
    union {
        u16 mTextFlags;
        Bits mBits;
    };
    u8 mTextPosition;
    bool mIsUtf8;
    float mItalicRatio;
    nn::util::Float2 mShadowOffset;
    nn::util::Float2 mShadowScale;
    nn::util::Unorm8x4 mShadowTopColor;
    nn::util::Unorm8x4 mShadowBottomColor;
    float mShadowItalicRatio;
    LineWidthOffset* mLineWidthOffset;
    Material* mMaterial;
    nn::font::DispStringBuffer* mDispStringBuffer;
    PerCharacterTransform* mPerCharacterTransform;
};
static_assert(sizeof(TextBox) == 0x158, "TextBox size");
}
