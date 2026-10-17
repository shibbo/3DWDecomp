#include "LuigiBros/Emulator/HvcPpu.hpp"
#include "LuigiBros/Common/Serialize.hpp"
#include <cstring>
namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
/** @brief Initializes PPU identity and raster callbacks. */
CHvcPpu::CHvcPpu() {
    _18 = 4;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    memset(mName, ' ', 15);
    strncpy(mName, "HVC PPU        ", 15);
    mRasterPhase0 = &CHvcPpu::_RasterizePhase0;
    mRasterPhase1 = &CHvcPpu::_RasterizePhase1;
    mRasterPhase2 = &CHvcPpu::_RasterizePhase2;
    mRasterPhase3 = &CHvcPpu::_RasterizePhase3;
    mRasterPhase4 = &CHvcPpu::_RasterizePhase4;
    mCharacterLatch = &CHvcPpu::_LatchCharacter;
}
/** @brief Releases PPU platform state. */
CHvcPpu::~CHvcPpu() {}
/** @brief Reads and writes PPU registers and transfers sprite DMA data. */
unsigned int CHvcPpu::Access(CPlatformUnit*, unsigned long, unsigned long operation, UArgument* arguments) {
    unsigned char* memory = mpEntity->pMemory;
    unsigned char* registers = memory + 0x303800;
    unsigned long address = arguments->values[2];
    if (operation == 1) {
        switch (address) {
        case 0x2000: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[8] = value;
            registers[0] = value;
            mRegisters[7] = (value >> 1) & 1;
            mRegisters[4] = registers[8] & 1;
            mScrollX = (((mRegisters[4] << 8) | (mRegisters[2] << 3)) & 0x1f8) | (mRegisters[3] & 7);
            mRasterScrollX = mScrollX;
            unsigned long line = mpEntity->cycle.line;
            unsigned int vertical = (((mRegisters[7] << 8) | (mRegisters[5] << 3)) & 0x1f8) | (mRegisters[6] & 7);
            mScrollY = vertical;
            if (vertical > 0x1ef) mScrollY = 0x100;
            else if ((vertical & 0x1f0) == 0xf0) mScrollY = 0;
            if ((line >> 8) >= 239) mRawScrollY = vertical;
            return 0;
        }
        case 0x2001: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[9] = value;
            registers[1] = value;
            return 0;
        }
        case 0x2003: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[11] = value;
            registers[3] = value;
            mOamAddress = value;
            return 0;
        }
        case 0x2004: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[12] = value;
            registers[4] = value;
            unsigned long transferAddress = 0x304410 + mOamAddress;
            CPlatformUnit* parent = mpParent;
            UArgument transfer;
            transfer.values[0] = 1;
            transfer.pointers[4] = registers + 12;
            transfer.values[2] = transferAddress;
            parent->Access(this, 1, 1, &transfer);
            ++mOamAddress;
            return 0;
        }
        case 0x2005: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[13] = value;
            registers[5] = value;
            if (!(mRegisters[0] & 1)) {
                mRegisters[2] = value >> 3;
                mRegisters[3] = registers[13] & 7;
                mScrollX = (((mRegisters[4] << 8) | (mRegisters[2] << 3)) & 0x1f8) | (mRegisters[3] & 7);
                mRasterScrollX = mScrollX;
            } else {
                mRegisters[5] = value >> 3;
                mRegisters[6] = registers[13] & 7;
                unsigned long line = mpEntity->cycle.line;
                unsigned int vertical = (((mRegisters[7] << 8) | (mRegisters[5] << 3)) & 0x1f8) | (mRegisters[6] & 7);
                mScrollY = vertical;
                if (vertical > 0x1ef) mScrollY = 0x100;
                else if ((vertical & 0x1f0) == 0xf0) mScrollY = 0;
                if ((line >> 8) >= 239) mRawScrollY = vertical;
            }
            ++mRegisters[0];
            return 0;
        }
        case 0x2006: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[14] = value;
            registers[6] = value;
            if (!(mRegisters[0] & 1)) {
                mRegisters[6] = (value >> 4) & 7;
                mRegisters[7] = (registers[14] >> 3) & 1;
                mRegisters[4] = (registers[14] >> 2) & 1;
                mRegisters[5] &= 7;
                mRegisters[5] |= (registers[14] << 3) & 0x18;
                mScrollX = (((mRegisters[4] << 8) | (mRegisters[2] << 3)) & 0x1f8) | (mRegisters[3] & 7);
                mRasterScrollX = mScrollX;
                unsigned long line = mpEntity->cycle.line;
                unsigned int vertical = (((mRegisters[7] << 8) | (mRegisters[5] << 3)) & 0x1f8) | (mRegisters[6] & 7);
                mScrollY = vertical;
                if (vertical > 0x1ef) mScrollY = 0x100;
                else if ((vertical & 0x1f0) == 0xf0) mScrollY = 0;
                if ((line >> 8) >= 239) mRawScrollY = vertical;
            } else {
                mRegisters[5] &= 0x18;
                mRegisters[5] |= registers[14] >> 5;
                mRegisters[2] = registers[14] & 0x1f;
                mScrollX = (((mRegisters[4] << 8) | (mRegisters[2] << 3)) & 0x1f8) | (mRegisters[3] & 7);
                mRasterScrollX = mScrollX;
                unsigned int vertical = (((mRegisters[7] << 8) | (mRegisters[5] << 3)) & 0x1f8) | (mRegisters[6] & 7);
                mScrollY = vertical;
                if (vertical > 0x1ef) mScrollY = 0x100;
                else if ((vertical & 0x1f0) == 0xf0) mScrollY = 0;
                mRasterScrollY = mScrollY;
                mRawScrollY = vertical;
                mRegisters[1] = 0;
                mVramAddress = (mRegisters[2] & 0x1f) | ((mRegisters[4] & 1) << 10) |
                    ((mRegisters[7] & 1) << 11) | ((mRegisters[5] & 0x1f) << 5) |
                    ((mRegisters[6] & 3) << 12);
            }
            ++mRegisters[0];
            return 0;
        }
        case 0x2007: {
            mRegisters[1] = 0;
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[15] = value;
            registers[7] = value;
            unsigned long transferAddress = mVramAddress;
            CPlatformUnit* parent = mpParent;
            UArgument transfer;
            transfer.values[0] = 1;
            transfer.pointers[4] = registers + 15;
            transfer.values[2] = transferAddress;
            parent->Access(this, 1, 1, &transfer);
            mVramAddress += (registers[8] & 4) ? 32 : 1;
            return 0;
        }
        case 0x4014: {
            unsigned char page = *static_cast<unsigned char*>(arguments->pointers[4]);
            if (mpEntity->cpuBanks[page >> 4].readType == 1) {
                unsigned long offset = mpEntity->cpuBanks[page >> 4].readOffset;
                memory[0x303a24] = page;
                memory[0x303824] = page;
                CPlatformUnit* parent = mpParent;
                UArgument transfer;
                transfer.values[0] = 0x100;
                transfer.pointers[4] = memory + offset + ((page & 0xf) << 8);
                transfer.values[2] = 0x3800;
                parent->Access(this, 1, 1, &transfer);
                ++mDmaCount;
            }
            return 0;
        }
        default: return 4;
        }
    }
    switch (address) {
    case 0x2000: *static_cast<unsigned char*>(arguments->pointers[4]) = registers[0]; return 0;
    case 0x2001: *static_cast<unsigned char*>(arguments->pointers[4]) = registers[1]; return 0;
    case 0x2002:
        *static_cast<unsigned char*>(arguments->pointers[4]) = registers[2];
        mRegisters[0] = mRegisters[1] = 0;
        registers[2] &= 0x7f;
        return 0;
    case 0x2003: *static_cast<unsigned char*>(arguments->pointers[4]) = registers[3]; return 0;
    case 0x2004: *static_cast<unsigned char*>(arguments->pointers[4]) = registers[4]; return 0;
    case 0x2005: *static_cast<unsigned char*>(arguments->pointers[4]) = registers[5]; return 0;
    case 0x2006: *static_cast<unsigned char*>(arguments->pointers[4]) = registers[6]; return 0;
    case 0x2007: {
        registers[7] = mReadBuffer;
        unsigned long transferAddress = mVramAddress;
        CPlatformUnit* parent = mpParent;
            UArgument transfer;
        transfer.values[0] = 1;
        transfer.pointers[4] = &mReadBuffer;
        transfer.values[2] = transferAddress;
        parent->Access(this, 1, 0, &transfer);
        *static_cast<unsigned char*>(arguments->pointers[4]) = registers[7];
        (this->*mCharacterLatch)(mVramAddress >> 4);
        mVramAddress += (registers[0] & 4) ? 32 : 1;
        ++mRegisters[1];
        return 0;
    }
    default: return 4;
    }
}
/** @brief Handles reset, display timing, raster callbacks, and elapsed cycles. */
unsigned int CHvcPpu::Process(CPlatformUnit*, unsigned long, unsigned long, UArgument* arguments) {
    struct RasterParameters {
        unsigned char reserved[0x36];
        unsigned short scroll[240][2];
    };
    unsigned char* memory = mpEntity->pMemory;
    unsigned char* control = memory + 0x303808;
    if ((1 << _18) & mpEntity->order.words[0]) {
        memory[0x303a24] = 0;
        memory[0x303824] = 0;
        memset(memory + 0x303808, 0, 8);
        memset(memory + 0x303800, 0, 8);
        memset(mRegisters + 2, 0, 6);
        mScrollX = mScrollY = mVramAddress = 0;
        mOamAddress = mNmiLatched = 0;
        mRasterScrollX = mRasterScrollY = 0;
        mStatus = mReadBuffer = 0;
        mDmaCount = 0;
        _SEntity* entity = mpEntity;
        unsigned int resetMask = 1 << _18;
        entity->order.words[0] &= ~resetMask;
        memset(mpEntity->pMemory + 0x304510, 0x0f, 0x20);
        if (static_cast<unsigned int>(mpEntity->mapper - 9) <= 1)
            mCharacterLatch = &CHvcPpu::_LatchCharacter_Mmc2orMmc4;
    }
    if (*control & 0x80) {
        if (arguments->values[0] & 8) {
            if (!mNmiLatched) {
                mpEntity->order.words[2] |= 1;
                mNmiLatched = 1;
            }
        } else if (mNmiLatched) {
            mNmiLatched = 0;
        }
    }
    if ((arguments->values[0] & 0x20) && !(mStatus & 0x80)) {
        mStatus |= 0x80;
        memory[0x303802] |= 0x80;
    }
    if (arguments->values[0] & 0x80) {
        mStatus &= 0x1f;
        memory[0x303802] &= 0x1f;
    }
    if (arguments->values[0] & 0x100) {
        unsigned long line = mpEntity->cycle.line >> 8;
        if (line < 240) {
            unsigned short horizontal = mRasterScrollX;
            unsigned short vertical = mRasterScrollY;
            unsigned short x = reinterpret_cast<RasterParameters*>(mpEntity->pCustom)->scroll[line][0];
            if (x < 0x200)
                mRasterScrollX = x;
            unsigned short y = reinterpret_cast<RasterParameters*>(mpEntity->pCustom)->scroll[line][1];
            if (y < 0x200)
                mRasterScrollY = y;
            (this->*mRasterPhase0)(line);
            (this->*mRasterPhase1)(line);
            (this->*mRasterPhase2)(line);
            (this->*mRasterPhase3)(line);
            (this->*mRasterPhase4)(line);
            mRasterScrollX = horizontal;
            mRasterScrollY = vertical;
            if ((static_cast<unsigned int>(mRawScrollY & ~0xf) | 0x100) == 0x1f0) {
                ++mRawScrollY;
                mRasterScrollY = mScrollY;
            } else {
                mRasterScrollY = vertical + 1;
                if (static_cast<unsigned int>(vertical - 0x1ef) <= 0xf)
                    mRasterScrollY = 0;
                else if (static_cast<unsigned int>(vertical - 0xef) <= 0xf)
                    mRasterScrollY = 0x100;
            }
        } else {
            mRasterScrollX = mScrollX;
            mRasterScrollY = mScrollY;
        }
    }
    if (arguments->pointers[2])
        *static_cast<unsigned long*>(arguments->pointers[2]) = mpEntity->cycle._a38 * mDmaCount;
    mDmaCount = 0;
    return 0;
}
/** @brief Selects the title-specific raster callbacks when attaching the PPU. */
unsigned int CHvcPpu::System(CPlatformUnit*, unsigned long, unsigned long operation, UArgument*) {
    if (operation == 0 && reinterpret_cast<const unsigned long*>(mpEntity->pTitle)[6] == 0xd3) {
        mRasterPhase1 = &CHvcPpu::_RasterizePhase1_Mother;
        mRasterPhase4 = &CHvcPpu::_RasterizePhase4_Mother;
    }
    return 0;
}
/** @brief Resets PPU registers and installs the default raster callbacks. */
unsigned int CHvcPpu::_Prologue() {
    std::memset(mRegisters, 0, sizeof(mRegisters));
    mScrollX = mScrollY = mVramAddress = 0;
    mOamAddress = mNmiLatched = 0;
    mStatus = mReadBuffer = 0;
    mRasterPhase0 = &CHvcPpu::_RasterizePhase0;
    mRasterPhase1 = &CHvcPpu::_RasterizePhase1;
    mRasterPhase2 = &CHvcPpu::_RasterizePhase2;
    mRasterPhase3 = &CHvcPpu::_RasterizePhase3;
    mRasterPhase4 = &CHvcPpu::_RasterizePhase4;
    mCharacterLatch = &CHvcPpu::_LatchCharacter;
    return 0;
}
/** @brief Clears raster working storage at the beginning of a frame. */
void CHvcPpu::_RasterizePhase0(unsigned long line) {
    if (static_cast<unsigned int>(line) == 0) {
        std::memset(mRasterPixels, 0, 0x200);
        std::memset(mRasterPixels + 256, 0, 0x200);
    }
}
/** @brief Fetches background tiles and expands their visible scanline pixels. */
void CHvcPpu::_RasterizePhase1(unsigned long) {
    _SEntity* entity = mpEntity;
    unsigned char* memory = entity->pMemory;
    memset(mRasterBuffer, 0, 0x200);
    unsigned long vertical = mRasterScrollY;
    if (((vertical & ~0xf) | 0x100) == 0x1f0)
        return;
    unsigned long row = vertical & 7;
    unsigned int patternBase = (memory[0x303808] & 0x10) << 4;
    unsigned long horizontal = mRasterScrollX;
    unsigned char mask = memory[0x303809];
    unsigned long tileRow = (vertical << 2) & 0x3e0;
    unsigned long attributeRow = (vertical >> 2) & 0x38;
    unsigned long attributeShift = (vertical >> 2) & 4;
    unsigned long nameTable = (vertical >> 7) & 2;
    unsigned long pixel = 0;
    unsigned long fine = horizontal & 7;
    do {
        unsigned long bank = nameTable | ((horizontal >> 8) & 1);
        unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
        unsigned short tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
        unsigned long end = pixel + 8 - fine;
        if (mask & 8) {
            unsigned int palette = (names[0x3c0 | attributeRow | ((horizontal >> 5) & 7)] >>
                (attributeShift | ((horizontal >> 3) & 2))) & 3;
            unsigned short address = tile << 4;
            unsigned char* pattern = entity->pMemory + entity->ppuBanks[(address >> 10) & 7].readOffset +
                ((address & 0x3f0) | row);
            unsigned long dots = _PatternDotNoFlip(pattern);
            horizontal += 8 - fine;
            if (mask & 2) {
                for (unsigned long i = 0; i < 8; ++i) {
                    if (pixel + i == end) break;
                    unsigned int color = (dots >> ((fine + i) * 4)) & 3;
                    if (color) {
                        mRasterPixels[pixel + i].color = color | (palette << 2);
                        mRasterPixels[pixel + i].flags = 0x42;
                    }
                }
            } else {
                for (unsigned long i = 0; i < 8; ++i) {
                    if (pixel + i == end) break;
                    if (pixel + i >= 8) {
                        unsigned int color = (dots >> ((fine + i) * 4)) & 3;
                        if (color) {
                            mRasterPixels[pixel + i].color = color | (palette << 2);
                            mRasterPixels[pixel + i].flags = 0x42;
                        }
                    }
                }
            }
        } else {
            horizontal += 8 - fine;
        }
        pixel = end;
        (this->*mCharacterLatch)(tile);
        entity = mpEntity;
        fine = 0;
    } while (pixel < 8);
    if (!(mask & 8)) {
        while (pixel < 256) {
            entity = mpEntity;
            unsigned long bank = nameTable | ((horizontal >> 8) & 1);
            unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
            unsigned short tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
            unsigned long count = 256 - pixel < 8 ? 256 - pixel : 8;
            horizontal += count;
            pixel += count;
            (this->*mCharacterLatch)(tile);
        }
    } else {
        while (pixel < 256) {
            entity = mpEntity;
            unsigned long bank = nameTable | ((horizontal >> 8) & 1);
            unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
            unsigned short tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
            unsigned int palette = (names[0x3c0 | attributeRow | ((horizontal >> 5) & 7)] >>
                (attributeShift | ((horizontal >> 3) & 2))) & 3;
            unsigned short address = tile << 4;
            unsigned char* pattern = entity->pMemory + entity->ppuBanks[(address >> 10) & 7].readOffset +
                ((address & 0x3f0) | row);
            unsigned long dots = _PatternDotNoFlip(pattern);
            for (unsigned int i = 0; i < 8; ++i) {
                if (pixel >= 256) break;
                unsigned int color = (dots >> (i * 4)) & 3;
                if (color) {
                    mRasterPixels[pixel].color = color | (palette << 2);
                    mRasterPixels[pixel].flags = 0x42;
                }
                ++pixel;
            }
            horizontal += 8;
            (this->*mCharacterLatch)(tile);
        }
    }
    entity = mpEntity;
    if (static_cast<unsigned int>(entity->mapper - 9) <= 1) {
        unsigned long bank = nameTable | ((horizontal >> 8) & 1);
        unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
        unsigned short tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
        (this->*mCharacterLatch)(tile);
    }
}
/** @brief Renders backgrounds using the title-specific character-bank history. */
void CHvcPpu::_RasterizePhase1_Mother(unsigned long) {
    _SEntity* entity = mpEntity;
    unsigned char* memory = entity->pMemory;
    memset(mRasterBuffer, 0, 0x200);
    unsigned long vertical = mRasterScrollY;
    if (((vertical & ~0xf) | 0x100) == 0x1f0)
        return;
    int history[8] = {};
    unsigned long row = vertical & 7;
    unsigned int patternBase = (memory[0x303808] & 0x10) << 4;
    unsigned long horizontal = mRasterScrollX;
    unsigned char mask = memory[0x303809];
    unsigned long tileRow = (vertical << 2) & 0x3e0;
    unsigned long attributeRow = (vertical >> 2) & 0x38;
    unsigned long attributeShift = (vertical >> 2) & 4;
    unsigned long nameTable = (vertical >> 7) & 2;
    unsigned long pixel = 0;
    unsigned long fine = horizontal & 7;
    do {
        unsigned long bank = nameTable | ((horizontal >> 8) & 1);
        unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
        unsigned int tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
        unsigned long end = pixel + 8 - fine;
        if (mask & 8) {
            unsigned int palette = (names[0x3c0 | attributeRow | ((horizontal >> 5) & 7)] >>
                (attributeShift | ((horizontal >> 3) & 2))) & 3;
            if (entity->chrHistory[0 + history[0]]._01 * 3UL + 18 <= pixel)
                history[0] += 8;
            if (entity->chrHistory[1 + history[1]]._01 * 3UL + 18 <= pixel)
                history[1] += 8;
            if (entity->chrHistory[2 + history[2]]._01 * 3UL + 18 <= pixel)
                history[2] += 8;
            if (entity->chrHistory[3 + history[3]]._01 * 3UL + 18 <= pixel)
                history[3] += 8;
            if (entity->chrHistory[4 + history[4]]._01 * 3UL + 18 <= pixel)
                history[4] += 8;
            if (entity->chrHistory[5 + history[5]]._01 * 3UL + 18 <= pixel)
                history[5] += 8;
            if (entity->chrHistory[6 + history[6]]._01 * 3UL + 18 <= pixel)
                history[6] += 8;
            if (entity->chrHistory[7 + history[7]]._01 * 3UL + 18 <= pixel)
                history[7] += 8;
            unsigned int address = tile << 4;
            unsigned char* pattern = entity->pMemory + entity->chrHistory[(address >> 10) + history[address >> 10]].readOffset +
                ((address & 0x3f0) | row);
            unsigned long dots = _PatternDotNoFlip(pattern);
            horizontal += 8 - fine;
            if (mask & 2) {
                for (unsigned long i = 0; i < 8; ++i) {
                    if (pixel + i == end) break;
                    unsigned int color = (dots >> ((fine + i) * 4)) & 3;
                    if (color) {
                        mRasterPixels[pixel + i].color = color | (palette << 2);
                        mRasterPixels[pixel + i].flags = 0x42;
                    }
                }
            } else {
                for (unsigned long i = 0; i < 8; ++i) {
                    if (pixel + i == end) break;
                    if (pixel + i >= 8) {
                        unsigned int color = (dots >> ((fine + i) * 4)) & 3;
                        if (color) {
                            mRasterPixels[pixel + i].color = color | (palette << 2);
                            mRasterPixels[pixel + i].flags = 0x42;
                        }
                    }
                }
            }
        } else {
            horizontal += 8 - fine;
        }
        pixel = end;
        (this->*mCharacterLatch)(tile);
        entity = mpEntity;
        fine = 0;
    } while (pixel < 8);
    if (!(mask & 8)) {
        while (pixel < 256) {
            entity = mpEntity;
            unsigned long bank = nameTable | ((horizontal >> 8) & 1);
            unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
            unsigned int tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
            unsigned long count = 256 - pixel < 8 ? 256 - pixel : 8;
            horizontal += count;
            pixel += count;
            (this->*mCharacterLatch)(tile);
        }
    } else {
        while (pixel < 256) {
            entity = mpEntity;
            unsigned long bank = nameTable | ((horizontal >> 8) & 1);
            unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
            unsigned int tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
            unsigned int palette = (names[0x3c0 | attributeRow | ((horizontal >> 5) & 7)] >>
                (attributeShift | ((horizontal >> 3) & 2))) & 3;
            if (entity->chrHistory[0 + history[0]]._01 * 3UL + 18 <= pixel)
                history[0] += 8;
            if (entity->chrHistory[1 + history[1]]._01 * 3UL + 18 <= pixel)
                history[1] += 8;
            if (entity->chrHistory[2 + history[2]]._01 * 3UL + 18 <= pixel)
                history[2] += 8;
            if (entity->chrHistory[3 + history[3]]._01 * 3UL + 18 <= pixel)
                history[3] += 8;
            if (entity->chrHistory[4 + history[4]]._01 * 3UL + 18 <= pixel)
                history[4] += 8;
            if (entity->chrHistory[5 + history[5]]._01 * 3UL + 18 <= pixel)
                history[5] += 8;
            if (entity->chrHistory[6 + history[6]]._01 * 3UL + 18 <= pixel)
                history[6] += 8;
            if (entity->chrHistory[7 + history[7]]._01 * 3UL + 18 <= pixel)
                history[7] += 8;
            unsigned int address = tile << 4;
            unsigned char* pattern = entity->pMemory + entity->chrHistory[(address >> 10) + history[address >> 10]].readOffset +
                ((address & 0x3f0) | row);
            unsigned long dots = _PatternDotNoFlip(pattern);
            for (unsigned int i = 0; i < 8; ++i) {
                if (pixel >= 256) break;
                unsigned int color = (dots >> (i * 4)) & 3;
                if (color) {
                    mRasterPixels[pixel].color = color | (palette << 2);
                    mRasterPixels[pixel].flags = 0x42;
                }
                ++pixel;
            }
            horizontal += 8;
            (this->*mCharacterLatch)(tile);
        }
    }
    entity = mpEntity;
    if (static_cast<unsigned int>(entity->mapper - 9) <= 1) {
        unsigned long bank = nameTable | ((horizontal >> 8) & 1);
        unsigned char* names = entity->pMemory + entity->ppuBanks[8 + bank].readOffset;
        unsigned int tile = patternBase | names[tileRow | ((horizontal >> 3) & 0x1f)];
        (this->*mCharacterLatch)(tile);
    }
}
/** @brief Expands visible sprites while preserving OAM order and priority. */
void CHvcPpu::_RasterizePhase3(unsigned long line) {
    _SEntity* entity = mpEntity;
    unsigned char* memory = entity->pMemory;
    memset(mRasterBuffer + 0x200, 0, 0x200);
    unsigned char* sprites = entity->pMemory + 0x304410;
    unsigned char mask = memory[0x303809];
    unsigned char control = memory[0x303808];
    int count = 0;
    if (!(control & 0x20)) {
        unsigned int patternBase = (control << 5) & 0x100;
        for (unsigned long offset = 0; offset < 0x100; offset += 4) {
            unsigned long top = sprites[offset];
            if (line < top || line > top + 7) continue;
            unsigned char attributes = sprites[offset + 2];
            unsigned long row = (line - top) & 7;
            if (attributes & 0x80) row ^= 7;
            int limit = reinterpret_cast<unsigned char*>(mpEntity->pCustom)[0x34];
            unsigned int tile = sprites[offset + 1];
            tile |= patternBase;
            bool draw = (mask & 0x10) && count < limit;
            unsigned char flags = (attributes & 0x20) ? 1 : 3;
            flags |= (offset == 0) << 6;
            flags |= (count >= limit) << 5;
            const unsigned long* table = (attributes & 0x40) ? _PatternDotData : _PatternDotDataFlip;
            if (draw) {
                unsigned int address = tile << 4;
                unsigned char* pattern = mpEntity->pMemory + mpEntity->ppuBanks[(address >> 10) & 7].readOffset +
                    ((address & 0x3f0) | (row & 7));
                unsigned long x = sprites[offset + 3];
                unsigned long dots = table[pattern[0]] + (table[pattern[8]] << 1);
                unsigned int palette = 0x10 | ((attributes & 3) << 2);
                if (mask & 4) {
                    for (unsigned int i = 0; i < 8; ++i) {
                        if (x + i >= 256) break;
                        unsigned int color = (dots >> (i * 4)) & 3;
                        RasterPixel* pixel = &mRasterPixels[256 + x + i];
                        if (color && !pixel->color) {
                            pixel->color = palette | color;
                            pixel->flags = flags;
                        }
                    }
                } else {
                    for (unsigned int i = 0; i < 8; ++i) {
                        if (x + i >= 256) break;
                        if (x + i < 8) continue;
                        unsigned int color = (dots >> (i * 4)) & 3;
                        RasterPixel* pixel = &mRasterPixels[256 + x + i];
                        if (color && !pixel->color) {
                            pixel->color = palette | color;
                            pixel->flags = flags;
                        }
                    }
                }
            }
            (this->*mCharacterLatch)(tile);
            ++count;
        }
    } else {
        for (unsigned long offset = 0; offset < 0x100; offset += 4) {
            unsigned long top = sprites[offset];
            if (line < top || line > top + 15) continue;
            unsigned char attributes = sprites[offset + 2];
            unsigned long row = (line - top) & 15;
            if (attributes & 0x80) row ^= 15;
            int limit = reinterpret_cast<unsigned char*>(mpEntity->pCustom)[0x34];
            unsigned int tile = sprites[offset + 1];
            tile = ((tile | (tile << 8)) & 0x1fe) | (row > 7);
            bool draw = (mask & 0x10) && count < limit;
            unsigned char flags = (attributes & 0x20) ? 1 : 3;
            flags |= (offset == 0) << 6;
            flags |= (count >= limit) << 5;
            const unsigned long* table = (attributes & 0x40) ? _PatternDotData : _PatternDotDataFlip;
            if (draw) {
                unsigned int address = tile << 4;
                unsigned char* pattern = mpEntity->pMemory + mpEntity->ppuBanks[(address >> 10) & 7].readOffset +
                    ((address & 0x3f0) | (row & 7));
                unsigned long x = sprites[offset + 3];
                unsigned long dots = table[pattern[0]] + (table[pattern[8]] << 1);
                unsigned int palette = 0x10 | ((attributes & 3) << 2);
                if (mask & 4) {
                    for (unsigned int i = 0; i < 8; ++i) {
                        if (x + i >= 256) break;
                        unsigned int color = (dots >> (i * 4)) & 3;
                        RasterPixel* pixel = &mRasterPixels[256 + x + i];
                        if (color && !pixel->color) {
                            pixel->color = palette | color;
                            pixel->flags = flags;
                        }
                    }
                } else {
                    for (unsigned int i = 0; i < 8; ++i) {
                        if (x + i >= 256) break;
                        if (x + i < 8) continue;
                        unsigned int color = (dots >> (i * 4)) & 3;
                        RasterPixel* pixel = &mRasterPixels[256 + x + i];
                        if (color && !pixel->color) {
                            pixel->color = palette | color;
                            pixel->flags = flags;
                        }
                    }
                }
            }
            (this->*mCharacterLatch)(tile);
            ++count;
        }
    }
    if (count > 8) memory[0x303802] |= 0x20;
    else memory[0x303802] &= ~0x20;
}
/** @brief Resolves sprite overlap and combines the two raster layers. */
void CHvcPpu::_RasterizePhase2(unsigned long line) {
    unsigned char* custom = reinterpret_cast<unsigned char*>(mpEntity->pCustom);
    unsigned char* memory = mpEntity->pMemory;
    bool visible = line >= custom[0x32] && line <= custom[0x33];
    unsigned char* status = &mStatus;
    unsigned char* mappedStatus = memory + 0x303802;
    if (!visible) {
        for (unsigned long offset = 0; offset != 256; ++offset) {
            RasterPixel* background = &mRasterPixels[offset];
            RasterPixel* sprite = &mRasterPixels[offset + 256];
            unsigned char forceOverlap = reinterpret_cast<unsigned char*>(mpEntity->pCustom)[0x35];
            unsigned char spriteFlags = sprite->flags;
            if ((forceOverlap ? spriteFlags : background->flags & spriteFlags) & 0x40) {
                if (!(*status & 0x40) && ((background->color & 3) || (sprite->color & 3))) {
                    *status |= 0x40;
                    *mappedStatus |= 0x40;
                }
            }
        }
    } else {
        unsigned char* output = mpEntity->pVideo;
        unsigned char* palette = memory + 0x304510;
        unsigned char attributes = memory[0x303809] & 0xe1;
        output += line << 10;
        for (unsigned long offset = 0; offset != 256; ++offset, output += 4) {
            RasterPixel* background = &mRasterPixels[offset];
            RasterPixel* sprite = &mRasterPixels[offset + 256];
            unsigned char forceOverlap = reinterpret_cast<unsigned char*>(mpEntity->pCustom)[0x35];
            unsigned char spriteFlags = sprite->flags;
            if ((forceOverlap ? spriteFlags : background->flags & spriteFlags) & 0x40) {
                if (!(*status & 0x40) && ((background->color & 3) || (sprite->color & 3))) {
                    *status |= 0x40;
                    *mappedStatus |= 0x40;
                }
            }
            unsigned int backgroundPriority = background->flags & 3;
            unsigned int spritePriority = sprite->flags & 3;
            unsigned char flags = spritePriority ? attributes | 8 : attributes;
            bool useBackground = spritePriority < backgroundPriority;
            RasterPixel* pixel = useBackground ? background : sprite;
            unsigned char color = pixel->color;
            unsigned char paletteColor = palette[color];
            if (useBackground) flags = attributes | 8;
            output[1] = flags;
            output[0] = paletteColor & 0x3f;
            output[2] = color;
        }
    }
}
/** @brief Expands the two pattern bitplanes into packed pixel values. */
unsigned long CHvcPpu::_PatternDot(unsigned char* pattern, bool flip) {
    unsigned char low = pattern[0];
    const unsigned long* table = flip ? _PatternDotData : _PatternDotDataFlip;
    return table[low] + (table[pattern[8]] << 1);
}
/** @brief Expands a pattern without horizontal flipping. */
unsigned long CHvcPpu::_PatternDotNoFlip(unsigned char* pattern) {
    return _PatternDotDataFlip[pattern[0]] + (_PatternDotDataFlip[pattern[8]] << 1);
}
/** @brief Default final raster phase has no work. */
void CHvcPpu::_RasterizePhase4(unsigned long) {}
/** @brief Saves character-bank mappings and resets their scanline history. */
void CHvcPpu::_RasterizePhase4_Mother(unsigned long) {
    mpEntity->chrHistory[0].readOffset = mpEntity->ppuBanks[0].readOffset;
    mpEntity->chrHistory[0]._01 = 0xc8;
    mpEntity->ppuBanks[0]._01 = 0;
    mpEntity->ppuBanks[0]._11 = mpEntity->cycle.lineCycle >> 8;
    mpEntity->chrHistory[1].readOffset = mpEntity->ppuBanks[1].readOffset;
    mpEntity->chrHistory[1]._01 = 0xc8;
    mpEntity->ppuBanks[1]._01 = 0;
    mpEntity->ppuBanks[1]._11 = mpEntity->cycle.lineCycle >> 8;
    mpEntity->chrHistory[2].readOffset = mpEntity->ppuBanks[2].readOffset;
    mpEntity->chrHistory[2]._01 = 0xc8;
    mpEntity->ppuBanks[2]._01 = 0;
    mpEntity->ppuBanks[2]._11 = mpEntity->cycle.lineCycle >> 8;
    mpEntity->chrHistory[3].readOffset = mpEntity->ppuBanks[3].readOffset;
    mpEntity->chrHistory[3]._01 = 0xc8;
    mpEntity->ppuBanks[3]._01 = 0;
    mpEntity->ppuBanks[3]._11 = mpEntity->cycle.lineCycle >> 8;
    mpEntity->chrHistory[4].readOffset = mpEntity->ppuBanks[4].readOffset;
    mpEntity->chrHistory[4]._01 = 0xc8;
    mpEntity->ppuBanks[4]._01 = 0;
    mpEntity->ppuBanks[4]._11 = mpEntity->cycle.lineCycle >> 8;
    mpEntity->chrHistory[5].readOffset = mpEntity->ppuBanks[5].readOffset;
    mpEntity->chrHistory[5]._01 = 0xc8;
    mpEntity->ppuBanks[5]._01 = 0;
    mpEntity->ppuBanks[5]._11 = mpEntity->cycle.lineCycle >> 8;
    mpEntity->chrHistory[6].readOffset = mpEntity->ppuBanks[6].readOffset;
    mpEntity->chrHistory[6]._01 = 0xc8;
    mpEntity->ppuBanks[6]._01 = 0;
    mpEntity->ppuBanks[6]._11 = mpEntity->cycle.lineCycle >> 8;
    mpEntity->chrHistory[7].readOffset = mpEntity->ppuBanks[7].readOffset;
    mpEntity->chrHistory[7]._01 = 0xc8;
    mpEntity->ppuBanks[7]._01 = 0;
    mpEntity->ppuBanks[7]._11 = mpEntity->cycle.lineCycle >> 8;
}
/** @brief Default character latch has no work. */
void CHvcPpu::_LatchCharacter(unsigned short) {}
/** @brief Forwards latch-triggering pattern addresses to MMC2 and MMC4. */
void CHvcPpu::_LatchCharacter_Mmc2orMmc4(unsigned short address) {
    if (static_cast<unsigned int>(mpEntity->mapper - 9) > 1)
        return;
    if (static_cast<unsigned short>((address & 0xff) - 0xfd) > 1)
        return;
    CPlatformUnit* parent = mpParent;
    unsigned long latchAddress = address;
    UArgument arguments;
    arguments.values[0] = 4;
    arguments.values[1] = 1;
    arguments.values[2] = latchAddress;
    arguments.values[3] = 0;
    parent->System(this, 0, 3, &arguments);
}
/** @brief Finalizes the PPU. @return Zero. */
unsigned int CHvcPpu::_Epilogue() { return 0; }
/** @brief Reports the aligned serialized state size. */
unsigned long CHvcPpu::_SerializeCoreInferSize() { return 0x30; }
/** @brief Identifies PPU state content. */
unsigned char CHvcPpu::_SerializeTagContent() { return 4; }
/** @brief Identifies the default PPU state species. */
unsigned char CHvcPpu::_SerializeTagSpecies() { return 0; }
/** @brief Reports PPU state version. */
unsigned short CHvcPpu::_SerializeTagVersion() { return 0x100; }
/** @brief Supplies the PPU state label. */
const unsigned char* CHvcPpu::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("PPU    ");
}
/** @brief Transfers PPU register and address state in little-endian order. */
unsigned long CHvcPpu::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mRegisters[2]);
    stream.Entry(mRegisters[3]);
    stream.Entry(mRegisters[4]);
    stream.Entry(mRegisters[5]);
    stream.Entry(mRegisters[6]);
    stream.Entry(mRegisters[7]);
    stream.Entry(mScrollX);
    stream.Entry(mScrollY);
    stream.Entry(mVramAddress);
    stream.Entry(mOamAddress);
    stream.Entry(mNmiLatched);
    stream.Entry(mRasterScrollX);
    stream.Entry(mRasterScrollY);
    stream.Entry(mStatus);
    stream.Entry(mReadBuffer);
    stream.Entry(mRawScrollY);
    stream.Entry(mAddress60);
    stream.Entry(mDmaCount);
    return stream.GetSize();
}
/** @brief Exports PPU state. */
void CHvcPpu::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports PPU state. */
void CHvcPpu::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
} } } }
