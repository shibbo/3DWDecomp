#pragma once

#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"
#include <attributes.h>

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Picture processing unit of the emulated Famicom.
 *
 * Emulates the $2000-$2007 registers and OAM DMA ($4014), and renders the picture one scanline
 * at a time through a chain of rasterizer phases (clear, background, compose, sprites, finish).
 */
class CHvcPpu : public CPlatformHvcUnit {
public:
    using Rasterizer = void (CHvcPpu::*)(u64);
    using CharacterLatcher = void (CHvcPpu::*)(u16);

    /// Number of rasterizer phases run per scanline.
    static constexpr s32 cRasterizePhaseNum = 5;

    /// Width of a scanline in dots.
    static constexpr u64 cLineWidth = 256;

    /// Lines of the dot buffers.
    enum LineKind {
        cLine_Background = 0,
        cLine_Sprite = 1,
    };

    /// Bits of the PPUCTRL ($2000) register.
    enum Control {
        cControl_Increment32 = 1 << 2,
        cControl_SpritePattern = 1 << 3,
        cControl_BackgroundPattern = 1 << 4,
        cControl_SpriteSize16 = 1 << 5,
        cControl_Nmi = 1 << 7,
    };

    /// Bits of the PPUMASK ($2001) register.
    enum Mask {
        cMask_BackgroundLeft = 1 << 1,
        cMask_SpriteLeft = 1 << 2,
        cMask_Background = 1 << 3,
        cMask_Sprite = 1 << 4,
    };

    /// Bits of the PPUSTATUS ($2002) register.
    enum Status {
        cStatus_SpriteOverflow = 1 << 5,
        cStatus_Sprite0Hit = 1 << 6,
        cStatus_VBlank = 1 << 7,
    };

    /// Bits of the sprite attribute byte.
    enum SpriteAttribute {
        cSpriteAttribute_Palette = 3,
        cSpriteAttribute_Behind = 1 << 5,
        cSpriteAttribute_FlipX = 1 << 6,
        cSpriteAttribute_FlipY = 1 << 7,
    };

    /// Bits of a dot's flags.
    enum DotFlag {
        cDotFlag_Priority = 3,
        cDotFlag_Overflow = 1 << 5,
        cDotFlag_Opaque = 1 << 6,
    };

    /// Events passed to Process.
    enum Event {
        cEvent_Nmi = 1 << 3,
        cEvent_VBlankStart = 1 << 5,
        cEvent_VBlankEnd = 1 << 7,
        cEvent_Line = 1 << 8,
    };

    /**
     * @brief One rendered dot of a scanline.
     */
    struct SDot {
        u8 mColor;
        u8 mFlags;
    };

    CHvcPpu();
    void _RasterizePhase0(u64 line);
    void _RasterizePhase1(u64 line);
    void _RasterizePhase2(u64 line);
    void _RasterizePhase3(u64 line);
    void _RasterizePhase4(u64 line);
    void _LatchCharacter(u16 character);
    ~CHvcPpu() override;
    int _Prologue() override;
    int _Epilogue() override;
    int System(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    void _RasterizePhase1_Mother(u64 line);
    void _RasterizePhase4_Mother(u64 line);
    int Access(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    int Process(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    void _LatchCharacter_Mmc2orMmc4(u16 character);
    u64 _PatternDot(u8* pPattern, bool isFlip);
    u64 _PatternDotNoFlip(u8* pPattern);
    u64 _SerializeCore(void* pBuffer, bool isImport);
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;

    /// Content tag of the serialized state.
    u8 _SerializeTagContent() override { return 4; }

    /// Species tag of the serialized state.
    u8 _SerializeTagSpecies() override { return 0; }

    /// Version of the serialized state.
    u16 _SerializeTagVersion() override { return 0x100; }

    /// Comment stored with the serialized state.
    const char* _SerializeTagComment() override { return "PPU    "; }

    /// Pattern byte to dot expansion (4 bits per dot), with the leftmost dot in the high bits.
    static const u64 _PatternDotData[256];

    /// Pattern byte to dot expansion (4 bits per dot), with the leftmost dot in the low bits.
    static const u64 _PatternDotDataFlip[256];

private:
    /// Copy the register state to or from a buffer (see _SerializeCore).
    ALWAYS_INLINE inline void serializeState(void* pBuffer, bool isImport);

    /**
     * @brief Transfer data on the bus through the parent unit.
     * @param operation cOperation_Read or cOperation_Write.
     * @param size The number of bytes.
     * @param address The bus address.
     * @param pData The data to write, or the buffer that receives the data read.
     */
    void accessBus(u64 operation, u64 size, u64 address, void* pData) {
        CPlatformUnit* pParent = mParent;
        UArgument argument;
        argument.mAccess.mSize = size;
        argument.mAccess.mAddress = address;
        argument.mAccess.mData = pData;
        pParent->Access(this, 1, operation, &argument);
    }

    /**
     * @brief Advance the MOTHER character banks whose switch point has been reached.
     * @param pEntity The machine state.
     * @param pBanks The bank offset of each character page.
     * @param dot The current dot.
     */
    ALWAYS_INLINE static void updateMotherBanks(const _SEntity* pEntity, s32* pBanks, u64 dot) {
        for (s32 i = 0; i < 8; i++) {
            if (pEntity->mPpuPages[16 + i + pBanks[i]].mLimit * 3 + 18 <= dot) {
                pBanks[i] += 8;
            }
        }
    }

    /**
     * @brief Get a pointer into the MOTHER character banks.
     * @param pEntity The machine state.
     * @param pBanks The bank offset of each character page.
     * @param address The character address.
     * @return The backing memory of that address.
     */
    static u8* getMotherPatternPointer(const _SEntity* pEntity, const s32* pBanks, u64 address) {
        u64 page = address >> 10;
        return pEntity->mMemory + pEntity->mPpuPages[16 + page + pBanks[page]].mOffset +
               (address & 0x3ff);
    }

    /**
     * @brief Call the character latch for a character.
     * @param character The character (tile) number.
     */
    void latchCharacter(u16 character) { (this->*mLatchCharacter)(character); }

    /**
     * @brief Raise the sprite 0 hit flag if an opaque sprite 0 dot overlaps the background.
     * @param pBackground The background dot.
     * @param pSprite The sprite dot.
     * @param pRegisterStatus The status register seen by the CPU.
     */
    void checkSprite0Hit(const SDot* pBackground, const SDot* pSprite, u8* pRegisterStatus) {
        u8 spriteFlags = pSprite->mFlags;
        bool isOpaque = mEntity->mTitleRender->mIsSprite0HitAlways ?
                            (spriteFlags & cDotFlag_Opaque) != 0 :
                            (pBackground->mFlags & spriteFlags & cDotFlag_Opaque) != 0;
        if (isOpaque && !(mStatus & cStatus_Sprite0Hit) &&
            ((pBackground->mColor & 3) || (pSprite->mColor & 3))) {
            mStatus |= cStatus_Sprite0Hit;
            *pRegisterStatus |= cStatus_Sprite0Hit;
        }
    }

    /// Compose the horizontal scroll from its parts.
    u16 composeScrollX() const {
        return (((mNameTableX << 8) | (mCoarseX << 3)) & 0x1f8) | (mFineX & 7);
    }

    /// Compose the vertical scroll from its parts.
    u16 composeScrollY() const {
        return (((mNameTableY << 8) | (mCoarseY << 3)) & 0x1f8) | (mFineY & 7);
    }

    /**
     * @brief Set the vertical scroll, skipping the attribute rows of the name table.
     * @param scrollY The vertical scroll.
     */
    void setScrollY(u16 scrollY) {
        mScrollY = scrollY;
        if (scrollY > 0x1ef) {
            mScrollY = 0x100;
        } else if ((scrollY & 0x1f0) == 0xf0) {
            mScrollY = 0;
        }
    }

    /**
     * @brief Recompose the vertical scroll from its parts.
     *
     * Writes after the visible lines also become the scroll of the next frame.
     */
    void updateScrollY() {
        u64 clock = mEntity->mScanlineClock;
        u16 scrollY = composeScrollY();
        setScrollY(scrollY);
        if ((clock >> 8) >= 0xef) {
            mFrameScrollY = scrollY;
        }
    }

    /**
     * @brief Check whether a vertical scroll points into the attribute rows of a name table.
     * @param scrollY The vertical scroll.
     * @return Whether no tiles are visible at that position.
     */
    static bool isAttributeRow(u16 scrollY) { return ((scrollY | 0x100) & 0xfff0) == 0x1f0; }

public:
    u8 mToggle;
    u8 mReadCount;
    u8 mCoarseX;
    u8 mFineX;
    u8 mNameTableX;
    u8 mCoarseY;
    u8 mFineY;
    u8 mNameTableY;
    u16 mScrollX;
    u16 mScrollY;
    u16 mAddress;
    u8 mOamAddress;
    u8 mIsNmiRaised;
    u16 mLineX;
    u16 mLineY;
    u8 mStatus;
    u8 mReadBuffer;
    u16 mFrameScrollY;
    u16 _60;
    u16 mDmaCount;
    Rasterizer mRasterize[cRasterizePhaseNum];
    CharacterLatcher mLatchCharacter;
    SDot mLines[2][cLineWidth];
};

static_assert(__builtin_offsetof(CHvcPpu, mRasterize) == 0x68, "CHvcPpu::mRasterize");
static_assert(sizeof(CHvcPpu) == 0x4c8, "CHvcPpu size");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
