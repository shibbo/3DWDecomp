#include "LuigiBros/Emulator/HvcPpu.hpp"
#include <cstring>

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Construct the PPU with the default rasterizer phases and character latch.
 */
CHvcPpu::CHvcPpu() : CPlatformHvcUnit(0, 4, 1) {
    std::strncpy(mDescriptor.mName, "HVC PPU        ", sizeof(mDescriptor.mName) - 1);
    mRasterize[0] = &CHvcPpu::_RasterizePhase0;
    mRasterize[1] = &CHvcPpu::_RasterizePhase1;
    mRasterize[2] = &CHvcPpu::_RasterizePhase2;
    mRasterize[3] = &CHvcPpu::_RasterizePhase3;
    mRasterize[4] = &CHvcPpu::_RasterizePhase4;
    mLatchCharacter = &CHvcPpu::_LatchCharacter;
}

/**
 * @brief Clear both dot buffers at the start of a frame.
 * @param line The scanline being rendered.
 */
void CHvcPpu::_RasterizePhase0(u64 line) {
    if (static_cast<u32>(line) == 0) {
        for (s32 i = 0; i < 2; i++) {
            for (u64 j = 0; j < cLineWidth; j++) {
                mLines[i][j].mColor = 0;
                mLines[i][j].mFlags = 0;
            }
        }
    }
}

/**
 * @brief Render the background of a scanline into the background dot buffer.
 * @param line The scanline being rendered.
 */
void CHvcPpu::_RasterizePhase1(u64 line) {
    u8* pMemory = mEntity->mMemory;
    std::memset(mLines[cLine_Background], 0, sizeof(mLines[cLine_Background]));

    u64 scrollY = mLineY;
    if (isAttributeRow(scrollY)) {
        return;
    }

    u64 fineY = scrollY & 7;
    u8 control = pMemory[cMemory_PpuLatch + 0];
    u64 scrollX = mLineX;
    u8 mask = pMemory[cMemory_PpuLatch + 1];
    u64 nameTableY = (scrollY >> 7) & 2;
    u64 tileRow = ((scrollY >> 3) & 0x1f) << 5;
    u64 attributeRow = ((scrollY >> 5) & 7) << 3;
    u64 attributeShiftY = (scrollY >> 2) & 4;
    u16 characterBase = (control & cControl_BackgroundPattern) << 4;
    u64 dot = 0;
    u64 fineX = scrollX & 7;

    while (dot < 8) {
        u64 nameTable = nameTableY | ((scrollX >> 8) & 1);
        u8* pNameTable = mEntity->GetPpuPointer(0x2000 + nameTable * 0x400);
        u16 character = characterBase | pNameTable[((scrollX >> 3) & 0x1f) | tileRow];

        if (!(mask & cMask_Background)) {
            scrollX += 8 - fineX;
            dot += 8 - fineX;
        } else {
            u8 attribute = pNameTable[0x3c0 | attributeRow | ((scrollX >> 5) & 7)];
            u8 palette = ((attribute >> (((scrollX >> 3) & 2) | attributeShiftY)) & 3) << 2;
            u64 pattern = _PatternDotNoFlip(mEntity->GetPpuPointer((character << 4) | fineY));
            u64 end = dot + 8 - fineX;
            scrollX += 8 - fineX;

            for (u64 shift = fineX * 4; dot != end; dot++, shift += 4) {
                if ((mask & cMask_BackgroundLeft) || dot >= 8) {
                    u8 color = (pattern >> shift) & 3;
                    if (color != 0) {
                        mLines[cLine_Background][dot].mColor = color | palette;
                        mLines[cLine_Background][dot].mFlags = 0x42;
                    }
                }
            }
        }

        latchCharacter(character);
        fineX = 0;
    }

    u64 nameTablePage = nameTableY | 8;
    if (mask & cMask_Background) {
        while (dot < cLineWidth) {
            u8* pNameTable =
                mEntity->GetPpuPointer((nameTablePage | ((scrollX >> 8) & 1)) * 0x400);
            u16 character = characterBase | pNameTable[((scrollX >> 3) & 0x1f) | tileRow];
            u8 attribute = pNameTable[0x3c0 | attributeRow | ((scrollX >> 5) & 7)];
            u8 palette = ((attribute >> (((scrollX >> 3) & 2) | attributeShiftY)) & 3) << 2;
            u64 pattern = _PatternDotNoFlip(mEntity->GetPpuPointer((character << 4) | fineY));

            for (u64 i = 0; i < 8 && dot < cLineWidth; i++, dot++) {
                u8 color = (pattern >> (i * 4)) & 3;
                if (color != 0) {
                    mLines[cLine_Background][dot].mColor = color | palette;
                    mLines[cLine_Background][dot].mFlags = 0x42;
                }
            }

            scrollX += 8;
            latchCharacter(character);
        }
    } else {
        while (dot < cLineWidth) {
            u8* pNameTable =
                mEntity->GetPpuPointer((nameTablePage | ((scrollX >> 8) & 1)) * 0x400);
            u16 character = characterBase | pNameTable[((scrollX >> 3) & 0x1f) | tileRow];
            u64 dots = cLineWidth - dot;
            if (dots > 8) {
                dots = 8;
            }

            scrollX += dots;
            dot += dots;
            latchCharacter(character);
        }
    }

    if (mEntity->IsMmc2orMmc4()) {
        u64 nameTable = nameTableY | ((scrollX >> 8) & 1);
        u8* pNameTable = mEntity->GetPpuPointer(0x2000 + nameTable * 0x400);
        latchCharacter(characterBase | pNameTable[((scrollX >> 3) & 0x1f) | tileRow]);
    }
}

/**
 * @brief Compose the background and sprite dot buffers into the frame buffer.
 *
 * Also detects the sprite 0 hit, even on lines that are not displayed.
 *
 * @param line The scanline being rendered.
 */
void CHvcPpu::_RasterizePhase2(u64 line) {
    _STitleRender* pRender = mEntity->mTitleRender;
    u8* pMemory = mEntity->mMemory;
    u8* pRegisterStatus = &pMemory[cMemory_PpuRegister + 2];

    if (line < pRender->mVisibleTop || line > pRender->mVisibleBottom) {
        for (u64 i = 0; i < cLineWidth; i++) {
            checkSprite0Hit(&mLines[cLine_Background][i], &mLines[cLine_Sprite][i],
                            pRegisterStatus);
        }

        return;
    }

    u8* pPixel = mEntity->mFrameBuffer + line * 0x400;
    u8* pPalette = &pMemory[cMemory_Palette];
    u8 emphasis = pMemory[cMemory_PpuLatch + 1] & ~0x1e;
    u8 emphasisShow = emphasis | cMask_Background;

    for (u64 i = 0; i < cLineWidth; i++) {
        SDot* pBackground = &mLines[cLine_Background][i];
        SDot* pSprite = &mLines[cLine_Sprite][i];
        checkSprite0Hit(pBackground, pSprite, pRegisterStatus);

        u8 spritePriority = pSprite->mFlags & cDotFlag_Priority;
        const SDot* pDot;
        u8 attribute;
        if (spritePriority < (pBackground->mFlags & cDotFlag_Priority)) {
            pDot = pBackground;
            attribute = emphasisShow;
        } else {
            pDot = pSprite;
            attribute = spritePriority != 0 ? emphasisShow : emphasis;
        }

        u8 color = pDot->mColor;
        pPixel[1] = attribute;
        pPixel[0] = pPalette[color] & 0x3f;
        pPixel[2] = color;
        pPixel += 4;
    }
}

/**
 * @brief Render the sprites of a scanline into the sprite dot buffer.
 * @param line The scanline being rendered.
 */
void CHvcPpu::_RasterizePhase3(u64 line) {
    u8* pMemory = mEntity->mMemory;
    std::memset(mLines[cLine_Sprite], 0, sizeof(mLines[cLine_Sprite]));

    const u8* pOam = &mEntity->mMemory[cMemory_Oam];
    u8 mask = pMemory[cMemory_PpuLatch + 1];
    u8 control = pMemory[cMemory_PpuLatch + 0];
    s32 count = 0;

    if (!(control & cControl_SpriteSize16)) {
        u16 characterBase = (control << 5) & 0x100;
        for (u64 i = 0; i < 0x100; i += 4) {
            u64 y = pOam[i];
            if (y > line || y + 7 < line) {
                continue;
            }

            u8 attribute = pOam[i + 2];
            u64 row = (line - y) & 7;
            if (attribute & cSpriteAttribute_FlipY) {
                row ^= 7;
            }

            s32 limit = mEntity->mTitleRender->mSpriteLimit;
            u16 character = characterBase | pOam[i + 1];
            u8 flags = ((attribute & cSpriteAttribute_Behind) ? 1 : 3) | ((i == 0) << 6) |
                       ((count >= limit) << 5);
            const u64* pTable =
                (attribute & cSpriteAttribute_FlipX) ? _PatternDotData : _PatternDotDataFlip;

            if ((mask & cMask_Sprite) && count < limit) {
                u8* pPattern = mEntity->GetPpuPointer((character << 4) | row);
                u64 pattern = pTable[pPattern[0]] + (pTable[pPattern[8]] << 1);
                u64 x = pOam[i + 3];
                u8 palette = 0x10 | ((attribute & cSpriteAttribute_Palette) << 2);

                for (u64 j = 0, dot = x; j < 8 && dot <= 0xff; j++, dot++) {
                    if ((mask & cMask_SpriteLeft) || dot >= 8) {
                        u8 color = (pattern >> (j * 4)) & 3;
                        if (color != 0 && mLines[cLine_Sprite][dot].mColor == 0) {
                            mLines[cLine_Sprite][dot].mColor = palette | color;
                            mLines[cLine_Sprite][dot].mFlags = flags;
                        }
                    }
                }
            }

            latchCharacter(character);
            count++;
        }
    } else {
        for (u64 i = 0; i < 0x100; i += 4) {
            u64 y = pOam[i];
            if (y > line || y + 15 < line) {
                continue;
            }

            u8 attribute = pOam[i + 2];
            u64 row = (line - y) & 15;
            if (attribute & cSpriteAttribute_FlipY) {
                row ^= 15;
            }

            s32 limit = mEntity->mTitleRender->mSpriteLimit;
            u8 tile = pOam[i + 1];
            // Bit 0 of the tile selects the pattern table, the rest the top half's character.
            u16 character = (((tile << 8) | tile) & 0x1fe) | (row > 7 ? 1 : 0);
            u8 flags = ((attribute & cSpriteAttribute_Behind) ? 1 : 3) | ((i == 0) << 6) |
                       ((count >= limit) << 5);
            const u64* pTable =
                (attribute & cSpriteAttribute_FlipX) ? _PatternDotData : _PatternDotDataFlip;

            if ((mask & cMask_Sprite) && count < limit) {
                u8* pPattern = mEntity->GetPpuPointer((character << 4) | (row & 7));
                u64 pattern = pTable[pPattern[0]] + (pTable[pPattern[8]] << 1);
                u64 x = pOam[i + 3];
                u8 palette = 0x10 | ((attribute & cSpriteAttribute_Palette) << 2);

                for (u64 j = 0, dot = x; j < 8 && dot <= 0xff; j++, dot++) {
                    if ((mask & cMask_SpriteLeft) || dot >= 8) {
                        u8 color = (pattern >> (j * 4)) & 3;
                        if (color != 0 && mLines[cLine_Sprite][dot].mColor == 0) {
                            mLines[cLine_Sprite][dot].mColor = palette | color;
                            mLines[cLine_Sprite][dot].mFlags = flags;
                        }
                    }
                }
            }

            latchCharacter(character);
            count++;
        }
    }

    u8& rStatus = pMemory[cMemory_PpuRegister + 2];
    rStatus = count > 8 ? rStatus | cStatus_SpriteOverflow : rStatus & ~cStatus_SpriteOverflow;
}

/**
 * @brief Finish a scanline (nothing to do on standard cartridges).
 * @param line The scanline being rendered.
 */
void CHvcPpu::_RasterizePhase4(u64 line) {}

/**
 * @brief Default character latch (no mapper reacts to character fetches).
 * @param character The fetched character.
 */
void CHvcPpu::_LatchCharacter(u16 character) {}

/**
 * @brief Destroy the PPU.
 */
CHvcPpu::~CHvcPpu() {}

/**
 * @brief Reset the registers and rasterizer phases before running.
 * @return The result (always success).
 */
int CHvcPpu::_Prologue() {
    mToggle = 0;
    mReadCount = 0;
    mCoarseX = 0;
    mFineX = 0;
    mNameTableX = 0;
    mCoarseY = 0;
    mFineY = 0;
    mNameTableY = 0;
    mScrollX = 0;
    mScrollY = 0;
    mAddress = 0;
    mOamAddress = 0;
    mIsNmiRaised = 0;
    mStatus = 0;
    mReadBuffer = 0;
    mRasterize[0] = &CHvcPpu::_RasterizePhase0;
    mRasterize[1] = &CHvcPpu::_RasterizePhase1;
    mRasterize[2] = &CHvcPpu::_RasterizePhase2;
    mRasterize[3] = &CHvcPpu::_RasterizePhase3;
    mRasterize[4] = &CHvcPpu::_RasterizePhase4;
    mLatchCharacter = &CHvcPpu::_LatchCharacter;
    return cResult_Success;
}

/**
 * @brief Finish running.
 * @return The result (always success).
 */
int CHvcPpu::_Epilogue() {
    return cResult_Success;
}

/**
 * @brief Handle a system command; on attach, install the MOTHER specific rasterizer phases.
 * @param pUnit The calling unit.
 * @param channel The channel.
 * @param operation The command.
 * @param pArgument The argument.
 * @return The result (always success).
 */
int CHvcPpu::System(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) {
    if (operation == 0 && mEntity->mTitleSetting->mTitleId == 0xd3) {
        mRasterize[1] = &CHvcPpu::_RasterizePhase1_Mother;
        mRasterize[4] = &CHvcPpu::_RasterizePhase4_Mother;
    }

    return cResult_Success;
}

/**
 * @brief Render the background of a scanline for MOTHER, which switches character banks
 * mid-line.
 * @param line The scanline being rendered.
 */
void CHvcPpu::_RasterizePhase1_Mother(u64 line) {
    u8* pMemory = mEntity->mMemory;
    std::memset(mLines[cLine_Background], 0, sizeof(mLines[cLine_Background]));

    u64 scrollY = mLineY;
    if (isAttributeRow(scrollY)) {
        return;
    }

    s32 banks[8] = {};
    u64 fineY = scrollY & 7;
    u8 control = pMemory[cMemory_PpuLatch + 0];
    u64 scrollX = mLineX;
    u8 mask = pMemory[cMemory_PpuLatch + 1];
    u64 nameTableY = (scrollY >> 7) & 2;
    u64 tileRow = ((scrollY >> 3) & 0x1f) << 5;
    u64 attributeRow = ((scrollY >> 5) & 7) << 3;
    u64 attributeShiftY = (scrollY >> 2) & 4;
    u16 characterBase = (control & cControl_BackgroundPattern) << 4;
    u64 dot = 0;
    u64 fineX = scrollX & 7;

    while (dot < 8) {
        _SEntity* pEntity = mEntity;
        u64 nameTable = nameTableY | ((scrollX >> 8) & 1);
        u8* pNameTable = pEntity->GetPpuPointer(0x2000 + nameTable * 0x400);
        u16 character = characterBase | pNameTable[((scrollX >> 3) & 0x1f) | tileRow];

        if (!(mask & cMask_Background)) {
            scrollX += 8 - fineX;
            dot += 8 - fineX;
        } else {
            u8 attribute = pNameTable[0x3c0 | attributeRow | ((scrollX >> 5) & 7)];
            updateMotherBanks(pEntity, banks, dot);

            u8 palette = ((attribute >> (((scrollX >> 3) & 2) | attributeShiftY)) & 3) << 2;
            u64 pattern =
                _PatternDotNoFlip(getMotherPatternPointer(pEntity, banks, (character << 4) | fineY));
            u64 end = dot + 8 - fineX;
            scrollX += 8 - fineX;

            for (u64 shift = fineX * 4; dot != end; dot++, shift += 4) {
                if ((mask & cMask_BackgroundLeft) || dot >= 8) {
                    u8 color = (pattern >> shift) & 3;
                    if (color != 0) {
                        mLines[cLine_Background][dot].mColor = color | palette;
                        mLines[cLine_Background][dot].mFlags = 0x42;
                    }
                }
            }
        }

        latchCharacter(character);
        fineX = 0;
    }

    while (dot < cLineWidth) {
        _SEntity* pEntity = mEntity;
        u64 nameTable = nameTableY | ((scrollX >> 8) & 1);
        u8* pNameTable = pEntity->GetPpuPointer(0x2000 + nameTable * 0x400);
        u16 character = characterBase | pNameTable[((scrollX >> 3) & 0x1f) | tileRow];

        if (!(mask & cMask_Background)) {
            u64 dots = cLineWidth - dot;
            if (dots > 8) {
                dots = 8;
            }

            scrollX += dots;
            dot += dots;
        } else {
            u8 attribute = pNameTable[0x3c0 | attributeRow | ((scrollX >> 5) & 7)];
            updateMotherBanks(pEntity, banks, dot);

            if (dot < cLineWidth) {
                u8 palette = ((attribute >> (((scrollX >> 3) & 2) | attributeShiftY)) & 3) << 2;
                u64 pattern = _PatternDotNoFlip(
                    getMotherPatternPointer(pEntity, banks, (character << 4) | fineY));

                for (u64 i = 0; i < 8 && dot < cLineWidth; i++, dot++) {
                    u8 color = (pattern >> (i * 4)) & 3;
                    if (color != 0) {
                        mLines[cLine_Background][dot].mColor = color | palette;
                        mLines[cLine_Background][dot].mFlags = 0x42;
                    }
                }
            }

            scrollX += 8;
        }

        latchCharacter(character);
    }

    if (mEntity->IsMmc2orMmc4()) {
        u64 nameTable = nameTableY | ((scrollX >> 8) & 1);
        u8* pNameTable = mEntity->GetPpuPointer(0x2000 + nameTable * 0x400);
        latchCharacter(characterBase | pNameTable[((scrollX >> 3) & 0x1f) | tileRow]);
    }
}

/**
 * @brief Finish a scanline for MOTHER: save the character banks used on this line.
 * @param line The scanline being rendered.
 */
void CHvcPpu::_RasterizePhase4_Mother(u64 line) {
    for (s32 i = 0; i < 8; i++) {
        mEntity->mPpuPages[16 + i].mOffset = mEntity->mPpuPages[i].mOffset;
        mEntity->mPpuPages[16 + i].mLimit = 200;
        mEntity->mPpuPages[i].mLimit = 0;
        mEntity->mPpuPages[i].mLine = mEntity->mFrameClock >> 8;
    }
}

/**
 * @brief Read or write a PPU register ($2000-$2007) or start an OAM DMA ($4014).
 * @param pUnit The calling unit.
 * @param channel The channel.
 * @param operation cOperation_Read or cOperation_Write.
 * @param pArgument The transfer.
 * @return cResult_Success, or cResult_Unhandled if the address is not a PPU register.
 */
int CHvcPpu::Access(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) {
    _SEntity* pEntity = mEntity;
    u8* pMemory = pEntity->mMemory;
    u8* pRegister = &pMemory[cMemory_PpuRegister];
    u64 address = pArgument->mAccess.mAddress;
    int result = cResult_Unhandled;

    if (operation == cOperation_Write) {
        u8* pLatch = &pMemory[cMemory_PpuLatch];

        switch (address) {
        case 0x2000: {
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            pLatch[0] = value;
            pRegister[0] = value;
            mNameTableY = (value >> 1) & 1;
            mNameTableX = pLatch[0] & 1;
            mScrollX = composeScrollX();
            mLineX = mScrollX;
            updateScrollY();

            result = cResult_Success;
            break;
        }
        case 0x2001: {
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            pLatch[1] = value;
            pRegister[1] = value;
            result = cResult_Success;
            break;
        }
        case 0x2003: {
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            pLatch[3] = value;
            pRegister[3] = value;
            mOamAddress = value;
            result = cResult_Success;
            break;
        }
        case 0x2004: {
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            pLatch[4] = value;
            pRegister[4] = value;
            accessBus(cOperation_Write, 1, cMemory_Oam + mOamAddress, &pLatch[4]);
            mOamAddress++;
            result = cResult_Success;
            break;
        }
        case 0x2005: {
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            pLatch[5] = value;
            pRegister[5] = value;
            if (!(mToggle & 1)) {
                mCoarseX = value >> 3;
                mFineX = pLatch[5] & 7;
                mScrollX = composeScrollX();
                mLineX = mScrollX;
            } else {
                mCoarseY = value >> 3;
                mFineY = pLatch[5] & 7;
                updateScrollY();
            }

            mToggle++;
            result = cResult_Success;
            break;
        }
        case 0x2006: {
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            pLatch[6] = value;
            pRegister[6] = value;
            if (!(mToggle & 1)) {
                mFineY = (value >> 4) & 7;
                mNameTableY = (pLatch[6] >> 3) & 1;
                mNameTableX = (pLatch[6] >> 2) & 1;
                mCoarseY &= 7;
                mCoarseY |= (pLatch[6] & 3) << 3;
                mScrollX = composeScrollX();
                mLineX = mScrollX;
                updateScrollY();
            } else {
                mCoarseY &= 0x18;
                mCoarseY |= pLatch[6] >> 5;
                mCoarseX = pLatch[6] & 0x1f;
                mScrollX = composeScrollX();
                mLineX = mScrollX;
                u16 scrollY = composeScrollY();
                setScrollY(scrollY);
                mLineY = mScrollY;
                mFrameScrollY = scrollY;
                mReadCount = 0;
                mAddress = (mCoarseX & 0x1f) | ((mNameTableX & 1) << 10) |
                           ((mNameTableY & 1) << 11) | ((mCoarseY & 0x1f) << 5) |
                           ((mFineY & 3) << 12);
            }

            mToggle++;
            result = cResult_Success;
            break;
        }
        case 0x2007: {
            mReadCount = 0;
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            pLatch[7] = value;
            pRegister[7] = value;
            accessBus(cOperation_Write, 1, mAddress, &pLatch[7]);
            mAddress += (pLatch[0] & cControl_Increment32) ? 32 : 1;
            result = cResult_Success;
            break;
        }
        case 0x4014: {
            u8 value = *static_cast<u8*>(pArgument->mAccess.mData);
            const _SPage& rPage = pEntity->mCpuPages[value >> 4];
            if (rPage.mType == 1) {
                pMemory[cMemory_IoShadow + 0x14] = value;
                pMemory[cMemory_IoRegister + 0x14] = value;
                accessBus(cOperation_Write, 0x100, 0x3800,
                          pMemory + rPage.mOffset + ((value & 0xf) << 8));
                mDmaCount++;
            }

            result = cResult_Success;
            break;
        }
        }
    } else {
        switch (address) {
        case 0x2000:
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[0];
            result = cResult_Success;
            break;
        case 0x2001:
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[1];
            result = cResult_Success;
            break;
        case 0x2002:
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[2];
            mToggle = 0;
            mReadCount = 0;
            pRegister[2] &= ~cStatus_VBlank;
            result = cResult_Success;
            break;
        case 0x2003:
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[3];
            result = cResult_Success;
            break;
        case 0x2004:
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[4];
            result = cResult_Success;
            break;
        case 0x2005:
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[5];
            result = cResult_Success;
            break;
        case 0x2006:
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[6];
            result = cResult_Success;
            break;
        case 0x2007:
            pRegister[7] = mReadBuffer;
            accessBus(cOperation_Read, 1, mAddress, &mReadBuffer);
            *static_cast<u8*>(pArgument->mAccess.mData) = pRegister[7];
            latchCharacter(mAddress >> 4);
            mAddress += (pRegister[0] & cControl_Increment32) ? 32 : 1;
            mReadCount++;
            result = cResult_Success;
            break;
        }
    }

    return result;
}

/**
 * @brief Advance the PPU: handle reset, NMI and vertical blank events, and render a scanline.
 * @param pUnit The calling unit.
 * @param channel The channel.
 * @param operation The operation.
 * @param pArgument The events; receives the cycles spent in OAM DMA.
 * @return The result (always success).
 */
int CHvcPpu::Process(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) {
    _SEntity* pEntity = mEntity;
    u8* pMemory = pEntity->mMemory;
    u8* pLatch = &pMemory[cMemory_PpuLatch];

    if (pEntity->mResetRequest & (1 << mDescriptor.mIndex)) {
        pMemory[cMemory_IoShadow + 0x14] = 0;
        pMemory[cMemory_IoRegister + 0x14] = 0;
        std::memset(pLatch, 0, 8);
        std::memset(&pMemory[cMemory_PpuRegister], 0, 8);
        mDmaCount = 0;
        mCoarseX = 0;
        mFineX = 0;
        mNameTableX = 0;
        mCoarseY = 0;
        mFineY = 0;
        mNameTableY = 0;
        mScrollX = 0;
        mScrollY = 0;
        mAddress = 0;
        mOamAddress = 0;
        mIsNmiRaised = 0;
        mLineX = 0;
        mLineY = 0;
        mStatus = 0;
        mReadBuffer = 0;
        mEntity->mResetRequest &= ~(1 << mDescriptor.mIndex);
        std::memset(&mEntity->mMemory[cMemory_Palette], 0x0f, 0x20);
        if (mEntity->IsMmc2orMmc4()) {
            mLatchCharacter = &CHvcPpu::_LatchCharacter_Mmc2orMmc4;
        }
    }

    if (pLatch[0] & cControl_Nmi) {
        if (pArgument->mProcess.mEvents & cEvent_Nmi) {
            if (!mIsNmiRaised) {
                pEntity->mInterruptRequest |= 1;
                mIsNmiRaised = 1;
            }
        } else if (mIsNmiRaised) {
            mIsNmiRaised = 0;
        }
    }

    if ((pArgument->mProcess.mEvents & cEvent_VBlankStart) && !(mStatus & cStatus_VBlank)) {
        mStatus |= cStatus_VBlank;
        pMemory[cMemory_PpuRegister + 2] |= cStatus_VBlank;
    }

    if (pArgument->mProcess.mEvents & cEvent_VBlankEnd) {
        mStatus &= 0x1f;
        pMemory[cMemory_PpuRegister + 2] &= 0x1f;
    }

    if (pArgument->mProcess.mEvents & cEvent_Line) {
        u64 line = mEntity->mScanlineClock >> 8;
        if (line < 0xf0) {
            u16 lineX = mLineX;
            u16 lineY = mLineY;
            if (mEntity->mTitleRender->mLineScroll[line].mX < 0x200) {
                mLineX = mEntity->mTitleRender->mLineScroll[line].mX;
            }

            if (mEntity->mTitleRender->mLineScroll[line].mY < 0x200) {
                mLineY = mEntity->mTitleRender->mLineScroll[line].mY;
            }

            for (s32 i = 0; i < cRasterizePhaseNum; i++) {
                (this->*mRasterize[i])(line);
            }

            mLineX = lineX;
            mLineY = lineY;
            if (isAttributeRow(mFrameScrollY)) {
                mFrameScrollY++;
                mLineY = mScrollY;
            } else {
                u32 nextY = lineY + 1;
                mLineY = nextY;
                if (nextY - 0x1f0 < 0x10) {
                    mLineY = 0;
                } else if (nextY - 0xf0 < 0x10) {
                    mLineY = 0x100;
                }
            }
        } else {
            mLineX = mScrollX;
            mLineY = mScrollY;
        }
    }

    if (pArgument->mProcess.mCycles != nullptr) {
        *pArgument->mProcess.mCycles = mEntity->mDmaCycles * mDmaCount;
    }

    mDmaCount = 0;
    return cResult_Success;
}

/**
 * @brief Character latch of MMC2/MMC4 cartridges: fetching character $FD or $FE switches banks.
 * @param character The fetched character.
 */
void CHvcPpu::_LatchCharacter_Mmc2orMmc4(u16 character) {
    if (!mEntity->IsMmc2orMmc4()) {
        return;
    }

    u16 low = character & 0xff;
    if (low != 0xfd && low != 0xfe) {
        return;
    }

    CPlatformUnit* pParent = mParent;
    UArgument argument;
    argument.mSystem = {4, 1, character, 0};
    pParent->System(this, 0, 3, &argument);
}

/**
 * @brief Expand a character row into dots.
 * @param pPattern The low plane byte of the row (the high plane follows 8 bytes later).
 * @param isFlip Whether the row is flipped horizontally.
 * @return The dots, 4 bits each.
 */
u64 CHvcPpu::_PatternDot(u8* pPattern, bool isFlip) {
    u8 low = pPattern[0];
    const u64* pTable = isFlip ? _PatternDotData : _PatternDotDataFlip;
    return pTable[low] + (pTable[pPattern[8]] << 1);
}

/**
 * @brief Expand a character row into dots, unflipped.
 * @param pPattern The low plane byte of the row (the high plane follows 8 bytes later).
 * @return The dots, 4 bits each.
 */
u64 CHvcPpu::_PatternDotNoFlip(u8* pPattern) {
    return _PatternDotDataFlip[pPattern[0]] + (_PatternDotDataFlip[pPattern[8]] << 1);
}

namespace {

/**
 * @brief Copy a value to or from a little endian buffer.
 * @param rpBuffer The buffer cursor, advanced past the value.
 * @param rValue The value.
 * @param isImport Whether to read the value from the buffer.
 */
template <typename T>
inline void serialize(u8*& rpBuffer, T& rValue, bool isImport) {
    if (isImport) {
        rValue = rpBuffer[0];
        for (u32 i = 1; i < sizeof(T); i++) {
            rValue |= rpBuffer[i] << (i * 8);
        }
    } else {
        T value = rValue;
        for (u32 i = 0; i < sizeof(T); i++) {
            rpBuffer[i] = value >> (i * 8);
        }
    }

    rpBuffer += sizeof(T);
}

}  // namespace

/**
 * @brief Copy the register state to or from a buffer.
 * @param pBuffer The buffer, or null to do nothing.
 * @param isImport Whether to load the state from the buffer.
 */
void CHvcPpu::serializeState(void* pBuffer, bool isImport) {
    if (pBuffer == nullptr) {
        return;
    }

    u8* pCursor = static_cast<u8*>(pBuffer);
    serialize(pCursor, mToggle, isImport);
    serialize(pCursor, mReadCount, isImport);
    serialize(pCursor, mCoarseX, isImport);
    serialize(pCursor, mFineX, isImport);
    serialize(pCursor, mNameTableX, isImport);
    serialize(pCursor, mCoarseY, isImport);
    serialize(pCursor, mFineY, isImport);
    serialize(pCursor, mNameTableY, isImport);
    serialize(pCursor, mScrollX, isImport);
    serialize(pCursor, mScrollY, isImport);
    serialize(pCursor, mAddress, isImport);
    serialize(pCursor, mOamAddress, isImport);
    serialize(pCursor, mIsNmiRaised, isImport);
    serialize(pCursor, mLineX, isImport);
    serialize(pCursor, mLineY, isImport);
    serialize(pCursor, mStatus, isImport);
    serialize(pCursor, mReadBuffer, isImport);
    serialize(pCursor, mFrameScrollY, isImport);
    serialize(pCursor, _60, isImport);
    serialize(pCursor, mDmaCount, isImport);
}

/**
 * @brief Save or load the register state.
 * @param pBuffer The buffer, or null to only get the size.
 * @param isImport Whether to load the state from the buffer.
 * @return The size of the state.
 */
u64 CHvcPpu::_SerializeCore(void* pBuffer, bool isImport) {
    serializeState(pBuffer, isImport);
    return 0x1c;
}

/**
 * @brief Save the register state.
 * @param pBuffer The buffer.
 */
void CHvcPpu::_SerializeCoreExport(void* pBuffer) {
    serializeState(pBuffer, false);
}

/**
 * @brief Load the register state.
 * @param pBuffer The buffer.
 */
void CHvcPpu::_SerializeCoreImport(const void* pBuffer) {
    _SerializeCore(const_cast<void*>(pBuffer), true);
}

/**
 * @brief Get the size of the serialized state, including its tag.
 * @return The size.
 */
u64 CHvcPpu::_SerializeCoreInferSize() {
    return 0x30;
}

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
