#include <eui/euiScalableFontTextBoxEx.h>
#include <eui/euiScalableFontMgr.h>
#include <eui/euiFontMgr.h>
#include <eui/euiTagProcessor.h>
#include <eui/euiTextSearcher.h>
#include <eui/euiUtility.h>
#include <nn/ui2d/ui2d_BuildArgSet.h>

namespace eui {
/**
 * @brief Creates an empty text box using a scalable font cache.
 * @param pMgr Glyph cache manager used when calculating this text box; must not be null.
 */
ScalableFontTextBoxEx::ScalableFontTextBoxEx(ScalableFontMgr* pMgr)
    : mFontMgr(pMgr), mGlyphState(0), mGlyphUpdateCount(0), mNextPendingTextBox(nullptr) {}

/**
 * @brief Builds a text box and obtains its glyph cache from the layout's text searcher.
 * @param pResource Base text box resource; must not be null.
 * @param pOverride Optional resource overriding the base text box.
 * @param rArgs Layout build arguments containing an eui text searcher and font manager.
 * @param pParam String initialization data forwarded to the base constructor.
 */
ScalableFontTextBoxEx::ScalableFontTextBoxEx(const nn::ui2d::ResTextBox* pResource,
                                             const nn::ui2d::ResTextBox* pOverride,
                                             const nn::ui2d::BuildArgSet& rArgs,
                                             InitializeStringParam* pParam)
    : TextBoxEx(pResource, pOverride, rArgs, pParam), mGlyphState(0), mGlyphUpdateCount(0),
      mNextPendingTextBox(nullptr) {
    mFontMgr = static_cast<TextSearcher*>(rArgs.pTextSearcher)->mProcessor->mFontMgr->getScalableFontMgr();
}

/**
 * @brief Copies a text box while invalidating its cached glyph readiness.
 * @param rOther Text box supplying properties and the glyph cache manager.
 * @param pLayout Layout that owns the new text box.
 */
ScalableFontTextBoxEx::ScalableFontTextBoxEx(const ScalableFontTextBoxEx& rOther, LayoutEx* pLayout)
    : TextBoxEx(rOther, pLayout), mFontMgr(rOther.mFontMgr), mGlyphState(0), mGlyphUpdateCount(0) {}

/**
 * @brief Returns the scalable font associated with this text box.
 * @return The scalable font, or null when the active font has another runtime type.
 */
inline const nn::font::ScalableFont* ScalableFontTextBoxEx::getScalableFont_() const {
    return DynamicCast<const nn::font::ScalableFont>(GetFont());
}

/**
 * @brief Views the text buffer as UTF-16 input for the glyph cache.
 * @return The text buffer, which must contain UTF-16 text when registering glyphs.
 */
inline const char16_t* ScalableFontTextBoxEx::getGlyphText_() const {
    return reinterpret_cast<const char16_t*>(GetStringBuffer());
}

/** @brief Marks glyphs ready and invalidates the cached text geometry. */
inline void ScalableFontTextBoxEx::markGlyphsReady_() {
    mGlyphState = 2;
    mTextFlags |= 4;
}

/**
 * @brief Registers this text box's glyphs and removes it from the pending chain.
 * @param pMgr Manager processing the pending chain; must not be null.
 * @return The next pending text box, or null at the end of the chain.
 */
ScalableFontTextBoxEx* ScalableFontTextBoxEx::registerGlyphsAndGetNext(ScalableFontMgr* pMgr) {
    const auto* pFont = getScalableFont_();
    if (pMgr->registerGlyphs(getGlyphText_(), GetStringLength(), pFont, -1)) {
        markGlyphsReady_();
    } else {
        mGlyphState = 1;
    }
    mGlyphUpdateCount = pMgr->getUpdateCount();
    auto* pNext = mNextPendingTextBox;
    mNextPendingTextBox = nullptr;
    return pNext;
}

/**
 * @brief Refreshes glyph readiness before calculating the text geometry.
 * @param rInfo Drawing parameters used by the base text box.
 * @param rContext Calculation context, marked dirty while glyphs are not ready.
 * @param forceDirty Whether the caller requires recalculation regardless of cached state.
 */
void ScalableFontTextBoxEx::Calculate(nn::ui2d::DrawInfo& rInfo, CalculateContext& rContext,
                                      bool forceDirty) {
    ScalableFontMgr* pMgr = mFontMgr;
    if (mGlyphState == 0) {
        reserveRegisterGlyphs_(pMgr);
    } else if (mGlyphState == 1) {
        if (mGlyphUpdateCount < pMgr->getUpdateCount()) {
            if (mGlyphUpdateCount + 1 < pMgr->getUpdateCount()) {
                reserveRegisterGlyphs_(pMgr);
            } else {
                mGlyphState = 2;
                mGlyphUpdateCount = pMgr->getUpdateCount();
                mTextFlags |= 4;
            }
        }
    } else if (mGlyphUpdateCount < pMgr->getUpdateCount()) {
        reserveRegisterGlyphs_(pMgr);
    }
    if (mGlyphState != 2) {
        rContext.globalMatrixDirty = true;
    }
    nn::ui2d::TextBox::Calculate(rInfo, rContext, forceDirty);
}

/**
 * @brief Uses available glyphs immediately or queues their registration when the cache is idle.
 * @param pMgr Glyph cache manager to query and enqueue with; must not be null.
 */
void ScalableFontTextBoxEx::reserveRegisterGlyphs_(ScalableFontMgr* pMgr) {
    const auto* pFont = getScalableFont_();
    if (pMgr->isGlyphsReady(getGlyphText_(), GetStringLength(), pFont)) {
        markGlyphsReady_();
        mGlyphUpdateCount = pMgr->getUpdateCount();
    } else if (!pMgr->isCacheUpdatePending()) {
        mNextPendingTextBox = pMgr->reserveRegisterGlyphs(this);
    }
}

/**
 * @brief Replaces the string without preprocessing and invalidates cached glyph readiness.
 * @param pText UTF-16 source text accepted by the base text box.
 * @param length Number of UTF-16 code units supplied.
 * @return Number of code units copied by the base text box.
 */
u16 ScalableFontTextBoxEx::setStringNoPreproces(const char16_t* pText, u16 length) {
    const u16 result = TextBoxEx::setStringNoPreproces(pText, length);
    mGlyphState = 0;
    return result;
}

/**
 * @brief Draws the text only when its glyphs are ready.
 * @param rInfo Drawing parameters for this pane.
 * @param rCommands Command buffer receiving text draw commands.
 */
void ScalableFontTextBoxEx::DrawSelf(nn::ui2d::DrawInfo& rInfo, nn::gfx::CommandBuffer& rCommands) {
    if (mGlyphState == 2) {
        nn::ui2d::TextBox::DrawSelf(rInfo, rCommands);
    }
}

/**
 * @brief Forwards text preprocessing and invalidates cached glyph readiness.
 * @param pOutput Destination string passed to the base preprocessor.
 * @param pOutputLength Output length/result parameter forwarded unchanged.
 * @param pProcessedLength Input progress/result parameter forwarded unchanged.
 * @param pText UTF-16 input text passed to the base preprocessor.
 * @param length Input text length forwarded unchanged.
 * @param page Page selection passed to the base preprocessor.
 * @param flag Preprocessing option forwarded unchanged; its precise meaning is not yet recovered.
 * @param pUserData Caller data passed through to tag processing.
 */
void ScalableFontTextBoxEx::doPreprocess_(sead::WBufferedSafeString* pOutput, u32* pOutputLength,
                                          u32* pProcessedLength, const char16_t* pText, u32 length, int page,
                                          bool flag, void* pUserData) {
    TextBoxEx::doPreprocess_(pOutput, pOutputLength, pProcessedLength, pText, length, page, flag, pUserData);
    mGlyphState = 0;
}
} // namespace eui
