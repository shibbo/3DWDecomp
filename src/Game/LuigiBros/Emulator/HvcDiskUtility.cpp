#include "LuigiBros/Emulator/HvcDiskUtility.hpp"
#include <cstring>

extern unsigned char g_nDiskSideMax;

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {

/** @brief Initializes the disk utility's virtual interface. */
CHvcDiskUtility::CHvcDiskUtility() = default;

/** @brief Destroys the disk utility. */
CHvcDiskUtility::~CHvcDiskUtility() = default;

/**
 * @brief Loads the initial disk's boot files into mapped CPU or character memory.
 * @param pEntity Emulator memory and bank state.
 * @return Zero after loading, or 0x31 if the initial side is unavailable.
 */
long CHvcDiskUtility::LoadOnColdStart(CPlatformHvcUnit::_SEntity* pEntity) {
    unsigned char* pMemory = pEntity->pMemory;
    if (pMemory[0x100015] != 0) {
        return 0x31;
    }
    std::memset(pMemory + 0x300200, 0, 0x600);
    unsigned char count = pMemory[0x10003b];
    unsigned char* pBootId = pMemory + 0x100019;
    unsigned char* pFile = pBootId + 0x25;
    while (count != 0) {
        if (pFile[0] != 3 || pFile[0x12] != 4) {
            break;
        }
        unsigned short size = pFile[0xd] | (pFile[0xe] << 8);
        if (static_cast<short>(size - 1) < 0) {
            break;
        }
        if (pFile[2] <= *pBootId) {
            unsigned short address = pFile[0xb] | (pFile[0xc] << 8);
            switch (pFile[0xf]) {
            case 1:
                if (address < 0x2000 && address + size <= 0x2000) {
                    unsigned char* memory = pEntity->pMemory;
                    unsigned long offset = pEntity->banks[16 + (address >> 10)].readOffset;
                    std::memcpy(memory + offset +
                                    (address & 0x3ff), pFile + 0x13, size);
                }
                break;
            case 0:
                if (address >= 0x6000 && address + size <= 0xe000) {
                    std::memcpy(pEntity->pMemory + pEntity->banks[address >> 12].readOffset +
                                    (address & 0xfff), pFile + 0x13, size);
                } else if (address + size <= 0x800) {
                    std::memcpy(pEntity->pMemory + 0x300000 + address, pFile + 0x13, size);
                }
                break;
            }
        }
        pFile += size + 0x15;
        --count;
    }
    return 0;
}

/**
 * @brief Replaces a disk file with data copied from the CPU address space.
 * @param pEntity Emulator memory and bank state.
 * @param pDiskId Eight-byte disk identifier.
 * @param fileIndex Destination file index on the matching side.
 * @param pHeader File metadata and source address descriptor.
 * @return Zero on success, or negative one for missing or invalid disk data.
 */
long CHvcDiskUtility::PurgeSave(CPlatformHvcUnit::_SEntity* pEntity,
                              const unsigned char* pDiskId, unsigned char fileIndex,
                              const unsigned char* pHeader) {
    for (int side = 0; side < g_nDiskSideMax; ++side) {
        unsigned char* pDisk = pEntity->pMemory + 0x100000 + (side << 16);
        if (std::memcmp(pDisk + 0xf, pDiskId, 8) != 0) {
            continue;
        }
        unsigned char* pFirst = pDisk + 0x3e;
        unsigned long offset = 0x3e;
        unsigned char* pFile = pFirst;
        for (unsigned int i = 0; i < fileIndex; ++i) {
            if (pFile[0] != 3) {
                return -1;
            }
            if (offset >= 0x10000) {
                return -1;
            }
            if (pFile[0x12] != 4) {
                return -1;
            }
            unsigned long size = pFile[0xd] | (pFile[0xe] << 8);
            offset += size + 0x15;
            pFile = pFirst + offset - 0x3e;
        }
        unsigned int size = pHeader[0xb] | (pHeader[0xc] << 8);
        if (offset + 0x15 + size >= 0x10000) {
            return -1;
        }
        unsigned int address = pHeader[0xe] | (pHeader[0xf] << 8);
        pFile[1] = fileIndex;
        pFile[0] = 3;
        std::memcpy(pFile + 2, pHeader, 14);
        pFile[0x12] = 4;
        unsigned char* pDestination = pFile + 0x13;
        while (size != 0) {
            unsigned char* pSource = pEntity->pMemory + pEntity->banks[(address >> 12) & 15].readOffset;
            *pDestination++ = pSource[address & 0xfff];
            ++address;
            --size;
        }
        pFirst[-3] = fileIndex + 1;
        return 0;
    }
    return -1;
}

/**
 * @brief Dispatches the supported disk BIOS entry point.
 * @param pRegisters CPU register state.
 * @param address BIOS entry address.
 * @param pEntity Emulator memory and bank state.
 * @return Always false.
 */
bool CHvcDiskUtility::CallSubroutine(_S6502Register* pRegisters, unsigned short address,
                                   CPlatformHvcUnit::_SEntity* pEntity) {
    if (address == 0xe1f8) {
        _Load(pRegisters, pEntity);
    }
    return false;
}

/**
 * @brief Loads requested disk files and updates the emulated BIOS result registers.
 * @param pRegisters CPU registers with the parameter addresses at the program counter.
 * @param pEntity Emulator memory and bank state.
 */
void CHvcDiskUtility::_Load(_S6502Register* pRegisters, CPlatformHvcUnit::_SEntity* pEntity) {
    unsigned short diskAddress = _GetAddress(pRegisters->pc, pEntity);
    unsigned short listAddress = _GetAddress(pRegisters->pc + 2, pEntity);
    unsigned char* pMemory = pEntity->pMemory;
    unsigned char* pHeader = pMemory + pEntity->banks[diskAddress >> 12].readOffset +
                            (diskAddress & 0xfff);
    unsigned char* pDisk = nullptr;
    for (unsigned int side = 0; side < 16; ++side) {
        unsigned char* pCandidate = pMemory + 0x100000 + side * 0x10000;
        if (std::memcmp(pCandidate + 1, pHeader + 1, 14) != 0) {
            break;
        }
        if (std::strncmp(reinterpret_cast<const char*>(pCandidate + 0x10),
                         reinterpret_cast<const char*>(pHeader + 0x10), 4) == 0 &&
            pCandidate[0x15] == pHeader[0x15]) {
            pDisk = pCandidate;
            break;
        }
    }
    if (pDisk == nullptr) {
        pRegisters->pc += 4;
        pRegisters->accumulator = 0x28;
        unsigned char status = pRegisters->status;
        pRegisters->y = 0;
        pRegisters->status = status & ~2;
        return;
    }
    unsigned char* pList = pMemory + pEntity->banks[listAddress >> 12].readOffset +
                          (listAddress & 0xfff);
    bool found = false;
    unsigned char loaded = 0;
    for (unsigned int i = 0; i < 20; ++i) {
        unsigned char fileId = *pList;
        if (fileId == 0xff) {
            break;
        }
        unsigned char* pFile = pDisk + 0x3e;
        for (unsigned int file = 0; file < pDisk[0x3b]; ++file) {
            if (fileId == pFile[2]) {
                unsigned short address = *reinterpret_cast<unsigned short*>(pFile + 0xc);
                unsigned short size = *reinterpret_cast<unsigned short*>(pFile + 0xe);
                std::memcpy(pEntity->pMemory + pEntity->banks[address >> 12].readOffset +
                                (address & 0xfff), pFile + 0x15, size);
                ++loaded;
                found = true;
            }
            pFile += *reinterpret_cast<unsigned short*>(pFile + 0xe) + 0x17;
        }
    }
    pRegisters->pc += 4;
    if (found) {
        unsigned char status = pRegisters->status | 2;
        pRegisters->accumulator = 0;
        pRegisters->y = loaded;
        pRegisters->status = status;
    } else {
        pRegisters->accumulator = 0x28;
        unsigned char status = pRegisters->status;
        pRegisters->y = 0;
        pRegisters->status = status & ~2;
    }
}

/**
 * @brief Reads a little-endian address through the CPU bank mapping.
 * @param address Address of the low byte in CPU memory.
 * @param pEntity Emulator memory and bank state.
 * @return The decoded two-byte address.
 */
unsigned short CHvcDiskUtility::_GetAddress(unsigned short address, CPlatformHvcUnit::_SEntity* pEntity) {
    unsigned short value = 0;
    for (unsigned long i = 0; i < 2; ++i) {
        unsigned long current = address + i;
        unsigned char* pByte = pEntity->pMemory + pEntity->banks[current >> 12].readOffset;
        value |= pByte[current & 0xfff] << (i * 8);
    }
    return value;
}

/** @brief Original platform stub for the Save BIOS hook. */
void CHvcDiskUtility::_Save() {}

/** @brief Original platform stub for the Erase BIOS hook. */
void CHvcDiskUtility::_Erase() {}

/** @brief Original platform stub for the Keep BIOS hook. */
void CHvcDiskUtility::_Keep() {}

/** @brief Original platform stub for the Purge BIOS hook. */
void CHvcDiskUtility::_Purge() {}

/** @brief Original platform stub for the Recover BIOS hook. */
void CHvcDiskUtility::_Recover() {}

/** @brief Original platform stub for the PurgeSave BIOS hook. */
void CHvcDiskUtility::_PurgeSave() {}

/** @brief Original platform stub for the DumpDirectory BIOS hook. */
void CHvcDiskUtility::_DumpDirectory() {}

/** @brief Original platform stub for the DumpFullDirectory BIOS hook. */
void CHvcDiskUtility::_DumpFullDirectory() {}

/** @brief Original platform stub for the WriteVram BIOS hook. */
void CHvcDiskUtility::_WriteVram() {}

/** @brief Original platform stub for the ReserveWriteBgData BIOS hook. */
void CHvcDiskUtility::_ReserveWriteBgData() {}

/** @brief Original platform stub for the ReserveWriteCharacterData BIOS hook. */
void CHvcDiskUtility::_ReserveWriteCharacterData() {}

/** @brief Original platform stub for the ReadVram BIOS hook. */
void CHvcDiskUtility::_ReadVram() {}

/** @brief Original platform stub for the ReserveReadBgData BIOS hook. */
void CHvcDiskUtility::_ReserveReadBgData() {}

/** @brief Original platform stub for the ConvertObgHVToBgAddress BIOS hook. */
void CHvcDiskUtility::_ConvertObgHVToBgAddress() {}

/** @brief Original platform stub for the ConvertBgAddressToObgHV BIOS hook. */
void CHvcDiskUtility::_ConvertBgAddressToObgHV() {}

/** @brief Original platform stub for the JumpIndex BIOS hook. */
void CHvcDiskUtility::_JumpIndex() {}

/** @brief Original platform stub for the WaitNmi BIOS hook. */
void CHvcDiskUtility::_WaitNmi() {}

/** @brief Original platform stub for the BlankObjBg BIOS hook. */
void CHvcDiskUtility::_BlankObjBg() {}

/** @brief Original platform stub for the AppearObjBG BIOS hook. */
void CHvcDiskUtility::_AppearObjBG() {}

/** @brief Original platform stub for the BlankObj BIOS hook. */
void CHvcDiskUtility::_BlankObj() {}

/** @brief Original platform stub for the AppearObj BIOS hook. */
void CHvcDiskUtility::_AppearObj() {}

/** @brief Original platform stub for the BlankBg BIOS hook. */
void CHvcDiskUtility::_BlankBg() {}

/** @brief Original platform stub for the AppearBg BIOS hook. */
void CHvcDiskUtility::_AppearBg() {}

/** @brief Original platform stub for the RefreshTimer BIOS hook. */
void CHvcDiskUtility::_RefreshTimer() {}

/** @brief Original platform stub for the ClearVram BIOS hook. */
void CHvcDiskUtility::_ClearVram() {}

/** @brief Original platform stub for the ClearWram BIOS hook. */
void CHvcDiskUtility::_ClearWram() {}

/** @brief Original platform stub for the InputController1 BIOS hook. */
void CHvcDiskUtility::_InputController1() {}

/** @brief Original platform stub for the InputController3 BIOS hook. */
void CHvcDiskUtility::_InputController3() {}

/** @brief Original platform stub for the InputController4 BIOS hook. */
void CHvcDiskUtility::_InputController4() {}

/** @brief Original platform stub for the InputController5 BIOS hook. */
void CHvcDiskUtility::_InputController5() {}

/** @brief Original platform stub for the SenseKeyboard BIOS hook. */
void CHvcDiskUtility::_SenseKeyboard() {}

/** @brief Original platform stub for the SetOamForDmaTransfer BIOS hook. */
void CHvcDiskUtility::_SetOamForDmaTransfer() {}

/** @brief Original platform stub for the SetScrollCounter BIOS hook. */
void CHvcDiskUtility::_SetScrollCounter() {}

/** @brief Original platform stub for the InputController2 BIOS hook. */
void CHvcDiskUtility::_InputController2() {}

/** @brief Original platform stub for the SetAreaSoftTimer BIOS hook. */
void CHvcDiskUtility::_SetAreaSoftTimer() {}

/** @brief Original platform stub for the SetAreaRandom BIOS hook. */
void CHvcDiskUtility::_SetAreaRandom() {}

/** @brief Original platform stub for the TransferOamByDma BIOS hook. */
void CHvcDiskUtility::_TransferOamByDma() {}

/** @brief Original platform stub for the SetMilliSecondTimer BIOS hook. */
void CHvcDiskUtility::_SetMilliSecondTimer() {}

/**
 * @brief Original platform stub for the TransferCharacterRamData BIOS hook.
 * @param pRegisters Unused CPU register state.
 * @param pEntity Unused emulator state.
 */
void CHvcDiskUtility::_TransferCharacterRamData(_S6502Register* pRegisters, CPlatformHvcUnit::_SEntity* pEntity) {}

/**
 * @brief Original platform stub for the TransferVram BIOS hook.
 * @param pRegisters Unused CPU register state.
 * @param pEntity Unused emulator state.
 */
void CHvcDiskUtility::_TransferVram(_S6502Register* pRegisters, CPlatformHvcUnit::_SEntity* pEntity) {}

} } } }
