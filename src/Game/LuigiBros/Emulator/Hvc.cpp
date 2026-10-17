#include <cstring>
#include "LuigiBros/Emulator/Hvc.hpp"
#include "LuigiBros/Emulator/HvcPatch.hpp"
#include "LuigiBros/Common/Serialize.hpp"
#include "LuigiBros/Emulator/inputdevice.hpp"
#include "LuigiBros/Vessel/Debug.hpp"

extern unsigned char g_nDiskSideMax;

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
/** @brief Measures selected device states and fixed memory content blocks. */
unsigned long CHvc::ContentSize(unsigned long mask) {
    unsigned long i = 0;
    unsigned long total = 0;
    for (; i < 25; ++i) {
        unsigned long size = 0;
        if (mask & static_cast<long>(1 << i)) {
            if (i == 8) {
                size = (static_cast<unsigned long>(g_nDiskSideMax) << 14) + 16;
            } else if (i == 24) {
                size = 0x8010;
            } else if (i >= 6 && i <= 24) {
                size = (static_cast<unsigned int>(c_aSerializeInformationTable[i].firstSize) +
                        static_cast<unsigned int>(c_aSerializeInformationTable[i].secondSize) + 31) & ~15U;
            } else {
                mUnits[c_aSerializeInformationTable[i].unit]->InferContentSize(size);
            }
            total += size;
        }
    }
    return total;
}
/** @brief Dumps HVC memory regions when the controller debug flag is enabled. */
void CHvc::Debug(unsigned long mode) {
    if (mode || !(reinterpret_cast<unsigned char*>(&m_inputdevice_controller)[0x82] & 8)) return;
    Vessel::DebugDumpMemory(mEntity.pMemory + 0x300000, 0x800, 16, reinterpret_cast<const unsigned char*>("WRam"));
    Vessel::DebugDumpMemory(mEntity.pMemory + 0x300800, 0x2000, 16, reinterpret_cast<const unsigned char*>("SRam"));
    Vessel::DebugDumpMemory(mEntity.pMemory + 0x304410, 0x100, 16, reinterpret_cast<const unsigned char*>("Oam "));
    Vessel::DebugDumpMemory(mEntity.pMemory + 0x304510, 0x20, 16, reinterpret_cast<const unsigned char*>("Cgm "));
    Vessel::DebugDumpMemory(mEntity.pMemory + 0x302800, 0x800, 32, reinterpret_cast<const unsigned char*>("VRam"));
}
/** @brief Resolves the mapped device and forwards a bus access. */
unsigned int CHvc::Access(CPlatformUnit* sender, unsigned long category,
                         unsigned long operation, UArgument* arguments) {
    unsigned long address = arguments->values[2];
    CPlatformUnit* mapper = mUnits[1];
    unsigned char unit;
    UArgument query;
    query.values[0] = operation;
    query.values[2] = address;
    query.pointers[4] = &unit;
    mapper->System(sender, 0, 2, &query);
    return mUnits[unit]->Access(sender, category, operation, arguments);
}
/** @brief Advances the attached devices and carries CPU timing between iterations. */
unsigned int CHvc::Process(CPlatformUnit*, unsigned long category,
                          unsigned long operation, UArgument* arguments) {
    UArgument gio, mapper, cpu, video, audio;
    unsigned long input[12];
    if (arguments && arguments->pointers[4]) {
        unsigned char* source = static_cast<unsigned char*>(arguments->pointers[4]);
        input[0] = source[0];
        input[1] = source[8];
        input[2] = source[16];
        input[3] = source[24];
        input[7] = *reinterpret_cast<unsigned long*>(source + 32);
        input[8] = source[40];
    }
    unsigned short& cycleRemainder = *reinterpret_cast<unsigned short*>(mEntity._408 + 0x800);
    unsigned short& cpuDelay = *reinterpret_cast<unsigned short*>(mEntity._408 + 0x802);
    unsigned long events;
    do {
        unsigned long stepCycles = 0, inputCycles = 0, mapperCycles = 0;
        unsigned long cpuCycles = 0, videoCycles = 0, audioCycles = 0;
        if (cpuDelay >= 4) {
            cpuCycles = stepCycles = 0x400;
        } else {
            unsigned int unitId = _18;
            unsigned short pending = mEntity.order.words[2];
            unsigned int mask = 1U << unitId;
            if (pending & mask) mEntity.order.words[2] = (pending | 8) & ~mask;
            if (mEntity.order.words[4] & mask) mEntity.order.words[4] = (mEntity.order.words[4] | 8) & ~mask;
            if (mEntity.order.words[6] & mask) mEntity.order.words[6] = (mEntity.order.words[6] | 8) & ~mask;
            CPlatformUnit* cpuUnit = mUnits[3];
            cpu.values[0] = 0;
            cpu.values[1] = 0;
            cpu.pointers[2] = &stepCycles;
            cpu.pointers[4] = nullptr;
            cpuUnit->Process(this, category, 3, &cpu);
        }
        events = mEntity.cycle.Progress(stepCycles + cycleRemainder);
        CPlatformUnit* mapperUnit = mUnits[1];
        mapper.values[0] = events;
        mapper.values[1] = stepCycles + cycleRemainder;
        mapper.pointers[2] = &mapperCycles;
        mapper.pointers[4] = nullptr;
        mapperUnit->Process(this, category, operation, &mapper);
        CPlatformUnit* gioUnit = mUnits[2];
        gio.values[0] = events;
        gio.values[1] = stepCycles + cycleRemainder;
        gio.pointers[2] = &inputCycles;
        gio.pointers[4] = input;
        gioUnit->Process(this, category, operation, &gio);
        CPlatformUnit* videoUnit = mUnits[4];
        video.values[0] = events;
        video.values[1] = stepCycles + cycleRemainder;
        video.pointers[2] = &videoCycles;
        video.pointers[4] = nullptr;
        videoUnit->Process(this, category, operation, &video);
        CPlatformUnit* audioUnit = mUnits[5];
        audio.values[0] = events;
        audio.values[1] = stepCycles + cycleRemainder;
        audio.pointers[2] = &audioCycles;
        audio.pointers[4] = nullptr;
        audioUnit->Process(this, category, operation, &audio);
        unsigned int remainingDelay;
        if (stepCycles > (static_cast<unsigned long>(cpuDelay) << 8)) {
            CPlatformUnit* cpuUnit = mUnits[3];
            cpu.values[0] = events;
            cpu.values[1] = stepCycles;
            cpu.pointers[2] = &cpuCycles;
            cpu.pointers[4] = nullptr;
            cpuUnit->Process(this, category, operation, &cpu);
            remainingDelay = 0;
        } else {
            remainingDelay = cpuDelay - (stepCycles >> 8);
        }
        cpuDelay = remainingDelay + ((static_cast<unsigned int>(videoCycles) + static_cast<unsigned int>(audioCycles)) >> 8);
        cycleRemainder = cpuCycles - stepCycles;
        if (operation - 1 > 1) break;
    } while (!(events & operation));
    return 0;
}
/** @brief Retains the original unused audio extraction entry point. */
void CHvc::ExtractAudioSignal(short*, unsigned long) {}
/** @brief Returns the generated audio buffer and consumes its sample count. */
void CHvc::TakeAudioSample(short*& output, unsigned long& count) {
    count = mEntity._1750;
    output = mEntity.pAudio;
    mEntity._1750 = 0;
}
/** @brief Serializes bank mappings, cycle counters, and pending platform orders. */
unsigned long CHvc::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mEntity.mapper);
    stream.Entry(mEntity.flags);
    stream.Entry(mEntity.prgBankCount);
    stream.Entry(mEntity.chrBankCount);
    for (unsigned long i = 0; i < 16; ++i) {
        stream.Entry(mEntity.cpuBanks[i].readType);
        stream.Entry(mEntity.cpuBanks[i]._01);
        stream.Entry(mEntity.cpuBanks[i].readMask);
        stream.Entry(mEntity.cpuBanks[i].readOffset);
        stream.Entry(mEntity.cpuBanks[i].writeType);
        stream.Entry(mEntity.cpuBanks[i]._11);
        stream.Entry(mEntity.cpuBanks[i].writeMask);
        stream.Entry(mEntity.cpuBanks[i].writeOffset);
    }
    for (unsigned long i = 0; i < 16; ++i) {
        stream.Entry(mEntity.ppuBanks[i].readType);
        stream.Entry(mEntity.ppuBanks[i]._01);
        stream.Entry(mEntity.ppuBanks[i].readMask);
        stream.Entry(mEntity.ppuBanks[i].readOffset);
        stream.Entry(mEntity.ppuBanks[i].writeType);
        stream.Entry(mEntity.ppuBanks[i]._11);
        stream.Entry(mEntity.ppuBanks[i].writeMask);
        stream.Entry(mEntity.ppuBanks[i].writeOffset);
    }
    for (unsigned long i = 0; i < 64; ++i) {
        stream.Entry(mEntity.chrHistory[i].readType);
        stream.Entry(mEntity.chrHistory[i]._01);
        stream.Entry(mEntity.chrHistory[i].readMask);
        stream.Entry(mEntity.chrHistory[i].readOffset);
        stream.Entry(mEntity.chrHistory[i].writeType);
        stream.Entry(mEntity.chrHistory[i]._11);
        stream.Entry(mEntity.chrHistory[i].writeMask);
        stream.Entry(mEntity.chrHistory[i].writeOffset);
    }
    stream.Entry(*reinterpret_cast<unsigned short*>(mEntity._408 + 0x800));
    stream.Entry(*reinterpret_cast<unsigned short*>(mEntity._408 + 0x802));
    stream.Entry(mEntity.cycle.lineCycle);
    stream.Entry(mEntity.cycle.frameCycle);
    stream.Entry(mEntity.cycle.line);
    stream.Entry(mEntity.cycle.frame);
    stream.Entry(mEntity.cycle.sequenceMode);
    stream.Entry(mEntity.cycle.sequenceIndex);
    stream.Entry(mEntity.cycle.sequenceCycle);
    stream.Entry(reinterpret_cast<long&>(mEntity.cycle._38));
    stream.Entry(mEntity.order.words[0]);
    stream.Entry(mEntity.order.words[1]);
    stream.Entry(mEntity.order.words[2]);
    stream.Entry(mEntity.order.words[3]);
    stream.Entry(mEntity.order.words[4]);
    stream.Entry(mEntity.order.words[5]);
    stream.Entry(mEntity.order.words[6]);
    stream.Entry(mEntity.order.words[7]);
    return stream.GetSize();
}
/** @brief Exports the HVC platform state. */
void CHvc::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports the HVC platform state. */
void CHvc::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Includes the state header and rounds the serialized size to sixteen bytes. */
unsigned long CHvc::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 0x2f) & ~15U;
}
unsigned char CHvc::_SerializeTagContent() { return 0; }
unsigned char CHvc::_SerializeTagSpecies() { return 0; }
unsigned short CHvc::_SerializeTagVersion() { return 0x100; }
const unsigned char* CHvc::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("Hvc    ");
}
inline __attribute__((used)) void CHvcPatch::_PatchDefault() {}
inline __attribute__((used)) void CHvcPatch::_PatchAttach_Mother() {}
/** @brief Clears the palette patch state when the title starts. */
inline __attribute__((used)) void CHvcPatch::_PatchPostCommence_ParthenaNoKagami() {
    CHvc* hvc = mpHvc;
    hvc->mPatchState[4] = 0;
    hvc->mPatchState[3] = 0;
    hvc->mPatchState[2] = 0;
    hvc->mPatchState[1] = 0;
    hvc->mPatchState[0] = 0;
}
/** @brief Saves the current background palette before emulation advances. */
inline __attribute__((used)) void CHvcPatch::_PatchAnteProgress_ParthenaNoKagami() {
    CHvc* hvc = mpHvc;
    unsigned char* memory = hvc->mEntity.pMemory;
    unsigned long first = *reinterpret_cast<unsigned long*>(memory + 0x304510);
    unsigned long second = *reinterpret_cast<unsigned long*>(memory + 0x304518);
    hvc->mPatchState[1] = second;
    hvc->mPatchState[0] = first;
}
/** @brief Adjusts three scanline thresholds for the selected Super Mario 3 title. */
inline __attribute__((used)) void CHvcPatch::_PatchPostProgress_SuperMario3() {
    if (mpHvc->mState17e8 == 0xd1) {
        unsigned long threshold = mpHvc->mEntity.cycle.linePeriod - mpHvc->mEntity.cycle.lineBlank + 0x1400;
        mpHvc->mEntity.cycle.lineThresholds[195] = threshold;
        mpHvc->mEntity.cycle.lineThresholds[194] = threshold;
        mpHvc->mEntity.cycle.lineThresholds[193] = threshold;
    }
}
/** @brief Saves all palette entries before the Link no Bouken patch advances. */
inline __attribute__((used)) void CHvcPatch::_PatchAnteProgress_LinkNoBouken() {
    CHvc* hvc = mpHvc;
    unsigned char* memory = hvc->mEntity.pMemory;
    unsigned long first = *reinterpret_cast<unsigned long*>(memory + 0x304510);
    unsigned long second = *reinterpret_cast<unsigned long*>(memory + 0x304518);
    unsigned long third = *reinterpret_cast<unsigned long*>(memory + 0x304520);
    unsigned long fourth = *reinterpret_cast<unsigned long*>(memory + 0x304528);
    hvc->mPatchState[3] = fourth;
    hvc->mPatchState[2] = third;
    hvc->mPatchState[1] = second;
    hvc->mPatchState[0] = first;
}
/** @brief Installs the original Metroid program patch. */
void CHvcPatch::_PatchAttach_Metroid() {
    static const unsigned char patch[] = {
        0xa4,0x98,0xc0,0x04,0xd0,0x11,0xa4,0x99,0xf0,0x15,0xc0,0x06,0xf0,0x15,0x88,0xf0,
        0x0a,0x29,0x10,0xf0,0x05,0xa9,0x02,0x29,0x02,0x4a,0x60,0xc9,0x09,0xb0,0xf6,0xc9,
        0x1e,0xb0,0xf2,0xa9,0x00,0x60
    };
    memcpy(mpHvc->mEntity.pMemory + 0xb25, patch, sizeof(patch));
}
/** @brief Adjusts Kirby scanline thresholds for the selected regional title. */
void CHvcPatch::_PatchAttach_Kirby() {
    static const unsigned long offsets[5] = {2560,3072,2560,3072,2560};
    unsigned long index = mpHvc->mState17e8 - 13;
    if (index <= 4) {
        unsigned long blank = mpHvc->mEntity.cycle.lineBlank;
        unsigned long offset = offsets[index];
        unsigned long threshold = mpHvc->mEntity.cycle.linePeriod - blank + offset;
        mpHvc->mEntity.cycle.lineThresholds[177] = threshold;
        mpHvc->mEntity.cycle.lineThresholds[176] = threshold;
        mpHvc->mEntity.cycle.lineThresholds[175] = threshold;
        mpHvc->mEntity.cycle.lineThresholds[174] = threshold;
    }
}
/** @brief Applies the title's custom audio parameters. */
inline __attribute__((used)) void CHvcPatch::_PatchAttach_YoshiNoCookie() {
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d28] = 0x1c;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d43] = 0;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d29] = 0x20;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d44] = 0;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d2a] = 0x18;
}
/** @brief Installs the original Wrecking Crew save/load program patch. */
inline __attribute__((used)) void CHvcPatch::_PatchAttach_WreckingCrew() {
    static const unsigned char asm_code[112] = {
        0x48, 0x8a, 0x48, 0x98, 0x48, 0x20, 0xd0, 0xff, 0xa2, 0x0, 0xbd, 0x0, 0x61, 0x9d, 0x0, 0x6,
        0xe8, 0xd0, 0xf7, 0x20, 0xe0, 0xff, 0x20, 0xd8, 0xff, 0x68, 0xa8, 0x68, 0xaa, 0x68, 0x60, 0xea,
        0x48, 0x8a, 0x48, 0x98, 0x48, 0x20, 0xd0, 0xff, 0xa2, 0x0, 0xbd, 0x0, 0x6, 0x9d, 0x0, 0x61,
        0xe8, 0xd0, 0xf7, 0x20, 0xe0, 0xff, 0x20, 0xd8, 0xff, 0x68, 0xa8, 0x68, 0xaa, 0x68, 0x60, 0xea,
        0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea, 0xea,
        0xa5, 0x9, 0x29, 0x7f, 0x8d, 0x0, 0x20, 0x60, 0xa5, 0x9, 0x8d, 0x0, 0x20, 0x60, 0xea, 0xea,
        0xa2, 0xff, 0xa0, 0xff, 0xea, 0xea, 0xea, 0x88, 0xd0, 0xfa, 0xca, 0xd0, 0xf5, 0x60, 0xea, 0xea,
    };
    unsigned char* memory = mpHvc->mEntity.pMemory;
    memcpy(memory + 0x7f80, asm_code, sizeof(asm_code));
    *reinterpret_cast<unsigned short*>(memory + 0x3844) = 0xff80;
    *reinterpret_cast<unsigned short*>(memory + 0x387a) = 0xffa0;
}
/** @brief Resets non-Japanese Link no Bouken patch history and its previous index. */
inline __attribute__((used)) void CHvcPatch::_PatchAttach_LinkNoBouken_NonJP() {
    CHvc* hvc = mpHvc;
    memset(reinterpret_cast<unsigned char*>(hvc) + 0x21a8, 0, 0x2e8);
    *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21c8) = -1;
}
/** @brief Enables the original Punch-Out title-specific flags. */
inline __attribute__((used)) void CHvcPatch::_PatchAttach_PunchOut() {
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31c4] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31c3] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31c2] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31c1] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31c0] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31bf] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31be] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31bd] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31bc] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31bb] = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x3d31ba] = 1;
    *reinterpret_cast<unsigned long*>(reinterpret_cast<unsigned char*>(mpHvc) + 0x3d31e0) = 14;
}
/** @brief Installs the Japanese Link no Bouken patch and resets its history. */
void CHvcPatch::_PatchAttach_LinkNoBouken() {
    static const unsigned char patch[18] = {
        0xae, 0x0e, 0x05, 0xe0, 0x06, 0xb0, 0x05, 0xa9, 0x16,
        0x4c, 0xbd, 0x7f, 0x29, 0x03, 0xaa, 0x4c, 0xba, 0x7f
    };
    memcpy(mpHvc->mEntity.pMemory + 0xb13, patch, sizeof(patch));
    mpHvc->mEntity.cycle.lineThresholds[32] = mpHvc->mEntity.cycle.linePeriod + 0xd00 - mpHvc->mEntity.cycle.lineBlank;
    CHvc* hvc = mpHvc;
    memset(reinterpret_cast<unsigned char*>(hvc) + 0x21a8, 0, 0x2e8);
    *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21c8) = -1;
}
/** @brief Applies the title's menu-state correction after checking its VRAM labels. */
inline __attribute__((used)) void CHvcPatch::_PatchPostProgress_YoshiNoTamago() {
    static const unsigned char checkVram030B_Level[5] = {0xe0, 0xe1, 0xe2, 0xe3, 0xe4};
    static const unsigned char checkVram032B_Speed[5] = {0xe5, 0xe6, 0xe7, 0xe8, 0xe9};
    unsigned char* memory = mpHvc->mEntity.pMemory;
    if (*reinterpret_cast<unsigned int*>(memory + 0x302aeb) == 0x7e6e5e4e &&
        !memcmp(checkVram030B_Level, memory + 0x302b0b, 5) &&
        !memcmp(checkVram032B_Speed, memory + 0x302b2b, 5)) {
        if (memory[0x300568] == 1) memory[0x300568] = 0x1f;
    }
}
/** @brief Applies Balloon Fight audio settings and its title-specific program correction. */
inline __attribute__((used)) void CHvcPatch::_PatchAttach_BalloonFight() {
    *reinterpret_cast<unsigned long*>(reinterpret_cast<unsigned char*>(mpHvc) + 0x1820) = 1;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d28] = 32;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d29] = 32;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d2a] = 32;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d2b] = 32;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d2c] = 32;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d3a] = 12;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d3b] = 12;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d3c] = 12;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d3d] = 8;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d3e] = 12;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d3f] = 0;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d40] = 0;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d41] = 0;
    reinterpret_cast<unsigned char*>(mpHvc)[0x1d42] = 0;
    CHvc* hvc = mpHvc;
    if (hvc->mState17e8 == 0xbd) {
        reinterpret_cast<unsigned char*>(hvc)[0x8225f] = 8;
        reinterpret_cast<unsigned char*>(hvc)[0x82261] = 0x80;
    }
}
/** @brief Prepares the original Super Mario USA palette replacements. */
inline __attribute__((used)) void CHvcPatch::_PatchAnteProgress_SuperMarioUSA() {
    unsigned char* memory = mpHvc->mEntity.pMemory;
    *reinterpret_cast<unsigned int*>(memory + 0x304564) = 0xffde8b8b;
    unsigned char first = memory[0x3045c8];
    memory[0x304568] = first;
    unsigned char second = memory[0x3045c9];
    memory[0x304569] = second;
    unsigned char third = memory[0x3045ca];
    memory[0x30456a] = third;
    unsigned char fourth = memory[0x3045cb];
    memory[0x30456b] = fourth;
    *reinterpret_cast<unsigned int*>(memory + 0x3045a8) = *reinterpret_cast<unsigned int*>(memory + 0x3045d8);
    memcpy(memory + 0x3045e8, memory + 0x3045b8, 2);
    memory[0x3045ea] = memory[0x3045ba];
    memory[0x3045eb] = memory[0x3045bb];
    memory[0x304628] = first;
    memory[0x304629] = second;
    memory[0x30462a] = third;
    memory[0x30462b] = fourth;
}
/** @brief Applies title-specific screen and palette corrections after rendering. */
inline __attribute__((used)) void CHvcPatch::_PatchPostProgress_SuperMarioUSA() {
    CHvc* hvc = mpHvc;
    if (hvc->mState17e8 == 0x9c) {
        unsigned char* memory = hvc->mEntity.pMemory;
        if (memory[0x304510] != 0x0f || memory[0x304511] != 0x30 ||
            memory[0x304512] != 0x0d || memory[0x304513] != 1) return;
        unsigned char* video = hvc->mEntity.pVideo;
        for (unsigned long y = 8; y < 240; ++y) {
            for (unsigned long x = 0; x < 256; ++x) {
                unsigned char* pixel = video + y * 1024 + x * 4;
                if (pixel[0] < 64 && pixel[2] == 0 && pixel[2 - 1024] == 1) pixel[0] = 0x80;
            }
        }
        *reinterpret_cast<unsigned long*>(reinterpret_cast<unsigned char*>(mpHvc) + 0x1d88) = 0x666699ff;
    } else if (hvc->mState17e8 == 0x9b) {
        unsigned char* memory = hvc->mEntity.pMemory;
        if (memory[0x302888] == 0x5a &&
            memory[0x302889] == 0x9a &&
            memory[0x302896] == 0x9a &&
            memory[0x302897] == 0x5c &&
            memory[0x302908] == 0x5b &&
            memory[0x302909] == 0x9a &&
            memory[0x302916] == 0x9a &&
            memory[0x302917] == 0x5d &&
            memory[0x302a2b] == 0x51 &&
            memory[0x302a2c] == 0x53 &&
            memory[0x302a33] == 0x51 &&
            memory[0x302a34] == 0x53 &&
            memory[0x302aac] == 0x3c &&
            memory[0x302aad] == 0x3d &&
            memory[0x302ab2] == 0x3c &&
            memory[0x302ab3] == 0x3d) {
            *reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(hvc) + 0x1b2a) = 0;
        } else {
            *reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(hvc) + 0x1b2a) = 0x200;
        }

    }
}
/** @brief Selects Excitebike scanline settings from the original VRAM signature. */
inline __attribute__((used)) void CHvcPatch::_PatchPostProgress_Excitebike() {
    static const unsigned char checkVram[192] = {
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xb5, 0xb7, 0xb7, 0x3, 0x1b, 0xd, 0xb7, 0xb7, 0xb6, 0xfe, 0x4c, 0x5a, 0x1d, 0xe, 0x16,
        0x19, 0x7a, 0x8b, 0xfe, 0xb5, 0xb7, 0xb7, 0x1d, 0x12, 0x16, 0xe, 0xb7, 0xb6, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xb0, 0x1, 0xfb, 0x2, 0x4, 0xfb, 0x0, 0x0, 0xb1, 0x4b, 0x4d, 0x5b, 0xfc, 0xfc,
        0xfc, 0xfc, 0x7b, 0x4d, 0x8c, 0xb0, 0x0, 0xfb, 0x1, 0x5, 0xfb, 0x3, 0x4, 0xb1, 0xfe, 0xfe,
        0xfe, 0xfe, 0xb2, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb4, 0xfe, 0xfe, 0x5c, 0x6a, 0x6a,
        0x6a, 0x6a, 0x7c, 0xfe, 0xfe, 0xb2, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb3, 0xb4, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x5d, 0x6b, 0x6c,
        0x6b, 0x6c, 0x8a, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
        0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
    };
    unsigned char* memory = mpHvc->mEntity.pMemory;
    unsigned short value = 0;
    if (memcmp(checkVram + 0x62, memory + 0x302b62, 28) ||
        memcmp(checkVram + 0x8d, memory + 0x302b8d, 6)) value = 0x200;
    for (unsigned int i = 0; i < 46; ++i) {
        *reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(mpHvc) + 0x1b32 + i * 4) = value;
    }
}
/** @brief Initializes Kirby palette history from its current palette. */
inline __attribute__((used)) void CHvcPatch::_PatchPostCommence_Kirby() {
    unsigned char* memory = mpHvc->mEntity.pMemory;
    memory[0x3046a1] = 0;
    memory[0x304622] = 1;
    for (unsigned int i = 0; i < 32; ++i) {
        mpHvc->mPaletteHistory[i] = *reinterpret_cast<unsigned long*>(memory + 0x304530);
    }
}
/** @brief Exports Parthena no Kagami patch state in the reserved memory block. */
inline __attribute__((used)) void CHvcPatch::_PatchExport_ParthenaNoKagami() {
    unsigned char* memory = mpHvc->mEntity.pMemory;
    memcpy(memory + 0x303000, mName, 8);
    CHvc* hvc = mpHvc;
    Common::Serialize::LittleEndian stream(memory + 0x303008, false);
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x2198));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x219c));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21a0));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21a4));
    stream.Entry(*reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(hvc) + 0x21a8));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21ac));
}
/** @brief Imports Parthena no Kagami patch state in the reserved memory block. */
inline __attribute__((used)) void CHvcPatch::_PatchImport_ParthenaNoKagami() {
    CHvc* hvc = mpHvc;
    unsigned char* memory = hvc->mEntity.pMemory;
    if (*reinterpret_cast<unsigned long*>(memory + 0x303000) == *reinterpret_cast<unsigned long*>(mName)) {
    Common::Serialize::LittleEndian stream(memory + 0x303008, true);
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x2198));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x219c));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21a0));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21a4));
    stream.Entry(*reinterpret_cast<unsigned short*>(reinterpret_cast<unsigned char*>(hvc) + 0x21a8));
    stream.Entry(*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(hvc) + 0x21ac));
    }
}
/** @brief Converts matching pixels using the first palette group matching the pattern. */
inline __attribute__((used)) bool CHvcPatch::_replacePaletteColorToDirectColorInOrder(
    const unsigned char* pattern, const unsigned int* colors, const unsigned char* palette) {
    unsigned int selected;
    if ((pattern[0] == 255 || pattern[0] == palette[0]) &&
        (pattern[1] == 255 || pattern[1] == palette[1]) &&
        (pattern[2] == 255 || pattern[2] == palette[2]) &&
        (pattern[3] == 255 || pattern[3] == palette[3])) selected = 0;
    else if ((pattern[0] == 255 || pattern[0] == palette[4]) &&
        (pattern[1] == 255 || pattern[1] == palette[5]) &&
        (pattern[2] == 255 || pattern[2] == palette[6]) &&
        (pattern[3] == 255 || pattern[3] == palette[7])) selected = 1;
    else if ((pattern[0] == 255 || pattern[0] == palette[8]) &&
        (pattern[1] == 255 || pattern[1] == palette[9]) &&
        (pattern[2] == 255 || pattern[2] == palette[10]) &&
        (pattern[3] == 255 || pattern[3] == palette[11])) selected = 2;
    else if ((pattern[0] == 255 || pattern[0] == palette[12]) &&
        (pattern[1] == 255 || pattern[1] == palette[13]) &&
        (pattern[2] == 255 || pattern[2] == palette[14]) &&
        (pattern[3] == 255 || pattern[3] == palette[15])) selected = 3;
    else return false;
    bool changed = false;
    unsigned char* pixel = mpHvc->mEntity.pVideo;
    for (unsigned int i = 0; i < 256 * 240; ++i, pixel += 4) {
        unsigned long bank = static_cast<unsigned int>(pixel[2]) >> 2;
        if (((pixel[0] == pattern[0] && colors[0]) ||
             (pixel[0] == pattern[1] && colors[1]) ||
             (pixel[0] == pattern[2] && colors[2]) ||
             (pixel[0] == pattern[3] && colors[3]))) {
            if (static_cast<unsigned int>(bank) != selected) continue;
            if (!changed) {
                const unsigned char* group = palette + bank * 4;
                for (unsigned int entry = 0; entry < 4; ++entry) {
                    if (group[entry] == pattern[entry] && colors[entry])
                        mpHvc->mPaletteHistory[static_cast<unsigned long>(bank * 4) + entry] = colors[entry];
                }
            }
            if (static_cast<signed char>(pixel[0]) >= 0) pixel[0] ^= 0x80;
            changed = true;
        }
    }
    return changed;
}
/** @brief Selects compatibility hooks for the active title. */
inline __attribute__((used)) void CHvcPatch::Configure() {
    for (unsigned int i = 0; i < 7; ++i) mHooks[i] = &CHvcPatch::_PatchDefault;
    switch (mpHvc->mState17e8) {
    case 0xd:
    case 0xe:
    case 0xf:
    case 0x10:
    case 0x11:
        mHooks[0] = &CHvcPatch::_PatchAttach_Kirby;
        mHooks[4] = &CHvcPatch::_PatchPostCommence_Kirby;
        mHooks[6] = &CHvcPatch::_PatchPostProgress_Kirby;
        break;
    case 0x7b:
    case 0x7c:
    case 0x7d:
        mHooks[0] = &CHvcPatch::_PatchAttach_PunchOut;
        break;
    case 0xbb:
    case 0xbc:
    case 0xbd:
        mHooks[0] = &CHvcPatch::_PatchAttach_BalloonFight;
        break;
    case 0x6e:
    case 0x6f:
    case 0x70:
        mHooks[6] = &CHvcPatch::_PatchPostProgress_Excitebike;
        break;
    case 0x9b:
    case 0x9c:
    case 0x9d:
        mHooks[5] = &CHvcPatch::_PatchAnteProgress_SuperMarioUSA;
        mHooks[6] = &CHvcPatch::_PatchPostProgress_SuperMarioUSA;
        break;
    case 0x12:
    case 0x14:
        mHooks[0] = &CHvcPatch::_PatchAttach_LinkNoBouken_NonJP;
        mHooks[5] = &CHvcPatch::_PatchAnteProgress_LinkNoBouken;
        mHooks[6] = &CHvcPatch::_PatchPostProgress_LinkNoBouken;
        break;
    case 0x71:
    case 0x72:
        mHooks[2] = &CHvcPatch::_PatchExport_ParthenaNoKagami;
        mHooks[3] = &CHvcPatch::_PatchImport_ParthenaNoKagami;
        mHooks[4] = &CHvcPatch::_PatchPostCommence_ParthenaNoKagami;
        mHooks[5] = &CHvcPatch::_PatchAnteProgress_ParthenaNoKagami;
        mHooks[6] = &CHvcPatch::_PatchPostProgress_ParthenaNoKagami;
        break;
    case 0xcc:
    case 0xcd:
        mHooks[0] = &CHvcPatch::_PatchAttach_WreckingCrew;
        break;
    case 0x9:
    case 0xa:
    case 0x162:
        mHooks[6] = &CHvcPatch::_PatchPostProgress_YoshiNoTamago;
        break;
    case 0x13:
        mHooks[0] = &CHvcPatch::_PatchAttach_LinkNoBouken;
        mHooks[5] = &CHvcPatch::_PatchAnteProgress_LinkNoBouken;
        mHooks[6] = &CHvcPatch::_PatchPostProgress_LinkNoBouken;
        break;
    case 0xa0:
        mHooks[0] = &CHvcPatch::_PatchAttach_Metroid;
        break;
    case 0xd1:
        mHooks[6] = &CHvcPatch::_PatchPostProgress_SuperMario3;
        break;
    case 0xd3:
        mHooks[0] = &CHvcPatch::_PatchAttach_Mother;
        mHooks[5] = &CHvcPatch::_PatchAnteProgress_Mother;
        break;
    case 0xd4:
        mHooks[0] = &CHvcPatch::_PatchAttach_YoshiNoCookie;
        break;
    }
}
/** @brief Adjusts Mother text commands for the recognized message patterns. */
inline __attribute__((used)) void CHvcPatch::_PatchAnteProgress_Mother() {
    static const unsigned char text_table[3][16] = {
        {0x90, 0xdb, 0xee, 0x8f, 0xa3, 0xb4, 0xbb, 0x93, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
        {0xc0, 0xa0, 0xa0, 0x96, 0xba, 0xa1, 0xaf, 0x8f, 0xa0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0},
        {0x90, 0x95, 0xaf, 0xbc, 0xb8, 0x9b, 0xbd, 0xa6, 0xc0, 0x91, 0x8f, 0xa3, 0xb3, 0x9c, 0xb7, 0xbd},
    };
    static const unsigned char eTable[3] = {8, 9, 16};
    unsigned char* memory = mpHvc->mEntity.pMemory;
    unsigned long row;
    for (row = 0; row < 3; ++row) {
        unsigned int offset = memory[0x300442] == 5 ? 4 : 0;
        unsigned int column;
        for (column = 0; column < 16; ++column) {
            if ((memory + 0x300000)[0x442 + offset] != text_table[row][column]) break;
            offset += (memory + 0x300000)[0x443 + offset] == 5 ? 5 : 1;
        }
        if (column == 16) break;
    }
    if (row == 3) return;
    unsigned int offset = memory[0x300428] == 5 ? 4 : 0;
    for (unsigned int i = 0; i < eTable[row]; ++i) {
        offset += (memory + 0x300000 + offset)[0x429] == 5 ? 5 : 1;
    }
    unsigned int patchOffset = offset + (memory[0x300442] == 5 ? 4 : 0);
    switch (row) {
    case 0:
        (memory + 0x300000 + patchOffset)[0x43e] = 0xa4;
        break;
    case 1:
        (memory + 0x300000 + patchOffset)[0x43e] = 0xff;
        break;
    case 2:
        (memory + 0x300000 + offset)[0x428] = 0xff;
        *reinterpret_cast<unsigned short*>(memory + 0x30043e + patchOffset) = 0xb8ac;
        break;
    }
}
/** @brief Smooths Kirby palette changes and marks pixels using palette history. */
inline __attribute__((used)) void CHvcPatch::_PatchPostProgress_Kirby() {
    unsigned char* memory = mpHvc->mEntity.pMemory;
    if (memory[0x304622]) {
        memory[0x304622] = 0;
        return;
    }
    unsigned char* video = mpHvc->mEntity.pVideo;
    unsigned char* palette = memory + 0x304510;
    unsigned long* directColors = reinterpret_cast<unsigned long*>(memory + 0x304530);
    unsigned int selected;
    if (!memcmp("\x0c\x11\x21\x31\x0c\x1c\x2c\x0c\x0c\x38\x01\x12\x0c\x37\x27\x07", palette, 16)) selected = 12;
    else if (!memcmp("\x0f\x04\x14\x33\x0f\x1c\x2c\x0c\x0f\x38\x01\x12\x0f\x37\x27\x07", palette, 16)) selected = 15;
    else if (!memcmp("\x1c\x11\x21\x31\x1c\x1c\x2c\x1c\x1c\x38\x11\x12\x1c\x37\x27\x07", palette, 16)) selected = 28;
    else if (!memcmp("\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x37\x27\x07", palette, 16)) selected = 32;
    else if (!memcmp("\x2c\x21\x21\x31\x2c\x2c\x2c\x2c\x2c\x38\x21\x22\x2c\x37\x27\x07", palette, 16)) selected = 44;
    else if (!memcmp("\x30\x30\x30\x30\x30\x30\x30\x30\x30\x38\x21\x22\x30\x37\x27\x07", palette, 16)) selected = 48;
    else selected = 0;
    memory[0x3046a2] = selected;
    if (selected) {
        unsigned char* target = reinterpret_cast<unsigned char*>(directColors + selected);
        for (unsigned int component = 0; component < 3; ++component) {
            unsigned int previous = memory[0x3045a0 + component];
            unsigned int next = target[component];
            if (previous < next) {
                if (previous + 8 < next) next = previous + 8;
            } else if (next + 8 < previous) next = previous - 8;
            memory[0x3045a0 + component] = next;
        }
        memory[0x3045a3] = 255;
        for (unsigned long offset = 0; offset < 256 * 240 * 4; offset += 4) {
            if (video[offset] < 64 && !(video[offset + 2] & 3)) video[offset] = 14;
        }
    } else {
        *reinterpret_cast<unsigned long*>(memory + 0x3045a0) = 255;
    }
    unsigned int different = 0;
    for (unsigned int i = 1; i < 16; ++i) {
        if (palette[0] != palette[i]) ++different;
    }
    if (!different) memory[0x3046a1] = 15;
    else if (memory[0x3046a1]) --memory[0x3046a1];
    if (memory[0x3046a1]) {
        for (unsigned int i = 0; i < 16; ++i) {
            unsigned char* history = reinterpret_cast<unsigned char*>(mpHvc->mPaletteHistory + i);
            unsigned char* target = reinterpret_cast<unsigned char*>(directColors + palette[i]);
            for (unsigned int component = 0; component < 3; ++component) {
                unsigned int previous = history[component];
                unsigned int next = target[component];
                if (previous < next) {
                    if (previous + 8 < next) next = previous + 8;
                } else if (next + 8 < previous) next = previous - 8;
                history[component] = next;
            }
            history[3] = 255;
        }
        for (unsigned long offset = 0; offset < 256 * 240 * 4; offset += 4) {
            if (video[offset] < 64 && video[offset + 2] < 16) video[offset] = video[offset + 2] ^ 0x80;
        }
    } else {
        for (unsigned int i = 0; i < 32; ++i) mpHvc->mPaletteHistory[i] = directColors[palette[i]];
    }
    memory[0x304622] = 0;
}
} } } }
