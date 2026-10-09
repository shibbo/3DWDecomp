#include <nn/ui2d/ui2d_TextBox.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <nn/font/font_Font.h>
#include <nn/font/font_RectDrawer.h>
#include <nn/font/font_TextWriterBase.h>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/ui2d/ui2d_AnimResource.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_PaneEffect.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/util.h>
#include <nn/util/util_Arithmetic.h>

namespace nn::ui2d {
namespace {
typedef nn::font::TextWriterBase<char> TextWriterUtf8;
typedef nn::font::TextWriterBase<u16> TextWriterUtf16;

/** @brief Number of entries in each line width/offset table. */
const int LineOffsetCountMax = 16;

/** @brief Resource version that added the vertical origin offset to per-character transforms. */
const u32 PerCharacterTransformExtendedVersion = 0x8010000;

/** @brief Resource flags of a text box block. */
enum TextBoxFlag {
    TextBoxFlag_ShadowEnabled = 1 << 0,
    TextBoxFlag_ForceAssignTextLength = 1 << 1,
    TextBoxFlag_InvisibleBorderEnabled = 1 << 2,
    TextBoxFlag_DoubleDrawnBorderEnabled = 1 << 3,
    TextBoxFlag_PerCharacterTransformEnabled = 1 << 4,
    TextBoxFlag_CenterCeilingEnabled = 1 << 5,
    TextBoxFlag_LineWidthOffsetEnabled = 1 << 6,
    TextBoxFlag_ExtendedTagEnabled = 1 << 7,
    TextBoxFlag_PerCharacterTransformSplitByCharWidth = 1 << 8,
    TextBoxFlag_PerCharacterTransformAutoShadowAlpha = 1 << 9,
    TextBoxFlag_DrawFromRightToLeft = 1 << 10,
    TextBoxFlag_PerCharacterTransformOriginToCenter = 1 << 11,
    TextBoxFlag_KeepingFontScaleEnabled = 1 << 12,
    TextBoxFlag_PerCharacterTransformFixSpace = 1 << 13,
    TextBoxFlag_PerCharacterTransformSplitByCharWidthInsertSpace = 1 << 14,
};

/** @brief Kinds of per-character transform curves. */
enum PerCharacterTransformCurveType {
    PerCharacterTransformCurveType_TranslateX,
    PerCharacterTransformCurveType_TranslateY,
    PerCharacterTransformCurveType_TranslateZ,
    PerCharacterTransformCurveType_RotateX,
    PerCharacterTransformCurveType_RotateY,
    PerCharacterTransformCurveType_RotateZ,
    PerCharacterTransformCurveType_LeftTopR,
    PerCharacterTransformCurveType_LeftTopA = 9,
    PerCharacterTransformCurveType_LeftBottomR,
    PerCharacterTransformCurveType_LeftBottomA = 13,
    PerCharacterTransformCurveType_ScaleX,
};

/** @brief Per-character transform block of a text box resource. */
struct ResPerCharacterTransform {
    float evalTimeOffset;
    float evalTimeWidth;
    u8 loopType;
    u8 originV;
    u8 hasAnimationInfo;
    u8 padding;
    float originVOffset;
    float fixSpaceWidth;
    u8 fixSpaceOrigin;
};

/**
 * @brief Builds a color from its channels.
 * @param r Red channel.
 * @param g Green channel.
 * @param b Blue channel.
 * @param a Alpha channel.
 * @return The color.
 */
inline nn::util::Unorm8x4 MakeColor(u8 r, u8 g, u8 b, u8 a) {
    nn::util::Unorm8x4 color = {{r, g, b, a}};
    return color;
}

/**
 * @brief Returns whether the material stores its white color as floats.
 * @param rMaterial Material to check.
 * @return Whether the white color is a float color.
 */
inline bool IsWhiteColorFloat(const Material& rMaterial) {
    return (rMaterial.mOwnershipFlags & 0x10) != 0;
}

/**
 * @brief Returns whether the material stores its black color as floats.
 * @param rMaterial Material to check.
 * @return Whether the black color is a float color.
 */
inline bool IsBlackColorFloat(const Material& rMaterial) {
    return (rMaterial.mOwnershipFlags & 0x8) != 0;
}

/**
 * @brief Converts a byte color channel to a normalized float.
 * @param value Channel value.
 * @return The channel in [0, 1].
 */
inline float ToFloatColor(u8 value) {
    return static_cast<float>(value) * (1.0f / 255.0f);
}

/**
 * @brief Returns the size of a per-character transform with curveCount curves.
 * @param curveCount Number of animation curves.
 * @return The allocation size in bytes.
 */
inline size_t CalculatePerCharacterTransformSize(int curveCount) {
    return curveCount > 1 ? sizeof(PerCharacterTransform) +
                                sizeof(PerCharacterTransformCurveInfo) * (curveCount - 1) :
                            sizeof(PerCharacterTransform);
}

/**
 * @brief Counts the UTF-16 code units of a null-terminated string.
 * @param pStr String to measure.
 * @return The length of the string.
 */
inline size_t GetUtf16StringLength(const u16* pStr) {
    size_t length = 0;

    if (pStr[0] != 0) {
        do {
            length++;
        } while (pStr[length] != 0);
    }

    return length;
}

/**
 * @brief Counts the characters of a null-terminated UTF-8 string.
 * @param pStr String to measure.
 * @return The number of characters.
 */
inline int GetUtf8CharacterCount(const char* pStr) {
    int count = 0;

    if (*pStr != '\0') {
        do {
            char character[4];
            nn::util::PickOutCharacterFromUtf8String(character, &pStr);
            count++;
        } while (*pStr != '\0');
    }

    return count;
}

/**
 * @brief Resets a per-character transform to the identity transform with white vertex colors.
 * @param pInfo Transform to reset.
 */
inline void InitializePerCharacterTransformInfo(nn::font::PerCharacterTransformInfo* pInfo) {
    pInfo->scale[0] = 1.0f;
    pInfo->scale[1] = 1.0f;

    for (int i = 0; i < 3; i++) {
        pInfo->rotationCos[i] = 1.0f;
        pInfo->rotationSin[i] = 0.0f;
        pInfo->translation[i] = 0.0f;
    }

    pInfo->lt = MakeColor(0xff, 0xff, 0xff, 0xff);
    pInfo->lb = MakeColor(0xff, 0xff, 0xff, 0xff);
}
}  // namespace

/** @brief Creates an empty UTF-16 text box with a default material. */
TextBox::TextBox() : Pane() {
    Initialize();
    InitializeMaterial();
}

/** @brief Resets every text box member to its empty state. */
void TextBox::Initialize() {
    mTextBuffer = nullptr;
    mTextId = nullptr;
    mTextColors[0] = MakeColor(0xff, 0xff, 0xff, 0xff);
    mTextColors[1] = MakeColor(0xff, 0xff, 0xff, 0xff);
    mFont = nullptr;
    mFontSize.width = 0.0f;
    mFontSize.height = 0.0f;
    mLineSpace = 0.0f;
    mCharSpace = 0.0f;
    mTagProcessor = nullptr;
    mTextBufferLength = 0;
    mTextLength = 0;
    mTextFlags = 0;
    mTextPosition = 0;
    mIsUtf8 = false;
    mItalicRatio = 0.0f;
    mShadowOffset.x = 0.0f;
    mShadowOffset.y = 0.0f;
    mShadowScale.x = 1.0f;
    mShadowScale.y = 1.0f;
    mShadowTopColor = MakeColor(0, 0, 0, 0xff);
    mShadowBottomColor = MakeColor(0, 0, 0, 0xff);
    mBits.isShadowEnabled = false;
    mShadowItalicRatio = 0.0f;
    mLineWidthOffset = nullptr;
    mMaterial = nullptr;
    mDispStringBuffer = nullptr;
    mPerCharacterTransform = nullptr;
}

/** @brief Creates the default material used to draw the text. */
void TextBox::InitializeMaterial() {
    mMaterial = Layout::NewObj<Material>();

    if (mMaterial != nullptr) {
        mMaterial->SetTextureNum(0);
        mMaterial->ReserveMem(0, 0, 0, 0, false, 0, false, 0, false, false, false, 0, 0);
    }
}

/**
 * @brief Creates an empty text box with a default material.
 * @param isUtf8 Whether the string buffer holds UTF-8 instead of UTF-16 text.
 */
TextBox::TextBox(bool isUtf8) : Pane() {
    Initialize();
    InitializeMaterial();
    mIsUtf8 = isUtf8;
}

/**
 * @brief Builds a text box from its layout resource.
 * @param pResult Receives the required buffer sizes.
 * @param pDevice Device that owns the graphics resources.
 * @param pParam Receives the string data used by InitializeString.
 * @param pBaseBlock Text box resource.
 * @param pOverrideBlock Optional resource overriding parts of pBaseBlock.
 * @param rArgs Build arguments.
 */
TextBox::TextBox(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                 InitializeStringParam* pParam, const ResTextBox* pBaseBlock,
                 const ResTextBox* pOverrideBlock, const BuildArgSet& rArgs)
    : Pane(pResult, pDevice, pBaseBlock, rArgs) {
    const ResTextBox* pBlock;
    const ResTextBox* pTextBlock;
    const ResTextBox* pPositionBlock;
    const ResTextBox* pShadowBlock;
    const BuildResSet* pBuildResSet;
    const BuildResSet* pTextBuildResSet;

    if (pOverrideBlock == nullptr) {
        pBlock = pBaseBlock;
        pTextBlock = pBaseBlock;
        pPositionBlock = pBaseBlock;
        pShadowBlock = pBaseBlock;
        pBuildResSet = rArgs.pCurrentBuildResSet;
        pTextBuildResSet = rArgs.pCurrentBuildResSet;
    } else if (rArgs.overridePaneUsageFlag == 0 && rArgs.overrideUsageFlag == 0) {
        pBlock = pOverrideBlock;
        pTextBlock = pOverrideBlock;
        pPositionBlock = pOverrideBlock;
        pShadowBlock = pOverrideBlock;
        pBuildResSet = rArgs.pOverrideBuildResSet;
        pTextBuildResSet = rArgs.pOverrideBuildResSet;
    } else {
        const bool isTextOverridden = (rArgs.overridePaneUsageFlag & 1) != 0;
        pBlock = pBaseBlock;
        pBuildResSet = rArgs.pCurrentBuildResSet;
        pTextBlock = isTextOverridden ? pOverrideBlock : pBaseBlock;
        pTextBuildResSet =
            isTextOverridden ? rArgs.pOverrideBuildResSet : rArgs.pCurrentBuildResSet;
        pPositionBlock = (rArgs.overridePaneUsageFlag & 2) != 0 ? pOverrideBlock : pBaseBlock;
        pShadowBlock = (rArgs.overridePaneUsageFlag & 4) != 0 ? pOverrideBlock : pBaseBlock;
    }

    Initialize();

    u32 allocStringLength = pBlock->textBufBytes / sizeof(u16);
    const u16 shadowTextBoxFlag = pShadowBlock->textBoxFlag;

    if (allocStringLength > 0) {
        allocStringLength -= 1;
    }

    int resStringLength = pTextBlock->textStrBytes / 2;

    if (resStringLength > 0) {
        resStringLength -= 1;
    }

    mTextColors[0] = pBlock->textCols[0];
    mTextColors[1] = pBlock->textCols[1];
    mTextPosition = pPositionBlock->textPosition;
    mBits.textAlignment = pPositionBlock->textAlignment;
    mCharSpace = pBlock->charSpace;
    mLineSpace = pBlock->lineSpace;
    mItalicRatio = pBlock->italicRatio;
    mBits.isCenterCeilingEnabled = (pBlock->textBoxFlag & TextBoxFlag_CenterCeilingEnabled) != 0;
    mBits.isDrawFromRightToLeft = (pBlock->textBoxFlag & TextBoxFlag_DrawFromRightToLeft) != 0;
    mBits.isShadowEnabled = (shadowTextBoxFlag & TextBoxFlag_ShadowEnabled) != 0;
    const u16 textBoxFlag = pBlock->textBoxFlag;
    const float shadowOffsetX = pShadowBlock->shadowOffset.x;
    const float shadowOffsetY = pShadowBlock->shadowOffset.y;
    mShadowOffset.x = shadowOffsetX;
    mShadowOffset.y = shadowOffsetY;
    const float shadowScaleX = pShadowBlock->shadowScale.x;
    const float shadowScaleY = pShadowBlock->shadowScale.y;
    mShadowScale.x = shadowScaleX;
    mShadowScale.y = shadowScaleY;
    mShadowTopColor = pShadowBlock->shadowCols[0];
    mShadowBottomColor = pShadowBlock->shadowCols[1];
    mShadowItalicRatio = pShadowBlock->shadowItalicRatio;
    mBits.isInvisibleBorderEnabled =
        (pBlock->textBoxFlag & TextBoxFlag_InvisibleBorderEnabled) != 0;
    mBits.isDoubleDrawnBorderEnabled =
        (pBlock->textBoxFlag & TextBoxFlag_DoubleDrawnBorderEnabled) != 0;
    mBits.isPerCharacterTransformEnabled =
        (pBlock->textBoxFlag & TextBoxFlag_PerCharacterTransformEnabled) != 0;
    mBits.isPerCharacterTransformSplitByCharWidth =
        (pBlock->textBoxFlag & TextBoxFlag_PerCharacterTransformSplitByCharWidth) != 0;
    mBits.isPerCharacterTransformAutoShadowAlpha =
        (pBlock->textBoxFlag & TextBoxFlag_PerCharacterTransformAutoShadowAlpha) != 0;
    mBits.isPerCharacterTransformOriginToCenter =
        (pBlock->textBoxFlag & TextBoxFlag_PerCharacterTransformOriginToCenter) != 0;
    mBits.isPerCharacterTransformFixSpace =
        (pBlock->textBoxFlag & TextBoxFlag_PerCharacterTransformFixSpace) != 0;
    mBits.isPerCharacterTransformSplitByCharWidthInsertSpace =
        (pBlock->textBoxFlag & TextBoxFlag_PerCharacterTransformSplitByCharWidthInsertSpace) != 0;
    mBits.isLinefeedByCharacterHeightEnabled = rArgs._D2;
    mBits.isWidthLimitEnabled = true;

    if ((textBoxFlag & TextBoxFlag_LineWidthOffsetEnabled) != 0 &&
        pBlock->lineWidthOffsetOffset != 0) {
        const u8* pData = reinterpret_cast<const u8*>(pBlock) + pBlock->lineWidthOffsetOffset;
        const u8 lineCount = *pData;
        const float* pValues = reinterpret_cast<const float*>(pData + 1);

        mLineWidthOffset =
            static_cast<LineWidthOffset*>(Layout::AllocateMemory(sizeof(LineWidthOffset)));
        mLineWidthOffset->pLineOffset =
            static_cast<float*>(Layout::AllocateMemory(sizeof(float) * LineOffsetCountMax));
        mLineWidthOffset->pLineWidth =
            static_cast<float*>(Layout::AllocateMemory(sizeof(float) * LineOffsetCountMax));

        for (int i = 0; i < LineOffsetCountMax; i++) {
            if (i < lineCount) {
                mLineWidthOffset->pLineOffset[i] = *pValues++;
            } else {
                mLineWidthOffset->pLineOffset[i] = 0.0f;
            }
        }

        for (int i = 0; i < LineOffsetCountMax; i++) {
            if (i < lineCount) {
                mLineWidthOffset->pLineWidth[i] = *pValues++;
            } else {
                mLineWidthOffset->pLineWidth[i] = 0.0f;
            }
        }
    } else {
        mLineWidthOffset = nullptr;
    }

    if ((textBoxFlag & TextBoxFlag_ExtendedTagEnabled) != 0) {
        if (rArgs.isUtf8) {
            mTagProcessor = reinterpret_cast<nn::font::TagProcessorBase<u16>*>(
                TextWriterUtf8::GetExtendedTagProcessor());
        } else {
            mTagProcessor = TextWriterUtf16::GetExtendedTagProcessor();
        }
    }

    if (mBits.isPerCharacterTransformEnabled && pBlock->perCharacterTransformOffset != 0) {
        const ResPerCharacterTransform* pResTransform =
            reinterpret_cast<const ResPerCharacterTransform*>(
                reinterpret_cast<const u8*>(pBlock) + pBlock->perCharacterTransformOffset);
        const ResAnimationInfo* pAnimationInfo = nullptr;
        size_t size = sizeof(PerCharacterTransform);

        if (pResTransform->hasAnimationInfo != 0) {
            pAnimationInfo = reinterpret_cast<const ResAnimationInfo*>(
                reinterpret_cast<const u8*>(pResTransform) +
                (rArgs.resourceVersion < PerCharacterTransformExtendedVersion ? 0xc : 0x20));

            if (pAnimationInfo->count != 0) {
                size = sizeof(PerCharacterTransform) +
                       sizeof(PerCharacterTransformCurveInfo) * (pAnimationInfo->count - 1);
            }
        }

        mPerCharacterTransform = static_cast<PerCharacterTransform*>(Layout::AllocateMemory(size));
        mPerCharacterTransform->offset = pResTransform->evalTimeOffset;
        mPerCharacterTransform->width = pResTransform->evalTimeWidth;
        mPerCharacterTransform->pPerCharacterTransformInfos = nullptr;
        mPerCharacterTransform->loopType = pResTransform->loopType;
        mPerCharacterTransform->originV = pResTransform->originV;

        if (rArgs.resourceVersion < PerCharacterTransformExtendedVersion) {
            mPerCharacterTransform->originVOffset = 0.0f;
            mPerCharacterTransform->fixSpaceWidth = 0.0f;
            mPerCharacterTransform->fixSpaceOrigin = 0;
        } else {
            mPerCharacterTransform->originVOffset = pResTransform->originVOffset;
            mPerCharacterTransform->fixSpaceWidth = pResTransform->fixSpaceWidth;
            mPerCharacterTransform->fixSpaceOrigin = pResTransform->fixSpaceOrigin;
        }

        if (pAnimationInfo != nullptr) {
            mPerCharacterTransform->curveCount = pAnimationInfo->count;
            InitializePerCharacterTransformCurves(pAnimationInfo);
        } else {
            mPerCharacterTransform->curveCount = 0;
        }
    }

    const char* pFontName = pBuildResSet->pFontList->GetFontName(pBlock->fontIdx);
    mFont = pBuildResSet->pResAccessor->AcquireFont(pDevice, pFontName);

    if ((pBlock->textBoxFlag & TextBoxFlag_KeepingFontScaleEnabled) != 0) {
        mFontSize.width = pBlock->fontSize.x * mFont->GetWidth();
        mFontSize.height = pBlock->fontSize.y * mFont->GetHeight();
    } else {
        mFontSize.width = pBlock->fontSize.x;
        mFontSize.height = pBlock->fontSize.y;
    }

    const ResMaterial* pResMaterial =
        detail::GetResMaterial(rArgs.pCurrentBuildResSet, pBaseBlock->materialIdx);
    const ResMaterial* pOverrideResMaterial = nullptr;

    if (pOverrideBlock != nullptr) {
        pOverrideResMaterial =
            detail::GetResMaterial(rArgs.pOverrideBuildResSet, pOverrideBlock->materialIdx);
    }

    mMaterial = Layout::NewObj<Material>(pResult, pDevice, pResMaterial, pOverrideResMaterial, rArgs);

    if (pTextBlock->textIdOffset != 0) {
        mTextId = reinterpret_cast<const char*>(pTextBlock) + pTextBlock->textIdOffset;
    }

    pParam->textBoxFlag = pBlock->textBoxFlag;
    pParam->pRootLayout = pTextBuildResSet->pLayout;
    pParam->pTextUtf8 = reinterpret_cast<const char*>(pTextBlock) + pTextBlock->textStrOffset;
    pParam->allocStringLength = allocStringLength;
    pParam->resStringLength = resStringLength;
    mIsUtf8 = rArgs.isUtf8;

    if (IsPaneEffectEnabled()) {
        GetPaneEffectInstance()->SetSpreadPaneSizeOfItalicEnabled(rArgs._DC);
    }
}

/**
 * @brief Binds the per-character transform curves of an animation info block.
 * @param pInfo Animation info holding the curves.
 */
void TextBox::InitializePerCharacterTransformCurves(const ResAnimationInfo* pInfo) {
    for (int i = 0; i < pInfo->count; i++) {
        const ResAnimationTarget* pTarget = pInfo->GetTarget(i);
        mPerCharacterTransform->curveInfos[i].pCurve = pTarget;
        mPerCharacterTransform->curveInfos[i].curveType = pTarget->target;
    }
}

/**
 * @brief Allocates the string buffer and sets the initial text, asking the text searcher first.
 * @param pResult Receives the required constant buffer size.
 * @param pDevice Device that owns the graphics resources.
 * @param rArgs Build arguments.
 * @param rParam String data collected by the constructor.
 */
void TextBox::InitializeString(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                               const BuildArgSet& rArgs, const InitializeStringParam& rParam) {
    bool isSearched = false;

    if (rArgs.pTextSearcher != nullptr) {
        if (rArgs.isUtf8) {
            TextSearcher::TextInfoUtf8 info;
            info.pText = nullptr;
            info.length = 0;
            info.bufferLength = 0;
            info.bufferLengthOverride = -1;

            if ((rParam.textBoxFlag & TextBoxFlag_ForceAssignTextLength) != 0) {
                info.bufferLengthOverride = rParam.allocStringLength;
            }

            rArgs.pTextSearcher->SearchTextUtf8(&info, mTextId, rArgs.m_pPartsLayout, this,
                                                rParam.pRootLayout);
            isSearched = InitializeStringWithTextSearcherInfoUtf8(pDevice, rArgs, info);
        } else {
            TextSearcher::TextInfo info;
            info.pText = nullptr;
            info.length = 0;
            info.bufferLength = 0;
            info.bufferLengthOverride = -1;

            if ((rParam.textBoxFlag & TextBoxFlag_ForceAssignTextLength) != 0) {
                info.bufferLengthOverride = rParam.allocStringLength;
            }

            rArgs.pTextSearcher->SearchText(&info, mTextId, rArgs.m_pPartsLayout, this,
                                            rParam.pRootLayout);
            isSearched = InitializeStringWithTextSearcherInfo(pDevice, rArgs, info);
        }
    }

    if (!isSearched) {
        AllocateStringBuffer(pDevice,
                             std::max(rParam.allocStringLength,
                                      static_cast<size_t>(rParam.resStringLength)));
    }

    if (rParam.resStringLength > 0 && !(isSearched | (mTextBuffer == nullptr))) {
        if (rArgs.isUtf8) {
            SetStringUtf8(rParam.pTextUtf8, 0, rParam.resStringLength);
        } else {
            SetString(rParam.pText, 0, rParam.resStringLength);
        }
    }

    nn::font::DispStringBuffer::InitializeArg arg;
    arg.charCountMax = GetDrawStringBufferLength();
    arg.isShadowEnabled = mBits.isShadowEnabled;
    arg.isDoubleDrawnBorder = mBits.isDoubleDrawnBorderEnabled;
    arg.isPerCharacterTransformEnabled = mBits.isPerCharacterTransformEnabled;
    arg.isPerCharacterTransformAutoShadowAlpha = mBits.isPerCharacterTransformAutoShadowAlpha;
    size_t constantBufferSize =
        nn::font::DispStringBuffer::GetRequiredConstantBufferSize(pDevice, arg);

    if (pResult != nullptr) {
        pResult->_8 += GetAlignedBufferSize(pDevice, nn::gfx::GpuAccess_ConstantBuffer,
                                            constantBufferSize);
    }
}

/**
 * @brief Sets the UTF-16 text found by the text searcher.
 * @param pDevice Device that owns the graphics resources.
 * @param rArgs Build arguments.
 * @param rInfo Text returned by the searcher.
 * @return Whether the searcher supplied a text.
 */
bool TextBox::InitializeStringWithTextSearcherInfo(nn::gfx::Device* pDevice,
                                                   const BuildArgSet& rArgs,
                                                   const TextSearcher::TextInfo& rInfo) {
    if (rInfo.pText == nullptr) {
        return false;
    }

    u16 length = rInfo.length;

    if (length == 0) {
        length = GetUtf16StringLength(reinterpret_cast<const u16*>(rInfo.pText));
    }

    if (rInfo.bufferLength != 0) {
        AllocateStringBuffer(pDevice, rInfo.bufferLength);
    } else {
        AllocateStringBuffer(pDevice, length);
    }

    if (mTextBuffer != nullptr) {
        SetString(reinterpret_cast<const u16*>(rInfo.pText), 0, length);
    }

    return true;
}

/**
 * @brief Sets the UTF-8 text found by the text searcher.
 * @param pDevice Device that owns the graphics resources.
 * @param rArgs Build arguments.
 * @param rInfo Text returned by the searcher.
 * @return Whether the searcher supplied a text.
 */
bool TextBox::InitializeStringWithTextSearcherInfoUtf8(nn::gfx::Device* pDevice,
                                                       const BuildArgSet& rArgs,
                                                       const TextSearcher::TextInfoUtf8& rInfo) {
    if (rInfo.pText == nullptr) {
        return false;
    }

    u16 length = rInfo.length;

    if (length == 0) {
        length = GetUtf8CharacterCount(rInfo.pText);
    }

    if (rInfo.bufferLength != 0) {
        AllocateStringBuffer(pDevice, rInfo.bufferLength);
    } else {
        AllocateStringBuffer(pDevice, length);
    }

    if (mTextBuffer != nullptr) {
        SetStringUtf8(rInfo.pText, 0, length);
    }

    return true;
}

/**
 * @brief Copies the line width and offset tables of another text box.
 * @param rOther Text box to copy from.
 */
void TextBox::CopyLineWidthOffset(const TextBox& rOther) {
    if (rOther.mLineWidthOffset == nullptr) {
        return;
    }

    mLineWidthOffset =
        static_cast<LineWidthOffset*>(Layout::AllocateMemory(sizeof(LineWidthOffset)));
    mLineWidthOffset->pLineOffset =
        static_cast<float*>(Layout::AllocateMemory(sizeof(float) * LineOffsetCountMax));
    mLineWidthOffset->pLineWidth =
        static_cast<float*>(Layout::AllocateMemory(sizeof(float) * LineOffsetCountMax));

    for (int i = 0; i < LineOffsetCountMax; i++) {
        mLineWidthOffset->pLineOffset[i] = rOther.mLineWidthOffset->pLineOffset[i];
        mLineWidthOffset->pLineWidth[i] = rOther.mLineWidthOffset->pLineWidth[i];
    }
}

/**
 * @brief Copies the per-character transform of another text box without its transform buffer.
 * @param rOther Text box to copy from.
 */
void TextBox::CopyPerCharacterTransform(const TextBox& rOther) {
    if (rOther.mPerCharacterTransform == nullptr) {
        return;
    }

    size_t size = CalculatePerCharacterTransformSize(rOther.mPerCharacterTransform->curveCount);
    mPerCharacterTransform = static_cast<PerCharacterTransform*>(Layout::AllocateMemory(size));
    std::memcpy(mPerCharacterTransform, rOther.mPerCharacterTransform, size);
    mPerCharacterTransform->pPerCharacterTransformInfos = nullptr;
}

/**
 * @brief Copies the settings shared by every copy constructor of a text box.
 * @param rOther Text box to copy from.
 */
void TextBox::CopyCommonImpl(const TextBox& rOther) {
    mTextId = rOther.mTextId;
    mFont = rOther.mFont;
    mFontSize = rOther.mFontSize;
    mLineSpace = rOther.mLineSpace;
    mCharSpace = rOther.mCharSpace;
    mTagProcessor = rOther.mTagProcessor;
    mTextBufferLength = 0;
    mTextLength = 0;
    mBits = rOther.mBits;
    mTextPosition = rOther.mTextPosition;
    mIsUtf8 = rOther.mIsUtf8;
    mItalicRatio = rOther.mItalicRatio;
    mShadowOffset = rOther.mShadowOffset;
    mShadowScale = rOther.mShadowScale;
    mShadowTopColor = rOther.mShadowTopColor;
    mShadowBottomColor = rOther.mShadowBottomColor;
    mShadowItalicRatio = rOther.mShadowItalicRatio;
    mTextBuffer = nullptr;
    mDispStringBuffer = nullptr;
    mPerCharacterTransform = nullptr;
    mLineWidthOffset = nullptr;
    mMaterial = nullptr;
    mTextColors[0] = rOther.mTextColors[0];
    mTextColors[1] = rOther.mTextColors[1];
    CopyLineWidthOffset(rOther);
    CopyPerCharacterTransform(rOther);
}

/**
 * @brief Copies another text box, including its string and material.
 * @param rOther Text box to copy from.
 * @param pDevice Device that owns the graphics resources.
 */
void TextBox::CopyImpl(const TextBox& rOther, nn::gfx::Device* pDevice) {
    CopyCommonImpl(rOther);

    if (rOther.GetStringBufferLength() != 0) {
        AllocateStringBuffer(pDevice, rOther.GetStringBufferLength(),
                             rOther.GetDrawStringBufferLength());

        if (mIsUtf8) {
            SetStringUtf8(rOther.GetStringBufferUtf8(), 0, rOther.mTextLength);
        } else {
            SetString(rOther.GetStringBuffer(), 0, rOther.mTextLength);
        }
    }

    mMaterial = Layout::NewObj<Material>(*rOther.mMaterial, pDevice);
}

/**
 * @brief Returns the number of characters the string buffer can hold.
 * @return The capacity excluding the terminator.
 */
u16 TextBox::GetStringBufferLength() const {
    if (mTextBufferLength == 0) {
        return 0;
    }

    return mTextBufferLength - 1;
}

/**
 * @brief Returns the number of characters the draw buffer can hold.
 * @return The capacity of the display string buffer.
 */
int TextBox::GetDrawStringBufferLength() const {
    if (mDispStringBuffer == nullptr) {
        return 0;
    }

    return mDispStringBuffer->m_CharCountMax;
}

/**
 * @brief Copies another text box with a string buffer of the given size.
 * @param rOther Text box to copy from.
 * @param pDevice Device that owns the graphics resources.
 * @param bufferLength Capacity of the new string buffer.
 */
void TextBox::CopyImpl(const TextBox& rOther, nn::gfx::Device* pDevice, u16 bufferLength) {
    CopyCommonImpl(rOther);

    if (bufferLength != 0) {
        AllocateStringBuffer(pDevice, bufferLength);
    }

    mMaterial = Layout::NewObj<Material>(*rOther.mMaterial, pDevice);
}

/** @brief Destroys the text box; Finalize releases its resources. */
TextBox::~TextBox() {}

/**
 * @brief Releases the material, the string buffers and the per-character tables.
 * @param pDevice Device that owns the graphics resources.
 */
void TextBox::Finalize(nn::gfx::Device* pDevice) {
    Pane::Finalize(pDevice);

    if (mMaterial != nullptr && !mMaterial->IsUserAllocated()) {
        mMaterial->Finalize(pDevice);
        Layout::DeleteObj(mMaterial);
        mMaterial = nullptr;
    }

    FreeStringBuffer(pDevice);

    if (mLineWidthOffset != nullptr) {
        Layout::FreeMemory(mLineWidthOffset->pLineOffset);
        Layout::FreeMemory(mLineWidthOffset->pLineWidth);
        Layout::FreeMemory(mLineWidthOffset);
        mLineWidthOffset = nullptr;
    }

    if (mPerCharacterTransform != nullptr) {
        Layout::FreeMemory(mPerCharacterTransform);
        mPerCharacterTransform = nullptr;
    }
}

/**
 * @brief Returns the number of materials of the text box.
 * @return One when the text material exists, otherwise zero.
 */
u32 TextBox::GetMaterialCount() const {
    return mMaterial != nullptr;
}

/**
 * @brief Returns a material of the text box.
 * @param index Material index; only zero selects the text material.
 * @return The material, or nullptr.
 */
Material* TextBox::GetMaterial(int index) const {
    GetMaterialCount();
    return index == 0 ? mMaterial : nullptr;
}

/**
 * @brief Replaces the text material, deleting the old one unless it is user allocated.
 * @param pMaterial New material.
 */
void TextBox::SetMaterial(Material* pMaterial) {
    if (mMaterial != nullptr && !mMaterial->IsUserAllocated()) {
        Layout::DeleteObj(mMaterial);
    }

    mMaterial = pMaterial;
}

/** @brief Detaches the text material without deleting it. */
void TextBox::UnsetMaterial() {
    mMaterial = nullptr;
}

/**
 * @brief Returns the pane size, enlarged to the string when the capture effect spreads it.
 * @param pSize Receives the size.
 */
void TextBox::GetSizeWithCaptureEffect(Size* pSize) const {
    if (IsSpreadPaneSizeOfItalicEnabled()) {
        nn::font::Rectangle rect = {};
        CalculateStringRect(&rect);
        pSize->width = std::max(mSizeX, rect.GetWidth()) + GetItalicSize() * 2.0f;
        pSize->height = std::max(mSizeY, rect.GetHeight());
    } else {
        *pSize = GetSize();
    }
}

/**
 * @brief Returns whether the capture effect spreads the pane size to fit italic text.
 * @return Whether the spread is enabled.
 */
bool TextBox::IsSpreadPaneSizeOfItalicEnabled() const {
    if (!IsPaneEffectEnabled()) {
        return false;
    }

    return GetPaneEffectInstance()->IsSpreadPaneSizeOfItalicEnabled();
}

/**
 * @brief Measures the string with the text box's font settings.
 * @param pRect Receives the string rectangle.
 */
void TextBox::CalculateStringRect(nn::font::Rectangle* pRect) const {
    if (mIsUtf8) {
        TextWriterUtf8 writer;
        writer.SetCursor(0.0f, 0.0f);
        writer.SetCenterCeilingEnabled(mBits.isCenterCeilingEnabled);
        writer.SetItalicRatio(mItalicRatio);
        writer.SetLinefeedByCharacterHeightEnabled(mBits.isLinefeedByCharacterHeightEnabled);
        SetFontInfoUtf8(&writer);
        writer.CalculateStringRect(pRect, GetStringBufferUtf8(), mTextLength);
    } else {
        TextWriterUtf16 writer;
        writer.SetCursor(0.0f, 0.0f);
        writer.SetCenterCeilingEnabled(mBits.isCenterCeilingEnabled);
        writer.SetItalicRatio(mItalicRatio);
        writer.SetLinefeedByCharacterHeightEnabled(mBits.isLinefeedByCharacterHeightEnabled);
        SetFontInfo(&writer);
        writer.CalculateStringRect(pRect, GetStringBuffer(), mTextLength);
    }
}

/**
 * @brief Returns the base position of the pane including the capture effect size.
 * @param pPos Receives the position of the top-left corner relative to the pane origin.
 */
void TextBox::GetVertexPosWithCaptureEffect(nn::util::Float2* pPos) const {
    Size size;
    GetSizeWithCaptureEffect(&size);

    switch (GetBasePositionX()) {
    case 0:
        pPos->x = -size.width * 0.5f;
        break;
    case 2:
        pPos->x = -size.width;
        break;
    case 1:
    default:
        pPos->x = 0.0f;
        break;
    }

    switch (GetBasePositionY()) {
    case 0:
        pPos->y = size.height * 0.5f;
        break;
    case 2:
        pPos->y = size.height;
        break;
    case 1:
    default:
        pPos->y = 0.0f;
        break;
    }
}

/**
 * @brief Returns how far italic glyphs lean out of the pane when the capture effect spreads it.
 * @return The italic offset, or zero.
 */
float TextBox::GetItalicSize() const {
    if (!IsSpreadPaneSizeOfItalicEnabled()) {
        return 0.0f;
    }

    const float shadowItalicRatio = mBits.isShadowEnabled ? mShadowItalicRatio : 0.0f;
    const float italicRatio = mItalicRatio;
    const float maxRatio = std::max(std::fabs(italicRatio), std::fabs(shadowItalicRatio));
    const float ratio = shadowItalicRatio + italicRatio < maxRatio ?
                            maxRatio :
                            std::fabs(shadowItalicRatio) + std::fabs(italicRatio);
    return mFontSize.width * ratio;
}

/**
 * @brief Returns the italic slant as an angle.
 * @return The angle in radians.
 */
float TextBox::GetAngleFromItalicRatio() const {
    return std::atan(mItalicRatio * mFontSize.width / mFontSize.height);
}

/**
 * @brief Sets the italic slant from an angle.
 * @param angle Angle in radians.
 */
void TextBox::SetAngleToItalicRatio(float angle) {
    const float ratio = std::tan(angle) / mFontSize.width * mFontSize.height;
    const bool isChanged = mItalicRatio != ratio;
    SetDirtyFlag(isChanged);

    if (isChanged) {
        mItalicRatio = ratio;
    }
}

/**
 * @brief Returns the shadow italic slant as an angle.
 * @return The angle in radians.
 */
float TextBox::GetAngleFromShadowItalicRatio() const {
    return std::atan(mShadowItalicRatio * mFontSize.width / mFontSize.height);
}

/**
 * @brief Sets the shadow italic slant from an angle.
 * @param angle Angle in radians.
 */
void TextBox::SetAngleToShadowItalicRatio(float angle) {
    const float ratio = std::tan(angle) / mFontSize.width * mFontSize.height;
    const bool isChanged = mShadowItalicRatio != ratio;
    SetDirtyFlag(isChanged);

    if (isChanged) {
        mShadowItalicRatio = ratio;
    }
}

/**
 * @brief Returns the text color used at a vertex.
 * @param index Vertex index; the top vertices use the top color.
 * @return The color.
 */
nn::util::Unorm8x4 TextBox::GetVertexColor(int index) const {
    return mTextColors[index / 2];
}

/**
 * @brief Sets the text color used at a vertex.
 * @param index Vertex index; the top vertices use the top color.
 * @param rColor New color.
 */
void TextBox::SetVertexColor(int index, const nn::util::Unorm8x4& rColor) {
    nn::util::Unorm8x4& rDst = mTextColors[index / 2];
    const bool isChanged = rDst.v[0] != rColor.v[0] || rDst.v[1] != rColor.v[1] ||
                           rDst.v[2] != rColor.v[2] || rDst.v[3] != rColor.v[3];
    SetDirtyFlag(isChanged);
    rDst = rColor;
}

/**
 * @brief Returns one channel of a vertex text color.
 * @param index Vertex color element index.
 * @return The channel value.
 */
u8 TextBox::GetVertexColorElement(int index) const {
    return mTextColors[index / 4 / 2].v[index % 4];
}

/**
 * @brief Sets one channel of a vertex text color.
 * @param index Vertex color element index.
 * @param value New channel value.
 */
void TextBox::SetVertexColorElement(int index, u8 value) {
    u8& rElement = mTextColors[index / 4 / 2].v[index % 4];
    SetDirtyFlag(rElement != value);
    rElement = value;
}

/**
 * @brief Applies the font settings of the text box to a UTF-8 text writer.
 * @param pWriter Writer to set up.
 */
void TextBox::SetFontInfoUtf8(nn::font::TextWriterBase<char>* pWriter) const {
    pWriter->SetFont(mFont);

    if (mFont != nullptr) {
        pWriter->SetFontSize(mFontSize.width, mFontSize.height);
        pWriter->SetLineSpace(mLineSpace);
        pWriter->SetCharSpace(mCharSpace);

        if (mBits.isWidthLimitEnabled) {
            pWriter->SetWidthLimit(mSizeX);
        } else {
            pWriter->ResetWidthLimit();
        }
    }

    if (mTagProcessor != nullptr) {
        pWriter->SetTagProcessor(reinterpret_cast<nn::font::TagProcessorBase<char>*>(mTagProcessor));
    }
}

/**
 * @brief Applies the font settings of the text box to a UTF-16 text writer.
 * @param pWriter Writer to set up.
 */
void TextBox::SetFontInfo(nn::font::TextWriterBase<u16>* pWriter) const {
    pWriter->SetFont(mFont);

    if (mFont != nullptr) {
        pWriter->SetFontSize(mFontSize.width, mFontSize.height);
        pWriter->SetLineSpace(mLineSpace);
        pWriter->SetCharSpace(mCharSpace);

        if (mBits.isWidthLimitEnabled) {
            pWriter->SetWidthLimit(mSizeX);
        } else {
            pWriter->ResetWidthLimit();
        }
    }

    if (mTagProcessor != nullptr) {
        pWriter->SetTagProcessor(mTagProcessor);
    }
}

/**
 * @brief Returns the rectangle the string occupies in pane coordinates.
 * @return The rectangle, or an empty rectangle without a font.
 */
nn::font::Rectangle TextBox::GetTextDrawRect() const {
    nn::font::Rectangle textRect;

    if (mFont == nullptr) {
        textRect.SetEdge(0.0f, 0.0f, 0.0f, 0.0f);
        return textRect;
    }

    nn::font::Rectangle stringRect = {};
    CalculateStringRect(&stringRect);
    const Size stringSize = {stringRect.GetWidth(), stringRect.GetHeight()};
    const nn::util::Float2 basePos = GetVertexPos();
    const nn::util::Float2 textPos = AdjustTextPos(GetSize(), false);
    const nn::util::Float2 stringPos = AdjustTextPos(stringSize, mBits.isCenterCeilingEnabled);
    const float left = basePos.x + (textPos.x - stringPos.x);
    const float top = basePos.y - (textPos.y - stringPos.y);
    textRect.left = left;
    textRect.top = top;
    textRect.right = left + stringSize.width;
    textRect.bottom = top - stringSize.height;
    return textRect;
}

/**
 * @brief Returns the offset of the text origin inside a box of the given size.
 * @param rSize Size of the box.
 * @param isCeil Whether centered offsets are rounded up.
 * @return The offset from the top-left corner.
 */
nn::util::Float2 TextBox::AdjustTextPos(const Size& rSize, bool isCeil) const {
    nn::util::Float2 pos;

    if (mLineWidthOffset != nullptr) {
        pos.x = 0.0f;
        pos.y = 0.0f;
        return pos;
    }

    switch (GetTextPositionH()) {
    case 0:
        pos.x = rSize.width * 0.5f;

        if (isCeil) {
            pos.x = std::ceil(pos.x);
        }

        break;
    case 2:
        pos.x = rSize.width;
        break;
    default:
        pos.x = 0.0f;
        break;
    }

    if (mBits.isPerCharacterTransformOriginToCenter) {
        pos.y = rSize.height * 0.5f;

        if (isCeil) {
            pos.y = std::ceil(pos.y);
        }

        return pos;
    }

    switch (GetTextPositionV()) {
    case 0:
        pos.y = rSize.height * 0.5f;

        if (isCeil) {
            pos.y = std::ceil(pos.y);
        }

        break;
    case 2:
        pos.y = rSize.height;
        break;
    default:
        pos.y = 0.0f;
        break;
    }

    return pos;
}

/**
 * @brief Fills the shader parameters that depend on the text box and its material.
 * @param rContent Content to fill.
 */
void TextBox::SetupConstantBufferAdditionalContent(
    nn::font::ConstantBufferAdditionalContent& rContent) const {
    if (mBits.isInvisibleBorderEnabled) {
        rContent.m_ShaderVariationFlags |= nn::font::DispStringBuffer::ShaderVariationFlag_BorderPass;
    }

    const Material* pMaterial = mMaterial;
    nn::util::Float4 white;
    nn::util::Float3 black;

    if (IsWhiteColorFloat(*pMaterial)) {
        white = pMaterial->m_pFloatColors[1];
    } else {
        white.x = ToFloatColor(pMaterial->m_ByteColors[1].v[0]);
        white.y = ToFloatColor(pMaterial->m_ByteColors[1].v[1]);
        white.z = ToFloatColor(pMaterial->m_ByteColors[1].v[2]);
        white.w = ToFloatColor(pMaterial->m_ByteColors[1].v[3]);
    }

    if (IsBlackColorFloat(*pMaterial)) {
        black.x = pMaterial->m_pFloatColors[0].x;
        black.y = pMaterial->m_pFloatColors[0].y;
        black.z = pMaterial->m_pFloatColors[0].z;
    } else {
        black.x = ToFloatColor(pMaterial->m_ByteColors[0].v[0]);
        black.y = ToFloatColor(pMaterial->m_ByteColors[0].v[1]);
        black.z = ToFloatColor(pMaterial->m_ByteColors[0].v[2]);
    }

    rContent.m_InterpolateBlack.x = black.x;
    rContent.m_InterpolateBlack.y = black.y;
    rContent.m_InterpolateBlack.z = black.z;
    rContent.m_InterpolateBlack.w = 0.0f;
    rContent.m_InterpolateWhite = white;
    rContent.m_InterpolateAlpha = GetGlobalAlpha();

    if (mMaterial->m_MemCap.fontShadowParameter) {
        const ResFontShadowParameter* pShadow = mMaterial->GetFontShadowParameterPtr();
        rContent.m_ShadowInterpolateBlack.x = ToFloatColor(pShadow->blackInterporateColor[0]);
        rContent.m_ShadowInterpolateBlack.y = ToFloatColor(pShadow->blackInterporateColor[1]);
        rContent.m_ShadowInterpolateBlack.z = ToFloatColor(pShadow->blackInterporateColor[2]);
        rContent.m_ShadowInterpolateBlack.w = 0.0f;
        rContent.m_ShadowInterpolateWhite.x = ToFloatColor(pShadow->whiteInterporateColor[0]);
        rContent.m_ShadowInterpolateWhite.y = ToFloatColor(pShadow->whiteInterporateColor[1]);
        rContent.m_ShadowInterpolateWhite.z = ToFloatColor(pShadow->whiteInterporateColor[2]);
        rContent.m_ShadowInterpolateWhite.w = ToFloatColor(pShadow->whiteInterporateColor[3]);
        rContent.m_ShadowInterpolateAlpha = GetGlobalAlpha();
    } else {
        const u8 alpha = GetGlobalAlpha();
        rContent.m_ShadowInterpolateBlack.x = 0.0f;
        rContent.m_ShadowInterpolateBlack.y = 0.0f;
        rContent.m_ShadowInterpolateBlack.z = 0.0f;
        rContent.m_ShadowInterpolateBlack.w = 0.0f;
        rContent.m_ShadowInterpolateWhite.x = 1.0f;
        rContent.m_ShadowInterpolateWhite.y = 1.0f;
        rContent.m_ShadowInterpolateWhite.z = 1.0f;
        rContent.m_ShadowInterpolateWhite.w = 1.0f;
        rContent.m_ShadowInterpolateAlpha = alpha;
    }

    if (mBits.isPerCharacterTransformEnabled) {
        rContent.m_pPerCharacterTransformInfos = mPerCharacterTransform->pPerCharacterTransformInfos;
        rContent.m_PerCharacterTransformCenter =
            static_cast<nn::font::ConstantBufferAdditionalContent::PerCharacterTransformCenter>(
                mPerCharacterTransform->originV);
        rContent.m_PerCharacterTransformCenterOffset = mPerCharacterTransform->originVOffset;
    }
}

/**
 * @brief Builds the font constant buffer for the current frame.
 * @param rDrawInfo Draw state holding the projection matrix.
 */
void TextBox::BuildConstantBuffer(DrawInfo& rDrawInfo) const {
    nn::font::ConstantBufferAdditionalContent content;
    nn::util::MatrixT4x3fType globalMtx;
    GetTextGlobalMtx(&globalMtx, rDrawInfo);
    nn::util::MatrixT4x3fType localMtx;
    localMtx._m.val[0] = float32x4_t{1.0f, 0.0f, 0.0f, 0.0f};
    localMtx._m.val[1] = float32x4_t{0.0f, 1.0f, 0.0f, 0.0f};
    localMtx._m.val[2] = float32x4_t{0.0f, 0.0f, 1.0f, 0.0f};
    content.m_pViewMatrix = &globalMtx;
    content.m_pLocalMatrix = &localMtx;
    nn::font::ShadowParameter shadowParam;

    if (mBits.isShadowEnabled) {
        shadowParam.topColor = mShadowTopColor;
        shadowParam.bottomColor = mShadowBottomColor;
        shadowParam.offset = mShadowOffset;
        shadowParam.scale = mShadowScale;
        shadowParam.italicRatio = mShadowItalicRatio * mFontSize.width;
        content.m_pShadowParam = &shadowParam;
    }

    SetupConstantBufferAdditionalContent(content);
    const nn::util::MatrixT4x4fType* pProjection = &rDrawInfo.m_ProjMtx;
    nn::util::MatrixT4x4fType captureProjection;

    if (IsPaneEffectEnabled()) {
        CalculateCaptureProjectionMatrix(captureProjection);
        pProjection = &captureProjection;

        if (IsPaneEffectStaticCacheRenderingNeeded()) {
            content.m_InterpolateAlpha = 0xff;
        }
    }

    mDispStringBuffer->BuildConstantBuffer(*pProjection, &content, mBits.isDrawFromRightToLeft,
                                           mBits.isPerCharacterTransformOriginToCenter);
}

/**
 * @brief Returns the matrix that places the string inside the pane.
 * @param pMtx Receives the matrix.
 * @param rDrawInfo Draw state used when the pane is captured.
 */
void TextBox::GetTextGlobalMtx(nn::util::MatrixT4x3fType* pMtx, const DrawInfo& rDrawInfo) const {
    if (IsPaneEffectEnabled()) {
        detail::CalculateCaptureRootMatrix(*pMtx, rDrawInfo);
    } else {
        *pMtx = GetGlobalMatrix();
    }

    const nn::util::Float2 basePos = GetVertexPos();
    nn::util::Float2 textPos = AdjustTextPos(GetSize(), false);
    const float italicSize = GetItalicSize();

    switch (GetBasePositionX()) {
    case 0:
        break;
    case 2:
        textPos.x -= italicSize;
        break;
    default:
        textPos.x += italicSize;
        break;
    }

    const float x = basePos.x + textPos.x;
    const float y = basePos.y - textPos.y;
    float32x4_t row0 = pMtx->_m.val[0];
    float32x4_t row1 = pMtx->_m.val[1];
    float32x4_t row2 = pMtx->_m.val[2];
    row0[3] = row0[3] + (x * row0[0] + y * row0[1]);
    row0[1] = -row0[1];
    row1[3] = row1[3] + (x * row1[0] + y * row1[1]);
    row1[1] = -row1[1];
    row2[3] = row2[3] + (x * row2[0] + y * row2[1]);
    row2[1] = -row2[1];
    pMtx->_m.val[0] = row0;
    pMtx->_m.val[1] = row1;
    pMtx->_m.val[2] = row2;
}

/**
 * @brief Updates the pane and rebuilds the string when it changed.
 * @param rDrawInfo Draw state.
 * @param rContext Calculation context.
 * @param isDirtyParentMtx Whether the parent matrix changed.
 */
void TextBox::Calculate(DrawInfo& rDrawInfo, CalculateContext& rContext, bool isDirtyParentMtx) {
    Pane::Calculate(rDrawInfo, rContext, isDirtyParentMtx);

    if (mTextLength == 0 || mFont == nullptr || mMaterial == nullptr) {
        ResetConstantBufferReady();
        return;
    }

    if (CheckInvisibleAndUpdateConstantBufferReady()) {
        return;
    }

    mDispStringBuffer->SetConstantBuffer(rDrawInfo.m_pFontConstantBuffer);
    mMaterial->SetupBlendState(&rDrawInfo);

    if (!mBits.isDirty) {
        BuildConstantBuffer(rDrawInfo);
        return;
    }

    mBits.isDirty = false;

    if (mIsUtf8) {
        TextWriterUtf8 writer;
        writer.SetCenterCeilingEnabled(mBits.isCenterCeilingEnabled);
        SetupTextWriterUtf8(&writer);
        writer.SetDispStringBuffer(mDispStringBuffer);
        writer.StartPrint();

        if (mLineWidthOffset != nullptr) {
            writer.Print(GetStringBufferUtf8(), mTextLength, LineOffsetCountMax,
                         mLineWidthOffset->pLineOffset, mLineWidthOffset->pLineWidth);
        } else {
            writer.Print(GetStringBufferUtf8(), mTextLength);
        }

        writer.EndPrint();

        if (mBits.isPerCharacterTransformEnabled) {
            UpdatePerCharacterTransform(&writer.GetTagProcessor());
        }
    } else {
        TextWriterUtf16 writer;
        writer.SetCenterCeilingEnabled(mBits.isCenterCeilingEnabled);
        SetupTextWriter(&writer);
        writer.SetDispStringBuffer(mDispStringBuffer);
        writer.StartPrint();

        if (mLineWidthOffset != nullptr) {
            writer.Print(GetStringBuffer(), mTextLength, LineOffsetCountMax,
                         mLineWidthOffset->pLineOffset, mLineWidthOffset->pLineWidth);
        } else {
            writer.Print(GetStringBuffer(), mTextLength);
        }

        writer.EndPrint();

        if (mBits.isPerCharacterTransformEnabled) {
            UpdatePerCharacterTransform(&writer.GetTagProcessor());
        }
    }

    BuildConstantBuffer(rDrawInfo);
}

/**
 * @brief Recomputes the per-character transforms of every drawn character.
 * @tparam CharType Character type of the string buffer.
 * @param pTagProcessor Tag processor used to skip tags in the string.
 */
template <typename CharType>
void TextBox::UpdatePerCharacterTransform(nn::font::TagProcessorBase<CharType>* pTagProcessor) {
    const u32 charCount = mDispStringBuffer->m_CharCount;

    if (charCount == 0) {
        return;
    }

    float totalWidth;
    const float stepTime = CalculateStepTime(&totalWidth, pTagProcessor);
    float time = CalculateBeginTime(stepTime, totalWidth);
    const CharType* pPos = static_cast<const CharType*>(mTextBuffer);
    float charWidth = 0.0f;

    for (u32 i = 0; i < charCount; i++) {
        if (IsFixSpaceInsertSpaceMode()) {
            const CharType* pPrintable;
            bool isPrintable;

            do {
                pPrintable = pPos;
                pPos = pTagProcessor->AcquireNextPrintableChar(&isPrintable, pPrintable);
            } while (!isPrintable && ValidateNextPrintableChar(pPrintable, pPos));

            if (!isPrintable) {
                return;
            }

            const u32 code = GetCharFromPointer(pPrintable);

            if (pPos == nullptr) {
                return;
            }

            const float scale = mFontSize.width / mFont->GetWidth();
            charWidth = scale * (mFont->GetCharWidth(code) + mCharSpace);
            time += stepTime * charWidth * 0.5f;
        } else if (IsFixSpaceMode()) {
            time += stepTime * 0.5f;
            charWidth = 0.0f;
        } else if (!mBits.isPerCharacterTransformSplitByCharWidth) {
            time = mPerCharacterTransform->offset + stepTime * i;
            charWidth = 0.0f;
        } else {
            const CharType* pPrintable;
            bool isPrintable;

            do {
                pPrintable = pPos;
                pPos = pTagProcessor->AcquireNextPrintableChar(&isPrintable, pPrintable);
            } while (!isPrintable && ValidateNextPrintableChar(pPrintable, pPos));

            if (!isPrintable) {
                return;
            }

            const u32 code = GetCharFromPointer(pPrintable);

            if (pPos == nullptr) {
                return;
            }

            charWidth = mFont->GetCharWidth(code) + mCharSpace;

            if (mBits.isPerCharacterTransformOriginToCenter) {
                time += stepTime * charWidth * 0.5f;
            } else if (i != 0) {
                time += stepTime * charWidth;
            }
        }

        ApplyPerCharacterTransformCurve(&mPerCharacterTransform->pPerCharacterTransformInfos[i],
                                        time);

        if (IsFixSpaceInsertSpaceMode()) {
            time += stepTime * charWidth * 0.5f;
        } else if (IsFixSpaceMode()) {
            time += stepTime * 0.5f;
        } else if (IsSplitByCharWidthCenterMode()) {
            time += stepTime * charWidth * 0.5f;
        }
    }
}

/**
 * @brief Sets the blend state used to render the pane into a capture texture.
 * @param rCommands Command buffer to record into.
 */
void TextBox::SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer& rCommands) const {
    mMaterial->SetCommandBufferOnlyBlend(rCommands);
}

/**
 * @brief Draws the string.
 * @param rDrawInfo Draw state.
 * @param rCommands Command buffer to record into.
 */
void TextBox::DrawSelf(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands) {
    if (mTextLength == 0 || mFont == nullptr || mMaterial == nullptr) {
        return;
    }

    mMaterial->SetCommandBufferOnlyBlend(rCommands);

    if (IsPaneEffectEnabled()) {
        GraphicsResource* pResource = const_cast<GraphicsResource*>(rDrawInfo.GetGraphicsResource());
        rCommands.SetBlendState(pResource->GetPresetBlendState(PresetBlendStateId(1)));
    }

    nn::font::RectDrawer* pDrawer = rDrawInfo.GetGraphicsResource()->m_pFontDrawer;
    rDrawInfo.ResetCurrentShader();
    pDrawer->Draw(rCommands, *mDispStringBuffer);
    rDrawInfo.mVertexBufferDirty = true;
}

/**
 * @brief Checks that a copied text box matches the original.
 * @param rOther Original text box.
 * @return Whether every copied member matches.
 */
bool TextBox::CompareCopiedInstanceTest(const TextBox& rOther) const {
    if (mIsUtf8) {
        if (std::memcmp(mTextBuffer, rOther.GetStringBufferUtf8(), mTextBufferLength) != 0) {
            return false;
        }
    } else {
        if (std::memcmp(mTextBuffer, rOther.GetStringBuffer(), mTextBufferLength * sizeof(u16)) !=
            0) {
            return false;
        }
    }

    if (mTagProcessor != rOther.mTagProcessor) {
        return false;
    }

    if (mTextId != rOther.mTextId) {
        return false;
    }

    if (mFont != rOther.mFont) {
        return false;
    }

    if (mLineSpace != rOther.mLineSpace) {
        return false;
    }

    if (mCharSpace != rOther.mCharSpace) {
        return false;
    }

    if (mTextBufferLength != rOther.mTextBufferLength) {
        return false;
    }

    if (mTextLength != rOther.mTextLength) {
        return false;
    }

    if (mTextPosition != rOther.mTextPosition) {
        return false;
    }

    if (mIsUtf8 != rOther.mIsUtf8) {
        return false;
    }

    if (mItalicRatio != rOther.mItalicRatio) {
        return false;
    }

    if (mShadowItalicRatio != rOther.mShadowItalicRatio) {
        return false;
    }

    if (std::memcmp(mTextColors, rOther.mTextColors, sizeof(mTextColors)) != 0) {
        return false;
    }

    if (std::memcmp(&mFontSize, &rOther.mFontSize, sizeof(mFontSize)) != 0) {
        return false;
    }

    if (std::memcmp(&mBits, &rOther.mBits, sizeof(mBits)) != 0) {
        return false;
    }

    if (std::memcmp(&mShadowOffset, &rOther.mShadowOffset, sizeof(mShadowOffset)) != 0) {
        return false;
    }

    if (std::memcmp(&mShadowScale, &rOther.mShadowScale, sizeof(mShadowScale)) != 0) {
        return false;
    }

    if (std::memcmp(&mShadowTopColor, &rOther.mShadowTopColor, sizeof(mShadowTopColor)) != 0) {
        return false;
    }

    if (std::memcmp(&mShadowBottomColor, &rOther.mShadowBottomColor,
                    sizeof(mShadowBottomColor)) != 0) {
        return false;
    }

    if (rOther.mMaterial != nullptr) {
        if (mMaterial == nullptr || !mMaterial->CompareCopiedInstanceTest(*rOther.mMaterial)) {
            return false;
        }
    }

    if (rOther.mLineWidthOffset != nullptr) {
        if (mLineWidthOffset == nullptr) {
            return false;
        }

        if (std::memcmp(mLineWidthOffset->pLineOffset, rOther.mLineWidthOffset->pLineOffset,
                        sizeof(float) * LineOffsetCountMax) != 0) {
            return false;
        }

        if (std::memcmp(mLineWidthOffset->pLineWidth, rOther.mLineWidthOffset->pLineWidth,
                        sizeof(float) * LineOffsetCountMax) != 0) {
            return false;
        }
    }

    if (rOther.mPerCharacterTransform != nullptr) {
        const PerCharacterTransform* pTransform = mPerCharacterTransform;
        const PerCharacterTransform* pOtherTransform = rOther.mPerCharacterTransform;

        if (pTransform == nullptr) {
            return false;
        }

        if (pTransform->offset != pOtherTransform->offset) {
            return false;
        }

        if (pTransform->width != pOtherTransform->width) {
            return false;
        }

        if (pTransform->loopType != pOtherTransform->loopType) {
            return false;
        }

        if (pTransform->originV != pOtherTransform->originV) {
            return false;
        }

        if (pTransform->originVOffset != pOtherTransform->originVOffset) {
            return false;
        }

        if (pTransform->curveCount != pOtherTransform->curveCount) {
            return false;
        }

        for (int i = 0; i < pTransform->curveCount; i++) {
            if (pTransform->curveInfos[i].pCurve != pOtherTransform->curveInfos[i].pCurve) {
                return false;
            }

            if (pTransform->curveInfos[i].curveType != pOtherTransform->curveInfos[i].curveType) {
                return false;
            }
        }
    }

    if (rOther.mDispStringBuffer != nullptr) {
        if (mDispStringBuffer == nullptr ||
            !mDispStringBuffer->CompareCopiedInstanceTest(*rOther.mDispStringBuffer)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Allocates a string buffer whose draw buffer holds as many characters.
 * @param pDevice Device that owns the graphics resources.
 * @param minLen Minimum number of characters.
 */
void TextBox::AllocateStringBuffer(nn::gfx::Device* pDevice, u16 minLen) {
    AllocateStringBuffer(pDevice, minLen, minLen);
}

/**
 * @brief Allocates the string buffer and the display string buffer.
 * @param pDevice Device that owns the graphics resources.
 * @param minLen Minimum number of characters of the string buffer.
 * @param minDrawLen Minimum number of characters of the draw buffer.
 */
void TextBox::AllocateStringBuffer(nn::gfx::Device* pDevice, u16 minLen, u16 minDrawLen) {
    if (minLen == 0 || minDrawLen == 0) {
        return;
    }

    if (minLen < mTextBufferLength && mDispStringBuffer != nullptr &&
        mDispStringBuffer->m_CharCountMax >= minDrawLen) {
        return;
    }

    const u16 allocLen = minLen + 1;
    FreeStringBuffer(pDevice);
    nn::font::DispStringBuffer::InitializeArg sizeArg;
    sizeArg.charCountMax = minDrawLen;
    const size_t drawBufferSize = nn::font::DispStringBuffer::GetRequiredDrawBufferSize(sizeArg);
    const size_t charSize = mIsUtf8 ? 4 : sizeof(u16);
    void* pTextBuffer = Layout::AllocateMemory(charSize * allocLen);
    void* pDispStringBuffer =
        Layout::AllocateMemory(sizeof(nn::font::DispStringBuffer) + drawBufferSize);

    if (mPerCharacterTransform != nullptr) {
        nn::font::PerCharacterTransformInfo* pInfos =
            static_cast<nn::font::PerCharacterTransformInfo*>(Layout::AllocateMemory(
                sizeof(nn::font::PerCharacterTransformInfo) * minDrawLen));

        if (pInfos != nullptr) {
            for (int i = 0; i < minDrawLen; i++) {
                InitializePerCharacterTransformInfo(&pInfos[i]);
            }
        }

        mPerCharacterTransform->pPerCharacterTransformInfos = pInfos;
    }

    if (pTextBuffer == nullptr || pDispStringBuffer == nullptr ||
        (mPerCharacterTransform != nullptr &&
         mPerCharacterTransform->pPerCharacterTransformInfos == nullptr)) {
        if (pTextBuffer != nullptr) {
            Layout::FreeMemory(pTextBuffer);
        }

        if (pDispStringBuffer != nullptr) {
            Layout::FreeMemory(pDispStringBuffer);
        }

        if (mPerCharacterTransform != nullptr &&
            mPerCharacterTransform->pPerCharacterTransformInfos != nullptr) {
            Layout::FreeMemory(mPerCharacterTransform->pPerCharacterTransformInfos);
            mPerCharacterTransform->pPerCharacterTransformInfos = nullptr;
        }

        return;
    }

    mTextBuffer = pTextBuffer;
    mTextBufferLength = allocLen;
    mDispStringBuffer = new (pDispStringBuffer) nn::font::DispStringBuffer();
    nn::font::DispStringBuffer::InitializeArg arg;
    arg.pDrawBuffer = static_cast<u8*>(pDispStringBuffer) + sizeof(nn::font::DispStringBuffer);
    arg.pConstantBuffer = nullptr;
    arg.charCountMax = minDrawLen;
    arg.isShadowEnabled = mBits.isShadowEnabled;
    arg.isDoubleDrawnBorder = mBits.isDoubleDrawnBorderEnabled;
    arg.isPerCharacterTransformEnabled = mBits.isPerCharacterTransformEnabled;
    arg.isPerCharacterTransformAutoShadowAlpha = mBits.isPerCharacterTransformAutoShadowAlpha;
    mDispStringBuffer->Initialize(pDevice, arg);
}

/**
 * @brief Releases the string buffer and the display string buffer.
 * @param pDevice Device that owns the graphics resources.
 */
void TextBox::FreeStringBuffer(nn::gfx::Device* pDevice) {
    if (mTextBuffer != nullptr) {
        if (mDispStringBuffer != nullptr) {
            mDispStringBuffer->Finalize(pDevice);
            mDispStringBuffer->~DispStringBuffer();
            Layout::FreeMemory(mDispStringBuffer);
            mDispStringBuffer = nullptr;
        }

        Layout::FreeMemory(mTextBuffer);
        mTextBuffer = nullptr;
        mTextBufferLength = 0;
        mTextLength = 0;
    }

    if (mPerCharacterTransform != nullptr &&
        mPerCharacterTransform->pPerCharacterTransformInfos != nullptr) {
        Layout::FreeMemory(mPerCharacterTransform->pPerCharacterTransformInfos);
        mPerCharacterTransform->pPerCharacterTransformInfos = nullptr;
    }
}

/**
 * @brief Sets a null-terminated UTF-16 string.
 * @param pStr String to copy.
 * @param dstIdx Index in the string buffer to copy to.
 * @return The number of copied characters.
 */
u16 TextBox::SetString(const u16* pStr, u16 dstIdx) {
    return SetStringImpl(pStr, dstIdx, GetUtf16StringLength(pStr));
}

/**
 * @brief Copies a UTF-16 string into the string buffer.
 * @param pStr String to copy.
 * @param dstIdx Index in the string buffer to copy to.
 * @param strLen Number of characters to copy.
 * @return The number of copied characters.
 */
u16 TextBox::SetStringImpl(const u16* pStr, u16 dstIdx, int strLen) {
    if (mFont == nullptr || mTextBuffer == nullptr) {
        return 0;
    }

    const u16 bufLen = GetStringBufferLength();

    if (dstIdx >= bufLen) {
        return 0;
    }

    const int copyLen = std::min(strLen, bufLen - dstIdx);
    std::memcpy(GetStringBuffer() + dstIdx, pStr, copyLen * sizeof(u16));
    u16* pBuffer = GetStringBuffer();
    mTextLength = pBuffer[0] == 0 ? 0 : dstIdx + copyLen;
    pBuffer[mTextLength] = 0;
    mBits.isDirty = true;
    ResetConstantBufferReady();
    return copyLen;
}

/**
 * @brief Sets a null-terminated UTF-8 string.
 * @param pStr String to copy.
 * @param dstIdx Index in the string buffer to copy to.
 * @return The number of copied bytes.
 */
u16 TextBox::SetStringUtf8(const char* pStr, u16 dstIdx) {
    return SetStringImplUtf8(pStr, dstIdx, std::strlen(pStr));
}

/**
 * @brief Copies a UTF-8 string into the string buffer.
 * @param pStr String to copy.
 * @param dstIdx Index in the string buffer to copy to.
 * @param strLen Number of bytes to copy.
 * @return The number of copied bytes.
 */
u16 TextBox::SetStringImplUtf8(const char* pStr, u16 dstIdx, int strLen) {
    if (mFont == nullptr || mTextBuffer == nullptr) {
        return 0;
    }

    const u16 bufLen = GetStringBufferLength();

    if (dstIdx >= bufLen) {
        return 0;
    }

    const int copyLen = std::min(strLen, bufLen - dstIdx);
    std::memcpy(GetStringBufferUtf8() + dstIdx, pStr, copyLen);
    char* pBuffer = GetStringBufferUtf8();
    mTextLength = pBuffer[0] == '\0' ? 0 : dstIdx + copyLen;
    pBuffer[mTextLength] = '\0';
    mBits.isDirty = true;
    ResetConstantBufferReady();
    return copyLen;
}

/**
 * @brief Copies a UTF-16 string of the given length into the string buffer.
 * @param pStr String to copy.
 * @param dstIdx Index in the string buffer to copy to.
 * @param strLen Number of characters to copy.
 * @return The number of copied characters.
 */
u16 TextBox::SetString(const u16* pStr, u16 dstIdx, u16 strLen) {
    return SetStringImpl(pStr, dstIdx, strLen);
}

/**
 * @brief Copies a UTF-8 string of the given length into the string buffer.
 * @param pStr String to copy.
 * @param dstIdx Index in the string buffer to copy to.
 * @param strLen Number of bytes to copy.
 * @return The number of copied bytes.
 */
u16 TextBox::SetStringUtf8(const char* pStr, u16 dstIdx, u16 strLen) {
    return SetStringImplUtf8(pStr, dstIdx, strLen);
}

/**
 * @brief Returns the font of the text box.
 * @return The font, or nullptr.
 */
const nn::font::Font* TextBox::GetFont() const {
    return mFont;
}

/**
 * @brief Changes the font and resets the font size to the font's native size.
 * @param pFont New font; only Unicode fonts are accepted.
 */
void TextBox::SetFont(const nn::font::Font* pFont) {
    if (pFont != nullptr && pFont->GetCharacterCode() != nn::font::CharacterCode_Unicode) {
        return;
    }

    const bool isChanged = mFont != pFont;
    SetDirtyFlag(isChanged);

    if (!isChanged) {
        return;
    }

    mFont = pFont;

    if (mFont != nullptr) {
        const Size size = {static_cast<float>(mFont->GetWidth()),
                           static_cast<float>(mFont->GetHeight())};
        SetFontSize(size);
    } else {
        const Size size = {0.0f, 0.0f};
        SetFontSize(size);
    }
}

/**
 * @brief Changes the font size, keeping the italic slant angles.
 * @param rSize New font size.
 */
void TextBox::SetFontSize(const Size& rSize) {
    const bool isChanged = !(mFontSize.width == rSize.width && mFontSize.height == rSize.height);
    mBits.isDirty |= isChanged;

    if (!isChanged) {
        return;
    }

    if (mFontSize.height == 0.0f) {
        mFontSize = rSize;
        return;
    }

    const float oldAspect = mFontSize.width / mFontSize.height;
    const float italicRatio = oldAspect * mItalicRatio;
    const float shadowItalicRatio = oldAspect * mShadowItalicRatio;
    mFontSize = rSize;
    const float newAspect = mFontSize.height / mFontSize.width;
    mItalicRatio = italicRatio * newAspect;
    mShadowItalicRatio = shadowItalicRatio * newAspect;
}

/**
 * @brief Does nothing; the text matrix is applied by the font constant buffer.
 * @param rDrawInfo Draw state.
 */
void TextBox::LoadMtx(DrawInfo& rDrawInfo) {}

/**
 * @brief Applies the text alignment and origin of the text box to a UTF-16 text writer.
 * @param pWriter Writer to set up.
 */
void TextBox::SetTextPos(nn::font::TextWriterBase<u16>* pWriter) const {
    if (mLineWidthOffset != nullptr) {
        pWriter->SetDrawFlag(0);
        return;
    }

    u32 drawFlag;

    switch (mBits.textAlignment) {
    case 1:
        drawFlag = TextWriterUtf16::HorizontalAlign_Left;
        break;
    case 2:
        drawFlag = TextWriterUtf16::HorizontalAlign_Center;
        break;
    case 3:
        drawFlag = TextWriterUtf16::HorizontalAlign_Right;
        break;
    default:
        if (GetTextPositionH() == 2) {
            drawFlag = TextWriterUtf16::HorizontalAlign_Right;
        } else if (GetTextPositionH() == 0) {
            drawFlag = TextWriterUtf16::HorizontalAlign_Center;
        } else {
            drawFlag = TextWriterUtf16::HorizontalAlign_Left;
        }

        break;
    }

    switch (GetTextPositionH()) {
    case 0:
        drawFlag |= TextWriterUtf16::HorizontalOrigin_Center;
        break;
    case 2:
        drawFlag |= TextWriterUtf16::HorizontalOrigin_Right;
        break;
    default:
        break;
    }

    if (mBits.isPerCharacterTransformOriginToCenter) {
        drawFlag |= TextWriterUtf16::VerticalOrigin_Middle;
    } else {
        switch (GetTextPositionV()) {
        case 0:
            drawFlag |= TextWriterUtf16::VerticalOrigin_Middle;
            break;
        case 2:
            drawFlag |= TextWriterUtf16::VerticalOrigin_Bottom;
            break;
        default:
            break;
        }
    }

    pWriter->SetDrawFlag(drawFlag);
}

/**
 * @brief Applies the text alignment and origin of the text box to a UTF-8 text writer.
 * @param pWriter Writer to set up.
 */
void TextBox::SetTextPosUtf8(nn::font::TextWriterBase<char>* pWriter) const {
    if (mLineWidthOffset != nullptr) {
        pWriter->SetDrawFlag(0);
        return;
    }

    u32 drawFlag;

    switch (mBits.textAlignment) {
    case 1:
        drawFlag = TextWriterUtf8::HorizontalAlign_Left;
        break;
    case 2:
        drawFlag = TextWriterUtf8::HorizontalAlign_Center;
        break;
    case 3:
        drawFlag = TextWriterUtf8::HorizontalAlign_Right;
        break;
    default:
        if (GetTextPositionH() == 2) {
            drawFlag = TextWriterUtf8::HorizontalAlign_Right;
        } else if (GetTextPositionH() == 0) {
            drawFlag = TextWriterUtf8::HorizontalAlign_Center;
        } else {
            drawFlag = TextWriterUtf8::HorizontalAlign_Left;
        }

        break;
    }

    switch (GetTextPositionH()) {
    case 0:
        drawFlag |= TextWriterUtf8::HorizontalOrigin_Center;
        break;
    case 2:
        drawFlag |= TextWriterUtf8::HorizontalOrigin_Right;
        break;
    default:
        break;
    }

    if (mBits.isPerCharacterTransformOriginToCenter) {
        drawFlag |= TextWriterUtf8::VerticalOrigin_Middle;
    } else {
        switch (GetTextPositionV()) {
        case 0:
            drawFlag |= TextWriterUtf8::VerticalOrigin_Middle;
            break;
        case 2:
            drawFlag |= TextWriterUtf8::VerticalOrigin_Bottom;
            break;
        default:
            break;
        }
    }

    pWriter->SetDrawFlag(drawFlag);
}

/**
 * @brief Applies every text box setting to a UTF-16 text writer.
 * @param pWriter Writer to set up.
 */
void TextBox::SetupTextWriter(nn::font::TextWriterBase<u16>* pWriter) const {
    pWriter->SetCursor(0.0f, 0.0f);
    SetFontInfo(pWriter);
    SetTextPos(pWriter);
    pWriter->SetGradationColor(mTextColors[0], mTextColors[1]);
    pWriter->SetItalicRatio(mItalicRatio);
    pWriter->SetLinefeedByCharacterHeightEnabled(mBits.isLinefeedByCharacterHeightEnabled);
}

/**
 * @brief Applies every text box setting to a UTF-8 text writer.
 * @param pWriter Writer to set up.
 */
void TextBox::SetupTextWriterUtf8(nn::font::TextWriterBase<char>* pWriter) const {
    pWriter->SetCursor(0.0f, 0.0f);
    SetFontInfoUtf8(pWriter);
    SetTextPosUtf8(pWriter);
    pWriter->SetGradationColor(mTextColors[0], mTextColors[1]);
    pWriter->SetItalicRatio(mItalicRatio);
    pWriter->SetLinefeedByCharacterHeightEnabled(mBits.isLinefeedByCharacterHeightEnabled);
}

/**
 * @brief Sets the evaluation offset or width of the per-character transform.
 * @param index Zero selects the offset, one the width.
 * @param value New value.
 */
void TextBox::SetPerCharacterTransform(int index, float value) {
    switch (index) {
    case 0:
        mPerCharacterTransform->offset = value;
        break;
    case 1:
        mPerCharacterTransform->width = value;
        break;
    default:
        break;
    }

    mBits.isDirty = true;
}

/**
 * @brief Returns the evaluation offset or width of the per-character transform.
 * @param index Zero selects the offset, one the width.
 * @return The value, or zero for other indices.
 */
float TextBox::GetPerCharacterTransform(int index) const {
    switch (index) {
    case 0:
        return mPerCharacterTransform->offset;
    case 1:
        return mPerCharacterTransform->width;
    default:
        return 0.0f;
    }
}

/**
 * @brief Returns the curve time of the first character.
 * @param stepTime Curve time per unit of advance.
 * @param totalWidth Total advance of the string.
 * @return The curve time of the first character.
 */
float TextBox::CalculateBeginTime(float stepTime, float totalWidth) {
    float time = mPerCharacterTransform->offset;

    if (IsFixSpaceInsertSpaceMode()) {
        switch (mPerCharacterTransform->fixSpaceOrigin) {
        case 1:
            time += (mPerCharacterTransform->width - stepTime * totalWidth) * 0.5f;
            break;
        case 2:
            time = time + mPerCharacterTransform->width - stepTime * totalWidth;
            break;
        default:
            break;
        }
    } else if (IsFixSpaceMode()) {
        const u32 charCount = mDispStringBuffer->m_CharCount;

        switch (mPerCharacterTransform->fixSpaceOrigin) {
        case 1:
            time += (mPerCharacterTransform->width -
                     mPerCharacterTransform->fixSpaceWidth * charCount) *
                    0.5f;
            break;
        case 2:
            time = time + mPerCharacterTransform->width -
                   mPerCharacterTransform->fixSpaceWidth * charCount;
            break;
        default:
            break;
        }
    }

    return time;
}

/**
 * @brief Checks that the next printable UTF-8 character is inside the string.
 * @param current Previous cursor.
 * @param next Cursor returned by the tag processor.
 * @return Whether next advanced and stays inside the string.
 */
bool TextBox::ValidateNextPrintableChar(const char* current, const char* next) {
    return (next <= GetStringBufferUtf8() + mTextLength) & (next > current);
}

/**
 * @brief Checks that the next printable UTF-16 character is inside the string.
 * @param current Previous cursor.
 * @param next Cursor returned by the tag processor.
 * @return Whether next advanced and stays inside the string.
 */
bool TextBox::ValidateNextPrintableChar(const u16* current, const u16* next) {
    return (next <= GetStringBuffer() + mTextLength) & (next > current);
}

/**
 * @brief Decodes the UTF-8 character at a cursor.
 * @param text Cursor of the character.
 * @return The UTF-32 code point.
 */
u32 TextBox::GetCharFromPointer(const char* text) {
    char character[4] = {};
    u32 value;
    const char* pStr = text;
    nn::util::PickOutCharacterFromUtf8String(character, &pStr);
    value = 0;
    nn::util::ConvertCharacterUtf8ToUtf32(&value, character);
    return value;
}

/**
 * @brief Returns the UTF-16 code unit at a cursor.
 * @param text Cursor of the character.
 * @return The code unit.
 */
u32 TextBox::GetCharFromPointer(const u16* text) {
    return *text;
}

/**
 * @brief Evaluates every per-character transform curve at a time.
 * @param pInfo Receives the transform.
 * @param frame Curve time.
 */
void TextBox::ApplyPerCharacterTransformCurve(nn::font::PerCharacterTransformInfo* pInfo,
                                              float frame) {
    const int curveCount = mPerCharacterTransform->curveCount;
    const float degreeToAngleIndex =
        nn::util::detail::AngleIndexHalfRound / nn::util::detail::FloatDegree180;

    for (int i = 0; i < curveCount; i++) {
        const PerCharacterTransformCurveInfo& rCurveInfo = mPerCharacterTransform->curveInfos[i];
        const ResAnimationTarget* pTarget = rCurveInfo.pCurve;
        const int curveType = rCurveInfo.curveType;
        const ResHermiteKey* pKeys = pTarget->GetKeys();
        float time = frame;

        if (mPerCharacterTransform->loopType == 1 && pTarget->keyCount >= 2) {
            const float firstFrame = pKeys[0].frame;
            const float lastFrame = pKeys[pTarget->keyCount - 1].frame;

            if (lastFrame < frame) {
                time = firstFrame + std::fmod(frame - firstFrame, lastFrame - firstFrame);
            } else if (firstFrame > frame) {
                time = lastFrame - std::fmod(firstFrame - frame, lastFrame - firstFrame);
            }
        }

        const float value = GetHermiteCurveValue(time, pKeys, pTarget->keyCount);

        if (curveType <= PerCharacterTransformCurveType_TranslateZ) {
            pInfo->translation[static_cast<u32>(curveType)] =
                curveType == PerCharacterTransformCurveType_TranslateY ? -value : value;
        } else if (curveType <= PerCharacterTransformCurveType_RotateZ) {
            const float angle = curveType == PerCharacterTransformCurveType_RotateY ? value : -value;
            const nn::util::AngleIndex angleIndex =
                static_cast<int64_t>(degreeToAngleIndex * angle);
            const u32 axis = static_cast<u32>(curveType - PerCharacterTransformCurveType_RotateX);
            pInfo->rotationSin[axis] = nn::util::SinTable(angleIndex);
            pInfo->rotationCos[axis] = nn::util::CosTable(angleIndex);
        } else if (curveType <= PerCharacterTransformCurveType_LeftTopA) {
            pInfo->lt.v[static_cast<u32>(curveType - PerCharacterTransformCurveType_LeftTopR)] =
                static_cast<int>(value);
        } else if (curveType <= PerCharacterTransformCurveType_LeftBottomA) {
            pInfo->lb.v[static_cast<u32>(curveType - PerCharacterTransformCurveType_LeftBottomR)] =
                static_cast<int>(value);
        } else {
            pInfo->scale[static_cast<u32>(curveType - PerCharacterTransformCurveType_ScaleX)] = value;
        }
    }
}

/**
 * @brief Returns the curve time per unit of advance and the total advance of the string.
 * @tparam CharType Character type of the string buffer.
 * @param pTotalWidth Receives the total advance used by the current transform mode.
 * @param pTagProcessor Tag processor used to skip tags in the string.
 * @return The curve time per unit of advance.
 */
template <typename CharType>
float TextBox::CalculateStepTime(float* pTotalWidth,
                                 nn::font::TagProcessorBase<CharType>* pTagProcessor) {
    const u32 charCount = mDispStringBuffer->m_CharCount;
    *pTotalWidth = 0.0f;

    if (IsFixSpaceInsertSpaceMode()) {
        const float fontWidth = mFontSize.width;
        const CharType* pPos = static_cast<const CharType*>(mTextBuffer);
        const float scale = fontWidth / mFont->GetWidth();

        for (u32 i = 0; i < charCount; i++) {
            const CharType* pPrintable;
            bool isPrintable;

            do {
                pPrintable = pPos;
                pPos = pTagProcessor->AcquireNextPrintableChar(&isPrintable, pPrintable);
            } while (!isPrintable && ValidateNextPrintableChar(pPrintable, pPos));

            if (!isPrintable) {
                break;
            }

            const u32 code = GetCharFromPointer(pPrintable);

            if (pPos == nullptr) {
                break;
            }

            *pTotalWidth += scale * (mFont->GetCharWidth(code) + mCharSpace);
        }

        if (mSizeX == 0.0f) {
            return 0.0f;
        }

        return mPerCharacterTransform->width / mSizeX;
    }

    if (IsFixSpaceMode()) {
        return mPerCharacterTransform->fixSpaceWidth;
    }

    if (!mBits.isPerCharacterTransformSplitByCharWidth) {
        if (charCount - 1 == 0) {
            return 0.0f;
        }

        return mPerCharacterTransform->width / (charCount - 1);
    }

    const CharType* pPos = static_cast<const CharType*>(mTextBuffer);

    for (u32 i = 0; i < charCount; i++) {
        const CharType* pPrintable;
        bool isPrintable;

        do {
            pPrintable = pPos;
            pPos = pTagProcessor->AcquireNextPrintableChar(&isPrintable, pPrintable);
        } while (!isPrintable && ValidateNextPrintableChar(pPrintable, pPos));

        if (!isPrintable) {
            break;
        }

        const u32 code = GetCharFromPointer(pPrintable);

        if (pPos == nullptr) {
            break;
        }

        if (mBits.isPerCharacterTransformOriginToCenter || i != 0) {
            *pTotalWidth += mFont->GetCharWidth(code) + mCharSpace;
        }
    }

    if (*pTotalWidth == 0.0f) {
        return 0.0f;
    }

    return mPerCharacterTransform->width / *pTotalWidth;
}
}  // namespace nn::ui2d
