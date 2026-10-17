#include "LuigiBros/Emulator/HvcSpu.hpp"
#include "LuigiBros/Emulator/Hvc.hpp"
#include <cstring>
#include "LuigiBros/Emulator/HVCEmu.hpp"
namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
/** @brief Initializes the SPU platform identity. */
CHvcSpu::CHvcSpu() {
    _18 = 5;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    memset(mName, ' ', 15);
    strncpy(mName, "HVC SPU        ", 15);
}
/** @brief Releases SPU platform state. */
CHvcSpu::~CHvcSpu() {}
/** @brief Resets audio counters and all nine sound channels. */
unsigned int CHvcSpu::_Prologue() {
    mPendingCycles = mSampleCycleRemainder = mPendingEvents = 0;
    mOutputLeft = mOutputRight = 0;
    mSquare0.Reset(0, 0);
    mSquare1.Reset(0, 1);
    mTriangle.Reset(0, 2);
    mNoise.Reset(0, 3);
    mDelta.Reset(0, 4);
    mDiskAudio.Reset(0, 5);
    mExtraSquare0.Reset(0, 6);
    mExtraSquare1.Reset(0, 7);
    mExtraTriangle.Reset(0, 8);
    return 0;
}
/** @brief Completes SPU shutdown. */
unsigned int CHvcSpu::_Epilogue() { return 0; }
/** @brief Identifies SPU serialization content. */
unsigned char CHvcSpu::_SerializeTagContent() { return 5; }
/** @brief Identifies SPU serialization species. */
unsigned char CHvcSpu::_SerializeTagSpecies() { return 0; }
/** @brief Reports the SPU state version. */
unsigned short CHvcSpu::_SerializeTagVersion() { return 0x100; }
/** @brief Labels serialized SPU state. */
const unsigned char* CHvcSpu::_SerializeTagComment() { return reinterpret_cast<const unsigned char*>("SPU    "); }
/** @brief Dispatches standard, disk, MMC5, and VRC6 audio register accesses. */
unsigned int CHvcSpu::Access(CPlatformUnit*, unsigned long, unsigned long operation, UArgument* arguments) {
    unsigned char* memory = mpEntity->pMemory;
    unsigned long address = arguments->values[2];
    unsigned char* registers = memory + 0x303a10;
    if (operation == 1) {
        switch (address) {
        case 0x4000: case 0x4001: case 0x4002: case 0x4003: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[address & 255] = value;
            mSquare0.Setup(address, value, mpEntity);
            return 0;
        }
        case 0x4004: case 0x4005: case 0x4006: case 0x4007: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[address & 255] = value;
            mSquare1.Setup(address - 4, value, mpEntity);
            return 0;
        }
        case 0x4008: case 0x4009: case 0x400a: case 0x400b: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[address & 255] = value;
            mTriangle.Setup(address - 8, value, mpEntity);
            return 0;
        }
        case 0x400c: case 0x400d: case 0x400e: case 0x400f: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[address & 255] = value;
            mNoise.Setup(address - 12, value, mpEntity);
            return 0;
        }
        case 0x4010: case 0x4011: case 0x4012: case 0x4013: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            registers[address & 255] = value;
            mDelta.Setup(address - 16, value, mpEntity);
            return 0;
        }
        case 0x4015: {
            memory[0x303a25] = *static_cast<unsigned char*>(arguments->pointers[4]);
            mSquare0.Setup(16, memory[0x303a25], mpEntity);
            mSquare1.Setup(16, memory[0x303a25], mpEntity);
            mTriangle.Setup(16, memory[0x303a25], mpEntity);
            mNoise.Setup(16, memory[0x303a25], mpEntity);
            mDelta.Setup(16, memory[0x303a25], mpEntity);
            mState48 |= ((~memory[0x303a25]) & 15) | 16;
            return 0;
        }
        }
    } else {
        if (address == 0x4015) { *static_cast<unsigned char*>(arguments->pointers[4]) = memory[0x303825]; return 0; }
        *static_cast<unsigned char*>(arguments->pointers[4]) = 0x40;
    }
    if (mpEntity->mapper == 0xff) {
        address = arguments->values[2];
        unsigned char* data = static_cast<unsigned char*>(arguments->pointers[4]);
        if (operation == 1) {
            int value = *reinterpret_cast<signed char*>(data);
            switch (address) {
            case 0x4020: case 0x4021: registers[address & 255] = value; return 0;
            case 0x4022: memory[0x303a32] = value; mDisk.SetTimerStart(mpEntity); return 0;
            case 0x4023: memory[0x303a33] = value; mDiskAudio.Setup(address, value, mpEntity); return 0;
            case 0x4024:
                memory[0x303a34] = value;
                if ((memory[0x303a35] & 0x87) == 0x81) {
                    mpEntity->pMemory[mDisk.GetQdOffset() + 0x100000] = value;
                    mDisk.SetQdDirty(true);
                }
                return 0;
            case 0x4025:
                memory[0x303a35] = value;
                if (value < 0) mpEntity->order.words[4] |= 1;
                else if (memory[0x303a32] & 2) {
                    memory[0x303a42] &= ~4;
                    mDisk.SetQdOffsetHalf(0);
                }
                BusMapSwitchVRAM(mpEntity, (memory[0x303a35] & 8) ? 1 : 0);
                return 0;
            case 0x4026: memory[0x303a36] = value; return 0;
            case 0x4080: case 0x4082: case 0x4083: case 0x4084: case 0x4085:
            case 0x4086: case 0x4087: case 0x4088: case 0x4089: case 0x408a:
                mDiskAudio.Setup(address, value, mpEntity); return 0;
            default:
                if ((address >> 6) == 0x101) { mDiskAudio.Setup(address, value, mpEntity); return 0; }
                return 4;
            }
        }
        switch (address) {
        case 0x4030: *data = memory[0x303840]; memory[0x303840] = 0; return 0;
        case 0x4031: *data = memory[0x303841]; mDisk.IncQdOffsetHalf(); return 0;
        case 0x4032: {
            unsigned short pc = *reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(GetCHvc()) + 0x3d623e);
            *data = memory[0x303842];
            if (pc < 0xe000) {
                *data |= 0x47;
                if (mDisk.mediaCount <= 194) *data = (*data & 0xf8) | (memory[0x307a35] & 2);
            } else if (pc == 0xef36 || pc == 0xeee2) *data &= mDisk.status;
            else *data = 0x40;
            memory[0x303842] = *data;
            return 0;
        }
        case 0x4033: *data = memory[0x303843]; return 0;
        case 0x4090: *data = mDiskAudio.mState60 | 0x40; return 0;
        case 0x4091: *data = reinterpret_cast<unsigned char*>(&mDiskAudio.mState10)[1]; return 0;
        case 0x4092: *data = mDiskAudio.mStatee0 | 0x40; return 0;
        case 0x4093: *data = reinterpret_cast<unsigned char*>(&mDiskAudio.mState88)[1]; return 0;
        case 0x4094: case 0x4095: case 0x4096: case 0x4097: return 0;
        default:
            if ((address >> 6) != 0x101) return 4;
            *data = mDiskAudio.mState09 ? 0 : mDiskAudio.mWave[address - 0x4040] | 0x40;
            return 0;
        }
    }
    if (mpEntity->mapper == 5) {
        memory = mpEntity->pMemory;
        address = arguments->values[2];
        if (operation != 1) {
            if (address != 0x5015) return 4;
            *static_cast<unsigned char*>(arguments->pointers[4]) = memory[0x303c25];
            return 0;
        }
        unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
        switch (address) {
        case 0x5000: case 0x5002: case 0x5003:
            memory[0x304010 + (address & 255)] = value;
            mExtraSquare0.Setup(address, value, mpEntity); return 0;
        case 0x5004: case 0x5006: case 0x5007:
            memory[0x304010 + (address & 255)] = value;
            mExtraSquare1.Setup(address - 4, value, mpEntity); return 0;
        case 0x5010: memory[0x304020] = value; return 0;
        case 0x5011: memory[0x304021] = value; return 0;
        case 0x5015: memory[0x304025] = value; mState48 |= (~value) & 3; return 0;
        default: return 4;
        }
    }
    if (mpEntity->mapper == 24) {
        if (operation != 1) return 0;
        address = arguments->values[2];
        if (((address - 0x9000) >> 12) > 2) return 0;
        unsigned long index = ((address & 0x3000) - 0x1000) >> 10;
        if (address & 3) index |= (address + 1) & 3;
        memory = mpEntity->pMemory;
        switch (index | 0x5000) {
        case 0x5000: case 0x5004: {
            _SWavwR& channel = index == 0 ? mExtraSquare0 : mExtraSquare1;
            unsigned char value = ((*static_cast<unsigned char*>(arguments->pointers[4]) >> 3) & 16) | (*static_cast<unsigned char*>(arguments->pointers[4]) & 15) | 32;
            memory[0x304010 + index] = value;
            channel.Setup(0, value, mpEntity);
            value = (*static_cast<unsigned char*>(arguments->pointers[4]) >> 4) & 7;
            memory[0x304011 + index] = value;
            channel.Setup(1, value, mpEntity);
            return 0;
        }
        case 0x5002: case 0x5003: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            memory[0x304010 + (index & 255)] = value;
            mExtraSquare0.Setup(index, value, mpEntity);
            memory[0x304025] |= memory[0x304013] >> 7;
            break;
        }
        case 0x5006: case 0x5007: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            memory[0x304010 + (index & 255)] = value;
            mExtraSquare1.Setup(index - 4, value, mpEntity);
            memory[0x304025] |= (memory[0x304025] >> 6) & 2;
            break;
        }
        case 0x5008: case 0x500a: case 0x500b: {
            unsigned char value = *static_cast<unsigned char*>(arguments->pointers[4]);
            memory[0x304010 + (index & 255)] = value;
            mExtraTriangle.Setup(index - 8, value, mpEntity);
            memory[0x304025] |= (memory[0x30401b] >> 5) & 4;
            break;
        }
        default: return 4;
        }
        mState48 = memory[0x304025];
    }
    return 0;
}
/** @brief Resets audio state, advances channels, and writes mixed samples to the output buffer. */
unsigned int CHvcSpu::Process(CPlatformUnit*, unsigned long, unsigned long, UArgument* arguments) {
    unsigned char* memory = mpEntity->pMemory;
    if ((1U << _18) & mpEntity->order.words[0]) {
        memory[0x303a25] = 0;
        memory[0x303825] = 0;
        memory[0x303a30] = 0;
        memory[0x303830] = 0;
        *reinterpret_cast<unsigned short*>(memory + 0x303a31) = 0;
        *reinterpret_cast<unsigned short*>(memory + 0x303831) = 0;
        memset(memory + 0x303810, 0, 20);
        memset(memory + 0x303a10, 0, 20);
        if (mpEntity->mapper == 24 || mpEntity->mapper == 5) {
            unsigned char* expansionRead = memory + 0x303c10;
            unsigned char* expansionWrite = memory + 0x304010;
            expansionWrite[0] = 0;
            expansionRead[0] = 0;
            *reinterpret_cast<unsigned short*>(memory + 0x304012) = 0;
            memory[0x303c12] = 0;
            *reinterpret_cast<unsigned short*>(memory + 0x303c13) = 0;
            memory[0x304014] = 0;
            memory[0x304016] = 0;
            memory[0x303c16] = 0;
            *reinterpret_cast<unsigned short*>(memory + 0x304017) = 0;
            *reinterpret_cast<unsigned short*>(memory + 0x303c17) = 0;
            *reinterpret_cast<unsigned short*>(memory + 0x304020) = 0;
            *reinterpret_cast<unsigned short*>(memory + 0x303c20) = 0;
            memory[0x304025] = 0;
            memory[0x303c25] = 0;
        }
        mPendingCycles = mSampleCycleRemainder = mPendingEvents = 0;
        mOutputLeft = mOutputRight = 0;
        mSquare0.Reset(0, 0);
        mSquare1.Reset(0, 1);
        mTriangle.Reset(0, 2);
        mNoise.Reset(0, 3);
        mDelta.Reset(0, 4);
        mDiskAudio.Reset(0, 5);
        mExtraSquare0.Reset(0, 6);
        mExtraSquare1.Reset(0, 7);
        mExtraTriangle.Reset(0, 8);
        if ((m_HVCEmu.mState7a & 0xfff0) == 0xf0f0) mDisk.Reset(mpEntity);
        _SEntity* entity = mpEntity;
        unsigned int mask = 1U << _18;
        entity->order.words[0] &= ~mask;
    }
    if ((m_HVCEmu.mState7a & 0xfff0) == 0xf0f0 && (arguments->values[0] & 16)) {
        if ((mpEntity->cycle.line & ~255UL) == 0xf000) mDisk.IncMediaCount();
    }
    mPendingEvents |= arguments->values[0];
    if ((m_HVCEmu.mState7a & 0xfff0) == 0xf0f0) mDisk.Pulse(arguments->values[1] >> 8, mPendingEvents, mpEntity);
    mDelta.mState26 = 0;
    mPendingCycles += arguments->values[1];
    while (mSampleCycleRemainder + mpEntity->cycle._a48 <= mPendingCycles) {
        short cycles = (mSampleCycleRemainder + mpEntity->cycle._a48) >> 8;
        memory[0x303825] = (memory[0x303825] & 0xe0) | mSquare0.mChannelMask | mSquare1.mChannelMask |
            mTriangle.mChannelMask | mNoise.mChannelMask | mDelta.mChannelMask;
        mSquare0.Pulse(cycles, mPendingEvents, mpEntity);
        mSquare1.Pulse(cycles, mPendingEvents, mpEntity);
        mTriangle.Pulse(cycles, mPendingEvents, mpEntity);
        mNoise.Pulse(cycles, mPendingEvents, mpEntity);
        mDelta.Pulse(cycles, mPendingEvents, mpEntity);
        mDiskAudio.Pulse(cycles, mPendingEvents, mpEntity);
        mPendingCycles -= cycles << 8;
        mSampleCycleRemainder += mpEntity->cycle._a48 - (cycles << 8);
        const signed char* custom = reinterpret_cast<const signed char*>(mpEntity->pCustom);
        int square0Product = custom[0x530] * mSquare0.mState04;
        long sample;
        if (custom[0x28] & 1) {
            int square0 = square0Product / 4;
            int square1 = custom[0x531] * mSquare1.mState04 / 4;
            int triangle = custom[0x532] * mTriangle.mState04 / 4;
            int noise = custom[0x533] * mNoise.mState04 / 4;
            int delta = custom[0x534] * mDelta.mState04 / 32;
            int mix = square0 + square1 - triangle - noise + delta;
            sample = static_cast<long>(mix) * 85 / 256;
        } else {
            int square0 = square0Product / 32;
            int square1 = custom[0x531] * mSquare1.mState04 / 32;
            int triangle = custom[0x532] * mTriangle.mState04 / 32;
            int noise = custom[0x533] * mNoise.mState04 / 32;
            int delta = custom[0x534] * mDelta.mState04 / 32;
            int disk = custom[0x535] * mDiskAudio.mState02 / 32;
            sample = -(square0 * 1016 / 1024 + square1 * 1016 / 1024 + triangle * 1143 / 1024
                + noise * 635 / 1024 + delta * 508 / 1024 + disk * 4);
        }
        mOutputLeft = mOutputRight = sample;
        mpEntity->pAudio[mpEntity->_1750] = sample;
        ++mpEntity->_1750;
        mPendingEvents = 0;
    }
    if (arguments->pointers[2]) *static_cast<unsigned long*>(arguments->pointers[2]) = 0;
    mDelta.mState26 = 0;
    return 0;
}
/** @brief Transfers SPU counters, optional disk state, and all sound channels. */
unsigned long CHvcSpu::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mState48);
    stream.Entry(mPendingCycles);
    stream.Entry(mSampleCycleRemainder);
    stream.Entry(mPendingEvents);
    stream.Entry(mOutputLeft);
    stream.Entry(mOutputRight);
    if ((m_HVCEmu.mState7a & 0xfff0) == 0xf0f0) {
        unsigned long start = stream.GetSize();
        unsigned long offset = mDisk.GetQdOffset();
        if (reading) {
            stream.Entry(offset);
            mDisk.SetQdOffset(offset);
        } else {
            offset = mDisk.GetQdOffset();
            stream.Entry(offset);
        }
        stream.Entry(mDisk.timer);
        stream.Entry(mDisk.status);
        stream.Entry(mDisk.dirty);
        stream.Entry(mDisk.timerStarted);
        stream.SetSize(start + 64);
    }
    mSquare0.Serialize(stream);
    mSquare1.Serialize(stream);
    mTriangle.Serialize(stream);
    mNoise.Serialize(stream);
    mDelta.Serialize(stream);
    mDiskAudio.Serialize(stream);
    mExtraSquare0.Serialize(stream);
    mExtraSquare1.Serialize(stream);
    mExtraTriangle.Serialize(stream);
    return stream.GetSize();
}
/** @brief Exports the SPU state. */
void CHvcSpu::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports the SPU state. */
void CHvcSpu::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Computes the aligned SPU state record size. */
unsigned long CHvcSpu::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 0x4f) & ~15U;
}
/** @brief Resets the square-wave channel counters and duty pattern. */
void CHvcSpu::_SWavwR::Reset(unsigned char type, unsigned char channel) {
    mType = type;
    mChannel = channel;
    mEnabled = 0;
    mChannelMask = 0;
    mState04 = 0;
    mState06 = 0;
    mState08 = 0;
    mState0a = 0;
    mState0b = 0;
    mState0c = 0;
    mState0e = 0;
    mPeriod = 0;
    mState12 = 1;
    mState13 = 0;
    mState14 = 1;
    mState16 = 0;
    mState18 = 0;
    mState19 = 0;
    mState1a = 0;
    mState1b = 0;
    mState1c = 0;
    mState1e = 0;
    mState20 = 0;
    mState21 = 0;
    mState22 = 0;
    mState23 = -1;
    mState24 = 0;
    mState26 = 0;
    mState28 = 0x3fc;
    mState30 = 0;
}
/** @brief Decodes square-wave envelope, sweep, timer, and enable writes. */
void CHvcSpu::_SWavwR::Setup(unsigned char index, unsigned char value, CPlatformHvcUnit::_SEntity*) {
    static const unsigned char lengthTable[32] = {
        5, 127, 10, 1, 20, 2, 40, 3, 80, 4, 30, 5, 7, 6, 13, 7,
        6, 8, 12, 9, 24, 10, 48, 11, 96, 12, 36, 13, 8, 14, 16, 15
    };
    static const unsigned short dutyTable[4] = { 12, 60, 1020, 65475 };
    switch (index) {
    case 0: {
        unsigned int duty = (value >> 6) & 3;
        unsigned long pattern = dutyTable[duty];
        unsigned int loop = (value >> 5) & 1;
        mState12 = loop ^ 1;
        mState18 = 1 & ~(value >> 4);
        mState28 = pattern;
        mState1e = value & 0xf;
        mState1b = loop;
        break;
    }
    case 1:
        mState20 = value >> 7;
        mState26 = ((value >> 4) & 7) + 1;
        mState22 = value & 7;
        mState23 = (value & 8) ? 1 : -1;
        mState21 = 1;
        break;
    case 2:
        mPeriod = (mPeriod & 0x700) | value;
        break;
    case 3:
        reinterpret_cast<unsigned char*>(&mPeriod)[1] = value & 7;
        mState16 = lengthTable[value >> 3] << 1;
        mState14 = mState16;
        if (mEnabled) {
            mState0a = 1;
            mState0b = 1;
        }
        mState13 = 1;
        mState19 = 1;
        mState21 = 1;
        mState30 = 0;
        mChannelMask = 1 << mChannel;
        break;
    case 0x10:
        if (static_cast<unsigned char>((1 << mChannel) & value)) {
            mEnabled = 1;
        } else {
            mState0a = 0;
            mState14 = mState16 = 0;
            mEnabled = mChannelMask = 0;
        }
        break;
    }
}
/** @brief Transfers square-wave channel state in its original field order. */
void CHvcSpu::_SWavwR::Serialize(Common::Serialize::LittleEndian& stream) {
    unsigned long start = stream.GetSize();
    stream.Entry(mType);
    stream.Entry(mChannel);
    stream.Entry(mEnabled);
    stream.Entry(mChannelMask);
    stream.Entry(mState04);
    stream.Entry(mState06);
    stream.Entry(mState08);
    stream.Entry(mState0a);
    stream.Entry(mState0b);
    stream.Entry(mState0c);
    stream.Entry(mState0e);
    stream.Entry(mPeriod);
    stream.Entry(mState12);
    stream.Entry(mState13);
    stream.Entry(mState14);
    stream.Entry(mState16);
    stream.Entry(mState18);
    stream.Entry(mState19);
    stream.Entry(reinterpret_cast<unsigned char&>(mState1a));
    stream.Entry(mState1b);
    stream.Entry(mState1c);
    stream.Entry(mState1e);
    stream.Entry(mState20);
    stream.Entry(mState21);
    stream.Entry(reinterpret_cast<unsigned char&>(mState22));
    stream.Entry(reinterpret_cast<unsigned char&>(mState23));
    stream.Entry(mState24);
    stream.Entry(mState26);
    stream.Entry(mState28);
    stream.Entry(mState30);
    stream.AlignBlock(start);
}
/** @brief Advances square-wave sweep, envelope, duty phase, and smoothed output. */
void CHvcSpu::_SWavwR::Pulse(short cycles, unsigned long events, CPlatformHvcUnit::_SEntity* entity) {
    static const short sweepLimits[8] = { 1023, 1365, 1638, 1820, 1927, 1985, 2016, 2032 };
    long enabled = mEnabled != 0;
    short period = mPeriod;
    short nextPeriod = period;
    if (entity->cycle._a58 & events) {
        if (mState12 && mState14 - 1 >= 0) --mState14;
        if (mState20 && mState26) {
            if (mState21) {
                mState24 = mState26;
                mState21 = 0;
            }
            if (mState24 > 0) --mState24;
            if (!mState24) {
                mState24 = mState26;
                if (mState22) {
                    if (mState23 > 0) nextPeriod = period - (period >> mState22) - (mChannel == 0);
                    else if (mState23 < 0) nextPeriod = period + (period >> mState22);
                }
            }
        }
    }
    if (entity->cycle._a60 & events) {
        if (mState19) {
            mState1c = mState1e;
            mState19 = 0;
            mState1a = 15;
        } else {
            if (mState1c - 1 >= 0) --mState1c;
            if (!mState1c) {
                mState1c = mState1e;
                if (mState1a - 1 >= 0) --mState1a;
                else if (!mState1a && mState1b) mState1a = 15;
            }
        }
    }
    const unsigned char* custom = reinterpret_cast<const unsigned char*>(entity->pCustom);
    short lower = *reinterpret_cast<const short*>(custom + 0x51e);
    const short* upper = mState23 > 0 ? reinterpret_cast<const short*>(custom + 0x520) : sweepLimits + mState22;
    long steps = 0;
    if (nextPeriod >= lower && nextPeriod <= *upper && period >= 4 && mState0a) {
        if (mState0b) {
            mState0e = period;
            mState0b = 0;
        } else {
            while (mState0e <= cycles) {
                mState0e += period;
                ++steps;
            }
            mState0e -= cycles;
        }
    }
    if (nextPeriod != period) {
        mPeriod = nextPeriod;
        mState0e = nextPeriod;
    }
    if (!mState14) {
        if (mState12) mChannelMask = 0;
        if (reinterpret_cast<const unsigned char*>(entity->pCustom)[0x54b + mChannel]) enabled = 0;
    }
    if (steps) {
        long volume = (mState18 ? mState1a : mState1e) << 8;
        long high = 0;
        long low = 0;
        while (steps > 0) {
            mState30 = (mState30 + 1) & 15;
            if (mState28 & (1U << mState30)) ++high;
            else ++low;
            --steps;
        }
        mState08 = high * volume / (low + high);
    }
    mState06 = mState08 * enabled;
    int shift = reinterpret_cast<signed char*>(entity->pCustom)[0x542 + mChannel];
    short current = mState04;
    int step = 1 << shift;
    if (current + step < mState06) mState04 += step;
    else if (mState06 + step < current) mState04 -= step;
    else mState04 = mState06;
}
/** @brief Resets the delta-sample channel and its sample address. */
void CHvcSpu::_SDelta::Reset(unsigned char type, unsigned char channel) {
    mType = type;
    mChannel = channel;
    mEnabled = 0;
    mChannelMask = 0;
    mState04 = 0;
    mState06 = 0;
    mState08 = 0;
    mState0a = 0;
    mState0b = 0;
    mState0c = 0;
    mState0e = 0;
    mPeriod = 0;
    mState12 = 0;
    mState13 = 0;
    mState14 = 0;
    mState16 = 0;
    mState18 = 0;
    mState1a = 0;
    mState1c = 0xc000;
    mState1e = 0xc000;
    mState20 = 0xaa;
    mState21 = 0;
    mState22 = 0;
    mState23 = 0;
    mState24 = 0;
    mState25 = 0;
    mState26 = 0;
}
/** @brief Transfers delta-sample state in the original field order. */
void CHvcSpu::_SDelta::Serialize(Common::Serialize::LittleEndian& stream) {
    unsigned long start = stream.GetSize();
    stream.Entry(mType);
    stream.Entry(mChannel);
    stream.Entry(mEnabled);
    stream.Entry(mChannelMask);
    stream.Entry(mState04);
    stream.Entry(mState06);
    stream.Entry(mState08);
    stream.Entry(mState0a);
    stream.Entry(mState0b);
    stream.Entry(mState0c);
    stream.Entry(mState0e);
    stream.Entry(mPeriod);
    stream.Entry(mState12);
    stream.Entry(mState13);
    stream.Entry(mState14);
    stream.Entry(mState16);
    stream.Entry(mState18);
    stream.Entry(mState1a);
    stream.Entry(mState1c);
    stream.Entry(mState1e);
    stream.Entry(mState20);
    stream.Entry(mState21);
    stream.Entry(mState22);
    stream.Entry(mState23);
    stream.Entry(mState24);
    stream.Entry(mState25);
    stream.Entry(mState27);
    stream.AlignBlock(start);
}
/** @brief Decodes delta-sample rate, output level, address, length, and enable writes. */
void CHvcSpu::_SDelta::Setup(unsigned char index, unsigned char value, CPlatformHvcUnit::_SEntity*) {
    static const unsigned short periodTable[16] = { 428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54 };
    switch (index) {
    case 0:
        mState23 = value >> 7;
        mState22 = (value >> 6) & 1;
        mState0a = mState0b = 1;
        mPeriod = periodTable[value & 15];
        if (!mState23) mState24 = 0;
        break;
    case 1:
        mState1a = value & 0x7f;
        mState18 = mState1a;
        break;
    case 2:
        mState1e = (value << 6) | 0xc000;
        mState1c = mState1e;
        break;
    case 3: {
        short length = (value << 7) | 8;
        mState16 = length;
        mState14 = length;
        mState12 = mState13 = 1;
        break;
    }
    case 0x10:
        if (static_cast<unsigned char>((1 << mChannel) & value)) {
            mChannelMask = 1 << mChannel;
            mEnabled = 1;
        } else {
            mState24 = 0;
            mEnabled = 0;
        }
        break;
    }
}
/** @brief Fetches and decodes delta samples, handles looping, and smooths the output. */
void CHvcSpu::_SDelta::Pulse(short cycles, unsigned long, CPlatformHvcUnit::_SEntity* entity) {
    long steps = 0;
    if (mState0a && mPeriod >= 4) {
        if (mState0b) {
            mState0e = mPeriod;
            mState0b = 0;
        } else {
            while (mState0e <= cycles) {
                mState0e += mPeriod;
                ++steps;
            }
            mState0e -= cycles;
        }
    }
    if (mState13 && mState12) {
        mState21 = 0;
        mState13 = 0;
    }
    int sample;
    if (steps) {
        long sum = 0;
        long count = 0;
        for (long i = steps; i > 0; --i) {
            unsigned char bit = mState21;
            if (bit < 8) {
                if (bit) goto decodeSample;
            } else {
                ++mState1c;
                if (mState1c >= 0) mState1c += 0x8000;
                mState21 = bit = 0;
            }
            {
                unsigned char nextFetchCount = mState26 + 1;
                unsigned long address = static_cast<unsigned short>(mState1c);
                unsigned char* source = entity->pMemory + entity->cpuBanks[address >> 12].readOffset;
                unsigned char sampleByte = source[address & 0xfff];
                mState26 = nextFetchCount;
                mState20 = sampleByte;
            }
        decodeSample:
            short remaining = mState14;
            if (remaining - 1 >= 0) {
                if (mEnabled) {
                    if (mState20 & (1 << bit)) {
                        if (mState18 <= 125) mState18 += 2;
                    } else {
                        if (mState18 - 2 >= 0) mState18 -= 2;
                    }
                    mState21 = bit + 1;
                    mState14 = --remaining;
                }
            }
            if (!remaining && mState12) {
                if (mState22) {
                    mState14 = mState16;
                    mState1c = mState1e;
                    mState12 = mState13 = 1;
                    mState18 = mState1a;
                } else {
                    mState12 = mState13 = 0;
                    mState24 = 1;
                    mChannelMask = 0;
                }
            }
            sum += mState18;
            ++count;
        }
        sample = sum / count;
    } else {
        sample = mState18;
    }
    mState08 = sample;
    mState06 = sample << 8;
    int shift = reinterpret_cast<signed char*>(entity->pCustom)[0x542 + mChannel];
    short current = mState04;
    int step = 1 << shift;
    if (current + step < mState06) mState04 += step;
    else if (mState06 + step < current) mState04 -= step;
    else mState04 = mState06;
}
/** @brief Resets noise-channel counters and feedback shift register. */
void CHvcSpu::_SNoise::Reset(unsigned char type, unsigned char channel) {
    mType = type;
    mChannel = channel;
    mEnabled = 0;
    mChannelMask = 0;
    mState04 = 0;
    mState06 = 0;
    mState08 = 0;
    mState0a = 0;
    mState0b = 0;
    mState0c = 0;
    mState0e = 0;
    mPeriod = 0;
    mState12 = 0;
    mState13 = 0;
    mState14 = 1;
    mState16 = 0;
    mState18 = 0;
    mState19 = 0;
    mState1a = 0;
    mState1b = 0;
    mState1c = 0;
    mState1e = 0;
    mState20 = 0;
    mState28 = 0;
    mState30 = 1;
    mState38 = 0;
    mState39 = 6;
}
/** @brief Decodes noise envelope, frequency, feedback mode, and enable writes. */
void CHvcSpu::_SNoise::Setup(unsigned char index, unsigned char value, CPlatformHvcUnit::_SEntity* entity) {
    static const unsigned char lengthTable[32] = {
        5, 127, 10, 1, 20, 2, 40, 3, 80, 4, 30, 5, 7, 6, 13, 7,
        6, 8, 12, 9, 24, 10, 48, 11, 96, 12, 36, 13, 8, 14, 16, 15
    };
    static const unsigned short periodTable[16] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };
    switch (index) {
    case 0: {
        unsigned int loop = (value >> 5) & 1;
        mState12 = loop ^ 1;
        mState18 = 1 & ~(value >> 4);
        mState1e = value & 0xf;
        mState1b = loop;
        break;
    }
    case 2: {
        unsigned char compatibility = reinterpret_cast<const unsigned char*>(entity->pCustom)[0x54b + mChannel];
        mPeriod = periodTable[value & 15];
        if (compatibility) {
            if (value & 0x80) {
                if (mState39 == 1) mState30 = 1;
                mState38 = 0;
                mState39 = 6;
            } else {
                if (mState39 == 6) mState30 = 1;
                mState38 = 0;
                mState39 = 1;
            }
        } else {
            mState38 = 0;
            mState39 = (value & 0x80) ? 6 : 3;
        }
        break;
    }
    case 3:
        mState16 = lengthTable[value >> 3] << 1;
        mState14 = mState16;
        if (mEnabled) {
            mState0a = 1;
            mState0b = 1;
            mChannelMask = 1 << mChannel;
        }
        mState13 = 1;
        mState19 = 1;
        mState28 = 0;
        break;
    case 0x10:
        if (static_cast<unsigned char>((1 << mChannel) & value)) {
            mEnabled = 1;
        } else {
            mState0a = 0;
            mState14 = mState16 = 0;
            mEnabled = mChannelMask = 0;
        }
        break;
    }
}
/** @brief Transfers noise-channel state in its original field order. */
void CHvcSpu::_SNoise::Serialize(Common::Serialize::LittleEndian& stream) {
    unsigned long start = stream.GetSize();
    stream.Entry(mType);
    stream.Entry(mChannel);
    stream.Entry(mEnabled);
    stream.Entry(mChannelMask);
    stream.Entry(mState04);
    stream.Entry(mState06);
    stream.Entry(mState08);
    stream.Entry(mState0a);
    stream.Entry(mState0b);
    stream.Entry(mState0c);
    stream.Entry(mState0e);
    stream.Entry(mPeriod);
    stream.Entry(mState12);
    stream.Entry(mState13);
    stream.Entry(mState14);
    stream.Entry(mState16);
    stream.Entry(mState18);
    stream.Entry(mState19);
    stream.Entry(reinterpret_cast<unsigned char&>(mState1a));
    stream.Entry(mState1b);
    stream.Entry(mState1c);
    stream.Entry(mState1e);
    stream.Entry(mState20);
    stream.Entry(mState28);
    stream.Entry(mState30);
    stream.Entry(mState38);
    stream.Entry(mState39);
    stream.AlignBlock(start);
}
/** @brief Advances the noise envelope and feedback generator, then smooths its output. */
void CHvcSpu::_SNoise::Pulse(short cycles, unsigned long events, CPlatformHvcUnit::_SEntity* entity) {
    int enabled = mEnabled != 0;
    if ((entity->cycle._a58 & events) && mState12) {
        if (mState14 - 1 >= 0) --mState14;
    }
    if (entity->cycle._a60 & events) {
        if (mState19) {
            mState1c = mState1e;
            mState19 = 0;
            mState1a = 15;
        } else {
            if (mState1c - 1 >= 0) --mState1c;
            if (!mState1c) {
                mState1c = mState1e;
                if (mState1a - 1 >= 0) --mState1a;
                else if (!mState1a && mState1b) mState1a = 15;
            }
        }
    }
    long steps = 0;
    if (mState0a && mPeriod >= 4) {
        if (mState0b) {
            mState0e = mPeriod;
            mState0b = 0;
        } else {
            while (mState0e <= cycles) {
                mState0e += mPeriod;
                ++steps;
            }
            mState0e -= cycles;
        }
    }
    if (!mState14) {
        mChannelMask = (mEnabled && !mState12) ? 1 << mChannel : 0;
        enabled = 0;
    }
    if (steps) {
        long volume = (mState18 ? mState1a : mState1e) << 8;
        long high = 0;
        long low = 0;
        if (reinterpret_cast<const unsigned char*>(entity->pCustom)[0x54b + mChannel]) {
            do {
                mState30 >>= 1;
                if (!mState30) mState30 = 1;
                unsigned int feedback = ((mState30 >> mState38) ^ (mState30 >> mState39));
                mState30 |= (feedback << 14) & 0x4000;
                high += mState30 & 1;
                low += (mState30 & 1) ^ 1;
            } while (--steps);
        } else {
            unsigned long shift = mState39 == 6 ? 15 : 17;
            do {
                unsigned int feedback = ((mState30 >> mState38) ^ (mState30 >> mState39));
                mState30 |= static_cast<unsigned long>(feedback & 1) << shift;
                mState30 >>= 1;
                if (!mState30) mState30 = 1;
                high += mState30 & 1;
                low += (mState30 & 1) ^ 1;
            } while (--steps);
        }
        mState08 = low * volume / (high + low);
    }
    mState06 = mState08 * enabled;
    int shift = reinterpret_cast<signed char*>(entity->pCustom)[0x542 + mChannel];
    short current = mState04;
    int step = 1 << shift;
    if (current + step < mState06) mState04 += step;
    else if (mState06 + step < current) mState04 -= step;
    else mState04 = mState06;
}
/** @brief Advances triangle counters and smooths the averaged waveform output. */
void CHvcSpu::_SWavwT::Pulse(short cycles, unsigned long events, CPlatformHvcUnit::_SEntity* entity) {
    const signed char* waveform = reinterpret_cast<const signed char*>("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x0f\x0e\x0d\x0c\x0b\x0a\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00");
    if ((entity->cycle._a58 & events) && mState12) {
        if (mState14 - 1 >= 0) --mState14;
    }
    if (entity->cycle._a60 & events) {
        if (mState19) {
            mState1a = mState1c;
            mState19 = 0;
        }
        if (mState18 && mState1a - 1 >= 0) --mState1a;
        if (!mState1c) mState1a = 0;
    }
    long steps = 0;
    if (mState0a && mPeriod >= 4) {
        if (mState0b) {
            mState0e = mPeriod;
            mState0b = 0;
        } else {
            while (mState0e <= cycles) {
                mState0e += mPeriod;
                ++steps;
            }
            mState0e -= cycles;
        }
    }
    if (mState12 && mState18) {
        if (!mState14 || !mState1a) {
            mChannelMask = 0;
            steps = 0;
        }
    } else if (!mState14 || !mState1a) {
        steps = 0;
    }
    int sample;
    if (steps) {
        long sum = 0;
        for (long i = 0; i < steps; ++i) {
            mState28 = (mState28 + 1) & 0x1f;
            sum += waveform[mState28];
        }
        sample = sum / steps;
    } else {
        sample = waveform[mState28 & 0x1f];
    }
    mState08 = sample;
    mState06 = sample << 8;
    int shift = reinterpret_cast<signed char*>(entity->pCustom)[0x542 + mChannel];
    short current = mState04;
    int step = 1 << shift;
    if (current + step < mState06)
        mState04 += step;
    else if (mState06 + step < current)
        mState04 -= step;
    else
        mState04 = mState06;
}
/** @brief Decodes triangle-channel control, timer, and enable writes. */
void CHvcSpu::_SWavwT::Setup(unsigned char index, unsigned char value, CPlatformHvcUnit::_SEntity*) {
    static const unsigned char lengthTable[32] = {
        5, 127, 10, 1, 20, 2, 40, 3, 80, 4, 30, 5, 7, 6, 13, 7,
        6, 8, 12, 9, 24, 10, 48, 11, 96, 12, 36, 13, 8, 14, 16, 15
    };
    switch (index) {
    case 0:
        mState18 = (~value >> 7) & 1;
        mState12 = mState18;
        mState1c = value & 0x7f;
        break;
    case 2:
        mPeriod = (mPeriod & 0x700) | value;
        break;
    case 3:
        reinterpret_cast<unsigned char*>(&mPeriod)[1] = value & 7;
        mState16 = lengthTable[value >> 3] << 1;
        mState14 = mState16;
        if (mEnabled) {
            mState0a = 1;
            mState0b = 1;
        }
        mState13 = 1;
        mState19 = 1;
        mChannelMask = 1 << mChannel;
        break;
    case 0x10:
        if (static_cast<unsigned char>((1 << mChannel) & value)) {
            mEnabled = 1;
        } else {
            mState0a = 0;
            mState14 = mState16 = 0;
            mState1a = mState1c = 0;
            mEnabled = mChannelMask = 0;
        }
        break;
    }
}
/** @brief Resets triangle-channel state and identifies its output channel. */
void CHvcSpu::_SWavwT::Reset(unsigned char type, unsigned char channel) {
    mType = type;
    mChannel = channel;
    mEnabled = 0;
    mChannelMask = 0;
    mState04 = 0;
    mState06 = 0;
    mState08 = 0;
    mState0a = 0;
    mState0b = 0;
    mState0c = 0;
    mState0e = 0;
    mPeriod = 0;
    mState12 = 0;
    mState13 = 0;
    mState14 = 1;
    mState16 = 0;
    mState18 = 0;
    mState19 = 0;
    mState1a = 0;
    mState1c = 0;
    mState20 = 0;
    mState28 = 0;
}
/** @brief Transfers triangle-channel state in its original field order. */
void CHvcSpu::_SWavwT::Serialize(Common::Serialize::LittleEndian& stream) {
    unsigned long start = stream.GetSize();
    stream.Entry(mType);
    stream.Entry(mChannel);
    stream.Entry(mEnabled);
    stream.Entry(mChannelMask);
    stream.Entry(mState04);
    stream.Entry(mState06);
    stream.Entry(mState08);
    stream.Entry(mState0a);
    stream.Entry(mState0b);
    stream.Entry(mState0c);
    stream.Entry(mState0e);
    stream.Entry(mPeriod);
    stream.Entry(mState12);
    stream.Entry(mState13);
    stream.Entry(mState14);
    stream.Entry(mState16);
    stream.Entry(mState18);
    stream.Entry(mState19);
    stream.Entry(mState1a);
    stream.Entry(mState1c);
    stream.Entry(mState20);
    stream.Entry(mState28);
    stream.AlignBlock(start);
}
/** @brief Resets disk audio waveform, modulation, and envelope state. */
void CHvcSpu::_SQdMfm::Reset(unsigned char type, unsigned char channel) {
    mType = type;
    mChannel = channel;
    mState02 = 0;
    mState04 = 0;
    mState05 = 0;
    mState06 = 0xe8;
    mState08 = 0;
    mState09 = 0;
    mState10 = 0;
    mState18 = 0;
    for (unsigned long i = 0; i < 64; ++i) mWave[i] = 0;
    mState60 = 0;
    mState61 = 0;
    mState62 = 0;
    mState68 = 0;
    mState70 = 0;
    mState71 = 0;
    mState78 = 0;
    mState80 = 0;
    mState88 = 0;
    mState90 = -1.0;
    mState98 = 0;
    mState99 = 0;
    mState9a = 0;
    for (unsigned long i = 0; i < 64; ++i) mModulation[i] = 0;
    mStatedb = 0xff;
    mStatee0 = 0;
    mStatee1 = 0;
    mStatee8 = 0;
    mStatef0 = 0;
    mStatef1 = 0;
    mStatef8 = 0;
}
/** @brief Decodes disk waveform, modulation, and envelope control writes. */
void CHvcSpu::_SQdMfm::Setup(unsigned char index, unsigned char value, CPlatformHvcUnit::_SEntity*) {
    static const unsigned char modulation[8] = { 0, 1, 2, 4, 127, 252, 254, 255 };
    switch (index) {
    case 0x23:
        mState04 = (value >> 1) & 1;
        break;
    case 0x80:
        if (value & 0x80) {
            mState70 = 0;
            mState60 = value & 0x3f;
        } else {
            mState70 = 1;
            mState71 = (value >> 6) & 1;
            mState62 = value & 0x3f;
            mState78 = (mState62 + 1UL) * mState06 * 7623676UL / 3712;
            mState68 = 0;
        }
        break;
    case 0x82:
        reinterpret_cast<unsigned char*>(&mState10)[0] = value;
        break;
    case 0x83: {
        mState10 = (mState10 & 0xf0ff) | ((value & 15) << 8);
        unsigned char previous = mState08;
        mState08 = (~value >> 7) & 1;
        if (!previous && mState04) {
            mState18 = 0.0;
            mState61 = mState60;
        }
        mState05 = (value >> 6) & 1;
        break;
    }
    case 0x84:
        if (value & 0x80) {
            mStatef0 = 0;
            mStatee0 = value & 0x3f;
        } else {
            mStatef0 = 1;
            mStatef1 = (value >> 6) & 1;
            mStatee1 = value & 0x3f;
            mStatef8 = (mStatee1 + 1UL) * mState06 * 7623676UL / 3712;
            mStatee8 = 0;
        }
        break;
    case 0x85:
        mState98 = (value & 0x40) ? value | 0xc0 : value & 0x3f;
        mState90 = -1.0;
        mStatedb = 0xff;
        break;
    case 0x86:
        reinterpret_cast<unsigned char*>(&mState88)[0] = value;
        break;
    case 0x87:
        mState88 = (mState88 & 0xf0ff) | ((value & 15) << 8);
        mState9a = value >> 7;
        break;
    case 0x88:
        if (mState9a) {
            unsigned char sample = modulation[value & 7];
            mModulation[mState99] = sample;
            mModulation[static_cast<unsigned char>(mState99 + 1)] = sample;
            mState99 = (mState99 + 2) & 0x3f;
        }
        break;
    case 0x89:
        mState09 = (~value >> 7) & 1;
        mState80 = value & 3;
        break;
    case 0x8a:
        mState78 = (mState62 + 1UL) * value * 7623676UL / 3712;
        mStatef8 = (mStatee1 + 1UL) * value * 7623676UL / 3712;
        mState06 = value;
        break;
    default:
        if (index >= 0x40 && index < 0x80 && !mState09) mWave[index - 0x40] = value & 0x3f;
        break;
    }
}
/** @brief Advances disk audio envelopes and modulation, then produces a smoothed waveform sample. */
void CHvcSpu::_SQdMfm::Pulse(short cycles, unsigned long, CPlatformHvcUnit::_SEntity* entity) {
    unsigned long enabled = (mState04 && mState08) ? mState09 : 0;
    if (!mState05) {
        if (mState70 && mState06) {
            unsigned long period = static_cast<unsigned int>(mState78);
            while (mState68 >= period) {
                if (mState71) {
                    if (mState60 < 32) ++mState60;
                } else if (mState60) {
                    --mState60;
                }
                mState68 -= period;
            }
            mState68 += cycles << 8;
        }
        if (mStatef0 && mState06) {
            unsigned long period = static_cast<unsigned int>(mStatef8);
            while (mStatee8 >= period) {
                if (mStatef1) {
                    if (mStatee0 < 32) ++mStatee0;
                } else if (mStatee0) {
                    --mStatee0;
                }
                mStatee8 -= period;
            }
            mStatee8 += cycles << 8;
        }
    }
    long depth = mStatee0 < 32 ? mStatee0 : 32;
    unsigned char modulation;
    if (!mState9a && mState88) {
        long phase = static_cast<long>(mState90);
        modulation = mState98;
        if (phase >= 0) {
            int result = modulation;
            while (static_cast<signed char>(mStatedb) != phase) {
                signed char next = mStatedb + 1;
                if (next > 63) next = 0;
                mStatedb = next;
                unsigned char value = mModulation[(mState99 + mStatedb) & 63];
                result += value;
                if (value == 127) result = 0;
                if (static_cast<signed char>(result + 64) < 0) result = ((result + 64) & 127) - 64;
                mState98 = result;
            }
        }
        mState90 += cycles * 1.5284183990330473e-05 * static_cast<unsigned short>(mState88);
        if (mState90 >= 64.0) mState90 -= 64.0;
    } else {
        modulation = mState98;
    }
    long adjustment = static_cast<signed char>(modulation) * depth;
    if (adjustment < 0) adjustment -= 8;
    else if (adjustment > 0) adjustment += 8;
    if (adjustment >= 3072) adjustment -= 4096;
    else if (adjustment < -1024) adjustment += 4096;
    long sample = 0;
    if (mState10) {
        double phase = mState18;
        long pitch = adjustment + 1024;
        long waveformIndex = static_cast<long>(phase);
        long volume = mState61 < 32 ? mState61 : 32;
        sample = static_cast<signed char>(mWave[waveformIndex]) * volume;
        mState18 += cycles * 1.4925960928057103e-08 * static_cast<unsigned short>(mState10) * pitch;
        if (mState18 >= 64.0) {
            mState61 = mState60;
            mState18 -= 64.0;
        }
    }
    switch (mState80) {
    case 1: sample = sample * 2 / 3; break;
    case 2: sample >>= 1; break;
    case 3: sample = sample * 2 / 5; break;
    }
    int shift = reinterpret_cast<signed char*>(entity->pCustom)[0x542 + mChannel];
    short target = sample * (enabled & 1);
    short current = mState02;
    int step = 1 << shift;
    if (current + step < target) mState02 += step;
    else if (target + step < current) mState02 -= step;
    else mState02 = target;
}
/** @brief Transfers disk audio state and both waveform tables. */
void CHvcSpu::_SQdMfm::Serialize(Common::Serialize::LittleEndian& stream) {
    unsigned long start = stream.GetSize();
    stream.Entry(mType);
    stream.Entry(mChannel);
    stream.Entry(mState02);
    stream.Entry(mState04);
    stream.Entry(mState05);
    stream.Entry(mState06);
    stream.Entry(mState08);
    stream.Entry(mState09);
    stream.Entry(mState10);
    stream.Entry(mState18);
    for (unsigned long i = 0; i < 64; ++i) stream.Entry(mWave[i]);
    stream.Entry(mState60);
    stream.Entry(mState61);
    stream.Entry(mState62);
    stream.Entry(mState70);
    stream.Entry(mState71);
    stream.Entry(mState68);
    stream.Entry(mState78);
    stream.Entry(mState80);
    stream.Entry(mState88);
    stream.Entry(mState90);
    stream.Entry(mState98);
    stream.Entry(mState99);
    stream.Entry(mState9a);
    for (unsigned long i = 0; i < 64; ++i) stream.Entry(mModulation[i]);
    stream.Entry(mStatedb);
    stream.Entry(mStatee0);
    stream.Entry(mStatee1);
    stream.Entry(mStatef0);
    stream.Entry(mStatef1);
    stream.Entry(mStatee8);
    stream.Entry(mStatef8);
    stream.AlignBlock(start);
}
/** @brief Resets disk position, timer, and mapped drive status. */
void CHvcSpu::_SQdDisk::Reset(CPlatformHvcUnit::_SEntity* entity) {
    unsigned char* memory = entity->pMemory;
    offset = 0;
    changed = 0;
    timer = 0;
    status = 0x47;
    mediaCount = 120;
    timerStarted = 0;
    *reinterpret_cast<unsigned short*>(memory + 0x303842) = 0x8047;
}
/** @brief Updates disk data reads and advances the programmable interrupt timer. */
void CHvcSpu::_SQdDisk::Pulse(short cycles, unsigned long, CPlatformHvcUnit::_SEntity* entity) {
    unsigned char* memory = entity->pMemory;
    unsigned char control = memory[0x303a32];
    if (changed) {
        signed char diskControl = memory[0x303a35];
        if (diskControl & 4) memory[0x303841] = memory[offset + 0x100000];
        if (diskControl < 0) {
            entity->order.words[4] |= 1;
            changed = 0;
        }
    }
    if (!timerStarted) return;
    for (int i = 0; i < cycles; ++i) {
        if (!timer) {
            if (control & 1) SetTimer(entity);
            else {
                timerStarted = 0;
                break;
            }
        }
        --timer;
        if (!timer && (control & 2)) {
            memory[0x303840] |= 1;
            entity->order.words[4] |= 1;
        }
    }
}
/** @brief Copies the disk timer reload value from mapped registers. */
void CHvcSpu::_SQdDisk::SetTimer(CPlatformHvcUnit::_SEntity* entity) {
    timer = *reinterpret_cast<unsigned short*>(entity->pMemory + 0x303a30);
}
/** @brief Returns the current disk-image offset. */
unsigned long CHvcSpu::_SQdDisk::GetQdOffset() { return offset; }
/** @brief Updates the disk-image offset and records the change. */
void CHvcSpu::_SQdDisk::SetQdOffset(unsigned long value) {
    offset = value;
    changed = 1;
}
/** @brief Replaces the low half of the disk-image offset. */
void CHvcSpu::_SQdDisk::SetQdOffsetHalf(unsigned long value) {
    unsigned long next = (offset & 0xffff0000) | (value & 0xffff);
    changed = 1;
    offset = next;
}
/** @brief Advances the low half of the disk-image offset with wrapping. */
void CHvcSpu::_SQdDisk::IncQdOffsetHalf() {
    unsigned long next = (offset & 0xffff0000) | ((offset + 1) & 0xffff);
    changed = 1;
    offset = next;
}
/** @brief Reloads and starts the disk timer. */
void CHvcSpu::_SQdDisk::SetTimerStart(CPlatformHvcUnit::_SEntity* entity) {
    timer = *reinterpret_cast<unsigned short*>(entity->pMemory + 0x303a30);
    timerStarted = 1;
}
/** @brief Records whether the disk image has been modified. */
void CHvcSpu::_SQdDisk::SetQdDirty(bool value) { dirty = value; }
/** @brief Advances the media transition counter when platform state permits. */
void CHvcSpu::_SQdDisk::IncMediaCount() {
    if (mediaCount >= 120 && mediaCount < 196) {
        if (reinterpret_cast<unsigned char*>(GetCHvc())[0x3d8ad8] & 9)
            mediaCount = 196;
    } else {
        ++mediaCount;
    }
}
} } } }
