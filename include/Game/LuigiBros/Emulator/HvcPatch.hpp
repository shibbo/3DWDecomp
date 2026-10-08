#pragma once

#include "LuigiBros/Emulator/Hvc.hpp"
#include "LuigiBros/Utility/Serialize.hpp"
#include <cmath>
#include <cstring>

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Title specific fixes applied around the emulation of a frame.
 *
 * Configure picks the hooks of the running title; the emulator calls them when it is attached,
 * detached, saved, loaded, started and around every frame.
 */
class CHvcPatch {
public:
    using Patch = void (CHvcPatch::*)();

    /// Offset of the output palette (32-bit colors) inside the machine memory.
    static constexpr u64 cMemory_OutputPalette = 0x304530;

    /// Offset of the PPU palette RAM inside the machine memory.
    static constexpr u64 cMemory_Palette = CPlatformHvcUnit::cMemory_Palette;

    using _SEntity = CPlatformHvcUnit::_SEntity;

    /// Get the only instance.
    static CHvcPatch& GetInstance() {
        static CHvcPatch instance;
        return instance;
    }

    /// Construct the patch set with every hook doing nothing.
    CHvcPatch() {
        mHvc = nullptr;
        std::strncpy(mName, "PATCH  ", sizeof(mName));
        mAttach = &CHvcPatch::_PatchDefault;
        mDetach = &CHvcPatch::_PatchDefault;
        mExport = &CHvcPatch::_PatchDefault;
        mImport = &CHvcPatch::_PatchDefault;
        mPostCommence = &CHvcPatch::_PatchDefault;
        mAnteProgress = &CHvcPatch::_PatchDefault;
        mPostProgress = &CHvcPatch::_PatchDefault;
    }

    virtual ~CHvcPatch() {}

    /**
     * @brief Bind the patch set to an emulator and reset every hook.
     * @param pHvc The emulator.
     */
    void Reset(CHvc* pHvc) {
        mHvc = pHvc;
        mAttach = &CHvcPatch::_PatchDefault;
        mDetach = &CHvcPatch::_PatchDefault;
        mExport = &CHvcPatch::_PatchDefault;
        mImport = &CHvcPatch::_PatchDefault;
        mPostCommence = &CHvcPatch::_PatchDefault;
        mAnteProgress = &CHvcPatch::_PatchDefault;
        mPostProgress = &CHvcPatch::_PatchDefault;
    }

    void Configure();

    /// Run the hook called when a title is attached.
    void Attach() { (this->*mAttach)(); }

    /// Run the hook called when a title is detached.
    void Detach() { (this->*mDetach)(); }

    /// Run the hook called before the state is saved.
    void Export() { (this->*mExport)(); }

    /// Run the hook called after the state is loaded.
    void Import() { (this->*mImport)(); }

    /// Run the hook called after the emulation has started.
    void PostCommence() { (this->*mPostCommence)(); }

    /// Run the hook called before a frame is emulated.
    void AnteProgress() { (this->*mAnteProgress)(); }

    /// Run the hook called after a frame is emulated.
    void PostProgress() { (this->*mPostProgress)(); }

    void _PatchDefault() {}

    void _PatchAttach_LinkNoBouken_NonJP();
    void _PatchAnteProgress_LinkNoBouken();
    void _PatchPostProgress_LinkNoBouken();
    void _PatchAttach_LinkNoBouken();
    void _PatchAttach_Kirby();
    void _PatchPostCommence_Kirby();
    void _PatchPostProgress_Kirby();
    void _PatchPostProgress_SuperMario3();
    void _PatchAnteProgress_SuperMarioUSA();
    void _PatchPostProgress_SuperMarioUSA();
    void _PatchPostProgress_Excitebike();
    void _PatchAttach_Metroid();
    void _PatchAttach_BalloonFight();
    void _PatchAttach_PunchOut();
    void _PatchPostProgress_YoshiNoTamago();
    void _PatchAttach_Mother();
    void _PatchAnteProgress_Mother();
    void _PatchAttach_WreckingCrew();
    void _PatchAttach_YoshiNoCookie();
    void _PatchExport_ParthenaNoKagami();
    void _PatchImport_ParthenaNoKagami();
    void _PatchPostCommence_ParthenaNoKagami();
    void _PatchAnteProgress_ParthenaNoKagami();
    void _PatchPostProgress_ParthenaNoKagami();
    bool _replacePaletteColorToDirectColorInOrder(const u8* pPalette, const u32* pColor,
                                                  const u8* pOrder);

private:
    /// Get the backing memory of the emulated machine.
    u8* getMemory() const { return mHvc->GetMemory(); }

public:
    CHvc* mHvc;
    char mName[8];
    Patch mAttach;
    Patch mDetach;
    Patch mExport;
    Patch mImport;
    Patch mPostCommence;
    Patch mAnteProgress;
    Patch mPostProgress;
};

static_assert(sizeof(CHvcPatch) == 0x88, "CHvcPatch size");

/**
 * @brief Pick the hooks of the running title.
 */
inline void CHvcPatch::Configure() {
    mAttach = &CHvcPatch::_PatchDefault;
    mDetach = &CHvcPatch::_PatchDefault;
    mExport = &CHvcPatch::_PatchDefault;
    mImport = &CHvcPatch::_PatchDefault;
    mPostCommence = &CHvcPatch::_PatchDefault;
    mAnteProgress = &CHvcPatch::_PatchDefault;
    mPostProgress = &CHvcPatch::_PatchDefault;

    switch (mHvc->mTitleElement.mTitleId) {
    case 9:
    case 10:
    case 354:
        mPostProgress = &CHvcPatch::_PatchPostProgress_YoshiNoTamago;
        break;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
        mAttach = &CHvcPatch::_PatchAttach_Kirby;
        mPostCommence = &CHvcPatch::_PatchPostCommence_Kirby;
        mPostProgress = &CHvcPatch::_PatchPostProgress_Kirby;
        break;
    case 18:
    case 20:
        mAttach = &CHvcPatch::_PatchAttach_LinkNoBouken_NonJP;
        mAnteProgress = &CHvcPatch::_PatchAnteProgress_LinkNoBouken;
        mPostProgress = &CHvcPatch::_PatchPostProgress_LinkNoBouken;
        break;
    case 19:
        mAttach = &CHvcPatch::_PatchAttach_LinkNoBouken;
        mAnteProgress = &CHvcPatch::_PatchAnteProgress_LinkNoBouken;
        mPostProgress = &CHvcPatch::_PatchPostProgress_LinkNoBouken;
        break;
    case 110:
    case 111:
    case 112:
        mPostProgress = &CHvcPatch::_PatchPostProgress_Excitebike;
        break;
    case 113:
    case 114:
        mExport = &CHvcPatch::_PatchExport_ParthenaNoKagami;
        mImport = &CHvcPatch::_PatchImport_ParthenaNoKagami;
        mPostCommence = &CHvcPatch::_PatchPostCommence_ParthenaNoKagami;
        mAnteProgress = &CHvcPatch::_PatchAnteProgress_ParthenaNoKagami;
        mPostProgress = &CHvcPatch::_PatchPostProgress_ParthenaNoKagami;
        break;
    case 123:
    case 124:
    case 125:
        mAttach = &CHvcPatch::_PatchAttach_PunchOut;
        break;
    case 155:
    case 156:
    case 157:
        mAnteProgress = &CHvcPatch::_PatchAnteProgress_SuperMarioUSA;
        mPostProgress = &CHvcPatch::_PatchPostProgress_SuperMarioUSA;
        break;
    case 160:
        mAttach = &CHvcPatch::_PatchAttach_Metroid;
        break;
    case 187:
    case 188:
    case 189:
        mAttach = &CHvcPatch::_PatchAttach_BalloonFight;
        break;
    case 204:
    case 205:
        mAttach = &CHvcPatch::_PatchAttach_WreckingCrew;
        break;
    case 209:
        mPostProgress = &CHvcPatch::_PatchPostProgress_SuperMario3;
        break;
    case 211:
        mAttach = &CHvcPatch::_PatchAttach_Mother;
        mAnteProgress = &CHvcPatch::_PatchAnteProgress_Mother;
        break;
    case 212:
        mAttach = &CHvcPatch::_PatchAttach_YoshiNoCookie;
        break;
    }
}

/// Offset of the Zelda II state inside CHvc::mPatchWork (the palette copy comes first).
static constexpr u64 cLinkNoBoukenWorkOffset = 0x20;

/**
 * @brief State of the Zelda II (Link no Bouken) fixes inside CHvc::mPatchWork.
 */
struct SPatchWorkLinkNoBouken {
    /**
     * @brief Colors of one analyzed frame.
     */
    struct SFrame {
        u32 mOutput[32];
        u32 mColor[32];
        u8 mLuminance[32];
    };

    u64 mFlashStep;
    u64 mFlashColor;
    s64 mFlashCount;
    s64 mFlashTimer;
    s32 mPhase;
    u16 mHistogram[32];
    u16 mFade[32];
    SFrame mFrame[2];
};

/**
 * @brief Work area of the Kid Icarus (Parthena no Kagami) fixes inside CHvc::mPatchWork.
 */
struct SPatchWorkParthenaNoKagami {
    u8 mPalette[16];
    u32 mColor;
    u32 mAverage;
    u32 mHistory[2];
    u16 mTimer;
    u32 mCounter;
};

/// Background colors of the Zelda II palace flash, in order.
static const u8 cLinkFlashSequence[4] = {0x16, 0x2a, 0x16, 0x12};

/// Replacement colors of the Zelda II palace flash.
static const u32 cLinkFlashColor[4] = {0x387038ff, 0x30783cff, 0x387038ff, 0x2c7440ff};

/// Delay of the Kirby's Adventure raster effects for each version.
static const u64 cKirbyRasterDelay[5] = {0xa00, 0xc00, 0xa00, 0xc00, 0xa00};

/// Background palettes of the Kid Icarus screen flashes.
static const u8 cParthenaFlashPalette[3][16] = {
    {0x16, 0x0f, 0x0f, 0x0f, 0x0f, 0x21, 0x13, 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x36, 0x14,
     0x15},
    {0x16, 0x16, 0x16, 0x0f, 0x0f, 0x21, 0x13, 0x02, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x36, 0x14,
     0x15},
    {0x10, 0x20, 0x22, 0x02, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
     0x10},
};

/// Background palettes of Kid Icarus once each flash is over.
static const u8 cParthenaNormalPalette0[16] = {0x0f, 0x30, 0x15, 0x0c, 0x0f, 0x21, 0x13, 0x02,
                                               0x0f, 0x27, 0x18, 0x09, 0x0f, 0x36, 0x14, 0x15};
static const u8 cParthenaNormalPalette1[16] = {0x0f, 0x0f, 0x0f, 0x0c, 0x0f, 0x21, 0x13, 0x02,
                                               0x0f, 0x27, 0x18, 0x09, 0x0f, 0x36, 0x14, 0x15};
static const u8 cParthenaNormalPalette2[16] = {0x0f, 0x20, 0x22, 0x02, 0x0f, 0x0f, 0x0f, 0x0f,
                                               0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f};

/**
 * @brief Compute the perceived brightness of a color.
 * @param color The color (red, green, blue and alpha from the highest byte).
 * @return The brightness (0-200).
 */
inline u8 calcPatchLuminance(u32 color) {
    u8 alpha = color;
    u8 red = color >> 24;
    u8 green = color >> 16;
    u8 blue = color >> 8;
    if (alpha == 0 || (green | red | blue) == 0) {
        return 0;
    }

    return std::pow((blue * 0.319 + (red * 0.838 + green * 1.643)) / 714.0, 2.2) * 200.0;
}

/**
 * @brief Move a color channel by up to 8 towards a target.
 * @param value The channel.
 * @param target The target.
 * @return The new channel.
 */
inline u8 approachChannel(u8 value, u8 target) {
    if (value < target) {
        return value + 8 < target ? value + 8 : target;
    }

    return target + 8 < value ? value - 8 : target;
}

/**
 * @brief Clear the Zelda II work area (versions outside Japan).
 */
inline void CHvcPatch::_PatchAttach_LinkNoBouken_NonJP() {
    auto pWork =
        reinterpret_cast<SPatchWorkLinkNoBouken*>(mHvc->mPatchWork + cLinkNoBoukenWorkOffset);
    std::memset(pWork, 0, sizeof(*pWork));
    pWork->mPhase = -1;
}

/**
 * @brief Remember the palette of Zelda II before a frame.
 */
inline void CHvcPatch::_PatchAnteProgress_LinkNoBouken() {
    CHvc* pHvc = mHvc;
    std::memcpy(pHvc->mPatchWork, pHvc->GetMemory() + cMemory_Palette, cLinkNoBoukenWorkOffset);
}

/**
 * @brief Soften the palette flashes of Zelda II after a frame.
 */
inline void CHvcPatch::_PatchPostProgress_LinkNoBouken() {
    CHvc* pHvc = mHvc;
    const u8* pPalette = pHvc->mPatchWork;
    for (s32 i = 0; i < 32; i++) {
        pHvc->mSpecialColor[i] = 0;
    }

    u8* pMemory = pHvc->GetMemory();
    u8 aIndex0[4] = {0xff, 0x00, 0x10, 0x30};
    u32 aColor0[4] = {0, 0, 0, 0x8c8c8cff};
    if (!_replacePaletteColorToDirectColorInOrder(aIndex0, aColor0, pPalette)) {
        u8 aIndex1[4] = {0xff, 0x1c, 0x0c, 0x2c};
        u32 aColor1[4] = {0, 0x004252ff, 0x074858ff, 0x0e525eff};
        if (!_replacePaletteColorToDirectColorInOrder(aIndex1, aColor1, pPalette)) {
            u8 aIndex2[4] = {0x16, 0x30, 0xff, 0xff};
            u32 aColor2[4] = {0x551a00ff, 0, 0, 0};
            if (*reinterpret_cast<u16*>(pMemory + cMemory_Palette) == 0x3006) {
                _replacePaletteColorToDirectColorInOrder(aIndex2, aColor2, pPalette);
            }
        }
    }

    auto pWork =
        reinterpret_cast<SPatchWorkLinkNoBouken*>(pHvc->mPatchWork + cLinkNoBoukenWorkOffset);
    SPatchWorkLinkNoBouken::SFrame* pPrevious;
    s32 current;
    if (static_cast<u32>(pWork->mPhase) < 2) {
        pPrevious = &pWork->mFrame[pWork->mPhase];
        current = pWork->mPhase == 1 ? 0 : pWork->mPhase + 1;
        pWork->mPhase = current;
    } else {
        current = 0;
        pPrevious = nullptr;
        pWork->mPhase = 0;
    }

    SPatchWorkLinkNoBouken::SFrame& rCurrent = pWork->mFrame[current];
    const u64* pOutput = reinterpret_cast<const u64*>(getMemory() + cMemory_OutputPalette);
    for (s32 i = 0; i < 32; i++) {
        rCurrent.mColor[i] = pOutput[pPalette[i]];
    }

    for (s32 i = 0; i < 32; i++) {
        pWork->mHistogram[i] = 0;
    }

    const u8* pPixel = mHvc->mEntityData.mFrameBuffer;
    for (u32 i = 0; i < CHvc::cScreenWidth * CHvc::cScreenHeight * 4; i += 4) {
        u8 palette = pPixel[i + 2];
        if (palette < 32) {
            pWork->mHistogram[palette]++;
        }
    }

    for (s32 i = 0; i < 32; i++) {
        rCurrent.mLuminance[i] = calcPatchLuminance(rCurrent.mColor[i]);
    }

    bool isFlash = false;
    if (pWork->mFlashCount != 0) {
        u64 step = pWork->mFlashStep;
        if (pPalette[0] == cLinkFlashSequence[step]) {
            s64 count = pWork->mFlashCount;
            pWork->mFlashCount = count + 1;
            pWork->mFlashStep = (step + 1) & 3;
            if (count >= 7) {
                pWork->mFlashCount = count - 3;
                isFlash = true;
            } else if (count >= 3) {
                isFlash = true;
            }
        } else {
            pWork->mFlashCount = 0;
            pWork->mFlashStep = 0;
        }
    } else {
        pWork->mFlashStep = 0;
        if (pPalette[0] == 0x12) {
            pWork->mFlashCount = 1;
            pWork->mFlashStep = 0;
        } else if (pPalette[0] == 0x16) {
            pWork->mFlashCount = 1;
            pWork->mFlashStep = 1;
        } else if (pPalette[0] == 0x2a) {
            pWork->mFlashCount = 1;
            pWork->mFlashStep = 2;
        } else {
            pWork->mFlashStep = 0;
        }
    }

    if (isFlash) {
        pWork->mFlashTimer = 0x20;
    } else if (pWork->mFlashTimer > 0) {
        pWork->mFlashTimer--;
        isFlash = pWork->mFlashTimer > 0;
    }

    if (isFlash) {
        u64 color = pWork->mFlashColor + 1;
        pWork->mFlashColor = color & 0x1f;
        u32 flashColor = cLinkFlashColor[(color >> 3) & 3];
        rCurrent.mColor[0] = flashColor;
        rCurrent.mLuminance[0] = calcPatchLuminance(flashColor);
        pWork->mFade[0] = 0;
    } else {
        pWork->mFlashColor = 0;
    }

    if (pPrevious == nullptr) {
        return;
    }

    for (s32 i = 0; i < 32; i++) {
        u32 output = rCurrent.mOutput[i];
        if (output != 0) {
            u32 color = rCurrent.mColor[i];
            u32 red = (output >> 24) + (color >> 24);
            u32 green = ((output >> 16) & 0xff) + ((color >> 16) & 0xff);
            u32 blue = ((output >> 8) & 0xff) + ((color >> 8) & 0xff);
            red = (red < 255 ? red : 255) >> 1;
            green = (green < 255 ? green : 255) >> 1;
            blue = (blue < 255 ? blue : 255) >> 1;
            red = red < 255 ? red : 255;
            green = green < 255 ? green : 255;
            blue = blue < 255 ? blue : 255;
            rCurrent.mOutput[i] = red << 24 | green << 16 | blue << 8 | 0xff;
        } else {
            rCurrent.mOutput[i] = rCurrent.mColor[i];
        }

        if (pWork->mFade[i] != 0) {
            pWork->mFade[i]--;
        }

        if (pWork->mHistogram[i] < 13107) {
            continue;
        }

        s32 difference = rCurrent.mLuminance[i] - pPrevious->mLuminance[i];
        if (difference < 0) {
            difference = -difference;
        }

        u16 fade;
        if (difference > 64) {
            fade = 0x80;
        } else if (difference > 32) {
            fade = 0x40;
        } else if (difference > 16) {
            fade = 0x30;
        } else if (difference > 8) {
            fade = 0x20;
        } else if (difference >= 5) {
            fade = 0x10;
        } else {
            fade = 0;
        }

        if (fade != 0) {
            pWork->mFade[i] += fade;
        }

        if (pWork->mFade[i] >= 1024) {
            pWork->mFade[i] = 1024;
        } else if (pWork->mFade[i] <= 64) {
            continue;
        }

        if (rCurrent.mColor[i] == pPrevious->mColor[i]) {
            pWork->mFade[i] = pWork->mFade[i] * 3 / 4;
        }

        u32 color = rCurrent.mColor[i];
        u32 previous = pPrevious->mColor[i];
        u32 red = color >> 24;
        u32 previousRed = previous >> 24;
        u32 green = (color >> 16) & 0xff;
        u32 previousGreen = (previous >> 16) & 0xff;
        u32 blue = (color >> 8) & 0xff;
        u32 previousBlue = (previous >> 8) & 0xff;
        red = red < previousRed ? previousRed - 1 : (red > previousRed ? previousRed + 1 : red);
        green = green < previousGreen ? previousGreen - 1 :
                                        (green > previousGreen ? previousGreen + 1 : green);
        blue = blue < previousBlue ? previousBlue - 1 :
                                     (blue > previousBlue ? previousBlue + 1 : blue);
        u32 newColor = (red & 0xff) << 24 | (green & 0xff) << 16 | (blue & 0xff) << 8 | 0xff;
        rCurrent.mColor[i] = newColor;
        rCurrent.mLuminance[i] = calcPatchLuminance(newColor);
    }

    for (s32 i = 0; i < 32; i++) {
        if (mHvc->mSpecialColor[i] == 0) {
            mHvc->mSpecialColor[i] = rCurrent.mColor[i];
        }
    }

    u8* pFrame = mHvc->mEntityData.mFrameBuffer;
    for (u32 i = 0; i < CHvc::cScreenWidth * CHvc::cScreenHeight * 4; i += 4) {
        if (pFrame[i + 2] < 32 && static_cast<s8>(pFrame[i]) >= 0) {
            pFrame[i] ^= 0x80;
        }
    }
}

/**
 * @brief Patch the program of Zelda II (Japanese version) and clear its work area.
 */
inline void CHvcPatch::_PatchAttach_LinkNoBouken() {
    const u8 cCode[18] = {0xae, 0x0e, 0x05, 0xe0, 0x06, 0xb0, 0x05, 0xa9, 0x16,
                          0x4c, 0xbd, 0x7f, 0x29, 0x03, 0xaa, 0x4c, 0xba, 0x7f};
    std::memcpy(getMemory() + 0xb13, cCode, sizeof(cCode));

    _SEntity& rEntity = mHvc->mEntityData;
    rEntity._d78 = rEntity.mCpuClock + 0xd00 - rEntity.mCpuClockBase;
    _PatchAttach_LinkNoBouken_NonJP();
}

/**
 * @brief Adjust the timing of the Kirby's Adventure raster effects.
 */
inline void CHvcPatch::_PatchAttach_Kirby() {
    u64 index = mHvc->mTitleElement.mTitleId - 13;
    if (index > 4) {
        return;
    }

    u64 delay = cKirbyRasterDelay[index];
    _SEntity& rEntity = mHvc->mEntityData;
    u64 clock = rEntity.mCpuClock - rEntity.mCpuClockBase + delay;
    mHvc->mEntityData._11e8[3] = clock;
    mHvc->mEntityData._11e8[2] = clock;
    mHvc->mEntityData._11e8[1] = clock;
    mHvc->mEntityData._11e8[0] = clock;
}

/**
 * @brief Prepare the Kirby's Adventure flash softening after the emulation started.
 */
inline void CHvcPatch::_PatchPostCommence_Kirby() {
    u8* pMemory = getMemory();
    pMemory[0x3046a1] = 0;
    pMemory[0x304622] = 1;
    for (s32 i = 0; i < 32; i++) {
        mHvc->mSpecialColor[i] = *reinterpret_cast<u64*>(pMemory + cMemory_OutputPalette);
    }
}

/**
 * @brief Soften the screen flashes of Kirby's Adventure after a frame.
 */
inline void CHvcPatch::_PatchPostProgress_Kirby() {
    u8* pMemory = mHvc->GetMemory();
    u8* pSkip = pMemory + 0x304622;
    if (*pSkip == 0) {
        u8* pFrame = mHvc->mEntityData.mFrameBuffer;
        const u8* pPalette = pMemory + cMemory_Palette;
        const u64* pOutput = reinterpret_cast<const u64*>(pMemory + cMemory_OutputPalette);
        u8* pFade = pMemory + 0x3046a1;
        u8* pFlashColor = pMemory + 0x3046a2;
        const u8 cFlashPalette[6][16] = {
            {0x0c, 0x11, 0x21, 0x31, 0x0c, 0x1c, 0x2c, 0x0c, 0x0c, 0x38, 0x01, 0x12, 0x0c, 0x37,
             0x27, 0x07},
            {0x0f, 0x04, 0x14, 0x33, 0x0f, 0x1c, 0x2c, 0x0c, 0x0f, 0x38, 0x01, 0x12, 0x0f, 0x37,
             0x27, 0x07},
            {0x1c, 0x11, 0x21, 0x31, 0x1c, 0x1c, 0x2c, 0x1c, 0x1c, 0x38, 0x11, 0x12, 0x1c, 0x37,
             0x27, 0x07},
            {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x37,
             0x27, 0x07},
            {0x2c, 0x21, 0x21, 0x31, 0x2c, 0x2c, 0x2c, 0x2c, 0x2c, 0x38, 0x21, 0x22, 0x2c, 0x37,
             0x27, 0x07},
            {0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x21, 0x22, 0x30, 0x37,
             0x27, 0x07},
        };

        u8 color;
        if (std::memcmp(cFlashPalette[0], pPalette, 16) == 0) {
            color = 0x0c;
        } else if (std::memcmp(cFlashPalette[1], pPalette, 16) == 0) {
            color = 0x0f;
        } else if (std::memcmp(cFlashPalette[2], pPalette, 16) == 0) {
            color = 0x1c;
        } else if (std::memcmp(cFlashPalette[3], pPalette, 16) == 0) {
            color = 0x20;
        } else if (std::memcmp(cFlashPalette[4], pPalette, 16) == 0) {
            color = 0x2c;
        } else if (std::memcmp(cFlashPalette[5], pPalette, 16) == 0) {
            color = 0x30;
        } else {
            color = 0;
        }

        if (color != 0) {
            *pFlashColor = color;
            const u8* pTarget = reinterpret_cast<const u8*>(&pOutput[color]);
            u8* pFlash = pMemory + 0x3045a0;
            pFlash[0] = approachChannel(pFlash[0], pTarget[0]);
            pFlash[1] = approachChannel(pFlash[1], pTarget[1]);
            pFlash[2] = approachChannel(pFlash[2], pTarget[2]);
            pFlash[3] = 0xff;
            for (u32 i = 0; i < CHvc::cScreenWidth * CHvc::cScreenHeight * 4; i += 4) {
                if (pFrame[i] < 64 && (pFrame[i + 2] & 3) == 0) {
                    pFrame[i] = 0x0e;
                }
            }
        } else {
            *pFlashColor = 0;
            *reinterpret_cast<u64*>(pMemory + 0x3045a0) = 0xff;
        }

        u8 background = pPalette[0];
        s32 differentNum = 0;
        for (s32 i = 1; i < 16; i++) {
            if (pPalette[i] != background) {
                differentNum++;
            }
        }

        bool isFading;
        if (differentNum == 0) {
            *pFade = 15;
            isFading = true;
        } else {
            isFading = *pFade != 0 && --*pFade != 0;
        }

        if (isFading) {
            for (s32 i = 0; i < 16; i++) {
                const u8* pTarget =
                    reinterpret_cast<const u8*>(&pOutput[i == 0 ? background : pPalette[i]]);
                u8* pColor = reinterpret_cast<u8*>(&mHvc->mSpecialColor[i]);
                pColor[0] = approachChannel(pColor[0], pTarget[0]);
                pColor[1] = approachChannel(pColor[1], pTarget[1]);
                pColor[3] = 0xff;
                pColor[2] = approachChannel(pColor[2], pTarget[2]);
            }

            for (u32 i = 0; i < CHvc::cScreenWidth * CHvc::cScreenHeight * 4; i += 4) {
                if (pFrame[i] < 64 && pFrame[i + 2] < 16) {
                    pFrame[i] ^= 0x80;
                }
            }
        } else {
            for (s32 i = 0; i < 32; i++) {
                mHvc->mSpecialColor[i] = pOutput[pPalette[i]];
            }
        }
    }

    *pSkip = 0;
}

/**
 * @brief Adjust the timing of the Super Mario Bros. 3 status bar split.
 */
inline void CHvcPatch::_PatchPostProgress_SuperMario3() {
    if (mHvc->mTitleElement.mTitleId != 209) {
        return;
    }

    _SEntity& rEntity = mHvc->mEntityData;
    u64 clock = rEntity.mCpuClock - rEntity.mCpuClockBase + 0x1400;
    mHvc->mEntityData._1280[2] = clock;
    mHvc->mEntityData._1280[1] = clock;
    mHvc->mEntityData._1280[0] = clock;
}

/**
 * @brief Copy a color of the output palette byte by byte.
 * @param pMemory The machine memory.
 * @param to The offset of the destination color.
 * @param from The offset of the source color.
 */
inline void copyPatchColor(u8* pMemory, u64 to, u64 from) {
    pMemory[to + 0] = pMemory[from + 0];
    pMemory[to + 1] = pMemory[from + 1];
    pMemory[to + 2] = pMemory[from + 2];
    pMemory[to + 3] = pMemory[from + 3];
}

/**
 * @brief Fix colors of the Super Mario USA output palette before a frame.
 */
inline void CHvcPatch::_PatchAnteProgress_SuperMarioUSA() {
    u8* pMemory = getMemory();
    *reinterpret_cast<u32*>(pMemory + 0x304564) = 0xffde8b8b;
    copyPatchColor(pMemory, 0x304568, 0x3045c8);
    copyPatchColor(pMemory, 0x3045a8, 0x3045d8);
    copyPatchColor(pMemory, 0x3045e8, 0x3045b8);
    copyPatchColor(pMemory, 0x304628, 0x3045c8);
}

/**
 * @brief Fix the scrolling and colors of Super Mario USA after a frame.
 */
inline void CHvcPatch::_PatchPostProgress_SuperMarioUSA() {
    CHvc* pHvc = mHvc;
    u64 titleId = pHvc->mTitleElement.mTitleId;
    if (titleId == 156) {
        const u8* pMemory = pHvc->GetMemory();
        if (pMemory[0x304510] != 0x0f || pMemory[0x304511] != 0x30 || pMemory[0x304512] != 0x0d ||
            pMemory[0x304513] != 0x01) {
            return;
        }

        u8* pLine = pHvc->mEntityData.mFrameBuffer + 8 * CHvc::cScreenWidth * 4;
        for (s32 y = 8; y < CHvc::cScreenHeight; y++) {
            for (s32 x = 0; x < CHvc::cScreenWidth * 4; x += 4) {
                u8* pPixel = pLine + x;
                if (pPixel[0] < 64 && pPixel[2] == 0 && pPixel[2 - CHvc::cScreenWidth * 4] == 1) {
                    pPixel[0] = 0x80;
                }
            }

            pLine += CHvc::cScreenWidth * 4;
        }

        mHvc->mSpecialColor[0] = 0x666699ff;
    } else if (titleId == 155) {
        const u8* pMemory = pHvc->GetMemory();
        if (pMemory[0x302888] == 0x5a && pMemory[0x302889] == 0x9a && pMemory[0x302896] == 0x9a &&
            pMemory[0x302897] == 0x5c && pMemory[0x302908] == 0x5b && pMemory[0x302909] == 0x9a &&
            pMemory[0x302916] == 0x9a && pMemory[0x302917] == 0x5d && pMemory[0x302a2b] == 0x51 &&
            pMemory[0x302a2c] == 0x53 && pMemory[0x302a33] == 0x51 && pMemory[0x302a34] == 0x53 &&
            pMemory[0x302aac] == 0x3c && pMemory[0x302aad] == 0x3d && pMemory[0x302ab2] == 0x3c &&
            pMemory[0x302ab3] == 0x3d) {
            pHvc->mCustomParameter.mLineScroll[191].mX = 0;
        } else {
            pHvc->mCustomParameter.mLineScroll[191].mX = 0x200;
        }
    }
}

/**
 * @brief Keep the Excitebike course from scrolling under the status bar.
 */
inline void CHvcPatch::_PatchPostProgress_Excitebike() {
    static const u8 checkVram[0xc0] = {
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xb5, 0xb7, 0xb7, 0x03, 0x1b, 0x0d, 0xb7, 0xb7, 0xb6, 0xfe, 0x4c, 0x5a,
        0x1d, 0x0e, 0x16, 0x19, 0x7a, 0x8b, 0xfe, 0xb5, 0xb7, 0xb7, 0x1d, 0x12, 0x16, 0x0e, 0xb7,
        0xb6, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xb0, 0x01, 0xfb, 0x02, 0x04, 0xfb, 0x00, 0x00, 0xb1,
        0x4b, 0x4d, 0x5b, 0xfc, 0xfc, 0xfc, 0xfc, 0x7b, 0x4d, 0x8c, 0xb0, 0x00, 0xfb, 0x01, 0x05,
        0xfb, 0x03, 0x04, 0xb1, 0xfe, 0xfe, 0xfe, 0xfe, 0xb2, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3,
        0xb3, 0xb4, 0xfe, 0xfe, 0x5c, 0x6a, 0x6a, 0x6a, 0x6a, 0x7c, 0xfe, 0xfe, 0xb2, 0xb3, 0xb3,
        0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb4, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x5d, 0x6b, 0x6c, 0x6b, 0x6c, 0x8a, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
    };

    CHvc* pHvc = mHvc;
    const u8* pMemory = pHvc->GetMemory();
    u16 scroll;
    if (std::memcmp(checkVram + 98, pMemory + 0x302b62, 28) == 0 &&
        std::memcmp(checkVram + 141, pMemory + 0x302b8d, 6) == 0) {
        scroll = 0;
    } else {
        scroll = 0x200;
    }

    pHvc->mCustomParameter.mLineScroll[193].mX = scroll;
    for (s32 i = 194; i < 239; i++) {
        mHvc->mCustomParameter.mLineScroll[i].mX = scroll;
    }
}

/**
 * @brief Patch the program of Metroid.
 */
inline void CHvcPatch::_PatchAttach_Metroid() {
    const u8 cCode[38] = {0xa4, 0x98, 0xc0, 0x04, 0xd0, 0x11, 0xa4, 0x99, 0xf0, 0x15,
                          0xc0, 0x06, 0xf0, 0x15, 0x88, 0xf0, 0x0a, 0x29, 0x10, 0xf0,
                          0x05, 0xa9, 0x02, 0x29, 0x02, 0x4a, 0x60, 0xc9, 0x09, 0xb0,
                          0xf6, 0xc9, 0x1e, 0xb0, 0xf2, 0xa9, 0x00, 0x60};
    std::memcpy(getMemory() + 0xb25, cCode, sizeof(cCode));
}

/**
 * @brief Adjust the sound mix of Balloon Fight.
 */
inline void CHvcPatch::_PatchAttach_BalloonFight() {
    mHvc->mCustomParameter._00[5] = 1;
    mHvc->mCustomParameter.mVolume[0] = 0x20;
    mHvc->mCustomParameter.mVolume[1] = 0x20;
    mHvc->mCustomParameter.mVolume[2] = 0x20;
    mHvc->mCustomParameter.mVolume[3] = 0x20;
    mHvc->mCustomParameter.mVolume[4] = 0x20;
    mHvc->mCustomParameter.mPan[0] = 0x0c;
    mHvc->mCustomParameter.mPan[1] = 0x0c;
    mHvc->mCustomParameter.mPan[2] = 0x0c;
    mHvc->mCustomParameter.mPan[3] = 0x08;
    mHvc->mCustomParameter.mPan[4] = 0x0c;
    mHvc->mCustomParameter.mPan[5] = 0;
    mHvc->mCustomParameter.mPan[6] = 0;
    mHvc->mCustomParameter.mPan[7] = 0;
    mHvc->mCustomParameter.mPan[8] = 0;
    CHvc* pHvc = mHvc;
    if (pHvc->mTitleElement.mTitleId == 189) {
        pHvc->mMachine.mMemory[0xd7] = 0x08;
        pHvc->mMachine.mMemory[0xd9] = 0x80;
    }
}

/**
 * @brief Skip the first frames of Punch-Out!!.
 */
inline void CHvcPatch::_PatchAttach_PunchOut() {
    for (s32 i = 12; i >= 2; i--) {
        mHvc->mSkipFrame[i] = 1;
    }

    mHvc->mSkipFrameNum = 14;
}

/**
 * @brief Fix the speed of the Yoshi no Tamago level select.
 */
inline void CHvcPatch::_PatchPostProgress_YoshiNoTamago() {
    static const u8 checkVram02EB_Time[4] = {0x4e, 0x5e, 0x6e, 0x7e};
    static const u8 checkVram030B_Level[5] = {0xe0, 0xe1, 0xe2, 0xe3, 0xe4};
    static const u8 checkVram032B_Speed[5] = {0xe5, 0xe6, 0xe7, 0xe8, 0xe9};

    u8* pMemory = getMemory();
    if (std::memcmp(checkVram02EB_Time, pMemory + 0x302aeb, sizeof(checkVram02EB_Time)) == 0 &&
        std::memcmp(checkVram030B_Level, pMemory + 0x302b0b, sizeof(checkVram030B_Level)) == 0 &&
        std::memcmp(checkVram032B_Speed, pMemory + 0x302b2b, sizeof(checkVram032B_Speed)) == 0 &&
        pMemory[0x300568] == 1) {
        pMemory[0x300568] = 0x1f;
    }
}

/**
 * @brief Nothing to do for MOTHER when it is attached.
 */
inline void CHvcPatch::_PatchAttach_Mother() {}

/**
 * @brief Fix three texts of MOTHER before a frame.
 */
inline void CHvcPatch::_PatchAnteProgress_Mother() {
    static const u8 text_table[3][16] = {
        {0x90, 0xdb, 0xee, 0x8f, 0xa3, 0xb4, 0xbb, 0x93, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
         0xc0},
        {0xc0, 0xa0, 0xa0, 0x96, 0xba, 0xa1, 0xaf, 0x8f, 0xa0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
         0xc0},
        {0x90, 0x95, 0xaf, 0xbc, 0xb8, 0x9b, 0xbd, 0xa6, 0xc0, 0x91, 0x8f, 0xa3, 0xb3, 0x9c, 0xb7,
         0xbd},
    };
    static const u8 eTable[3] = {0x08, 0x09, 0x10};

    u8* pWorkRam = getMemory() + 0x300000;
    u8* pText = pWorkRam + 0x442;
    u8 first = pText[0];
    for (u64 i = 0; i < 3; i++) {
        u32 position = first == 5 ? 4 : 0;
        bool isMatch = true;
        for (s32 j = 0; j < 16; j++) {
            if (pText[position] != text_table[i][j]) {
                isMatch = false;
                break;
            }

            position += pText[position + 1] == 5 ? 5 : 1;
        }

        if (!isMatch) {
            continue;
        }

        u8* pName = pWorkRam + 0x428;
        u32 end = pName[0] == 5 ? 4 : 0;
        for (u32 j = 0; j < eTable[i]; j++) {
            end += pName[end + 1] == 5 ? 5 : 1;
        }

        u32 target = first == 5 ? end + 4 : end;
        switch (i) {
        case 0:
            pWorkRam[0x43e + target] = 0xa4;
            break;
        case 1:
            pWorkRam[0x43e + target] = 0xff;
            break;
        case 2:
            pName[end] = 0xff;
            *reinterpret_cast<u16*>(pWorkRam + 0x43e + target) = 0xb8ac;
            break;
        }

        return;
    }
}

/**
 * @brief Patch the program of Wrecking Crew.
 */
inline void CHvcPatch::_PatchAttach_WreckingCrew() {
    static const u8 asm_code[0x70] = {
        0x48, 0x8a, 0x48, 0x98, 0x48, 0x20, 0xd0, 0xff, 0xa2, 0x00, 0xbd, 0x00, 0x61, 0x9d,
        0x00, 0x06, 0xe8, 0xd0, 0xf7, 0x20, 0xe0, 0xff, 0x20, 0xd8, 0xff, 0x68, 0xa8, 0x68,
        0xaa, 0x68, 0x60, 0xea, 0x48, 0x8a, 0x48, 0x98, 0x48, 0x20, 0xd0, 0xff, 0xa2, 0x00,
        0xbd, 0x00, 0x06, 0x9d, 0x00, 0x61, 0xe8, 0xd0, 0xf7, 0x20, 0xe0, 0xff, 0x20, 0xd8,
        0xff, 0x68, 0xa8, 0x68, 0xaa, 0x68, 0x60, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea,
        0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xa5, 0x09, 0x29, 0x7f,
        0x8d, 0x00, 0x20, 0x60, 0xa5, 0x09, 0x8d, 0x00, 0x20, 0x60, 0xea, 0xea, 0xa2, 0xff,
        0xa0, 0xff, 0xea, 0xea, 0xea, 0x88, 0xd0, 0xfa, 0xca, 0xd0, 0xf5, 0x60, 0xea, 0xea,
    };

    u8* pMemory = getMemory();
    std::memcpy(pMemory + 0x7f80, asm_code, sizeof(asm_code));
    *reinterpret_cast<u16*>(pMemory + 0x3844) = 0xff80;
    *reinterpret_cast<u16*>(pMemory + 0x387a) = 0xffa0;
}

/**
 * @brief Adjust the sound mix of Yoshi no Cookie.
 */
inline void CHvcPatch::_PatchAttach_YoshiNoCookie() {
    mHvc->mCustomParameter.mVolume[0] = 0x1c;
    mHvc->mCustomParameter.mPanMaster = 0;
    mHvc->mCustomParameter.mVolume[1] = 0x20;
    mHvc->mCustomParameter._54c[0] = 0;
    mHvc->mCustomParameter.mVolume[2] = 0x18;
}

/**
 * @brief Save the Kid Icarus flash softening state into the backup memory.
 */
inline void CHvcPatch::_PatchExport_ParthenaNoKagami() {
    u8* pMemory = mHvc->GetMemory();
    *reinterpret_cast<u64*>(pMemory + 0x303000) = *reinterpret_cast<u64*>(mName);

    auto pWork = reinterpret_cast<SPatchWorkParthenaNoKagami*>(mHvc->mPatchWork);
    Common::Serialize::LittleEndian serializer(pMemory + 0x303008, false);
    serializer.Entry(pWork->mColor);
    serializer.Entry(pWork->mAverage);
    serializer.Entry(pWork->mHistory[0]);
    serializer.Entry(pWork->mHistory[1]);
    serializer.Entry(pWork->mTimer);
    serializer.Entry(pWork->mCounter);
}

/**
 * @brief Restore the Kid Icarus flash softening state from the backup memory.
 */
inline void CHvcPatch::_PatchImport_ParthenaNoKagami() {
    u8* pMemory = mHvc->GetMemory();
    if (*reinterpret_cast<u64*>(pMemory + 0x303000) != *reinterpret_cast<u64*>(mName)) {
        return;
    }

    auto pWork = reinterpret_cast<SPatchWorkParthenaNoKagami*>(mHvc->mPatchWork);
    Common::Serialize::LittleEndian serializer(pMemory + 0x303008, true);
    serializer.Entry(pWork->mColor);
    serializer.Entry(pWork->mAverage);
    serializer.Entry(pWork->mHistory[0]);
    serializer.Entry(pWork->mHistory[1]);
    serializer.Entry(pWork->mTimer);
    serializer.Entry(pWork->mCounter);
}

/**
 * @brief Clear the Kid Icarus work area after the emulation started.
 */
inline void CHvcPatch::_PatchPostCommence_ParthenaNoKagami() {
    u64* pWork = reinterpret_cast<u64*>(mHvc->mPatchWork);
    pWork[4] = 0;
    pWork[3] = 0;
    pWork[2] = 0;
    pWork[1] = 0;
    pWork[0] = 0;
}

/**
 * @brief Remember the background palette of Kid Icarus before a frame.
 */
inline void CHvcPatch::_PatchAnteProgress_ParthenaNoKagami() {
    CHvc* pHvc = mHvc;
    auto pWork = reinterpret_cast<SPatchWorkParthenaNoKagami*>(pHvc->mPatchWork);
    std::memcpy(pWork->mPalette, pHvc->GetMemory() + cMemory_Palette, sizeof(pWork->mPalette));
}

/**
 * @brief Soften the screen flashes of Kid Icarus after a frame.
 */
inline void CHvcPatch::_PatchPostProgress_ParthenaNoKagami() {
    const u8 cFlash0[16] = {0x16, 0x0f, 0x0f, 0x0f, 0x0f, 0x21, 0x13, 0x02,
                            0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x36, 0x14, 0x15};
    const u8 cFlash1[16] = {0x16, 0x16, 0x16, 0x0f, 0x0f, 0x21, 0x13, 0x02,
                            0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x36, 0x14, 0x15};
    const u8 cFlash2[16] = {0x10, 0x20, 0x22, 0x02, 0x10, 0x10, 0x10, 0x10,
                            0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
    const u8 cNormal0[16] = {0x0f, 0x30, 0x15, 0x0c, 0x0f, 0x21, 0x13, 0x02,
                             0x0f, 0x27, 0x18, 0x09, 0x0f, 0x36, 0x14, 0x15};
    const u8 cNormal1[16] = {0x0f, 0x0f, 0x0f, 0x0c, 0x0f, 0x21, 0x13, 0x02,
                             0x0f, 0x27, 0x18, 0x09, 0x0f, 0x36, 0x14, 0x15};
    const u8 cNormal2[16] = {0x0f, 0x20, 0x22, 0x02, 0x0f, 0x0f, 0x0f, 0x0f,
                             0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f};

    CHvc* pHvc = mHvc;
    auto pWork = reinterpret_cast<SPatchWorkParthenaNoKagami*>(pHvc->mPatchWork);
    u8* pMemory = pHvc->GetMemory();
    u8 background = pWork->mPalette[0];
    const u64* pOutput = reinterpret_cast<const u64*>(pMemory + cMemory_OutputPalette);
    const u8* pPalette = pMemory + cMemory_Palette;

    const u8* pNormal;
    if (std::memcmp(pWork->mPalette, cParthenaFlashPalette[0], 16) == 0) {
        pNormal = cParthenaNormalPalette0;
    } else if (std::memcmp(pWork->mPalette, cParthenaFlashPalette[1], 16) == 0) {
        pNormal = cParthenaNormalPalette1;
    } else if (std::memcmp(pWork->mPalette, cParthenaFlashPalette[2], 16) == 0) {
        pNormal = cParthenaNormalPalette2;
    } else {
        pNormal = nullptr;
    }

    if (pNormal != nullptr) {
        bool isRestored = std::memcmp(pPalette, pNormal, 16) == 0;
        bool isIdle = pWork->mTimer == 0;
        if (isRestored) {
            pWork->mTimer = 8;
            if (isIdle) {
                pWork->mColor = pOutput[background];
                pWork->mCounter = 0;
            }
        } else if (isIdle) {
            pWork->mAverage = pOutput[background];
            return;
        }
    } else if (pWork->mTimer == 0) {
        pWork->mAverage = pOutput[background];
        return;
    }

    u32 color = pOutput[background];
    u32 average = pWork->mAverage;
    u32 blue = (((color >> 8) & 0xff) + ((average >> 8) & 0xff)) >> 1;
    u32 green = (((color >> 16) & 0xff) + ((average >> 16) & 0xff)) >> 1;
    u32 red = ((color >> 24) + (average >> 24)) >> 1;
    average = red << 24 | (green & 0xff) << 16 | (blue & 0xff) << 8 | 0xff;
    u32 counter = pWork->mCounter;
    pWork->mAverage = average;
    pWork->mHistory[counter & 1] = average;
    u32 target = pWork->mHistory[(counter >> 4) & 1];
    pWork->mCounter = counter + 1;

    u32 current = pWork->mColor;
    s32 currentRed = current >> 24;
    s32 currentGreen = (current >> 16) & 0xff;
    s32 currentBlue = (current >> 8) & 0xff;
    s32 difference = currentBlue - ((target >> 8) & 0xff);
    if (difference > 2) {
        currentBlue -= 2;
    } else if (difference < -2) {
        currentBlue += 2;
    }

    difference = currentGreen - ((target >> 16) & 0xff);
    if (difference > 2) {
        currentGreen -= 2;
    } else if (difference < -2) {
        currentGreen += 2;
    }

    difference = currentRed - (target >> 24);
    if (difference > 2) {
        currentRed -= 2;
    } else if (difference < -2) {
        currentRed += 2;
    }

    current = currentRed << 24 | (currentGreen & 0xff) << 16 | (currentBlue & 0xff) << 8 | 0xff;
    pWork->mColor = current;
    mHvc->mSpecialColor[0] = current;
    for (s32 i = 1; i < 16; i++) {
        if (pWork->mPalette[i] == pWork->mPalette[0]) {
            mHvc->mSpecialColor[i] = pWork->mColor;
        }
    }

    u8* pFrame = mHvc->mEntityData.mFrameBuffer;
    for (u32 i = 0; i < CHvc::cScreenWidth * CHvc::cScreenHeight * 4; i += 4) {
        if (pFrame[i + 2] < 16 && pFrame[i] == pWork->mPalette[0]) {
            pFrame[i] ^= 0x80;
        }
    }

    pWork->mTimer--;
}

/**
 * @brief Draw the pixels of a palette group with direct colors.
 *
 * The group is the first sub-palette (4 entries) whose colors match pIndex (0xff matches any).
 * Every pixel of that group drawn with a listed color gets the direct color of its entry.
 * @param pIndex The palette colors to replace (0xff for any).
 * @param pColor The direct color of each entry (0 to keep the palette color).
 * @param pPalette The palette of the frame (32 entries).
 * @return Whether a pixel was replaced.
 */
inline bool CHvcPatch::_replacePaletteColorToDirectColorInOrder(const u8* pIndex, const u32* pColor,
                                                                const u8* pPalette) {
    u32 group;
    for (group = 0; group < 4; group++) {
        bool isMatch = true;
        for (s32 i = 0; i < 4; i++) {
            if (pIndex[i] != 0xff && pIndex[i] != pPalette[group * 4 + i]) {
                isMatch = false;
                break;
            }
        }

        if (isMatch) {
            break;
        }
    }

    if (group == 4) {
        return false;
    }

    bool isReplaced = false;
    u8* pPixel = mHvc->mEntityData.mFrameBuffer;
    for (s32 i = CHvc::cScreenWidth * CHvc::cScreenHeight; i != 0; i--, pPixel += 4) {
        u8 color = pPixel[0];
        u8 attribute = pPixel[2];
        if (!((color == pIndex[0] && pColor[0] != 0) || (color == pIndex[1] && pColor[1] != 0) ||
              (color == pIndex[2] && pColor[2] != 0) || (color == pIndex[3] && pColor[3] != 0))) {
            continue;
        }

        if (group != attribute >> 2) {
            continue;
        }

        if (!isReplaced) {
            u32 base = attribute & 0xfc;
            const u8* pGroup = pPalette + base;
            for (s32 j = 0; j < 4; j++) {
                if (pGroup[j] == pIndex[j] && pColor[j] != 0) {
                    mHvc->mSpecialColor[base + j] = pColor[j];
                }
            }
        }

        if (static_cast<s8>(pPixel[0]) >= 0) {
            pPixel[0] ^= 0x80;
        }

        isReplaced = true;
    }

    return isReplaced;
}

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
