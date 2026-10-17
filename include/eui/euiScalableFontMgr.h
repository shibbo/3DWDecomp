#pragma once
#include <heap/seadDisposer.h>
#include <container/seadBuffer.h>
#include <prim/seadSafeString.h>
#include <nn/font/font_ScalableFont.h>
#include <thread/seadThread.h>
namespace eui {
class ScalableFontTextBoxEx;
class ScalableFontMgr {
    SEAD_SINGLETON_DISPOSER(ScalableFontMgr);
public:
    class UpdateTextureCacheThread : public sead::Thread {
    public:
        bool requestUpdate();
        void calc_(sead::MessageQueue::Element message) override;

        bool isUpdatePending() const { return mUpdatePending; }

        nn::font::TextureCache* mTextureCache;
        volatile bool mUpdatePending;
    };

    struct FontParameter {
        FontParameter();
        FontParameter(const char* pName, int size, u16 face, int value);
        const char* name;
        int size;
        u16 face;
        int _10;
    };

    struct InitializeArg {
        InitializeArg();
        sead::Heap* heap;
        nn::font::TextureCache::InitializeArg* textureCacheArg;
        const FontParameter* fontParameters;
        s32 fontParameterNum;
        s32 threadPriority;
        u32 threadCoreMask;
    };

    struct FontEntry { sead::SafeString name; nn::font::ScalableFont font; };
    ScalableFontMgr();
    virtual ~ScalableFontMgr();
    void initialize(const InitializeArg& rArg);
    bool isGlyphsReady(const char16_t* pText, u32 length, const nn::font::ScalableFont* pFont);
    bool registerGlyphs(const char16_t* pText, u32 length, const nn::font::ScalableFont* pFont, int lockGroup);
    bool registerGlyphs_(const char16_t* pText, u32 length, const nn::font::ScalableFont* pFont, int lockGroup, bool checkOnly);
    ScalableFontTextBoxEx* reserveRegisterGlyphs(ScalableFontTextBoxEx* pTextBox);
    void update();
    /**
     * @brief Returns the generation of the glyph texture cache.
     * @return Number of completed cache updates.
     */
    u32 getUpdateCount() const { return _58; }
    /**
     * @brief Reports whether the texture cache is being updated asynchronously.
     * @return True while a cache update is pending.
     */
    bool isCacheUpdatePending() const { return _5d != 0; }
    nn::font::ScalableFont* getFont(const sead::SafeString& rName);
    const nn::font::ScalableFont* getFont(const sead::SafeString& rName) const;
    const char* findFontName(const nn::font::ScalableFont* pFont) const;
    void dumpTextureCacheGlyphTreeMap() const;
    u32 getTextureCacheNoSpaceError() const;
    void clearNoSpaceError();
    void clearLockAllGlyphs(int lockGroup);
    bool isNeedPlot_(char16_t code, u32 size, u16 face);
    void* _28;
    nn::font::TextureCache* mTextureCache;
    sead::Buffer<FontEntry> mFonts;
    UpdateTextureCacheThread* mUpdateThread;
    ScalableFontTextBoxEx* volatile mReservedTextBox;
    u32 _58;
    u8 _5c, _5d, _5e;
};

static_assert(sizeof(ScalableFontMgr) == 0x60, "ScalableFontMgr size");
static_assert(sizeof(ScalableFontMgr::FontEntry) == 0x48, "FontEntry size");
}
