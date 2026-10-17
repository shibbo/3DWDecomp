#include "LuigiBros/Vessel/screenfilterCpu.hpp"
#include "LuigiBros/Vessel/screenfilter.hpp"
#include "LuigiBros/Vessel/FlashingFilter.hpp"
#include "LuigiBros/vessel.hpp"
#include "LuigiBros/Emulator/Emu.hpp"
#include <cstring>
#include "LuigiBros/Emulator/cafe.hpp"

/** @brief Destroys the CPU filter without additional cleanup. */
screenfilterCpu::~screenfilterCpu() {}

/** @brief Resets level transitions and their default frame increments. */
void screenfilterCpu::LvChanges_Init() {
    mLevelState = 0;
    mHoldProgress = 0.0f;
    mLevel = 0.0f;
    mRiseStep = 0.05f;
    mHoldStep = 1.0f / 60.0f;
    mFallStep = 1.0f / 60.0f;
}

/**
 * @brief Sets the level transition increments.
 * @param rise Rise increment per frame.
 * @param hold Hold-phase increment per frame.
 * @param fall Fall decrement per frame.
 */
void screenfilterCpu::LvChanges_Set(float rise, float hold, float fall) {
    mRiseStep = rise;
    mHoldStep = hold;
    mFallStep = fall;
}

/** @brief Clears the level transition state. */
void screenfilterCpu::LvChanges_Sub0() {
    mLevelState = 0;
    mHoldProgress = 0.0f;
    mLevel = 0.0f;
}

/** @brief Raises the level until the hold phase begins. */
void screenfilterCpu::LvChanges_Sub1() {
    mLevel = mRiseStep + mLevel;
    if (mLevel >= 1.0f) {
        mHoldProgress = 0.0f;
        mLevel = 1.0f;
        mLevelState = 2;
    }
}

/** @brief Advances the hold phase until the falling phase begins. */
void screenfilterCpu::LvChanges_Sub2() {
    mHoldProgress = mHoldStep + mHoldProgress;
    if (mHoldProgress >= 1.0f) { mLevelState = 3; }
}

/** @brief Lowers the level and returns to the idle state. */
void screenfilterCpu::LvChanges_Sub3() {
    mLevel -= mFallStep;
    if (mLevel <= 0.0f) {
        mLevel = 0.0f;
        mLevelState = 0;
    }
}

/**
 * @brief Advances or retriggers the level transition.
 * @param trigger Starts the rising phase when true.
 */
void screenfilterCpu::LvChanges_Main(bool trigger) {
    if (trigger) {
        mLevelState = 1;
        LvChanges_Sub1();
    } else {
        switch (mLevelState) {
        default: LvChanges_Sub0(); break;
        case 3: LvChanges_Sub3(); break;
        case 2: LvChanges_Sub2(); break;
        case 1: LvChanges_Sub1(); break;
        }
    }
}

/**
 * @brief Provides this port's empty fade hook.
 * @param amount Unused fade amount.
 */
void screenfilterCpu::Fade(float amount) {}

/** @brief Clears flash detection thresholds and recent history. */
void screenfilterCpu::CheckKidoFlash_Init() {
    mFlashPixelThreshold = 0;
    mFlashLevel = 0;
    mFlashMode = 0;
    mFlashDuration = 0;
    memset(&mFlashCountdown, 0, 0x30);
}

/** @brief Obtains emulator dimensions and selects its pixel sampling steps. */
void screenfilterCpu::GetScreenSize() {
    mWidth = 256;
    mHeight = 256;
    mStepX = 1;
    mStepY = 1;
    mStride = 256;
    if (m_vessel.mpEmulator) {
        m_vessel.mpEmulator->GetScreenSize(&mWidth, &mHeight);
        if (mWidth == 512) {
            mStepX = 2;
            mStepY = 2;
            mStride = 512;
        }
    }
}

/** @brief Selects the default flash level and quarter-screen threshold. */
void screenfilterCpu::SetKidoFlash() {
    mFlashLevel = 63;
    mFlashPixelThreshold = static_cast<int>(static_cast<float>((mHeight / mStepY) * (mWidth / mStepX)) * 0.25f);
    mFlashMode = 1;
    mFlashDuration = 0;
}

/**
 * @brief Configures flash detection in its default mode.
 * @param duration Flash duration parameter.
 * @param level Normalized brightness threshold.
 * @param fraction Fraction of sampled pixels required to trigger.
 */
void screenfilterCpu::SetKidoFlash(int duration, float level, float fraction) {
    mFlashMode = 1;
    mFlashDuration = duration;
    mFlashLevel = static_cast<int>(level * 127.0);
    mFlashPixelThreshold = static_cast<int>(static_cast<float>((mHeight / mStepY) * (mWidth / mStepX)) * fraction);
}

/**
 * @brief Configures flash detection with an explicit mode.
 * @param duration Flash duration parameter.
 * @param level Normalized brightness threshold.
 * @param fraction Fraction of sampled pixels required to trigger.
 * @param mode Detection mode.
 */
void screenfilterCpu::SetKidoFlash(int duration, float level, float fraction, int mode) {
    mFlashMode = mode;
    mFlashDuration = duration;
    mFlashLevel = static_cast<int>(level * 127.0);
    mFlashPixelThreshold = static_cast<int>(static_cast<float>((mHeight / mStepY) * (mWidth / mStepX)) * fraction);
}

/**
 * @brief Selects the Normal filter mode.
 */
void screenfilterCpu::Normal() {
    mMode = 0;

}

/**
 * @brief Selects the NoProc filter mode.
 */
void screenfilterCpu::NoProc() {
    mMode = 1;

}

/**
 * @brief Selects the AutoBlurKido filter mode.
 */
void screenfilterCpu::AutoBlurKido() {
    mMode = 4;

}

/**
 * @brief Selects the BlurRGB filter mode.
 * @param amount Filter parameter.
 */
void screenfilterCpu::BlurRGB(float amount) {
    mMode = 9;
    mBlur = amount;
}

/**
 * @brief Selects the Dark filter mode.
 * @param amount Filter parameter.
 */
void screenfilterCpu::Dark(float amount) {
    mMode = 13;
    mDark = amount;
}

/**
 * @brief Selects the DarkH filter mode.
 * @param amount Filter parameter.
 */
void screenfilterCpu::DarkH(float amount) {
    mMode = 12;
    mDark = amount;
}

/**
 * @brief Selects the DarkBlurTime filter mode.
 * @param darkness Filter parameter.
 * @param time Filter parameter.
 */
void screenfilterCpu::DarkBlurTime(float darkness, float time) {
    mMode = 14;
    mDark = darkness;
    mBlurTime = time;
}

/**
 * @brief Selects the Contrast filter mode.
 * @param contrast Filter parameter.
 * @param offset Filter parameter.
 */
void screenfilterCpu::Contrast(float contrast, float offset) {
    mMode = 10;
    mContrast = contrast;
    mContrastOffset = offset;
}

/** @brief Requests one paused processing frame. */
void screenfilterCpu::OnePause() { mPause = 1; }

/**
 * @brief Copies the unfiltered pixel buffer when the buffers differ.
 * @param pSource Source pixel buffer containing at least 0x100000 bytes.
 * @param pDestination Destination buffer of the same capacity.
 */
void screenfilterCpu::SubNormal(unsigned int* pSource, unsigned int* pDestination) {
    if (pSource != pDestination) { OSBlockMove(pDestination, pSource, 0x100000, true); }
}

namespace {
int sContrastTable[256];
}

/**
 * @brief Applies the contrast lookup table to sampled pixels and duplicates each result.
 * @param pSource Source RGBA pixels with the configured dimensions and stride.
 * @param pDestination Destination with room for the duplicated 512-pixel-wide rows.
 */
void screenfilterCpu::SubContrast(unsigned int* pSource, unsigned int* pDestination) {
    float value = mContrast * 255.0f;
    float step = mContrastOffset - mContrast;
    for (unsigned long i = 0; i < 256; ++i) {
        sContrastTable[i] = static_cast<int>(value);
        value += step;
    }
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pSource;
            unsigned int red = sContrastTable[pixel >> 24];
            unsigned int green = sContrastTable[(pixel >> 16) & 255];
            unsigned int blue = sContrastTable[(pixel >> 8) & 255];
            unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
            pDestination[513] = result;
            pDestination[512] = result;
            pDestination[1] = result;
            pDestination[0] = result;
            pSource += mStepX;
            pDestination += mStepX;
        }
        pSource += mStride;
        pDestination += mStride;
    }
}

/**
 * @brief Darkens every horizontal pixel and duplicates its output into the next row.
 * @param pSource Source RGBA pixels with the configured dimensions and stride.
 * @param pDestination Destination with room for duplicated 512-pixel-wide rows.
 */
void screenfilterCpu::SubDarkH(unsigned int* pSource, unsigned int* pDestination) {
    float value = 0.0f;
    for (unsigned long i = 0; i < 256; ++i) {
        sContrastTable[i] = static_cast<int>(value);
        value += mDark;
    }
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; ++x) {
            unsigned int pixel = *pSource++;
            unsigned int red = sContrastTable[pixel >> 24];
            unsigned int green = sContrastTable[(pixel >> 16) & 255];
            unsigned int blue = sContrastTable[(pixel >> 8) & 255];
            unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
            pDestination[512] = result;
            *pDestination++ = result;
        }
        pSource += mStride;
        pDestination += mStride;
    }
}

/**
 * @brief Selects combined darkening and contrast adjustment.
 * @param darkness Darkness multiplier.
 * @param contrast Contrast parameter.
 * @param offset Contrast offset parameter.
 */
void screenfilterCpu::DarkContrast(float darkness, float contrast, float offset) {
    mDark = darkness;
    mContrast = contrast;
    mContrastOffset = offset;
    mMode = 15;
}

/**
 * @brief Applies the combined darkness and contrast lookup table.
 * @param pSource Source pixels using configured dimensions and sampling steps.
 * @param pDestination Destination with room for the duplicated pixel blocks.
 */
void screenfilterCpu::SubDarkContrast(unsigned int* pSource, unsigned int* pDestination) {
    float offset = static_cast<float>((static_cast<double>(mContrastOffset) - 1.0) + 1.0);
    float contrast = mContrast * mDark;
    offset = mDark * offset;
    float value = contrast * 255.0f;
    float step = offset - contrast;
    for (unsigned long i = 0; i < 256; ++i) {
        sContrastTable[i] = static_cast<int>(value);
        value += step;
    }
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pSource;
            unsigned int red = sContrastTable[pixel >> 24];
            unsigned int green = sContrastTable[(pixel >> 16) & 255];
            unsigned int blue = sContrastTable[(pixel >> 8) & 255];
            unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
            pDestination[513] = result;
            pDestination[512] = result;
            pDestination[1] = result;
            pDestination[0] = result;
            pSource += mStepX;
            pDestination += mStepX;
        }
        pSource += mStride;
        pDestination += mStride;
    }
}

/**
 * @brief Selects flash-triggered contrast processing.
 * @param contrast Contrast parameter.
 * @param offset Contrast offset parameter.
 */
void screenfilterCpu::AutoContrast(float contrast, float offset) {
    mContrast = contrast;
    mContrastOffset = offset;
    mMode = 6;
}

/**
 * @brief Applies contrast on detected flashes and otherwise copies the source.
 * @param pSource Source RGBA pixels.
 * @param pDestination Destination pixel buffer.
 */
void screenfilterCpu::SubAutoContrast(unsigned int* pSource, unsigned int* pDestination) {
    SetKidoFlash();
    mFlashDuration = 2;
    bool flash = CheckKidoFlash(pSource);
    LvChanges_Main(flash);
    if (flash) { SubContrast(pSource, pDestination); }
    else { SubNormal(pSource, pDestination); }
}

/**
 * @brief Selects blending across up to three frames.
 * @param pWeights Four weights; a zero in either first two entries disables blending.
 */
void screenfilterCpu::NFrame(float* pWeights) {
    if (pWeights[0] == 0.0f) { mMode = 0; return; }
    if (pWeights[1] == 0.0f) { mMode = 0; return; }
    mFrameCount = pWeights[2] == 0.0f ? 2 : 3;
    for (unsigned int i = 0; i < 4; ++i) { mFrameWeights[i] = pWeights[i]; }
    mFrameWeights[mFrameCount] = 0.0f;
    mMode = 11;
}

/**
 * @brief Selects two-frame blending.
 * @param first Current-frame weight.
 * @param second Previous-frame weight.
 */
void screenfilterCpu::Frames2(float first, float second) {
    if (first == 0.0f || second == 0.0f) { mMode = 0; return; }
    mFrameCount = 2;
    mFrameWeights[0] = first;
    mFrameWeights[1] = second;
    mFrameWeights[2] = 0.0f;
    mMode = 11;
}

/**
 * @brief Selects two- or three-frame blending according to the third weight.
 * @param first Current-frame weight.
 * @param second Previous-frame weight.
 * @param third Older-frame weight.
 */
void screenfilterCpu::Frames3(float first, float second, float third) {
    float weights[4] = {first, second, third, 0.0f};
    NFrame(weights);
}

namespace {
unsigned int sFrameHistory[3][256 * 256];
unsigned int sFrameTables[3][256];
}

/**
 * @brief Rotates frame history and blends sampled RGB channels with stored frame weights.
 * @param pSource Source pixels with configured dimensions and sampling steps.
 * @param pDestination Destination with room for duplicated pixel blocks.
 */
void screenfilterCpu::SubNFrame(unsigned int* pSource, unsigned int* pDestination) {
    if (mpFrames[0] == sFrameHistory[0]) {
        mpFrames[0] = sFrameHistory[1];
        mpFrames[1] = sFrameHistory[0];
        mpFrames[2] = sFrameHistory[2];
    } else if (mpFrames[0] == sFrameHistory[1]) {
        mpFrames[0] = sFrameHistory[2];
        mpFrames[1] = sFrameHistory[1];
        mpFrames[2] = sFrameHistory[0];
    } else {
        mpFrames[0] = sFrameHistory[0];
        mpFrames[1] = sFrameHistory[2];
        mpFrames[2] = sFrameHistory[1];
    }
    float first = mFrameWeights[0];
    float second = mFrameWeights[1];
    if (mFrameCount == 3) {
        float third = mFrameWeights[2];
        float a = 0.0f, b = 0.0f, c = 0.0f;
        for (unsigned int i = 0; i < 256; ++i) {
            sFrameTables[2][i] = static_cast<unsigned int>(c); c += third;
            sFrameTables[1][i] = static_cast<unsigned int>(b); b += second;
            sFrameTables[0][i] = static_cast<unsigned int>(a); a += first;
        }
    } else if (mFrameCount == 2) {
        float a = 0.0f, b = 0.0f;
        for (unsigned int i = 0; i < 256; ++i) {
            sFrameTables[1][i] = static_cast<unsigned int>(b); b += second;
            sFrameTables[0][i] = static_cast<unsigned int>(a); a += first;
        }
    } else {
        float a = 0.0f;
        for (unsigned int i = 0; i < 256; ++i) {
            sFrameTables[0][i] = static_cast<unsigned int>(a); a += first;
        }
    }
    unsigned int* pCurrent = mpFrames[0];
    unsigned int* pPrevious = mpFrames[1];
    if (mFrameCount == 3) {
        unsigned int* pOlder = mpFrames[2];
        for (unsigned int y = 0; y < mHeight; y += mStepY) {
            for (unsigned int x = 0; x < mWidth; x += mStepX) {
                unsigned int current = *pSource;
                *pCurrent++ = current;
                unsigned int previous = *pPrevious++;
                unsigned int older = *pOlder++;
                unsigned int red = sFrameTables[0][(current >> 24) & 255] + sFrameTables[1][(previous >> 24) & 255] + sFrameTables[2][(older >> 24) & 255];
                unsigned int green = sFrameTables[0][(current >> 16) & 255] + sFrameTables[1][(previous >> 16) & 255] + sFrameTables[2][(older >> 16) & 255];
                unsigned int blue = sFrameTables[0][(current >> 8) & 255] + sFrameTables[1][(previous >> 8) & 255] + sFrameTables[2][(older >> 8) & 255];
                unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
                pDestination[513] = result;
                pDestination[512] = result;
                pDestination[1] = result;
                pDestination[0] = result;
                pSource += mStepX;
                pDestination += mStepX;
            }
            pSource += mStride;
            pDestination += mStride;
        }
    } else {
        for (unsigned int y = 0; y < mHeight; y += mStepY) {
            for (unsigned int x = 0; x < mWidth; x += mStepX) {
                unsigned int current = *pSource;
                *pCurrent++ = current;
                unsigned int previous = *pPrevious++;
                unsigned int red = sFrameTables[0][(current >> 24) & 255] + sFrameTables[1][(previous >> 24) & 255];
                unsigned int green = sFrameTables[0][(current >> 16) & 255] + sFrameTables[1][(previous >> 16) & 255];
                unsigned int blue = sFrameTables[0][(current >> 8) & 255] + sFrameTables[1][(previous >> 8) & 255];
                unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
                pDestination[513] = result;
                pDestination[512] = result;
                pDestination[1] = result;
                pDestination[0] = result;
                pSource += mStepX;
                pDestination += mStepX;
            }
            pSource += mStride;
            pDestination += mStride;
        }
    }
}

/**
 * @brief Sets automatic frame-blending weights while always selecting automatic mode.
 * @param pWeights Four weights; either first weight being zero preserves existing weights.
 */
void screenfilterCpu::AutoNFrame(float* pWeights) {
    NFrame(pWeights);
    mMode = 7;
}

/**
 * @brief Selects automatic two-frame blending.
 * @param first Current-frame weight; zero preserves existing weights.
 * @param second Previous-frame weight; zero preserves existing weights.
 */
void screenfilterCpu::AutoFrames2(float first, float second) {
    if (first != 0.0f && second != 0.0f) {
        mFrameCount = 2;
        mFrameWeights[0] = first;
        mFrameWeights[1] = second;
        mFrameWeights[2] = 0.0f;
    }
    mMode = 7;
}

/**
 * @brief Selects automatic two- or three-frame blending.
 * @param first Current-frame weight.
 * @param second Previous-frame weight.
 * @param third Older-frame weight.
 */
void screenfilterCpu::AutoFrames3(float first, float second, float third) {
    float weights[4] = {first, second, third, 0.0f};
    AutoNFrame(weights);
}

/**
 * @brief Blends frames on detected flashes and otherwise copies the source.
 * @param pSource Source RGBA pixel buffer.
 * @param pDestination Destination pixel buffer.
 */
void screenfilterCpu::SubAutoNFrame(unsigned int* pSource, unsigned int* pDestination) {
    SetKidoFlash();
    mFlashDuration = 2;
    bool flash = CheckKidoFlash(pSource);
    LvChanges_Main(flash);
    if (flash) { SubNFrame(pSource, pDestination); }
    else { SubNormal(pSource, pDestination); }
}

extern signed char aKidoTbl[32768];
extern unsigned char aRedTbl[32768];
namespace {
signed char sPreviousBrightness[256 * 256];
}

/**
 * @brief Detects brightness changes and updates the flash-history countdowns.
 * @param pSource Source pixels with configured sampling dimensions and strides.
 * @return Whether the flash response is active.
 */
bool screenfilterCpu::CheckKidoFlash(unsigned int* pSource) {
    int changed = 0;
    signed char* pPrevious = sPreviousBrightness;
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pSource;
            unsigned long index = ((pixel >> 17) & 0x7c00) |
                ((pixel >> 14) & 0x3e0) | ((pixel >> 11) & 0x1f);
            int brightness = aKidoTbl[index];
            int difference = brightness - *pPrevious;
            if (difference < 0) { difference = -difference; }
            if (aRedTbl[index]) { difference *= mFlashMode; }
            if (difference >= mFlashLevel) { ++changed; }
            *pPrevious++ = brightness;
            pSource += mStepX;
        }
        pSource += mStride;
    }
    if (mFlashDuration != 0) {
        for (unsigned int i = 0; i < 8; ++i) {
            if (mRecentFlashes[i] > 0) { --mRecentFlashes[i]; }
        }
        int remaining = mFlashCountdown;
        if (remaining > 0) { mFlashCountdown = --remaining; }
        if (changed >= mFlashPixelThreshold) {
            mFlashIndex = mFlashIndex > 6 ? 0 : mFlashIndex + 1;
            mRecentFlashes[mFlashIndex] = 61;
            remaining = mFlashCountdown;
            if (remaining > 0) { mFlashCountdown = remaining = 61; }
            else {
                int active = 0;
                for (unsigned int i = 0; i < 8; ++i) {
                    if (mRecentFlashes[i] > 0) { ++active; }
                }
                if (active >= mFlashDuration) { mFlashCountdown = remaining = 61; }
            }
        }
        return remaining > 0;
    }
    if (changed >= mFlashPixelThreshold) {
        mFlashCountdown = 22;
        int previousCount = mFlashCount++;
        if (previousCount >= 2) { mFlashWindow = 60; }
    }
    if (mFlashCountdown >= 1) { --mFlashCountdown; }
    if (mFlashWindow > 0) { --mFlashWindow; }
    if (mFlashCountdown == 0) { mFlashCount = 0; }
    return mFlashCountdown != 0;
}

/** @brief Builds brightness and red-dominance tables for RGB555 colors. */
void screenfilterCpu::KidoTbl_Init() {
    for (int r = 0; r < 32; ++r) {
        for (int g = 0; g < 32; ++g) {
            for (int b = 0; b < 32; ++b) {
                float red = float(r) / 31.0f;
                float green = float(g) / 31.0f;
                float blue = float(b) / 31.0f;
                int index = (r << 10) | (g << 5) | b;
                float brightness = red * 0.298912f + green * 0.586611f + blue * 0.114478f;
                aKidoTbl[index] = int(brightness * 127.0);
                aRedTbl[index] = red >= 0.29 && red / (red + green + blue) >= 0.6;
            }
        }
    }
}

/** @brief Initializes the CPU filter and its shared lookup tables. */
screenfilterCpu::screenfilterCpu() { Init(); }

/** @brief Resets the filter mode, level transitions, and flash detector. */
void screenfilterCpu::Init() {
    mMode = 0;
    mPause = 0;
    LvChanges_Init();
    KidoTbl_Init();
    BlurKido_Init();
    CheckKidoFlash_Init();
}

namespace {
signed char sBlurRed[32768];
signed char sBlurGreen[32768];
signed char sBlurBlue[32768];
}

namespace {
/**
 * @brief Clamps a blur-table component while preserving unordered values.
 * @param value Component adjustment.
 * @return Adjustment limited to the signed table range.
 */
float clampBlurTableValue(float value) {
    if (value > 127.0f) { value = 127.0f; }
    if (value < -127.0f) { value = -127.0f; }
    return value;
}
}

/** @brief Builds signed RGB adjustments for brightness-preserving blur. */
void screenfilterCpu::BlurKido_Init() {
    for (int r = -15; r < 16; ++r) {
        for (int g = -15; g < 16; ++g) {
            for (int b = -15; b < 16; ++b) {
                float red = float(r) * 0.298912f;
                float green = float(g) * 0.586611f;
                float blue = float(b) * 0.114478f;
                float brightness = red + green + blue;
                int index = ((r + 16) << 10) | ((g + 16) << 5) | (b + 16);
                if (brightness == 0.0f) {
                    sBlurRed[index] = 0;
                    sBlurGreen[index] = 0;
                    sBlurBlue[index] = 0;
                } else {
                    if (brightness < 0.0f) { brightness = -brightness; }
                    red = (red / brightness * 24.0f) / 0.298912f;
                    green = (green / brightness * 24.0f) / 0.586611f;
                    blue = (blue / brightness * 24.0f) / 0.114478f;
                    if (red > 0.0f) { if (red < 1.0f) { red = 1.0f; } }
                    else if (red > -1.0f && red < 0.0f) { red = -1.0f; }
                    if (green > 0.0f) { if (green < 1.0f) { green = 1.0f; } }
                    else if (green > -1.0f && green < 0.0f) { green = -1.0f; }
                    if (blue > 0.0f) { if (blue < 1.0f) { blue = 1.0f; } }
                    else if (blue > -1.0f && blue < 0.0f) { blue = -1.0f; }
                    sBlurRed[index] = int(clampBlurTableValue(red));
                    sBlurGreen[index] = int(clampBlurTableValue(green));
                    sBlurBlue[index] = int(clampBlurTableValue(blue));
                }
            }
        }
    }
}

screenfilterCpu m_ScreenFilterCpu;

namespace {
unsigned int sBlurHistory[256 * 256];

int quantizeBlurDifference(int difference) {
    if (difference > 0) {
        int value = (difference + 15) >> 4;
        return value < 15 ? value : 15;
    }
    int value = difference >> 4;
    return value > -15 ? value : -15;
}

int limitBlurDifference(int difference, int adjustment) {
    bool within;
    if (difference > 0) { within = difference < adjustment; }
    else { within = (difference != 0) & (difference > adjustment); }
    return within ? difference : adjustment;
}

int clampBlurChannel(int channel) {
    if (channel < 0) { channel = 0; }
    return channel < 255 ? channel : 255;
}
}

/** @brief Selects the brightness-based blur filter. */
void screenfilterCpu::BlurKido() { mMode = 8; }

/**
 * @brief Blends changed pixels toward the previous frame using brightness tables.
 * @param pSource Source pixels in packed RGBA order.
 * @param pDestination Output pixels with a fixed 512-pixel row pitch.
 */
void screenfilterCpu::SubBlurKido(unsigned int* pSource, unsigned int* pDestination) {
    unsigned int* pPrevious = sBlurHistory;
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pSource;
            unsigned int previous = *pPrevious;
            if (pixel != previous) {
                int sourceRed = (pixel >> 24) & 255;
                int sourceGreen = (pixel >> 16) & 255;
                int sourceBlue = (pixel >> 8) & 255;
                int red = (previous >> 24) & 255;
                int green = (previous >> 16) & 255;
                int blue = (previous >> 8) & 255;
                int dr = red - sourceRed;
                int dg = green - sourceGreen;
                int db = blue - sourceBlue;
                int qr = quantizeBlurDifference(dr);
                int qg = quantizeBlurDifference(dg);
                int qb = quantizeBlurDifference(db);
                int index = ((qr + 16) << 10) | ((qg + 16) << 5) | (qb + 16);
                int ar = sBlurRed[index];
                int ag = sBlurGreen[index];
                int ab = sBlurBlue[index];
                dr = limitBlurDifference(dr, ar);
                dg = limitBlurDifference(dg, ag);
                db = limitBlurDifference(db, ab);
                red = clampBlurChannel(red - dr);
                green = clampBlurChannel(green - dg);
                blue = clampBlurChannel(blue - db);
                unsigned int result = (unsigned(red) << 24) | (unsigned(green) << 16) | (unsigned(blue) << 8) | 255;
                *pPrevious = result;
                pDestination[513] = result;
                pDestination[512] = result;
                pDestination[1] = result;
                pDestination[0] = result;
            }
            pSource += mStepX;
            ++pPrevious;
            pDestination += mStepX;
        }
        pSource += mStride;
        pDestination += mStride;
    }
}

/**
 * @brief Darkens sampled pixels through the shared previous-frame buffer.
 * @param pSource Source RGBA pixels using the configured sampling steps.
 * @param pDestination Output with room for duplicated 512-pixel-wide rows.
 */
void screenfilterCpu::SubDark(unsigned int* pSource, unsigned int* pDestination) {
    unsigned int* pHistory = sBlurHistory;
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            *pHistory++ = *pSource;
            pSource += mStepX;
        }
        pSource += mStride;
    }
    float value = 0.0f;
    for (unsigned long i = 0; i < 256; ++i) {
        sContrastTable[i] = int(value);
        value += mDark;
    }
    pHistory = sBlurHistory;
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pHistory;
            unsigned int red = (pixel >> 24) & 255;
            unsigned int green = (pixel >> 16) & 255;
            unsigned int blue = (pixel >> 8) & 255;
            red = sContrastTable[red];
            green = sContrastTable[green];
            blue = sContrastTable[blue];
            *pHistory++ = (red << 24) | (green << 16) | (blue << 8) | 255;
        }
    }
    pHistory = sBlurHistory;
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pHistory++;
            pDestination[513] = pixel;
            pDestination[512] = pixel;
            pDestination[1] = pixel;
            pDestination[0] = pixel;
            pDestination += mStepX;
        }
        pDestination += mStride;
    }
}

namespace {
int sBlurDifference[512];
}

/**
 * @brief Limits per-channel changes from the previous frame.
 * @param pSource Source RGBA pixels using the configured sampling steps.
 * @param pDestination Output with room for duplicated 512-pixel-wide rows.
 * @param enabled Enables the configured blur limit; zero copies channel values.
 */
void screenfilterCpu::SubBlurRGB(unsigned int* pSource, unsigned int* pDestination, int enabled) {
    int limit = int(mBlur * 255.0);
    if (limit <= 1) { limit = 1; }
    if (!enabled) { limit = 255; }
    int* pTable = sBlurDifference + 255;
    pTable[0] = 0;
    for (int index = 1; index <= limit; ++index) {
        pTable[index] = index;
        pTable[-index] = -index;
    }
    for (int i = limit + 1; i < 256; ++i) {
        pTable[i] = limit;
        pTable[-int(i)] = -limit;
    }
    unsigned int* pPrevious = sBlurHistory;
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pSource;
            unsigned int previous = *pPrevious;
            unsigned int sourceRed = (pixel >> 24) & 255;
            unsigned int sourceGreen = (pixel >> 16) & 255;
            unsigned int sourceBlue = (pixel >> 8) & 255;
            unsigned int red = (previous >> 24) & 255;
            unsigned int green = (previous >> 16) & 255;
            unsigned int blue = (previous >> 8) & 255;
            unsigned int dr = red - sourceRed;
            unsigned int dg = green - sourceGreen;
            unsigned int db = blue - sourceBlue;
            red -= pTable[dr];
            green -= pTable[dg];
            blue -= pTable[db];
            unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
            *pPrevious++ = result;
            pDestination[513] = result;
            pDestination[512] = result;
            pDestination[1] = result;
            pDestination[0] = result;
            pSource += mStepX;
            pDestination += mStepX;
        }
        pSource += mStride;
        pDestination += mStride;
    }
}

namespace {
struct WhiteTables {
    int blue[256];
    unsigned int red[256];
    unsigned int green[256];
} sWhiteTables;
}

/**
 * @brief Darkens pixels and blends their channels over the requested frame interval.
 * @param pSource Source RGBA pixels using the configured sampling steps.
 * @param pDestination Output with room for duplicated 512-pixel-wide rows.
 * @param enabled Enables temporal blending when nonzero.
 */
void screenfilterCpu::SubDarkBlurTime(unsigned int* pSource, unsigned int* pDestination, int enabled) {
    if (mBlurTime < 0.0f) { mBlurTime = 0.0f; }
    int* pTable = sBlurDifference + 256;
    pTable[0] = 0;
    if (enabled && !(mBlurTime <= 1.0f)) {
        int duration = int(mBlurTime);
        for (int i = 1; i < 256; ++i) {
            int value = ((i >> 3) / duration) << 3;
            if (value <= 1) { value = 1; }
            pTable[i] = value;
            pTable[-i] = -value;
        }
    } else {
        for (int i = 1; i < 256; ++i) {
            pTable[i] = i;
            pTable[-i] = -int(i);
        }
    }
    float value = 0.0f;
    for (unsigned long i = 0; i < 256; ++i) {
        sWhiteTables.blue[i] = int(value);
        value += mDark;
    }
    unsigned int* pPrevious = sBlurHistory;
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pSource;
            unsigned int sourceRed = sWhiteTables.blue[pixel >> 24];
            unsigned int sourceGreen = sWhiteTables.blue[(pixel >> 16) & 255];
            unsigned int sourceBlue = sWhiteTables.blue[(pixel >> 8) & 255];
            unsigned int previous = *pPrevious;
            unsigned int red = previous >> 24;
            unsigned int green = (previous >> 16) & 255;
            unsigned int blue = (previous >> 8) & 255;
            unsigned int dr = pTable[red - sourceRed];
            unsigned int dg = pTable[green - sourceGreen];
            unsigned int db = pTable[blue - sourceBlue];
            red -= dr;
            green -= dg;
            blue -= db;
            unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
            *pPrevious++ = result;
            pDestination[513] = result;
            pDestination[512] = result;
            pDestination[1] = result;
            pDestination[0] = result;
            pSource += mStepX;
            pDestination += mStepX;
        }
        pSource += mStride;
        pDestination += mStride;
    }
}

/**
 * @brief Configures flash-triggered RGB blur and its level envelope.
 * @param duration Number of recent flashes required to trigger filtering.
 * @param level Brightness difference threshold.
 * @param fraction Fraction of sampled pixels required to count a flash.
 * @param rise Rising-phase increment.
 * @param hold Hold-phase increment.
 * @param fall Falling-phase decrement.
 * @param blur Per-channel blur limit.
 */
void screenfilterCpu::AutoBlurRGB(int duration, float level, float fraction, float rise, float hold, float fall, float blur) {
    mFlashMode = 1;
    mFlashDuration = duration;
    mFlashLevel = static_cast<int>(level * 127.0);
    mFlashPixelThreshold = int(float((mHeight / mStepY) * (mWidth / mStepX)) * fraction);
    LvChanges_Set(rise, hold, fall);
    mBlur = blur;
    mMode = 5;
}

/**
 * @brief Applies RGB blur while the flash-triggered level envelope is active.
 * @param pSource Source pixel buffer.
 * @param pDestination Output pixel buffer.
 */
void screenfilterCpu::SubAutoBlurRGB(unsigned int* pSource, unsigned int* pDestination) {
    LvChanges_Main(CheckKidoFlash(pSource));
    if (mLevel > 0.0f) { SubBlurRGB(pSource, pDestination, 1); }
    else { SubBlurRGB(pSource, pDestination, 0); }
}

/**
 * @brief Configures automatic darkening and RGB blur.
 * @param duration Number of recent flashes required.
 * @param level Brightness difference threshold.
 * @param fraction Fraction of sampled pixels required.
 * @param mode Red-dominance multiplier.
 * @param rise Rising-phase increment.
 * @param hold Hold-phase increment.
 * @param fall Falling-phase decrement.
 * @param darkness Darkness multiplier.
 * @param blur Per-channel blur limit.
 */
void screenfilterCpu::AutoDarkBlurRGB(int duration, float level, float fraction, int mode, float rise, float hold, float fall, float darkness, float blur) {
    mFlashLevel = static_cast<int>(level * 127.0);
    mFlashMode = mode;
    mFlashDuration = duration;
    mFlashPixelThreshold = int(float((mHeight / mStepY) * (mWidth / mStepX)) * fraction);
    LvChanges_Set(rise, hold, fall);
    mDark = darkness;
    mBlur = blur;
    mMode = 2;
}

/**
 * @brief Configures automatic darkening and contrast adjustment.
 * @param duration Number of recent flashes required.
 * @param level Brightness difference threshold.
 * @param fraction Fraction of sampled pixels required.
 * @param rise Rising-phase increment.
 * @param hold Hold-phase increment.
 * @param fall Falling-phase decrement.
 * @param darkness Darkness multiplier.
 * @param contrast Contrast parameter.
 * @param offset Contrast offset parameter.
 */
void screenfilterCpu::AutoDarkContrast(int duration, float level, float fraction, float rise, float hold, float fall, float darkness, float contrast, float offset) {
    mFlashMode = 1;
    mFlashDuration = duration;
    mFlashLevel = static_cast<int>(level * 127.0);
    mFlashPixelThreshold = int(float((mHeight / mStepY) * (mWidth / mStepX)) * fraction);
    LvChanges_Set(rise, hold, fall);
    mDark = darkness;
    mContrast = contrast;
    mContrastOffset = offset;
    mMode = 3;
}

/**
 * @brief Sets the white-level darkening filter parameters.
 * @param first First white-level parameter.
 * @param second Second white-level parameter.
 * @param whiteLevel White-level threshold.
 * @param darkness Darkness multiplier.
 */
void screenfilterCpu::Wii_Dark_WhiteLevel_V00(unsigned int first, unsigned int second, unsigned char whiteLevel, float darkness) {
    mWhiteFirst = first;
    mWhiteSecond = second;
    mWhiteLevel = whiteLevel;
    mDark = darkness;
    mMode = 16;
}

/**
 * @brief Selects brightness blur when the default flash detector triggers.
 * @param pSource Source pixel buffer.
 * @param pDestination Output pixel buffer.
 */
void screenfilterCpu::SubAutoBlurKido(unsigned int* pSource, unsigned int* pDestination) {
    SetKidoFlash();
    mFlashDuration = 2;
    bool flash = CheckKidoFlash(pSource);
    LvChanges_Main(flash);
    if (flash) { SubBlurKido(pSource, pDestination); }
    else { SubNormal(pSource, pDestination); }
}

/**
 * @brief Scales contrast with the flash envelope while applying darkness.
 * @param pSource Source RGBA pixels using the configured sampling steps.
 * @param pDestination Output with room for duplicated pixel blocks.
 */
void screenfilterCpu::SubAutoDarkContrast(unsigned int* pSource, unsigned int* pDestination) {
    LvChanges_Main(CheckKidoFlash(pSource));
    float offset = float(1.0 - (1.0 - double(mContrastOffset)) * double(mLevel));
    float contrast = (mContrast * mLevel) * mDark;
    offset = mDark * offset;
    float value = contrast * 255.0f;
    float step = offset - contrast;
    for (unsigned long i = 0; i < 256; ++i) {
        sContrastTable[i] = static_cast<int>(value);
        value += step;
    }
    for (unsigned int y = 0; y < mHeight; y += mStepY) {
        for (unsigned int x = 0; x < mWidth; x += mStepX) {
            unsigned int pixel = *pSource;
            unsigned int red = sContrastTable[pixel >> 24];
            unsigned int green = sContrastTable[(pixel >> 16) & 255];
            unsigned int blue = sContrastTable[(pixel >> 8) & 255];
            unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
            pDestination[513] = result;
            pDestination[512] = result;
            pDestination[1] = result;
            pDestination[0] = result;
            pSource += mStepX;
            pDestination += mStepX;
        }
        pSource += mStride;
        pDestination += mStride;
    }
}

/**
 * @brief Builds packed white-level tables and applies horizontal or vertical duplication.
 * @param pSource Source RGBA pixels.
 * @param pDestination Output pixel buffer with a 512-pixel row pitch.
 */
void screenfilterCpu::SubWii_Dark_WhiteLevel_V00(unsigned int* pSource, unsigned int* pDestination) {
    float white = (float(mWhiteLevel) / 255.0f) * 0.9996078f;
    float scale = mDark * white;
    for (unsigned long i = 0; i < 256; ++i) {
        unsigned int channel = unsigned(scale * float(int(i)) + 0.5f);
        sWhiteTables.red[i] = channel << 24;
        sWhiteTables.green[i] = channel << 16;
        sWhiteTables.blue[i] = (channel << 8) | 255;
    }
    if (mWhiteSecond == 1) {
        if (mWhiteFirst == 1) {
            for (unsigned int y = 0; y < mHeight; y += 1) {
                for (unsigned int x = 0; x < mWidth; x += 1) {
                    unsigned int pixel = *pSource;
                    pSource += 1;
                    unsigned int red = sWhiteTables.red[pixel >> 24];
                    unsigned int green = sWhiteTables.green[(pixel >> 16) & 255];
                    unsigned int blue = sWhiteTables.blue[(pixel >> 8) & 255];
                    unsigned int result = green | red | blue;
                    pDestination[0] = result;
                    pDestination += 1;
                }
            }
        }
        else {
            for (unsigned int y = 0; y < mHeight; y += 1) {
                for (unsigned int x = 0; x < mWidth; x += 2) {
                    unsigned int pixel = *pSource;
                    pSource += 2;
                    unsigned int red = sWhiteTables.red[pixel >> 24];
                    unsigned int green = sWhiteTables.green[(pixel >> 16) & 255];
                    unsigned int blue = sWhiteTables.blue[(pixel >> 8) & 255];
                    unsigned int result = green | red | blue;
                    pDestination[1] = result;
                    pDestination[0] = result;
                    pDestination += 2;
                }
            }
        }
    } else {
        if (mWhiteFirst == 1) {
            for (unsigned int y = 0; y < mHeight; y += 2) {
                for (unsigned int x = 0; x < mWidth; x += 1) {
                    unsigned int pixel = *pSource;
                    pSource += 1;
                    unsigned int red = sWhiteTables.red[pixel >> 24];
                    unsigned int green = sWhiteTables.green[(pixel >> 16) & 255];
                    unsigned int blue = sWhiteTables.blue[(pixel >> 8) & 255];
                    unsigned int result = green | red | blue;
                    pDestination[512] = result;
                    pDestination[0] = result;
                    pDestination += 1;
                }
                pSource += 512;
                pDestination += 512;
            }
        }
        else {
            for (unsigned int y = 0; y < mHeight; y += 2) {
                for (unsigned int x = 0; x < mWidth; x += 2) {
                    unsigned int pixel = *pSource;
                    pSource += 2;
                    unsigned int red = sWhiteTables.red[pixel >> 24];
                    unsigned int green = sWhiteTables.green[(pixel >> 16) & 255];
                    unsigned int blue = sWhiteTables.blue[(pixel >> 8) & 255];
                    unsigned int result = green | red | blue;
                    pDestination[513] = result;
                    pDestination[512] = result;
                    pDestination[1] = result;
                    pDestination[0] = result;
                    pDestination += 2;
                }
                pSource += 512;
                pDestination += 512;
            }
        }
    }
}

/**
 * @brief Applies darkness and enables RGB blur while the flash envelope is active.
 * @param pSource Source RGBA pixels using the configured sampling steps.
 * @param pDestination Output with room for duplicated pixel blocks.
 */
void screenfilterCpu::SubAutoDarkBlurRGB(unsigned int* pSource, unsigned int* pDestination) {
    LvChanges_Main(CheckKidoFlash(pSource));
    float level = mLevel;
    float value = 0.0f;
    for (unsigned long i = 0; i < 256; ++i) {
        sWhiteTables.blue[i] = unsigned(value);
        value += mDark;
    }
    int limit = int(mBlur * 255.0);
    if (limit <= 1) { limit = 1; }
    if (!(level > 0.0f)) {
        unsigned int* pPrevious = sBlurHistory;
        for (unsigned int y = 0; y < mHeight; y += mStepY) {
            for (unsigned int x = 0; x < mWidth; x += mStepX) {
                unsigned int pixel = *pSource;
                unsigned int red = (pixel >> 24) & 255;
                unsigned int green = (pixel >> 16) & 255;
                unsigned int blue = (pixel >> 8) & 255;
                red = sWhiteTables.blue[red];
                green = sWhiteTables.blue[green];
                blue = sWhiteTables.blue[blue];
                unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
                *pPrevious++ = result;
                pDestination[513] = result;
                pDestination[512] = result;
                pDestination[1] = result;
                pDestination[0] = result;
                pSource += mStepX;
                pDestination += mStepX;
            }
            pSource += mStride;
            pDestination += mStride;
        }
    } else {
        int* pTable = sBlurDifference + 255;
        pTable[0] = 0;
        for (int i = 1; i <= limit; ++i) {
            pTable[i] = i;
            pTable[-i] = -i;
        }
        for (int i = limit + 1; i < 256; ++i) {
            pTable[i] = limit;
            pTable[-int(i)] = -limit;
        }
        unsigned int* pPrevious = sBlurHistory;
        for (unsigned int y = 0; y < mHeight; y += mStepY) {
            for (unsigned int x = 0; x < mWidth; x += mStepX) {
                unsigned int pixel = *pSource;
                unsigned int previous = *pPrevious;
                unsigned int sourceRed = sWhiteTables.blue[pixel >> 24];
                unsigned int sourceGreen = sWhiteTables.blue[(pixel >> 16) & 255];
                unsigned int sourceBlue = sWhiteTables.blue[(pixel >> 8) & 255];
                unsigned int red = previous >> 24;
                unsigned int green = (previous >> 16) & 255;
                unsigned int blue = (previous >> 8) & 255;
                unsigned int dr = red - sourceRed;
                unsigned int dg = green - sourceGreen;
                unsigned int db = blue - sourceBlue;
                green -= pTable[dg];
                red -= pTable[dr];
                blue -= pTable[db];
                unsigned int result = (red << 24) | (green << 16) | (blue << 8) | 255;
                *pPrevious++ = result;
                pDestination[513] = result;
                pDestination[512] = result;
                pDestination[1] = result;
                pDestination[0] = result;
                pSource += mStepX;
                pDestination += mStepX;
            }
            pSource += mStride;
            pDestination += mStride;
        }
    }
}

/**
 * @brief Processes one emulator frame with the requested one-shot filter mode.
 * @param pSource Source RGBA frame.
 * @param pDestination Destination RGBA frame.
 */
void screenfilterCpu::Process(unsigned int* pSource, unsigned int* pDestination) {
    m_FlashingFilter.mpBuffer388 = pSource;
    m_FlashingFilter.mpBuffer390 = pDestination;
    m_vessel.mpEmulator->SetAlwaysFilter();
    if (mPause == 1) {
        mPause = 0;
        return;
    }
    GetScreenSize();
    switch (mMode) {
    case 2: SubAutoDarkBlurRGB(pSource, pDestination); break;
    default: SubNormal(pSource, pDestination); break;
    case 1: break;
    case 3: SubAutoDarkContrast(pSource, pDestination); break;
    case 4: SubAutoBlurKido(pSource, pDestination); break;
    case 5: SubAutoBlurRGB(pSource, pDestination); break;
    case 6: SubAutoContrast(pSource, pDestination); break;
    case 7: SubAutoNFrame(pSource, pDestination); break;
    case 8: SubBlurKido(pSource, pDestination); break;
    case 9: SubBlurRGB(pSource, pDestination, 1); break;
    case 10: SubContrast(pSource, pDestination); break;
    case 11: SubNFrame(pSource, pDestination); break;
    case 12: SubDarkH(pSource, pDestination); break;
    case 13: SubDark(pSource, pDestination); break;
    case 14: SubDarkBlurTime(pSource, pDestination, 1); break;
    case 15: SubDarkContrast(pSource, pDestination); break;
    case 16: SubWii_Dark_WhiteLevel_V00(pSource, pDestination); break;
    }
    mMode = 0;
}

/** @brief Constructs this port's inactive non-CPU filter. */
screenfilter::screenfilter() {}
/** @brief Destroys the inactive non-CPU filter. */
screenfilter::~screenfilter() {}
/** @brief Provides the inactive initialization hook. */
void screenfilter::Init() {}
/** @brief Provides the inactive normal-filter hook. */
void screenfilter::Normal() {}
/** @brief Provides the inactive frame-blending hook. @param pWeights Unused weights. */
void screenfilter::NFrame(float* pWeights) {}
/**
 * @brief Provides the inactive motion-blur hook.
 * @param first Unused first parameter.
 * @param second Unused second parameter.
 */
void screenfilter::MotionBlur(float first, float second) {}
/**
 * @brief Provides the inactive darkening hook.
 * @param mode Unused mode.
 * @param amount Unused darkening amount.
 */
void screenfilter::Dark(unsigned int mode, float amount) {}
/**
 * @brief Provides the inactive contrast hook.
 * @param mode Unused mode.
 * @param contrast Unused contrast parameter.
 * @param offset Unused offset parameter.
 */
void screenfilter::Contrast(unsigned int mode, float contrast, float offset) {}
/** @brief Provides the inactive fade hook. @param amount Unused fade amount. */
void screenfilter::Fade(float amount) {}
/** @brief Provides the inactive pause hook. */
void screenfilter::OnePause() {}
