#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"
#include "LuigiBros/Common/Serialize.hpp"
#include <cstring>

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {

/**
 * @brief Initializes video and sequence timing for the title's frame rate.
 * @param rTitle Title settings with the expected frame rate.
 */
void CPlatformHvcUnit::_SCycleController::Initialize(_STitleElement& rTitle) {
    region = rTitle.frameRate == 50;
    Finalize();
    AdjustParameter(0, 0, 0, 0, 0, 0);
    ChangeSequenceMode(0);
}

/**
 * @brief Adjusts regional timing constants and rebuilds sequence events.
 * @param lineAdjust Adjustment to the line period.
 * @param lineBlankAdjust Adjustment to horizontal blanking duration.
 * @param frameAdjust Adjustment to the frame period.
 * @param frameBlankAdjust Adjustment to vertical blanking duration.
 * @param thresholdAdjust Adjustment to the auxiliary timing threshold.
 * @param offsetAdjust Auxiliary timing offset.
 */
void CPlatformHvcUnit::_SCycleController::AdjustParameter(int lineAdjust, int lineBlankAdjust,
    int frameAdjust, int frameBlankAdjust, int thresholdAdjust, int offsetAdjust) {
    if (region == 1) {
        linePeriod = 0x6a8f;
        lineBlank = 0x1a8f;
        framePeriod = 0x81de48;
        frameBlank = 0x1d8da9;
        _a48 = 0x1a00;
    } else {
        linePeriod = 0x71aa;
        lineBlank = 0x1c55;
        framePeriod = 0x7453fc;
        frameBlank = 0x952f2;
        _a48 = 0x1bf0;
    }
    linePeriod += lineAdjust;
    lineBlank += lineBlankAdjust;
    framePeriod += frameAdjust;
    frameBlank += frameBlankAdjust;
    for (unsigned long i = 0; i < 314; ++i) {
        lineThresholds[i] = linePeriod - lineBlank;
    }
    _a38 = 0xa000L + thresholdAdjust;
    _a40 = offsetAdjust;
    for (unsigned int mode = 0; mode < 2; ++mode) {
        for (unsigned int i = 0; i < 5; ++i) {
            events[mode][i].cycle = framePeriod * (i + 1);
            events[mode][i].flags = i < 4 ? (1UL << (16 + i)) : 0;
        }
    }
}

/**
 * @brief Restarts the sequence clock in four-event or five-event mode.
 * @param mode Zero selects four events; any nonzero value selects five.
 */
void CPlatformHvcUnit::_SCycleController::ChangeSequenceMode(int mode) {
    sequenceIndex = 0;
    sequenceCycle = -0x800;
    sequenceMode = mode != 0;
    _a50 = 0x80000;
    _a58 = mode != 0 ? 0x50000 : 0xa0000;
    _a60 = 0xf0000;
}

/** @brief Clears accumulated video and sequence timing state. */
void CPlatformHvcUnit::_SCycleController::Finalize() {
    std::memset(this, 0, 0x40);
}

/**
 * @brief Advances clocks and reports video and sequence timing events.
 * @param cycles Number of elapsed emulator cycles.
 * @return Bit mask of boundaries, blanking phases, and sequence events.
 */
unsigned long CPlatformHvcUnit::_SCycleController::Progress(unsigned long cycles) {
    unsigned long previousLineCycle = lineCycle;
    unsigned long previousFrameCycle = frameCycle;
    lineCycle += cycles;
    frameCycle += cycles;
    unsigned long result = 0;
    if (lineCycle >= linePeriod) {
        lineCycle -= linePeriod;
        line += 0x100;
        result |= 1;
    }
    if (frameCycle >= framePeriod) {
        frameCycle -= framePeriod;
        lineCycle = 0;
        line = 0;
        ++frame;
        result |= 2;
    }
    unsigned long lineActive = linePeriod - lineBlank;
    unsigned long frameActive = framePeriod - frameBlank;
    bool lineBlankNow = lineActive <= lineCycle;
    bool frameBlankNow = frameActive <= frameCycle;
    bool lineBlankBefore = lineActive <= previousLineCycle;
    bool frameBlankBefore = frameActive <= previousFrameCycle;
    result |= static_cast<unsigned long>(lineBlankNow) << 2;
    result |= static_cast<unsigned long>(frameBlankNow) << 3;
    result |= (lineBlankNow & !lineBlankBefore) ? 0x10UL : 0;
    result |= (frameBlankNow & !frameBlankBefore) ? 0x20UL : 0;
    result |= (!lineBlankNow & lineBlankBefore) ? 0x40UL : 0;
    result |= (!frameBlankNow & frameBlankBefore) ? 0x80UL : 0;
    unsigned long threshold = lineThresholds[line >> 8];
    if (lineCycle >= threshold && previousLineCycle < threshold) { result |= 0x100; }
    sequenceCycle += cycles * 4;
    if (sequenceCycle >= events[sequenceMode][sequenceIndex].cycle) {
        result |= events[sequenceMode][sequenceIndex].flags;
        ++sequenceIndex;
    }
    if (sequenceIndex >= sequenceMode + 4) {
        sequenceIndex = 0;
        sequenceCycle -= events[sequenceMode][sequenceMode + 3].cycle;
    }
    return result;
}

/**
 * @brief Initializes the platform's command words.
 * @param rTitle Unused title settings.
 */
void CPlatformHvcUnit::_SOrderController::Initialize(_STitleElement& rTitle) {
    words[0] = 0x3e;
    words[2] = 0;
    words[4] = 0;
    words[6] = 0;
}

/** @brief Resets the platform's command words. */
void CPlatformHvcUnit::_SOrderController::Finalize() {
    words[0] = 0x3e;
    words[2] = 0;
    words[4] = 0;
    words[6] = 0;
}

/**
 * @brief Writes a platform serialization tag using the original field layout.
 * @param pTag Destination buffer of at least sixteen bytes.
 * @param type Serialized object type.
 * @param version Format version.
 * @param flags Two-byte format flags.
 * @param pIdentifier Seven-byte identifier, or null for the default identifier.
 * @param size Serialized payload size.
 * @return Zero on success, or two for a null destination.
 */
unsigned int CPlatformHvcUnit::GenerateSerializeTag(unsigned char* pTag, unsigned char type,
    unsigned char version, unsigned short flags, const unsigned char* pIdentifier,
    unsigned long size) {
    if (pTag == nullptr) {
        return 2;
    }
    std::memset(pTag, 0, 16);
    pTag[0] = type;
    pTag[1] = version;
    pTag[2] = flags;
    pTag[3] = flags >> 8;
    unsigned long storedSize = size;
    Common::Serialize::LittleEndian stream(pTag + 4, false);
    stream.Entry(storedSize);
    std::memcpy(pTag + 8, pIdentifier != nullptr ? pIdentifier :
        reinterpret_cast<const unsigned char*>("-------"), 7);
    return 0;
}

/**
 * @brief Checks a serialization tag's type, version, and stored size.
 * @param pTag Source tag buffer.
 * @param type Expected serialized object type.
 * @param version Expected format version.
 * @param flags Unused expected flags.
 * @param pIdentifier Unused expected identifier.
 * @param size Expected payload size.
 * @return Zero for a match, one for a mismatch, or two for a null source.
 */
unsigned int CPlatformHvcUnit::ValidateSerializeTag(const unsigned char* pTag, unsigned char type,
    unsigned char version, unsigned short flags, const unsigned char* pIdentifier,
    unsigned long size) {
    if (pTag == nullptr) {
        return 2;
    }
    if (pTag[0] != type || pTag[1] != version) {
        return 1;
    }
    unsigned long storedSize;
    Common::Serialize::LittleEndian stream(const_cast<unsigned char*>(pTag + 4), true);
    stream.Entry(storedSize);
    return storedSize != size;
}

/**
 * @brief Maps selected upper CPU pages to the end of program ROM.
 * @param pEntity Emulator bank state.
 * @param mask Bit mask selecting CPU pages eight through fifteen.
 */
void CPlatformHvcUnit::AdjustEntityPageTableForPrg(_SEntity* pEntity, unsigned long mask) {
    if (pEntity->prgBankCount == 0) {
        return;
    }
    if (pEntity->prgBankCount == 1) {
        for (int i = 15; i >= 8; --i) {
            if (mask & (1UL << i)) {
                pEntity->cpuBanks[i].readOffset = (i & 3) * 0x1000;
            }
        }
    } else {
        unsigned long end = static_cast<unsigned long>(pEntity->prgBankCount) * 0x4000;
        for (int i = 15; i >= 8; --i) {
            if (mask & (1UL << i)) {
                pEntity->cpuBanks[i].readOffset = end - (16 - i) * 0x1000;
            }
        }
    }
}

/**
 * @brief Maps selected PPU pages to the end of character ROM.
 * @param pEntity Emulator bank state.
 * @param mask Bit mask selecting character pages zero through seven.
 */
void CPlatformHvcUnit::AdjustEntityPageTableForChr(_SEntity* pEntity, unsigned long mask) {
    if (pEntity->chrBankCount == 0) {
        return;
    }
    unsigned long end = 0x200000 + static_cast<unsigned long>(pEntity->chrBankCount) * 0x2000;
    for (int i = 7; i >= 0; --i) {
        if (mask & (1UL << i)) {
            pEntity->ppuBanks[i].readOffset = end - (8 - i) * 0x400;
        }
    }
}

/**
 * @brief Applies cartridge nametable mirroring to PPU read and write pages.
 * @param pEntity Emulator bank state and cartridge flags.
 * @param mask Unused page selection mask.
 */
void CPlatformHvcUnit::AdjustEntityPageTableForVRam(_SEntity* pEntity, unsigned long mask) {
    pEntity->ppuBanks[8].readOffset = pEntity->ppuBanks[8].writeOffset = 0x302800;
    pEntity->ppuBanks[11].readOffset = pEntity->ppuBanks[11].writeOffset = 0x302c00;
    if (pEntity->flags & 1) {
        pEntity->ppuBanks[9].readOffset = pEntity->ppuBanks[9].writeOffset = 0x302c00;
        pEntity->ppuBanks[10].readOffset = pEntity->ppuBanks[10].writeOffset = 0x302800;
    } else {
        pEntity->ppuBanks[9].readOffset = pEntity->ppuBanks[9].writeOffset = 0x302800;
        pEntity->ppuBanks[10].readOffset = pEntity->ppuBanks[10].writeOffset = 0x302c00;
    }
}

/**
 * @brief Changes nametable mirroring or swaps the middle two nametable pages.
 * @param pEntity Emulator bank state.
 * @param mode Zero for vertical mirroring, one for horizontal, otherwise swap.
 */
void BusMapSwitchVRAM(CPlatformHvcUnit::_SEntity* pEntity, int mode) {
    switch (mode) {
    case 0:
        pEntity->ppuBanks[8].readOffset = pEntity->ppuBanks[8].writeOffset = 0x302800;
        pEntity->ppuBanks[9].readOffset = pEntity->ppuBanks[9].writeOffset = 0x302c00;
        pEntity->ppuBanks[10].readOffset = pEntity->ppuBanks[10].writeOffset = 0x302800;
        pEntity->ppuBanks[11].readOffset = pEntity->ppuBanks[11].writeOffset = 0x302c00;
        break;
    case 1:
        pEntity->ppuBanks[8].readOffset = pEntity->ppuBanks[8].writeOffset = 0x302800;
        pEntity->ppuBanks[9].readOffset = pEntity->ppuBanks[9].writeOffset = 0x302800;
        pEntity->ppuBanks[10].readOffset = pEntity->ppuBanks[10].writeOffset = 0x302c00;
        pEntity->ppuBanks[11].readOffset = pEntity->ppuBanks[11].writeOffset = 0x302c00;
        break;
    default: {
        unsigned long read = pEntity->ppuBanks[9].readOffset;
        unsigned long write = pEntity->ppuBanks[9].writeOffset;
        pEntity->ppuBanks[9].readOffset = pEntity->ppuBanks[10].readOffset;
        pEntity->ppuBanks[9].writeOffset = pEntity->ppuBanks[10].writeOffset;
        pEntity->ppuBanks[10].writeOffset = write;
        pEntity->ppuBanks[10].readOffset = read;
        break;
    }
    }
}

/**
 * @brief Finds the changed range using the platform's prefix and suffix comparisons.
 * @param pFirst First comparison buffer.
 * @param pSecond Second comparison buffer.
 * @param size Positive byte count shared by both buffers.
 * @param rOffset Receives the unchanged prefix length.
 * @param rLength Receives the remaining range after suffix trimming.
 */
void AnalyzeVariation(const void* pFirst, const void* pSecond, unsigned long size,
    unsigned long& rOffset, unsigned long& rLength) {
    const unsigned char* pLeft = static_cast<const unsigned char*>(pFirst);
    const unsigned char* pRight = static_cast<const unsigned char*>(pSecond);
    rOffset = 0;
    rLength = 0;
    while (*pLeft++ == *pRight++) {
        ++rOffset;
        if (size - 1 < rOffset) {
            break;
        }
    }
    rLength = size - rOffset;
    if (rLength != 0) {
        const unsigned char* pLeftEnd = static_cast<const unsigned char*>(pFirst) + size - 1;
        const unsigned char* pRightEnd = static_cast<const unsigned char*>(pSecond) + size - 1;
        do {
            if (*pLeftEnd-- != *pRightEnd--) {
                break;
            }
            --rLength;
        } while (pLeftEnd > pLeft && rLength > 1);
    }
}

/**
 * @brief Routes an initialized unit's notification to its category hook.
 * @param pSender Sending platform unit.
 * @param category System, access, or process category.
 * @param operation Operation identifier within the category.
 * @param pArguments Operation arguments.
 * @return Hook result, two when uninitialized, or one for an unknown category.
 */
unsigned int CPlatformHvcUnit::Notify(CPlatformUnit* pSender, unsigned long category,
    unsigned long operation, void* pArguments) {
    if (!mInitialized) { return 2; }
    UArgument* pArgs = static_cast<UArgument*>(pArguments);
    switch (category) {
    case 0: {
        if (operation == 0) { mpEntity = static_cast<_SEntity*>(pArgs->pointers[3]); }
        unsigned int result = System(pSender, category, operation, pArgs);
        if (operation == 1) { mpEntity = nullptr; }
        return result;
    }
    case 1: return Access(pSender, category, operation, pArgs);
    case 2: return Process(pSender, category, operation, pArgs);
    default: return 1;
    }
}

/**
 * @brief Attaches the entity before invoking the system hook.
 * @param pSender Sending platform unit.
 * @param category Notification category.
 * @param operation Operation identifier.
 * @param pArguments Operation arguments containing the entity pointer.
 * @return System hook result.
 */
unsigned int CPlatformHvcUnit::SystemAttach(CPlatformUnit* pSender, unsigned long category,
    unsigned long operation, UArgument* pArguments) {
    mpEntity = static_cast<_SEntity*>(pArguments->pointers[3]);
    return System(pSender, category, operation, pArguments);
}

/**
 * @brief Detaches the entity after invoking the system hook.
 * @param pSender Sending platform unit.
 * @param category Notification category.
 * @param operation Operation identifier.
 * @param pArguments Operation arguments containing the entity pointer.
 * @return System hook result.
 */
unsigned int CPlatformHvcUnit::SystemDetach(CPlatformUnit* pSender, unsigned long category,
    unsigned long operation, UArgument* pArguments) {
    unsigned int result = System(pSender, category, operation, pArguments);
    mpEntity = nullptr;
    return result;
}

/**
 * @brief Exports the tagged platform state.
 * @param pBuffer Serialized content buffer.
 * @param capacity Available buffer bytes.
 * @param rSize Receives the number of bytes processed.
 * @return Zero on success, or a buffer or tag error code.
 */
unsigned int CPlatformHvcUnit::ExportContent(void* pBuffer, unsigned long capacity, unsigned long& rSize) {
    rSize = 0;
    if (!pBuffer || capacity == 0) { return 2; }
    unsigned long total = _SerializeCoreInferSize() + 16;
    if (total > capacity) { return 1; }
    unsigned char content = _SerializeTagContent();
    unsigned char species = _SerializeTagSpecies();
    unsigned short version = _SerializeTagVersion();
    const unsigned char* pComment = _SerializeTagComment();
    GenerateSerializeTag(static_cast<unsigned char*>(pBuffer), content, species, version, pComment, total);
    _SerializeCoreExport(static_cast<unsigned char*>(pBuffer) + 16);
    rSize = total;
    return 0;
}

/**
 * @brief Validates and imports the tagged platform state.
 * @param pBuffer Serialized content buffer.
 * @param size Available buffer bytes.
 * @param rSize Receives the number of bytes processed.
 * @return Zero on success, or a buffer or tag error code.
 */
unsigned int CPlatformHvcUnit::ImportContent(const void* pBuffer, unsigned long size, unsigned long& rSize) {
    rSize = 0;
    if (!pBuffer || size == 0) { return 2; }
    unsigned long total = _SerializeCoreInferSize() + 16;
    if (total > size) { return 1; }
    unsigned char content = _SerializeTagContent();
    unsigned char species = _SerializeTagSpecies();
    unsigned short version = _SerializeTagVersion();
    const unsigned char* pComment = _SerializeTagComment();
    if (ValidateSerializeTag(static_cast<const unsigned char*>(pBuffer), content, species, version, pComment, total) != 0) { return 5; }
    _SerializeCoreImport(static_cast<const unsigned char*>(pBuffer) + 16);
    rSize = total;
    return 0;
}

/**
 * @brief Computes the tagged platform state size.
 * @param rSize Receives the core size plus the sixteen-byte tag.
 * @return Zero.
 */
unsigned int CPlatformHvcUnit::InferContentSize(unsigned long& rSize) {
    rSize = _SerializeCoreInferSize() + 16;
    return 0;
}

/**
 * @brief Copies bytes through the selected CPU or PPU bank mapping.
 * @param pEntity Emulator memory and page tables.
 * @param category Three for CPU memory or four for PPU memory.
 * @param operation One to write, otherwise read.
 * @param pArguments Transfer length, address, and buffer arguments.
 * @return Zero on success, four for an unsupported bank, or six for an unsupported operation.
 */
unsigned int CPlatformHvcUnit::_MapBasicOperation(_SEntity* pEntity, unsigned long category,
    unsigned long operation, UArgument* pArguments) {
    switch (category) {
    case 3: {
        unsigned long address = pArguments->values[2];
        _SBankAccess* pBank = &pEntity->cpuBanks[address >> 12].access[operation];
        unsigned char type = pBank->type;
        switch (type) {
        case 0: {
            if (operation != 1 && pArguments->values[0] != 0) {
                memset(pArguments->pointers[4], 0xff, pArguments->values[0]);
            }
            return 0;
        }
        default: return 4;
        case 1: {
        unsigned long offset = (address & pBank->mask) + pBank->offset;
        unsigned long length = pArguments->values[0];
        unsigned char* pSource;
        unsigned char* pDestination;
        if (operation == 1) {
            pDestination = pEntity->pMemory + offset;
            pSource = static_cast<unsigned char*>(pArguments->pointers[4]);
            for (unsigned long i = 0; i < length; ++i) {
                *pDestination++ = *pSource++;
            }
        } else {
            pSource = pEntity->pMemory + offset;
            pDestination = static_cast<unsigned char*>(pArguments->pointers[4]);
            for (unsigned long i = 0; i < length; ++i) {
                *pDestination++ = *pSource++;
            }
        }
        return 0;
        }
        }
    }
    case 4: {
        unsigned long address = pArguments->values[2];
        _SBankAccess* pBank = &pEntity->ppuBanks[address >> 10].access[operation];
        unsigned char type = pBank->type;
        switch (type) {
        case 0: {
            return 6;
        }
        default: return 4;
        case 1: {
        unsigned long offset = (address & pBank->mask) + pBank->offset;
        unsigned long length = pArguments->values[0];
        unsigned char* pSource;
        unsigned char* pDestination;
        if (operation == 1) {
            pDestination = pEntity->pMemory + offset;
            pSource = static_cast<unsigned char*>(pArguments->pointers[4]);
            if (address == 0x3f10) { pDestination[-16] = *pSource; }
            else if (address == 0x3f00) { pDestination[16] = *pSource; }
            for (unsigned long i = 0; i < length; ++i) {
                *pDestination++ = *pSource++;
            }
        } else {
            pSource = pEntity->pMemory + offset;
            pDestination = static_cast<unsigned char*>(pArguments->pointers[4]);
            for (unsigned long i = 0; i < length; ++i) {
                *pDestination++ = *pSource++;
            }
        }
        return 0;
        }
        }
    }
    default: return 6;
    }
}

extern "C" {
const unsigned char lbl_71014A0110[] = {
    0x4a, 0x4a, 0x4a, 0xff, 0x00, 0x00, 0x6a, 0xff, 0x08, 0x00, 0x62, 0xff, 0x29, 0x00, 0x5a, 0xff,
    0x41, 0x00, 0x4a, 0xff, 0x4a, 0x00, 0x00, 0xff, 0x41, 0x00, 0x00, 0xff, 0x29, 0x10, 0x00, 0xff,
    0x18, 0x29, 0x00, 0xff, 0x00, 0x31, 0x10, 0xff, 0x00, 0x31, 0x00, 0xff, 0x00, 0x29, 0x10, 0xff,
    0x00, 0x20, 0x41, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
    0x73, 0x73, 0x73, 0xff, 0x00, 0x31, 0x83, 0xff, 0x31, 0x00, 0xac, 0xff, 0x4a, 0x00, 0x94, 0xff,
    0x62, 0x00, 0x7b, 0xff, 0x6a, 0x00, 0x39, 0xff, 0x6a, 0x20, 0x00, 0xff, 0x5a, 0x31, 0x00, 0xff,
    0x41, 0x4a, 0x00, 0xff, 0x18, 0x5a, 0x00, 0xff, 0x10, 0x5a, 0x00, 0xff, 0x00, 0x5a, 0x31, 0xff,
    0x00, 0x4a, 0x5a, 0xff, 0x10, 0x10, 0x10, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
    0xac, 0xac, 0xac, 0xff, 0x4a, 0x73, 0xb4, 0xff, 0x62, 0x5a, 0xd5, 0xff, 0x83, 0x52, 0xe6, 0xff,
    0xa4, 0x52, 0xac, 0xff, 0xac, 0x4a, 0x83, 0xff, 0xb4, 0x62, 0x4a, 0xff, 0x94, 0x73, 0x31, 0xff,
    0x7b, 0x73, 0x29, 0xff, 0x5a, 0x83, 0x00, 0xff, 0x39, 0x8b, 0x31, 0xff, 0x31, 0x8b, 0x5a, 0xff,
    0x39, 0x8b, 0x8b, 0xff, 0x39, 0x39, 0x39, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
    0xb4, 0xb4, 0xb4, 0xff, 0x8b, 0x9c, 0xb4, 0xff, 0x8b, 0x8b, 0xac, 0xff, 0x9c, 0x8b, 0xbd, 0xff,
    0xa4, 0x83, 0xbd, 0xff, 0xac, 0x8b, 0x9c, 0xff, 0xac, 0x94, 0x8b, 0xff, 0x9c, 0x8b, 0x7b, 0xff,
    0x9c, 0x9c, 0x73, 0xff, 0x94, 0xa4, 0x7b, 0xff, 0x83, 0xa4, 0x7b, 0xff, 0x7b, 0x9c, 0x83, 0xff,
    0x73, 0x94, 0x8b, 0xff, 0x8b, 0x8b, 0x8b, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff,
};
const unsigned char lbl_71014A0210[] = {
    0x01, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3a, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char lbl_71014A0410[] = {
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x44, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x44, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x45, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x45, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
};
}

/**
 * @brief Initializes cartridge memory, output buffers, and address mappings.
 * @param rEntity Entity receiving the configuration.
 * @param pMemory Writable emulator memory of at least 0x304630 bytes.
 * @param pVideo Video output buffer.
 * @param pAudio Audio output buffer.
 * @param pOutput Additional output state.
 * @param pTitle Cartridge header, ROM data, and optional disk BIOS.
 * @param pCustom Custom emulator parameters.
 * @param pSurvey Survey parameters.
 * @return The configured entity.
 */
CPlatformHvcUnit::_SEntity* CPlatformHvcUnit::ConfigureEntity(_SEntity& rEntity, void* pMemory,
    unsigned char* pVideo, short* pAudio, unsigned long* pOutput,
    _STitleElement* pTitle, _SCustomParameter* pCustom, _SSurveyParameter* pSurvey) {
    memset(pMemory, 0, 0x304630);
    rEntity.pMemory = static_cast<unsigned char*>(pMemory);
    rEntity.pVideo = pVideo;
    rEntity._1748 = 0;
    rEntity.pAudio = pAudio;
    rEntity._1750 = 0;
    rEntity.pOutput = pOutput;
    if (pTitle->pDiskBios) {
        rEntity.mapper = 0xff;
        rEntity.flags = 0;
        rEntity.prgBankCount = 0;
        rEntity.chrBankCount = 1;
        unsigned char* pDisk = rEntity.pMemory + 0x100000;
        memcpy(pMemory, pTitle->pDiskBios, 0x2000);
        static_cast<unsigned char*>(pMemory)[0xebd] = 0x42;
        memcpy(pDisk, pTitle->pPrg, 0x100000);
    } else {
        const unsigned char* pHeader = pTitle->pHeader;
        rEntity.mapper = (pHeader[7] & 0xf0) | (pHeader[6] >> 4);
        rEntity.flags = pHeader[6] & 0xf;
        rEntity.prgBankCount = pHeader[4];
        rEntity.chrBankCount = pHeader[5];
        if (pTitle->pPrg) {
            memcpy(pMemory, pTitle->pPrg, static_cast<unsigned long>(rEntity.prgBankCount) << 14);
        }
        if (pTitle->pChr) {
            memcpy(rEntity.pMemory + 0x200000, pTitle->pChr, static_cast<unsigned long>(rEntity.chrBankCount) << 13);
        }
    }
    rEntity.pCustom = pCustom;
    rEntity.pSurvey = pSurvey;
    rEntity.pTitle = pTitle;
    memcpy(rEntity.pMemory + 0x304530, lbl_71014A0110, 0x100);
    memcpy(rEntity.cpuBanks, lbl_71014A0210, 0x200);
    memcpy(rEntity.ppuBanks, lbl_71014A0410, 0x200);
    AdjustEntityPageTableForPrg(&rEntity, 0xffff);
    AdjustEntityPageTableForChr(&rEntity, 0xffff);
    AdjustEntityPageTableForVRam(&rEntity, 0xffff);
    return &rEntity;
}

} } } }
