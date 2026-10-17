#include "LuigiBros/Emulator/HvcMmc.hpp"
#include <cstring>
#include "LuigiBros/Common/Serialize.hpp"

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
static const CPlatformHvcUnit::_SBankAccess diskCpuBankDefaults[32] = {
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x2, 0x0, 0x0, 0x0, 0x303800},
    {0x2, 0x0, 0x0, 0x0, 0x303808},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x303810},
    {0x2, 0x0, 0x0, 0x0, 0x303a10},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x2000},
    {0x1, 0x0, 0xfff, 0x0, 0x2000},
    {0x1, 0x0, 0xfff, 0x0, 0x3000},
    {0x1, 0x0, 0xfff, 0x0, 0x3000},
    {0x1, 0x0, 0xfff, 0x0, 0x4000},
    {0x1, 0x0, 0xfff, 0x0, 0x4000},
    {0x1, 0x0, 0xfff, 0x0, 0x5000},
    {0x1, 0x0, 0xfff, 0x0, 0x5000},
    {0x1, 0x0, 0xfff, 0x0, 0x6000},
    {0x1, 0x0, 0xfff, 0x0, 0x6000},
    {0x1, 0x0, 0xfff, 0x0, 0x7000},
    {0x1, 0x0, 0xfff, 0x0, 0x7000},
    {0x1, 0x0, 0xfff, 0x0, 0x8000},
    {0x1, 0x0, 0xfff, 0x0, 0x8000},
    {0x1, 0x0, 0xfff, 0x0, 0x9000},
    {0x1, 0x0, 0xfff, 0x0, 0x9000},
    {0x1, 0x0, 0xfff, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x1000},
    {0x1, 0x0, 0xfff, 0x0, 0x1000},
};
static const unsigned char diskDevices[160] = {
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 4, 5, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1,
    5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 1, 5, 5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1,
    5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 1, 1,
};
static const CPlatformHvcUnit::_SBankAccess mmc5CpuBankDefaults[32] = {
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x2, 0x0, 0x0, 0x0, 0x303800},
    {0x2, 0x0, 0x0, 0x0, 0x303808},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x303810},
    {0x2, 0x0, 0x0, 0x0, 0x303a10},
    {0x2, 0x0, 0x0, 0x0, 0x303c10},
    {0x2, 0x0, 0x0, 0x0, 0x304010},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x1000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x2000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x3000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x4000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x5000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x6000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x7000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
};
static const CPlatformHvcUnit::_SBankAccess mmc5PpuBankDefaults[32] = {
    {0x1, 0x0, 0x3ff, 0x0, 0x200000},
    {0x1, 0x0, 0x3ff, 0x0, 0x200000},
    {0x1, 0x0, 0x3ff, 0x0, 0x200400},
    {0x1, 0x0, 0x3ff, 0x0, 0x200400},
    {0x1, 0x0, 0x3ff, 0x0, 0x200800},
    {0x1, 0x0, 0x3ff, 0x0, 0x200800},
    {0x1, 0x0, 0x3ff, 0x0, 0x200c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x200c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x201000},
    {0x1, 0x0, 0x3ff, 0x0, 0x201000},
    {0x1, 0x0, 0x3ff, 0x0, 0x201400},
    {0x1, 0x0, 0x3ff, 0x0, 0x201400},
    {0x1, 0x0, 0x3ff, 0x0, 0x201800},
    {0x1, 0x0, 0x3ff, 0x0, 0x201800},
    {0x1, 0x0, 0x3ff, 0x0, 0x201c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x201c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x303000},
    {0x1, 0x0, 0x3ff, 0x0, 0x303000},
    {0x1, 0x0, 0x3ff, 0x0, 0x303400},
    {0x1, 0x0, 0x3ff, 0x0, 0x303400},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
};
static const CPlatformHvcUnit::_SBankAccess cpuBankDefaults[32] = {
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x2, 0x0, 0x0, 0x0, 0x303800},
    {0x2, 0x0, 0x0, 0x0, 0x303808},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x303810},
    {0x2, 0x0, 0x0, 0x0, 0x303a10},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x1000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x2000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x3000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x4000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x5000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x6000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x7000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
};
static const CPlatformHvcUnit::_SBankAccess ppuBankDefaults[32] = {
    {0x1, 0x0, 0x3ff, 0x0, 0x200000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x200400},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x200800},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x200c00},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x201000},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x201400},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x201800},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x201c00},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
};
/** @brief Initializes the unmapped cartridge controller identity. */
CHvcMmc::CHvcMmc() {
    _18 = 1;
    mScale = 1;
    mpEntity = nullptr;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - none ", 15);
}
/** @brief Releases the mapper's platform base. */
CHvcMmc::~CHvcMmc() {}
/** @brief Supplies the empty mapper initialization hook. @return Zero. */
unsigned int CHvcMmc::_Prologue() { return 0; }
/** @brief Supplies the empty mapper finalization hook. @return Zero. */
unsigned int CHvcMmc::_Epilogue() { return 0; }
/** @brief Installs default banks or identifies the unit responsible for an address.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Zero to initialize, two to resolve a device.
 * @param args Address and output device buffer. @return Zero.
 */
unsigned int CHvcMmc::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, cpuBankDefaults, sizeof(cpuBankDefaults));
        memcpy(mpEntity->ppuBanks, ppuBankDefaults, sizeof(ppuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes CPU or PPU memory access through the selected bank.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read or write bank selector. @param args Bus arguments.
 * @return Bus result or six for an unsupported bank.
 */
unsigned int CHvcMmc::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Acknowledges the mapper reset request.
 * @param sender Unused sender. @param category Unused category.
 * @param operation Unused operation. @param args Unused arguments. @return Zero.
 */
unsigned int CHvcMmc::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (mpEntity->order.words[0] & (1u << _18))
        mpEntity->order.words[0] &= ~(1u << _18);
    return 0;
}
/** @brief Reports the absence of mapper-specific state. @return Zero. */
unsigned long CHvcMmc::_SerializeCoreInferSize() { return 0; }
/** @brief Identifies mapper state. @return One. */
unsigned char CHvcMmc::_SerializeTagContent() { return 1; }
/** @brief Identifies the default mapper. @return Zero. */
unsigned char CHvcMmc::_SerializeTagSpecies() { return 0; }
/** @brief Returns the mapper save-state version. @return Version 0x100. */
unsigned short CHvcMmc::_SerializeTagVersion() { return 0x100; }
/** @brief Returns the mapper state label. @return Mapper identifier text. */
const unsigned char* CHvcMmc::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC    ");
}
static const CPlatformHvcUnit::_SBankAccess prgCpuBankDefaults[32] = {
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x2, 0x0, 0x0, 0x0, 0x303800},
    {0x2, 0x0, 0x0, 0x0, 0x303808},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x303810},
    {0x2, 0x0, 0x0, 0x0, 0x303a10},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x1000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x2000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x3000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x4000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x5000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x6000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x7000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
};
static const CPlatformHvcUnit::_SBankAccess prgPpuBankDefaults[32] = {
    {0x1, 0x0, 0x3ff, 0x0, 0x200000},
    {0x1, 0x0, 0x3ff, 0x0, 0x200000},
    {0x1, 0x0, 0x3ff, 0x0, 0x200400},
    {0x1, 0x0, 0x3ff, 0x0, 0x200400},
    {0x1, 0x0, 0x3ff, 0x0, 0x200800},
    {0x1, 0x0, 0x3ff, 0x0, 0x200800},
    {0x1, 0x0, 0x3ff, 0x0, 0x200c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x200c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x201000},
    {0x1, 0x0, 0x3ff, 0x0, 0x201000},
    {0x1, 0x0, 0x3ff, 0x0, 0x201400},
    {0x1, 0x0, 0x3ff, 0x0, 0x201400},
    {0x1, 0x0, 0x3ff, 0x0, 0x201800},
    {0x1, 0x0, 0x3ff, 0x0, 0x201800},
    {0x1, 0x0, 0x3ff, 0x0, 0x201c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x201c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
};
/** @brief Initializes the program-ROM bank controller identity. */
CHvcMmcPrg::CHvcMmcPrg() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - Prg  ", 15);
}
/** @brief Clears the selected bank. @return Zero. */
unsigned int CHvcMmcPrg::_Prologue() { mBank = 0; return 0; }
/** @brief Supplies the empty finalization hook. @return Zero. */
unsigned int CHvcMmcPrg::_Epilogue() { return 0; }
/** @brief Installs default banks or identifies the unit responsible for an address.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Zero to initialize, two to resolve a device.
 * @param args Address and output device buffer. @return Zero.
 */
unsigned int CHvcMmcPrg::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);

    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Reads mapped memory or updates the four switchable program pages.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read/write selector. @param args Bus arguments.
 * @return Bus status, zero for a bank write, or six for unsupported access.
 */
unsigned int CHvcMmcPrg::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            unsigned long bank = *static_cast<unsigned char*>(args->pointers[4]) & 7;
            mBank = bank;
            unsigned long offset = bank << 14;
            mpEntity->cpuBanks[8].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[9].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[10].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[11].readOffset = offset;
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Restores the reset bank selector and acknowledges reset.
 * @param sender Unused sender. @param category Unused category.
 * @param operation Unused operation. @param args Unused arguments. @return Zero.
 */
unsigned int CHvcMmcPrg::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (mpEntity->order.words[0] & (1u << _18)) {
        mBank = mpEntity->prgBankCount - 2;
        mpEntity->order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Transfers the selected program bank.
 * @param buffer State buffer or null to measure. @param reading Restore direction.
 * @return One byte of state.
 */
unsigned long CHvcMmcPrg::_SerializeCore(void* buffer, bool reading) {
    if (buffer) {
        unsigned char* input = reading ? static_cast<unsigned char*>(buffer) : &mBank;
        unsigned char* output = reading ? &mBank : static_cast<unsigned char*>(buffer);
        *output = *input;
    }
    return 1;
}
/** @brief Exports the bank selector. @param buffer State output. */
void CHvcMmcPrg::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Restores the bank selector. @param buffer State input. */
void CHvcMmcPrg::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Returns aligned mapper state size. @return Sixteen bytes. */
unsigned long CHvcMmcPrg::_SerializeCoreInferSize() { return 16; }
/** @brief Identifies the program-bank mapper species. @return One. */
unsigned char CHvcMmcPrg::_SerializeTagSpecies() { return 1; }
/** @brief Returns the program-bank state label. @return Mapper identifier. */
const unsigned char* CHvcMmcPrg::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-Prg");
}
static const CPlatformHvcUnit::_SBankAccess chrCpuBankDefaults[32] = {
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x1, 0x0, 0x7ff, 0x0, 0x300000},
    {0x2, 0x0, 0x0, 0x0, 0x303800},
    {0x2, 0x0, 0x0, 0x0, 0x303808},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x303810},
    {0x2, 0x0, 0x0, 0x0, 0x303a10},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x1000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x2000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x3000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x4000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x5000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x6000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xfff, 0x0, 0x7000},
    {0x2, 0x0, 0x0, 0x0, 0x0},
};
static const CPlatformHvcUnit::_SBankAccess chrPpuBankDefaults[32] = {
    {0x1, 0x0, 0x3ff, 0x0, 0x200000},
    {0x0, 0x0, 0x3ff, 0x0, 0x200000},
    {0x1, 0x0, 0x3ff, 0x0, 0x200400},
    {0x0, 0x0, 0x3ff, 0x0, 0x200400},
    {0x1, 0x0, 0x3ff, 0x0, 0x200800},
    {0x0, 0x0, 0x3ff, 0x0, 0x200800},
    {0x1, 0x0, 0x3ff, 0x0, 0x200c00},
    {0x0, 0x0, 0x3ff, 0x0, 0x200c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x201000},
    {0x0, 0x0, 0x3ff, 0x0, 0x201000},
    {0x1, 0x0, 0x3ff, 0x0, 0x201400},
    {0x0, 0x0, 0x3ff, 0x0, 0x201400},
    {0x1, 0x0, 0x3ff, 0x0, 0x201800},
    {0x0, 0x0, 0x3ff, 0x0, 0x201800},
    {0x1, 0x0, 0x3ff, 0x0, 0x201c00},
    {0x0, 0x0, 0x3ff, 0x0, 0x201c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302800},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x1, 0x0, 0x3ff, 0x0, 0x302c00},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0xff, 0x0, 0x304410},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
    {0x1, 0x0, 0x1f, 0x0, 0x304510},
};
/** @brief Initializes the character-ROM bank controller identity. */
CHvcMmcChr::CHvcMmcChr() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - Chr  ", 15);
}
/** @brief Clears the selected bank. @return Zero. */
unsigned int CHvcMmcChr::_Prologue() { mBank = 0; return 0; }
/** @brief Supplies the empty finalization hook. @return Zero. */
unsigned int CHvcMmcChr::_Epilogue() { return 0; }
/** @brief Installs default banks or identifies the unit responsible for an address.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Zero to initialize, two to resolve a device.
 * @param args Address and output device buffer. @return Zero.
 */
unsigned int CHvcMmcChr::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, chrCpuBankDefaults, sizeof(chrCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, chrPpuBankDefaults, sizeof(chrPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);

    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Reads mapped memory or updates the four switchable character pages.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read/write selector. @param args Bus arguments.
 * @return Bus status, zero for a bank write, or six for unsupported access.
 */
unsigned int CHvcMmcChr::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mBank = *static_cast<unsigned char*>(args->pointers[4]) & 3;
            switch (mpEntity->chrBankCount) {
            case 1: mBank &= 0; break;
            case 2: mBank &= 1; break;
            case 3: mBank &= 1; break;
            case 4: mBank &= 3; break;
            default: mBank = 0; break;
            }
            unsigned long bank = mBank;
            unsigned long offset = (bank << 13) + 0x200000;
            mpEntity->ppuBanks[0].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[1].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[2].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[3].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[4].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[5].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[6].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[7].readOffset = offset;
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Restores the reset bank selector and acknowledges reset.
 * @param sender Unused sender. @param category Unused category.
 * @param operation Unused operation. @param args Unused arguments. @return Zero.
 */
unsigned int CHvcMmcChr::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (mpEntity->order.words[0] & (1u << _18)) {
        mBank = mpEntity->chrBankCount - 1;
        mpEntity->order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Transfers the selected character bank.
 * @param buffer State buffer or null to measure. @param reading Restore direction.
 * @return One byte of state.
 */
unsigned long CHvcMmcChr::_SerializeCore(void* buffer, bool reading) {
    if (buffer) {
        unsigned char* input = reading ? static_cast<unsigned char*>(buffer) : &mBank;
        unsigned char* output = reading ? &mBank : static_cast<unsigned char*>(buffer);
        *output = *input;
    }
    return 1;
}
/** @brief Exports the bank selector. @param buffer State output. */
void CHvcMmcChr::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Restores the bank selector. @param buffer State input. */
void CHvcMmcChr::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Returns aligned mapper state size. @return Sixteen bytes. */
unsigned long CHvcMmcChr::_SerializeCoreInferSize() { return 16; }
/** @brief Identifies the character-bank mapper species. @return Two. */
unsigned char CHvcMmcChr::_SerializeTagSpecies() { return 2; }
/** @brief Returns the character-bank state label. @return Mapper identifier. */
const unsigned char* CHvcMmcChr::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-Chr");
}
/** @brief Initializes the serial mapper identity. */
CHvcMmc1::CHvcMmc1() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Initializes the serial registers and initial bit position. @return Zero. */
unsigned int CHvcMmc1::_Prologue() {
    mControl = 0x0c; mChrBank0 = 0; mChrBank1 = 0; mPrgBank = 0x10;
    mShiftCount = 4; mShiftValue = 0;
    return 0;
}
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMmc1::_Epilogue() { return 0; }
/** @brief Installs default banks or identifies the unit responsible for an address.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Zero to initialize, two to resolve a device.
 * @param args Address and output device buffer. @return Zero.
 */
unsigned int CHvcMmc1::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes memory accesses and collects five-bit serial register writes.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read/write selector. @param args Bus arguments.
 * @return Bus status, zero for a register write, or six for unsupported access.
 */
unsigned int CHvcMmc1::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    unsigned int result = 6;
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            result = 0;
            if (args->values[2] >= 0x8000) {
                unsigned char value = *static_cast<unsigned char*>(args->pointers[4]);
                if (value & 0x80) {
                    mControl |= 0x0c;
                } else {
                    mShiftValue |= (value & 1) << mShiftCount;
                    ++mShiftCount;
                    if (mShiftCount < 5) goto finished;
                    switch ((args->values[2] >> 13) & 3) {
                    case 0: mControl = mShiftValue; break;
                    case 1: mChrBank0 = mShiftValue; break;
                    case 2: mChrBank1 = mShiftValue; break;
                    default: mPrgBank = mShiftValue; break;
                    }
                }
                mShiftCount = 0; mShiftValue = 0;
                _AdjustBusMap();
            }
            goto finished;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
finished:
    return result;
}
/** @brief Applies mirroring and program/character bank registers to bus pages. */
void CHvcMmc1::_AdjustBusMap() {
    unsigned long mirror = mControl & 1;
    if (!(mControl & 2)) {
        unsigned long offset = 0x302800 + (mirror << 10);
        mpEntity->ppuBanks[11].writeOffset = offset;
        mpEntity->ppuBanks[11].readOffset = offset;
        mpEntity->ppuBanks[10].writeOffset = offset;
        mpEntity->ppuBanks[10].readOffset = offset;
        mpEntity->ppuBanks[9].writeOffset = offset;
        mpEntity->ppuBanks[9].readOffset = offset;
        mpEntity->ppuBanks[8].writeOffset = offset;
        mpEntity->ppuBanks[8].readOffset = offset;
    } else {
        mpEntity->ppuBanks[8].writeOffset = 0x302800;
        mpEntity->ppuBanks[8].readOffset = 0x302800;
        if (mirror) {
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
        } else {
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
        }
        mpEntity->ppuBanks[11].writeOffset = 0x302c00;
        mpEntity->ppuBanks[11].readOffset = 0x302c00;
    }
    unsigned long offset = static_cast<unsigned long>(mPrgBank) << 14;
    if (mControl & 8) {
        offset &= 0x3c000;
        if (!(mControl & 4)) {
            mpEntity->cpuBanks[12].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[13].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[14].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[15].readOffset = offset;
            offset = static_cast<unsigned long>(mpEntity->prgBankCount) << 14;
            mpEntity->cpuBanks[11].readOffset = offset - 0x5000;
            mpEntity->cpuBanks[10].readOffset = offset - 0x6000;
            mpEntity->cpuBanks[9].readOffset = offset - 0x7000;
            mpEntity->cpuBanks[8].readOffset = offset - 0x8000;
        } else {
            mpEntity->cpuBanks[8].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[9].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[10].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[11].readOffset = offset;
            offset = static_cast<unsigned long>(mpEntity->prgBankCount) << 14;
            mpEntity->cpuBanks[15].readOffset = offset - 0x1000;
            mpEntity->cpuBanks[14].readOffset = offset - 0x2000;
            mpEntity->cpuBanks[13].readOffset = offset - 0x3000;
            mpEntity->cpuBanks[12].readOffset = offset - 0x4000;

        }
    } else {
        offset &= 0x38000;
        mpEntity->cpuBanks[8].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[9].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[10].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[11].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[12].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[13].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[14].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[15].readOffset = offset;
    }
    if (mControl & 0x10) {
        if (!mpEntity->chrBankCount) { mChrBank0 &= 1; mChrBank1 &= 1; }
        offset = 0x200000 + (static_cast<unsigned long>(mChrBank0 & 31) << 12);
        mpEntity->ppuBanks[0].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[1].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[2].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[3].readOffset = offset;
        offset = 0x200000 + (static_cast<unsigned long>(mChrBank1 & 31) << 12);
        mpEntity->ppuBanks[4].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[5].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[6].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[7].readOffset = offset;
    } else {
        if (!mpEntity->chrBankCount) { mChrBank0 = 0; mChrBank1 = 0; }
        offset = 0x200000 + (static_cast<unsigned long>((mChrBank0 >> 1) & 15) << 13);
        mpEntity->ppuBanks[0].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[1].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[2].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[3].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[4].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[5].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[6].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[7].readOffset = offset;
    }
}
/** @brief Resets the serial registers, applies mapping, and acknowledges reset.
 * @param sender Unused sender. @param category Unused category.
 * @param operation Unused operation. @param args Unused arguments. @return Zero.
 */
unsigned int CHvcMmc1::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (mpEntity->order.words[0] & (1u << _18)) {
        mControl = 0x0c; mChrBank0 = 0; mChrBank1 = 0; mPrgBank = 0x10;
        mShiftCount = 0; mShiftValue = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Transfers the six serial-mapper state bytes.
 * @param buffer State buffer or null to measure. @param reading Restore direction.
 * @return Six bytes.
 */
unsigned long CHvcMmc1::_SerializeCore(void* buffer, bool reading) {
    unsigned char* data = static_cast<unsigned char*>(buffer);
    if (data) {
        if (reading) {
            mControl = data[0];
            mChrBank0 = data[1];
            mChrBank1 = data[2];
            mPrgBank = data[3];
            mShiftCount = data[4];
            mShiftValue = data[5];
        } else {
            data[0] = mControl;
            data[1] = mChrBank0;
            data[2] = mChrBank1;
            data[3] = mPrgBank;
            data[4] = mShiftCount;
            data[5] = mShiftValue;
        }
    }
    return 6;
}
/** @brief Exports serial mapper state. @param buffer State output. */
void CHvcMmc1::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Restores serial mapper state. @param buffer State input. */
void CHvcMmc1::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Returns aligned serial mapper state size. @return Sixteen bytes. */
unsigned long CHvcMmc1::_SerializeCoreInferSize() { return 16; }
/** @brief Identifies the serial mapper state species. @return Three. */
unsigned char CHvcMmc1::_SerializeTagSpecies() { return 3; }
/** @brief Returns the serial mapper state label. @return Mapper identifier. */
const unsigned char* CHvcMmc1::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-1  ");
}
/** @brief Initializes the latched mapper identity used by the original port. */
CHvcMmc2::CHvcMmc2() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMmc2::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMmc2::_Epilogue() { return 0; }
/** @brief Initializes bus tables, resolves devices, or handles character-latch notifications.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Initialization, routing, or latch-notification operation.
 * @param args Operation arguments. @return Zero.
 */
unsigned int CHvcMmc2::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    switch (operation) {
    case 0: {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
        break;
    }
    case 2: {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
        break;
    }
    case 3: {
        if (args->values[0] == 4 && args->values[1] == 1)
            _ApplyLatchChrBank(static_cast<unsigned short>(args->values[2]), true);
        break;
    }
    }
    return 0;
}
/** @brief Selects character read descriptors when a latch address is observed.
 * @param address Latch address selector. @param enabled Whether to apply it.
 */
void CHvcMmc2::_ApplyLatchChrBank(unsigned short address, bool enabled) {
    if (enabled) {
        switch (address) {
        case 0xfd:
            mChrLatch[0] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[0].readOffset = entity->latchBanks[0][0].readOffset;
                entity->ppuBanks[0]._00 = entity->latchBanks[0][0]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[1].readOffset = entity->latchBanks[0][1].readOffset;
                entity->ppuBanks[1]._00 = entity->latchBanks[0][1]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[2].readOffset = entity->latchBanks[0][2].readOffset;
                entity->ppuBanks[2]._00 = entity->latchBanks[0][2]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[3].readOffset = entity->latchBanks[0][3].readOffset;
                entity->ppuBanks[3]._00 = entity->latchBanks[0][3]._00;
            }
            return;
        case 0xfe:
            mChrLatch[0] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[0].readOffset = entity->latchBanks[1][0].readOffset;
                entity->ppuBanks[0]._00 = entity->latchBanks[1][0]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[1].readOffset = entity->latchBanks[1][1].readOffset;
                entity->ppuBanks[1]._00 = entity->latchBanks[1][1]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[2].readOffset = entity->latchBanks[1][2].readOffset;
                entity->ppuBanks[2]._00 = entity->latchBanks[1][2]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[3].readOffset = entity->latchBanks[1][3].readOffset;
                entity->ppuBanks[3]._00 = entity->latchBanks[1][3]._00;
            }
            return;
        case 0x1fd:
            mChrLatch[1] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[4].readOffset = entity->latchBanks[0][4].readOffset;
                entity->ppuBanks[4]._00 = entity->latchBanks[0][4]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[5].readOffset = entity->latchBanks[0][5].readOffset;
                entity->ppuBanks[5]._00 = entity->latchBanks[0][5]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[6].readOffset = entity->latchBanks[0][6].readOffset;
                entity->ppuBanks[6]._00 = entity->latchBanks[0][6]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[7].readOffset = entity->latchBanks[0][7].readOffset;
                entity->ppuBanks[7]._00 = entity->latchBanks[0][7]._00;
            }
            return;
        case 0x1fe:
            mChrLatch[1] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[4].readOffset = entity->latchBanks[1][4].readOffset;
                entity->ppuBanks[4]._00 = entity->latchBanks[1][4]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[5].readOffset = entity->latchBanks[1][5].readOffset;
                entity->ppuBanks[5]._00 = entity->latchBanks[1][5]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[6].readOffset = entity->latchBanks[1][6].readOffset;
                entity->ppuBanks[6]._00 = entity->latchBanks[1][6]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[7].readOffset = entity->latchBanks[1][7].readOffset;
                entity->ppuBanks[7]._00 = entity->latchBanks[1][7]._00;
            }
            return;
        }
    }
}
/** @brief Transfers registers, mapper timing state, and both character latches.
 * @param buffer State buffer or null to measure. @param reading Restore direction.
 * @return Bytes transferred.
 */
unsigned long CHvcMmc2::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mRegisters[2]);
    stream.Entry(mRegisters[3]);
    stream.Entry(mRegisters[4]);
    stream.Entry(mRegisters[5]);
    stream.Entry(mRegisters[6]);
    stream.Entry(mWriteAddress);
    stream.Entry(mChrLatch[0]);
    stream.Entry(mChrLatch[1]);
    return stream.GetSize();
}
/** @brief Exports latched mapper state. @param buffer State output. */
void CHvcMmc2::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Restores latched mapper state. @param buffer State input. */
void CHvcMmc2::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned state storage. @return Aligned state size. */
unsigned long CHvcMmc2::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 19) & ~15u;
}
/** @brief Routes mapped accesses or captures a mapper-register write.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read/write selector. @param args Bus arguments.
 * @return Bus status, zero for an accepted write, or six for unsupported access.
 */
unsigned int CHvcMmc2::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[6] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Identifies latched-mapper state. @return Four. */
unsigned char CHvcMmc2::_SerializeTagSpecies() { return 4; }
/** @brief Returns the latched-mapper state label. @return Mapper identifier. */
const unsigned char* CHvcMmc2::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-2  ");
}
/** @brief Applies the captured write to program banks, character latches, or mirroring. */
void CHvcMmc2::_AdjustBusMap() {
    switch (mWriteAddress & 0xf000) {
    case 0xa000: {
        unsigned long value = mRegisters[6];
        mRegisters[0] = value;
        unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
        unsigned long offset = bank << 13;
        mpEntity->cpuBanks[8].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[9].readOffset = offset;
        break;
    }
    case 0xb000: {
        unsigned long value = mRegisters[6];
        mRegisters[1] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[0][0].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][1].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][2].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][3].readOffset = offset;
        _ApplyLatchChrBank(0xfd, mChrLatch[0] == 0xfd);
        break;
    }
    case 0xc000: {
        unsigned long value = mRegisters[6];
        mRegisters[2] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[1][0].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][1].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][2].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][3].readOffset = offset;
        _ApplyLatchChrBank(0xfe, mChrLatch[0] == 0xfe);
        break;
    }
    case 0xd000: {
        unsigned long value = mRegisters[6];
        mRegisters[3] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[0][4].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][5].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][6].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][7].readOffset = offset;
        _ApplyLatchChrBank(0x1fd, mChrLatch[1] == 0x1fd);
        break;
    }
    case 0xe000: {
        unsigned long value = mRegisters[6];
        mRegisters[4] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[1][4].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][5].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][6].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][7].readOffset = offset;
        _ApplyLatchChrBank(0x1fe, mChrLatch[1] == 0x1fe);
        break;
    }
    case 0xf000: {
        unsigned char value = mRegisters[6];
        mRegisters[5] = value;
        mpEntity->ppuBanks[8].writeOffset = 0x302800;
        mpEntity->ppuBanks[8].readOffset = 0x302800;
        if (value & 1) {
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
        } else {
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
        }
        mpEntity->ppuBanks[11].writeOffset = 0x302c00;
        mpEntity->ppuBanks[11].readOffset = 0x302c00;
        break;
    }
    }
}
/** @brief Copies both latch tables and restores initial character selectors on reset.
 * @param sender Unused sender. @param category Unused category.
 * @param operation Unused operation. @param args Unused arguments. @return Zero.
 */
unsigned int CHvcMmc2::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* resetEntity = mpEntity;
    if ((1u << _18) & resetEntity->order.words[0]) {
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][0].readOffset = entity->ppuBanks[0].readOffset;
            entity->latchBanks[0][0]._00 = entity->ppuBanks[0]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][0].writeOffset = entity->ppuBanks[0].writeOffset;
            entity->latchBanks[0][0]._10 = entity->ppuBanks[0]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][0].readOffset = entity->ppuBanks[0].readOffset;
            entity->latchBanks[1][0]._00 = entity->ppuBanks[0]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][0].writeOffset = entity->ppuBanks[0].writeOffset;
            entity->latchBanks[1][0]._10 = entity->ppuBanks[0]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][1].readOffset = entity->ppuBanks[1].readOffset;
            entity->latchBanks[0][1]._00 = entity->ppuBanks[1]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][1].writeOffset = entity->ppuBanks[1].writeOffset;
            entity->latchBanks[0][1]._10 = entity->ppuBanks[1]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][1].readOffset = entity->ppuBanks[1].readOffset;
            entity->latchBanks[1][1]._00 = entity->ppuBanks[1]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][1].writeOffset = entity->ppuBanks[1].writeOffset;
            entity->latchBanks[1][1]._10 = entity->ppuBanks[1]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][2].readOffset = entity->ppuBanks[2].readOffset;
            entity->latchBanks[0][2]._00 = entity->ppuBanks[2]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][2]._10 = entity->ppuBanks[2]._10;
            entity->latchBanks[0][2].writeOffset = entity->ppuBanks[2].writeOffset;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][2]._00 = entity->ppuBanks[2]._00;
            entity->latchBanks[1][2].readOffset = entity->ppuBanks[2].readOffset;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][2].writeOffset = entity->ppuBanks[2].writeOffset;
            entity->latchBanks[1][2]._10 = entity->ppuBanks[2]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][3].readOffset = entity->ppuBanks[3].readOffset;
            entity->latchBanks[0][3]._00 = entity->ppuBanks[3]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][3].writeOffset = entity->ppuBanks[3].writeOffset;
            entity->latchBanks[0][3]._10 = entity->ppuBanks[3]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][3].readOffset = entity->ppuBanks[3].readOffset;
            entity->latchBanks[1][3]._00 = entity->ppuBanks[3]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][3].writeOffset = entity->ppuBanks[3].writeOffset;
            entity->latchBanks[1][3]._10 = entity->ppuBanks[3]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][4].readOffset = entity->ppuBanks[4].readOffset;
            entity->latchBanks[0][4]._00 = entity->ppuBanks[4]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][4].writeOffset = entity->ppuBanks[4].writeOffset;
            entity->latchBanks[0][4]._10 = entity->ppuBanks[4]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][4].readOffset = entity->ppuBanks[4].readOffset;
            entity->latchBanks[1][4]._00 = entity->ppuBanks[4]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][4].writeOffset = entity->ppuBanks[4].writeOffset;
            entity->latchBanks[1][4]._10 = entity->ppuBanks[4]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][5].readOffset = entity->ppuBanks[5].readOffset;
            entity->latchBanks[0][5]._00 = entity->ppuBanks[5]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][5].writeOffset = entity->ppuBanks[5].writeOffset;
            entity->latchBanks[0][5]._10 = entity->ppuBanks[5]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][5].readOffset = entity->ppuBanks[5].readOffset;
            entity->latchBanks[1][5]._00 = entity->ppuBanks[5]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][5].writeOffset = entity->ppuBanks[5].writeOffset;
            entity->latchBanks[1][5]._10 = entity->ppuBanks[5]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][6].readOffset = entity->ppuBanks[6].readOffset;
            entity->latchBanks[0][6]._00 = entity->ppuBanks[6]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][6].writeOffset = entity->ppuBanks[6].writeOffset;
            entity->latchBanks[0][6]._10 = entity->ppuBanks[6]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][6].readOffset = entity->ppuBanks[6].readOffset;
            entity->latchBanks[1][6]._00 = entity->ppuBanks[6]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][6].writeOffset = entity->ppuBanks[6].writeOffset;
            entity->latchBanks[1][6]._10 = entity->ppuBanks[6]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][7].readOffset = entity->ppuBanks[7].readOffset;
            entity->latchBanks[0][7]._00 = entity->ppuBanks[7]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][7].writeOffset = entity->ppuBanks[7].writeOffset;
            entity->latchBanks[0][7]._10 = entity->ppuBanks[7]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][7].readOffset = entity->ppuBanks[7].readOffset;
            entity->latchBanks[1][7]._00 = entity->ppuBanks[7]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][7].writeOffset = entity->ppuBanks[7].writeOffset;
            entity->latchBanks[1][7]._10 = entity->ppuBanks[7]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][8].readOffset = entity->ppuBanks[8].readOffset;
            entity->latchBanks[0][8]._00 = entity->ppuBanks[8]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][8].writeOffset = entity->ppuBanks[8].writeOffset;
            entity->latchBanks[0][8]._10 = entity->ppuBanks[8]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][8].readOffset = entity->ppuBanks[8].readOffset;
            entity->latchBanks[1][8]._00 = entity->ppuBanks[8]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][8].writeOffset = entity->ppuBanks[8].writeOffset;
            entity->latchBanks[1][8]._10 = entity->ppuBanks[8]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][9].readOffset = entity->ppuBanks[9].readOffset;
            entity->latchBanks[0][9]._00 = entity->ppuBanks[9]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][9].writeOffset = entity->ppuBanks[9].writeOffset;
            entity->latchBanks[0][9]._10 = entity->ppuBanks[9]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][9].readOffset = entity->ppuBanks[9].readOffset;
            entity->latchBanks[1][9]._00 = entity->ppuBanks[9]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][9].writeOffset = entity->ppuBanks[9].writeOffset;
            entity->latchBanks[1][9]._10 = entity->ppuBanks[9]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][10].readOffset = entity->ppuBanks[10].readOffset;
            entity->latchBanks[0][10]._00 = entity->ppuBanks[10]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][10].writeOffset = entity->ppuBanks[10].writeOffset;
            entity->latchBanks[0][10]._10 = entity->ppuBanks[10]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][10].readOffset = entity->ppuBanks[10].readOffset;
            entity->latchBanks[1][10]._00 = entity->ppuBanks[10]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][10].writeOffset = entity->ppuBanks[10].writeOffset;
            entity->latchBanks[1][10]._10 = entity->ppuBanks[10]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][11].readOffset = entity->ppuBanks[11].readOffset;
            entity->latchBanks[0][11]._00 = entity->ppuBanks[11]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][11].writeOffset = entity->ppuBanks[11].writeOffset;
            entity->latchBanks[0][11]._10 = entity->ppuBanks[11]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][11].readOffset = entity->ppuBanks[11].readOffset;
            entity->latchBanks[1][11]._00 = entity->ppuBanks[11]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][11].writeOffset = entity->ppuBanks[11].writeOffset;
            entity->latchBanks[1][11]._10 = entity->ppuBanks[11]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][12].readOffset = entity->ppuBanks[12].readOffset;
            entity->latchBanks[0][12]._00 = entity->ppuBanks[12]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][12].writeOffset = entity->ppuBanks[12].writeOffset;
            entity->latchBanks[0][12]._10 = entity->ppuBanks[12]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][12].readOffset = entity->ppuBanks[12].readOffset;
            entity->latchBanks[1][12]._00 = entity->ppuBanks[12]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][12].writeOffset = entity->ppuBanks[12].writeOffset;
            entity->latchBanks[1][12]._10 = entity->ppuBanks[12]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][13].readOffset = entity->ppuBanks[13].readOffset;
            entity->latchBanks[0][13]._00 = entity->ppuBanks[13]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][13].writeOffset = entity->ppuBanks[13].writeOffset;
            entity->latchBanks[0][13]._10 = entity->ppuBanks[13]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][13].readOffset = entity->ppuBanks[13].readOffset;
            entity->latchBanks[1][13]._00 = entity->ppuBanks[13]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][13].writeOffset = entity->ppuBanks[13].writeOffset;
            entity->latchBanks[1][13]._10 = entity->ppuBanks[13]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][14].readOffset = entity->ppuBanks[14].readOffset;
            entity->latchBanks[0][14]._00 = entity->ppuBanks[14]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][14].writeOffset = entity->ppuBanks[14].writeOffset;
            entity->latchBanks[0][14]._10 = entity->ppuBanks[14]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][14].readOffset = entity->ppuBanks[14].readOffset;
            entity->latchBanks[1][14]._00 = entity->ppuBanks[14]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][14].writeOffset = entity->ppuBanks[14].writeOffset;
            entity->latchBanks[1][14]._10 = entity->ppuBanks[14]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][15].readOffset = entity->ppuBanks[15].readOffset;
            entity->latchBanks[0][15]._00 = entity->ppuBanks[15]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][15].writeOffset = entity->ppuBanks[15].writeOffset;
            entity->latchBanks[0][15]._10 = entity->ppuBanks[15]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][15].readOffset = entity->ppuBanks[15].readOffset;
            entity->latchBanks[1][15]._00 = entity->ppuBanks[15]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][15].writeOffset = entity->ppuBanks[15].writeOffset;
            entity->latchBanks[1][15]._10 = entity->ppuBanks[15]._10;
        }
        mChrLatch[0] = 0xfe;
        mChrLatch[1] = 0x1fe;
        _SEntity* entity = mpEntity;
        mRegisters[1] = entity->chrBankCount - 8;
        mRegisters[2] = entity->chrBankCount - 8;
        mRegisters[3] = entity->chrBankCount - 4;
        mRegisters[4] = entity->chrBankCount - 4;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Initializes the original mapper identity and bank-update callback. */
CHvcMmc3::CHvcMmc3() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
    mAdjustBusMap = &CHvcMmc3::_AdjustBusMap;
}
/** @brief Restores the default bank-update callback. @return Zero. */
unsigned int CHvcMmc3::_Prologue() {
    mAdjustBusMap = &CHvcMmc3::_AdjustBusMap;
    return 0;
}
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMmc3::_Epilogue() { return 0; }
/** @brief Transfers mapper registers and interrupt state in original order.
 * @param buffer State storage, or null to measure. @param reading Import selector.
 * @return Bytes transferred.
 */
unsigned long CHvcMmc3::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mRegisters[2]);
    stream.Entry(mRegisters[3]);
    stream.Entry(mRegisters[4]);
    stream.Entry(mRegisters[5]);
    stream.Entry(mRegisters[6]);
    stream.Entry(mRegisters[7]);
    stream.Entry(mRegisters[8]);
    stream.Entry(mRegisters[9]);
    stream.Entry(mRegisters[10]);
    stream.Entry(mRegisters[11]);
    stream.Entry(mRegisters[12]);
    stream.Entry(mRegisters[13]);
    stream.Entry(mChrBanks[0]);
    stream.Entry(mChrBanks[1]);
    stream.Entry(mChrBanks[2]);
    stream.Entry(mChrBanks[3]);
    stream.Entry(mChrBanks[4]);
    stream.Entry(mChrBanks[5]);
    stream.Entry(mChrBanks[6]);
    stream.Entry(mChrBanks[7]);
    stream.Entry(mWriteAddress);
    stream.Entry(mIrqReload);
    stream.Entry(mIrqEnabled);
    stream.Entry(mIrqCounter);
    return stream.GetSize();
}
/** @brief Exports mapper state. @param buffer State output. */
void CHvcMmc3::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports mapper state. @param buffer State input. */
void CHvcMmc3::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned state storage. @return Aligned state size. */
unsigned long CHvcMmc3::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies mapper state species. @return Species five. */
unsigned char CHvcMmc3::_SerializeTagSpecies() { return 5; }
/** @brief Supplies the fixed mapper comment. @return Comment text. */
const unsigned char* CHvcMmc3::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-3  ");
}
/** @brief Routes mapped accesses or captures a mapper-register write.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read/write selector. @param args Bus arguments.
 * @return Bus status, zero for an accepted write, or six for unsupported access.
 */
unsigned int CHvcMmc3::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[11] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            (this->*mAdjustBusMap)();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Handles reset and clocks the enabled scanline interrupt counter.
 * @param sender Unused sender. @param category Unused category.
 * @param operation Unused operation. @param args Cycle event flags.
 * @return Zero.
 */
unsigned int CHvcMmc3::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mIrqReload = 0;
        mIrqEnabled = 0;
        mIrqCounter = 0;
        mRegisters[12] = (entity->prgBankCount << 1) - 4;
        mRegisters[13] = (entity->prgBankCount << 1) - 3;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    if (mIrqEnabled && mIrqCounter) {
        _SEntity* entity = mpEntity;
        if ((entity->cycle.line >> 12) < 15 && (args->values[0] & 0x10)
            && (entity->pMemory[0x303809] & 0x18)) {
            if (--mIrqCounter == 0) {
                mIrqCounter = mIrqReload;
                if (reinterpret_cast<const unsigned long*>(entity->pCustom)[1] & 1)
                    mIrqCounter = 0xff;
                entity->order.words[4] |= 1;
            }
        }
    }
    return 0;
}
/** @brief Applies the pending MMC3 register write to bus banks and interrupt controls. */
void CHvcMmc3::_AdjustBusMap() {
    switch (static_cast<unsigned int>(mWriteAddress) & 0xe001) {
    case 0x8000: {
        unsigned char value = mRegisters[11];
        mRegisters[0] = value;
        mRegisters[8] = value & 7;
        mRegisters[9] = (value >> 6) & 1;
        mRegisters[10] = value >> 7;
        unsigned long bank = mRegisters[12] & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
        unsigned long offset = bank << 13;
        if (mRegisters[9]) {
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
        } else {
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
        }
        bank = mRegisters[13] & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
        mpEntity->cpuBanks[10].readOffset = bank << 13;
        mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 0x8001: {
        unsigned long value = mRegisters[11];
        mRegisters[1] = value;
        switch (mRegisters[8]) {
        case 0: {
            mChrBanks[0] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[4].readOffset = offset;
                mpEntity->ppuBanks[5].readOffset = offset | 0x400;
            } else {
                mpEntity->ppuBanks[0].readOffset = offset;
                mpEntity->ppuBanks[1].readOffset = offset | 0x400;
            }
            break;
        }
        case 1: {
            mChrBanks[2] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[6].readOffset = offset;
                mpEntity->ppuBanks[7].readOffset = offset | 0x400;
            } else {
                mpEntity->ppuBanks[2].readOffset = offset;
                mpEntity->ppuBanks[3].readOffset = offset | 0x400;
            }
            break;
        }
        case 2: {
            mChrBanks[4] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[0].readOffset = offset;
            } else {
                mpEntity->ppuBanks[4].readOffset = offset;
            }
            break;
        }
        case 3: {
            mChrBanks[5] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[1].readOffset = offset;
            } else {
                mpEntity->ppuBanks[5].readOffset = offset;
            }
            break;
        }
        case 4: {
            mChrBanks[6] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[2].readOffset = offset;
            } else {
                mpEntity->ppuBanks[6].readOffset = offset;
            }
            break;
        }
        case 5: {
            mChrBanks[7] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[3].readOffset = offset;
            } else {
                mpEntity->ppuBanks[7].readOffset = offset;
            }
            break;
        }
        case 6: {
            mRegisters[12] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            unsigned long offset = bank << 13;
            if (mRegisters[9]) {
                mpEntity->cpuBanks[12].readOffset = offset;
                mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            } else {
                mpEntity->cpuBanks[8].readOffset = offset;
                mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            }
            break;
        }
        case 7: {
            mRegisters[13] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            mpEntity->cpuBanks[10].readOffset = bank << 13;
            mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
            break;
        }
        }
        break;
    }
    case 0xa000: {
        unsigned char value = mRegisters[11];
        mRegisters[2] = value;
        mpEntity->ppuBanks[8].writeOffset = 0x302800;
        mpEntity->ppuBanks[8].readOffset = 0x302800;
        if (value & 1) {
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
        } else {
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
        }
        mpEntity->ppuBanks[11].writeOffset = 0x302c00;
        mpEntity->ppuBanks[11].readOffset = 0x302c00;
        break;
    }
    case 0xa001:
        mRegisters[3] = mRegisters[11];
        break;
    case 0xc000: {
        unsigned char value = mRegisters[11];
        mRegisters[4] = value;
        mIrqReload = value;
        _SCycleController& cycle = mpEntity->cycle;
        if (cycle.framePeriod - cycle.frameBlank > cycle.frameCycle) {
            mIrqCounter = value;
            if (cycle.linePeriod - cycle.lineBlank > cycle.lineCycle)
                mIrqCounter = value + 1;
        }
        break;
    }
    case 0xc001: {
        mRegisters[5] = mRegisters[11];
        unsigned char value = mIrqReload;
        mIrqCounter = value;
        _SCycleController& cycle = mpEntity->cycle;
        if (cycle.framePeriod - cycle.frameBlank > cycle.frameCycle
            && cycle.linePeriod - cycle.lineBlank > cycle.lineCycle)
            mIrqCounter = value + 1;
        break;
    }
    case 0xe000:
        mRegisters[6] = mRegisters[11];
        mIrqEnabled = 0;
        break;
    case 0xe001:
        mRegisters[7] = mRegisters[11];
        mIrqEnabled = 1;
        break;
    }
}
/** @brief Initializes MMC3 bus tables and selects title-specific bank handling.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Initialization or device routing. @param args Routing arguments.
 * @return Zero.
 */
unsigned int CHvcMmc3::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
        if (reinterpret_cast<const unsigned long*>(mpEntity->pTitle)[6] == 0xd3)
            mAdjustBusMap = &CHvcMmc3::_AdjustBusMap_Mother;
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Applies title-specific MMC3 bank history and interrupt reload behavior. */
void CHvcMmc3::_AdjustBusMap_Mother() {
    switch (static_cast<unsigned int>(mWriteAddress) & 0xe001) {
    case 0x8000: {
        unsigned char value = mRegisters[11];
        mRegisters[0] = value;
        mRegisters[8] = value & 7;
        mRegisters[9] = (value >> 6) & 1;
        mRegisters[10] = value >> 7;
        unsigned long bank = mRegisters[12] & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
        unsigned long offset = bank << 13;
        if (mRegisters[9]) {
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
        } else {
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
        }
        bank = mRegisters[13] & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
        mpEntity->cpuBanks[10].readOffset = bank << 13;
        mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 0x8001: {
        unsigned long value = mRegisters[11];
        mRegisters[1] = value;
        switch (mRegisters[8]) {
        case 0: {
            mChrBanks[0] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[4].readOffset = offset;
                mpEntity->ppuBanks[5].readOffset = offset | 0x400;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[4].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[4].access[1]._01) {
                        history[index + 4].access[0]._01 = position;
                        entity->ppuBanks[4].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 4 + 8].readOffset = entity->ppuBanks[4].readOffset;
                        nextHistory[index + 4 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 4].readOffset = entity->ppuBanks[4].readOffset;
                        entity->ppuBanks[4].access[1]._01 = position;
                    }
                }
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[5].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[5].access[1]._01) {
                        history[index + 5].access[0]._01 = position;
                        entity->ppuBanks[5].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 5 + 8].readOffset = entity->ppuBanks[5].readOffset;
                        nextHistory[index + 5 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 5].readOffset = entity->ppuBanks[5].readOffset;
                        entity->ppuBanks[5].access[1]._01 = position;
                    }
                }
            } else {
                mpEntity->ppuBanks[0].readOffset = offset;
                mpEntity->ppuBanks[1].readOffset = offset | 0x400;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[0].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[0].access[1]._01) {
                        history[index + 0].access[0]._01 = position;
                        entity->ppuBanks[0].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 0 + 8].readOffset = entity->ppuBanks[0].readOffset;
                        nextHistory[index + 0 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 0].readOffset = entity->ppuBanks[0].readOffset;
                        entity->ppuBanks[0].access[1]._01 = position;
                    }
                }
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[1].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[1].access[1]._01) {
                        history[index + 1].access[0]._01 = position;
                        entity->ppuBanks[1].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 1 + 8].readOffset = entity->ppuBanks[1].readOffset;
                        nextHistory[index + 1 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 1].readOffset = entity->ppuBanks[1].readOffset;
                        entity->ppuBanks[1].access[1]._01 = position;
                    }
                }
            }
            break;
        }
        case 1: {
            mChrBanks[2] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[6].readOffset = offset;
                mpEntity->ppuBanks[7].readOffset = offset | 0x400;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[6].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[6].access[1]._01) {
                        history[index + 6].access[0]._01 = position;
                        entity->ppuBanks[6].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 6 + 8].readOffset = entity->ppuBanks[6].readOffset;
                        nextHistory[index + 6 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 6].readOffset = entity->ppuBanks[6].readOffset;
                        entity->ppuBanks[6].access[1]._01 = position;
                    }
                }
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[7].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[7].access[1]._01) {
                        history[index + 7].access[0]._01 = position;
                        entity->ppuBanks[7].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 7 + 8].readOffset = entity->ppuBanks[7].readOffset;
                        nextHistory[index + 7 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 7].readOffset = entity->ppuBanks[7].readOffset;
                        entity->ppuBanks[7].access[1]._01 = position;
                    }
                }
            } else {
                mpEntity->ppuBanks[2].readOffset = offset;
                mpEntity->ppuBanks[3].readOffset = offset | 0x400;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[2].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[2].access[1]._01) {
                        history[index + 2].access[0]._01 = position;
                        entity->ppuBanks[2].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 2 + 8].readOffset = entity->ppuBanks[2].readOffset;
                        nextHistory[index + 2 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 2].readOffset = entity->ppuBanks[2].readOffset;
                        entity->ppuBanks[2].access[1]._01 = position;
                    }
                }
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[3].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[3].access[1]._01) {
                        history[index + 3].access[0]._01 = position;
                        entity->ppuBanks[3].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 3 + 8].readOffset = entity->ppuBanks[3].readOffset;
                        nextHistory[index + 3 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 3].readOffset = entity->ppuBanks[3].readOffset;
                        entity->ppuBanks[3].access[1]._01 = position;
                    }
                }
            }
            break;
        }
        case 2: {
            mChrBanks[4] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[0].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[0].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[0].access[1]._01) {
                        history[index + 0].access[0]._01 = position;
                        entity->ppuBanks[0].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 0 + 8].readOffset = entity->ppuBanks[0].readOffset;
                        nextHistory[index + 0 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 0].readOffset = entity->ppuBanks[0].readOffset;
                        entity->ppuBanks[0].access[1]._01 = position;
                    }
                }
            } else {
                mpEntity->ppuBanks[4].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[4].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[4].access[1]._01) {
                        history[index + 4].access[0]._01 = position;
                        entity->ppuBanks[4].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 4 + 8].readOffset = entity->ppuBanks[4].readOffset;
                        nextHistory[index + 4 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 4].readOffset = entity->ppuBanks[4].readOffset;
                        entity->ppuBanks[4].access[1]._01 = position;
                    }
                }
            }
            break;
        }
        case 3: {
            mChrBanks[5] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[1].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[1].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[1].access[1]._01) {
                        history[index + 1].access[0]._01 = position;
                        entity->ppuBanks[1].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 1 + 8].readOffset = entity->ppuBanks[1].readOffset;
                        nextHistory[index + 1 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 1].readOffset = entity->ppuBanks[1].readOffset;
                        entity->ppuBanks[1].access[1]._01 = position;
                    }
                }
            } else {
                mpEntity->ppuBanks[5].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[5].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[5].access[1]._01) {
                        history[index + 5].access[0]._01 = position;
                        entity->ppuBanks[5].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 5 + 8].readOffset = entity->ppuBanks[5].readOffset;
                        nextHistory[index + 5 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 5].readOffset = entity->ppuBanks[5].readOffset;
                        entity->ppuBanks[5].access[1]._01 = position;
                    }
                }
            }
            break;
        }
        case 4: {
            mChrBanks[6] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[2].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[2].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[2].access[1]._01) {
                        history[index + 2].access[0]._01 = position;
                        entity->ppuBanks[2].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 2 + 8].readOffset = entity->ppuBanks[2].readOffset;
                        nextHistory[index + 2 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 2].readOffset = entity->ppuBanks[2].readOffset;
                        entity->ppuBanks[2].access[1]._01 = position;
                    }
                }
            } else {
                mpEntity->ppuBanks[6].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[6].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[6].access[1]._01) {
                        history[index + 6].access[0]._01 = position;
                        entity->ppuBanks[6].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 6 + 8].readOffset = entity->ppuBanks[6].readOffset;
                        nextHistory[index + 6 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 6].readOffset = entity->ppuBanks[6].readOffset;
                        entity->ppuBanks[6].access[1]._01 = position;
                    }
                }
            }
            break;
        }
        case 5: {
            mChrBanks[7] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[3].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[3].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[3].access[1]._01) {
                        history[index + 3].access[0]._01 = position;
                        entity->ppuBanks[3].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 3 + 8].readOffset = entity->ppuBanks[3].readOffset;
                        nextHistory[index + 3 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 3].readOffset = entity->ppuBanks[3].readOffset;
                        entity->ppuBanks[3].access[1]._01 = position;
                    }
                }
            } else {
                mpEntity->ppuBanks[7].readOffset = offset;
                {
                    _SEntity* entity = mpEntity;
                    int position = static_cast<int>(entity->cycle.lineCycle >> 8);
                    unsigned long index = entity->ppuBanks[7].access[0]._01;
                    _SBank* history = entity->chrHistory;
                    if (position < entity->ppuBanks[7].access[1]._01) {
                        history[index + 7].access[0]._01 = position;
                        entity->ppuBanks[7].access[0]._01 = index + 8;
                        _SBank* nextHistory = mpEntity->chrHistory;
                        nextHistory[index + 7 + 8].readOffset = entity->ppuBanks[7].readOffset;
                        nextHistory[index + 7 + 8].access[0]._01 = 0xb4;
                    } else {
                        history[index + 7].readOffset = entity->ppuBanks[7].readOffset;
                        entity->ppuBanks[7].access[1]._01 = position;
                    }
                }
            }
            break;
        }
        case 6: {
            mRegisters[12] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            unsigned long offset = bank << 13;
            if (mRegisters[9]) {
                mpEntity->cpuBanks[12].readOffset = offset;
                mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            } else {
                mpEntity->cpuBanks[8].readOffset = offset;
                mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            }
            break;
        }
        case 7: {
            mRegisters[13] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            mpEntity->cpuBanks[10].readOffset = bank << 13;
            mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
            break;
        }
        }
        break;
    }
    case 0xa000: {
        unsigned char value = mRegisters[11];
        mRegisters[2] = value;
        mpEntity->ppuBanks[8].writeOffset = 0x302800;
        mpEntity->ppuBanks[8].readOffset = 0x302800;
        if (value & 1) {
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
        } else {
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
        }
        mpEntity->ppuBanks[11].writeOffset = 0x302c00;
        mpEntity->ppuBanks[11].readOffset = 0x302c00;
        break;
    }
    case 0xa001:
        mRegisters[3] = mRegisters[11];
        break;
    case 0xc000: {
        unsigned char value = mRegisters[11];
        mRegisters[4] = value;
        mIrqReload = value;
        _SCycleController& cycle = mpEntity->cycle;
        if (cycle.framePeriod - cycle.frameBlank > cycle.frameCycle) {
            mIrqCounter = value;
            if (cycle.linePeriod - cycle.lineBlank > cycle.lineCycle)
                mIrqCounter = value + 1;
        }
        break;
    }
    case 0xc001: {
        mRegisters[5] = mRegisters[11];
        unsigned char value = mIrqReload;
        mIrqCounter = value;
        _SCycleController& cycle = mpEntity->cycle;
        if (cycle.framePeriod - cycle.frameBlank > cycle.frameCycle
            && cycle.linePeriod - cycle.lineBlank > cycle.lineCycle)
            mIrqCounter = value + 1;
        break;
    }
    case 0xe000:
        mRegisters[6] = mRegisters[11];
        mIrqEnabled = 0;
        break;
    case 0xe001: {
        mRegisters[7] = mRegisters[11];
        mIrqEnabled = 1;
        unsigned char value = mIrqReload;
        mIrqCounter = value;
        _SCycleController& cycle = mpEntity->cycle;
        if (cycle.framePeriod - cycle.frameBlank > cycle.frameCycle
            && cycle.linePeriod - cycle.lineBlank > cycle.lineCycle)
            mIrqCounter = value + 1;
        break;
    }
    }
}
/** @brief Initializes the original mapper identity. */
CHvcMmc3TLS::CHvcMmc3TLS() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMmc3TLS::_Prologue() {
    return 0;
}
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMmc3TLS::_Epilogue() { return 0; }
/** @brief Transfers mapper registers and interrupt state in original order.
 * @param buffer State storage, or null to measure. @param reading Import selector.
 * @return Bytes transferred.
 */
unsigned long CHvcMmc3TLS::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mRegisters[2]);
    stream.Entry(mRegisters[3]);
    stream.Entry(mRegisters[4]);
    stream.Entry(mRegisters[5]);
    stream.Entry(mRegisters[6]);
    stream.Entry(mRegisters[7]);
    stream.Entry(mRegisters[8]);
    stream.Entry(mRegisters[9]);
    stream.Entry(mRegisters[10]);
    stream.Entry(mRegisters[11]);
    stream.Entry(mRegisters[12]);
    stream.Entry(mRegisters[13]);
    stream.Entry(mChrBanks[0]);
    stream.Entry(mChrBanks[1]);
    stream.Entry(mChrBanks[2]);
    stream.Entry(mChrBanks[3]);
    stream.Entry(mChrBanks[4]);
    stream.Entry(mChrBanks[5]);
    stream.Entry(mChrBanks[6]);
    stream.Entry(mChrBanks[7]);
    stream.Entry(mWriteAddress);
    stream.Entry(mIrqReload);
    stream.Entry(mIrqEnabled);
    stream.Entry(mIrqCounter);
    return stream.GetSize();
}
/** @brief Exports mapper state. @param buffer State output. */
void CHvcMmc3TLS::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports mapper state. @param buffer State input. */
void CHvcMmc3TLS::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned state storage. @return Aligned state size. */
unsigned long CHvcMmc3TLS::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies mapper state species. @return Species five. */
unsigned char CHvcMmc3TLS::_SerializeTagSpecies() { return 6; }
/** @brief Supplies the fixed mapper comment. @return Comment text. */
const unsigned char* CHvcMmc3TLS::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-3-T");
}
/** @brief Initializes TLS mapper bus tables or resolves a device. @return Zero. */
unsigned int CHvcMmc3TLS::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes accesses or captures a TLS mapper write. @return Bus status. */
unsigned int CHvcMmc3TLS::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[11] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Applies TLS bank selection, mirroring, and interrupt registers. */
void CHvcMmc3TLS::_AdjustBusMap() {
    switch (static_cast<unsigned int>(mWriteAddress) & 0xe001) {
    case 0x8000: {
        unsigned char value = mRegisters[11];
        mRegisters[0] = value;
        mRegisters[8] = value & 7;
        mRegisters[9] = (value >> 6) & 1;
        mRegisters[10] = value >> 7;
        unsigned long bank = mRegisters[12] & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
        unsigned long offset = bank << 13;
        if (mRegisters[9]) {
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
        } else {
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
        }
        break;
    }
    case 0x8001: {
        mRegisters[1] = mRegisters[11];
        if (mRegisters[10] ? mRegisters[8] == 2 : mRegisters[8] == 0) {
            _SEntity* entity = mpEntity;
            unsigned long offset = (static_cast<signed char>(mRegisters[11]) < 0) ? 0x302800 : 0x302c00;
            entity->ppuBanks[8].writeOffset = offset;
            mpEntity->ppuBanks[8].readOffset = offset;
            mpEntity->ppuBanks[9].writeOffset = offset;
            mpEntity->ppuBanks[9].readOffset = offset;
            mpEntity->ppuBanks[10].writeOffset = offset;
            mpEntity->ppuBanks[10].readOffset = offset;
            mpEntity->ppuBanks[11].writeOffset = offset;
            mpEntity->ppuBanks[11].readOffset = offset;
        }
        switch (mRegisters[8]) {
        case 0: {
            unsigned long value = mRegisters[1];
            mChrBanks[0] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[4].readOffset = offset;
                mpEntity->ppuBanks[5].readOffset = offset | 0x400;
            } else {
                mpEntity->ppuBanks[0].readOffset = offset;
                mpEntity->ppuBanks[1].readOffset = offset | 0x400;
            }
            break;
        }
        case 1: {
            unsigned long value = mRegisters[1];
            mChrBanks[2] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[6].readOffset = offset;
                mpEntity->ppuBanks[7].readOffset = offset | 0x400;
            } else {
                mpEntity->ppuBanks[2].readOffset = offset;
                mpEntity->ppuBanks[3].readOffset = offset | 0x400;
            }
            break;
        }
        case 2: {
            unsigned long value = mRegisters[1];
            mChrBanks[4] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[0].readOffset = offset;
            } else {
                mpEntity->ppuBanks[4].readOffset = offset;
            }
            break;
        }
        case 3: {
            unsigned long value = mRegisters[1];
            mChrBanks[5] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[1].readOffset = offset;
            } else {
                mpEntity->ppuBanks[5].readOffset = offset;
            }
            break;
        }
        case 4: {
            unsigned long value = mRegisters[1];
            mChrBanks[6] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[2].readOffset = offset;
            } else {
                mpEntity->ppuBanks[6].readOffset = offset;
            }
            break;
        }
        case 5: {
            unsigned long value = mRegisters[1];
            mChrBanks[7] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[10]) {
                mpEntity->ppuBanks[3].readOffset = offset;
            } else {
                mpEntity->ppuBanks[7].readOffset = offset;
            }
            break;
        }
        case 6: {
            unsigned long value = mRegisters[1];
            mRegisters[12] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            unsigned long offset = bank << 13;
            if (mRegisters[9]) {
                mpEntity->cpuBanks[12].readOffset = offset;
                mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            } else {
                mpEntity->cpuBanks[8].readOffset = offset;
                mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            }
            break;
        }
        case 7: {
            unsigned long value = mRegisters[1];
            mRegisters[13] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            mpEntity->cpuBanks[10].readOffset = bank << 13;
            mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
            break;
        }
        }
        break;
    }
    case 0xc000:
        mRegisters[4] = mRegisters[11];
        mIrqCounter = mRegisters[11];
        break;
    case 0xc001:
        mRegisters[5] = mRegisters[11];
        mIrqReload = mRegisters[11];
        break;
    case 0xe000:
        mRegisters[6] = mRegisters[11];
        mIrqEnabled = 0;
        mIrqCounter = mIrqReload;
        break;
    case 0xe001:
        mRegisters[7] = mRegisters[11];
        mIrqEnabled = 1;
        break;
    }
}
/** @brief Handles TLS reset and scanline interrupts. @return Zero. */
unsigned int CHvcMmc3TLS::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mIrqReload = 0;
        mIrqEnabled = 0;
        mIrqCounter = 0;
        mRegisters[12] = 0;
        mRegisters[13] = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    if (mIrqEnabled) {
        _SEntity* entity = mpEntity;
        if ((entity->cycle.line >> 12) < 15 && (args->values[0] & 0x10)) {
            if (--mIrqCounter == 0) {
                mIrqCounter = mIrqReload;
                entity->order.words[4] |= 1;
            }
        }
    }
    return 0;
}
/** @brief Initializes the latched mapper identity used by the original port. */
CHvcMmc4::CHvcMmc4() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMmc4::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMmc4::_Epilogue() { return 0; }
/** @brief Initializes bus tables, resolves devices, or handles character-latch notifications.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Initialization, routing, or latch-notification operation.
 * @param args Operation arguments. @return Zero.
 */
unsigned int CHvcMmc4::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    switch (operation) {
    case 0: {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
        break;
    }
    case 2: {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
        break;
    }
    case 3: {
        if (args->values[0] == 4 && args->values[1] == 1)
            _ApplyLatchChrBank(static_cast<unsigned short>(args->values[2]), true);
        break;
    }
    }
    return 0;
}
/** @brief Selects character read descriptors when a latch address is observed.
 * @param address Latch address selector. @param enabled Whether to apply it.
 */
void CHvcMmc4::_ApplyLatchChrBank(unsigned short address, bool enabled) {
    if (enabled) {
        switch (address) {
        case 0xfd:
            mChrLatch[0] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[0].readOffset = entity->latchBanks[0][0].readOffset;
                entity->ppuBanks[0]._00 = entity->latchBanks[0][0]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[1].readOffset = entity->latchBanks[0][1].readOffset;
                entity->ppuBanks[1]._00 = entity->latchBanks[0][1]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[2].readOffset = entity->latchBanks[0][2].readOffset;
                entity->ppuBanks[2]._00 = entity->latchBanks[0][2]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[3].readOffset = entity->latchBanks[0][3].readOffset;
                entity->ppuBanks[3]._00 = entity->latchBanks[0][3]._00;
            }
            return;
        case 0xfe:
            mChrLatch[0] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[0].readOffset = entity->latchBanks[1][0].readOffset;
                entity->ppuBanks[0]._00 = entity->latchBanks[1][0]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[1].readOffset = entity->latchBanks[1][1].readOffset;
                entity->ppuBanks[1]._00 = entity->latchBanks[1][1]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[2].readOffset = entity->latchBanks[1][2].readOffset;
                entity->ppuBanks[2]._00 = entity->latchBanks[1][2]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[3].readOffset = entity->latchBanks[1][3].readOffset;
                entity->ppuBanks[3]._00 = entity->latchBanks[1][3]._00;
            }
            return;
        case 0x1fd:
            mChrLatch[1] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[4].readOffset = entity->latchBanks[0][4].readOffset;
                entity->ppuBanks[4]._00 = entity->latchBanks[0][4]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[5].readOffset = entity->latchBanks[0][5].readOffset;
                entity->ppuBanks[5]._00 = entity->latchBanks[0][5]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[6].readOffset = entity->latchBanks[0][6].readOffset;
                entity->ppuBanks[6]._00 = entity->latchBanks[0][6]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[7].readOffset = entity->latchBanks[0][7].readOffset;
                entity->ppuBanks[7]._00 = entity->latchBanks[0][7]._00;
            }
            return;
        case 0x1fe:
            mChrLatch[1] = address;
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[4].readOffset = entity->latchBanks[1][4].readOffset;
                entity->ppuBanks[4]._00 = entity->latchBanks[1][4]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[5].readOffset = entity->latchBanks[1][5].readOffset;
                entity->ppuBanks[5]._00 = entity->latchBanks[1][5]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[6].readOffset = entity->latchBanks[1][6].readOffset;
                entity->ppuBanks[6]._00 = entity->latchBanks[1][6]._00;
            }
            {
                _SEntity* entity = mpEntity;
                entity->ppuBanks[7].readOffset = entity->latchBanks[1][7].readOffset;
                entity->ppuBanks[7]._00 = entity->latchBanks[1][7]._00;
            }
            return;
        }
    }
}
/** @brief Transfers registers, mapper timing state, and both character latches.
 * @param buffer State buffer or null to measure. @param reading Restore direction.
 * @return Bytes transferred.
 */
unsigned long CHvcMmc4::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mRegisters[2]);
    stream.Entry(mRegisters[3]);
    stream.Entry(mRegisters[4]);
    stream.Entry(mRegisters[5]);
    stream.Entry(mRegisters[6]);
    stream.Entry(mWriteAddress);
    stream.Entry(mChrLatch[0]);
    stream.Entry(mChrLatch[1]);
    return stream.GetSize();
}
/** @brief Exports latched mapper state. @param buffer State output. */
void CHvcMmc4::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Restores latched mapper state. @param buffer State input. */
void CHvcMmc4::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned state storage. @return Aligned state size. */
unsigned long CHvcMmc4::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 19) & ~15u;
}
/** @brief Routes mapped accesses or captures a mapper-register write.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read/write selector. @param args Bus arguments.
 * @return Bus status, zero for an accepted write, or six for unsupported access.
 */
unsigned int CHvcMmc4::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[6] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Identifies latched-mapper state. @return Seven. */
unsigned char CHvcMmc4::_SerializeTagSpecies() { return 7; }
/** @brief Returns the latched-mapper state label. @return Mapper identifier. */
const unsigned char* CHvcMmc4::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-4  ");
}
/** @brief Applies the captured write to program banks, character latches, or mirroring. */
void CHvcMmc4::_AdjustBusMap() {
    switch (mWriteAddress & 0xf000) {
    case 0xa000: {
        unsigned long value = mRegisters[6];
        mRegisters[0] = value;
        unsigned long bank = value & (mpEntity->prgBankCount - 1) & 0xf;
        unsigned long offset = bank << 14;
        mpEntity->cpuBanks[8].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[9].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[10].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[11].readOffset = offset;
        break;
    }
    case 0xb000: {
        unsigned long value = mRegisters[6];
        mRegisters[1] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[0][0].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][1].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][2].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][3].readOffset = offset;
        _ApplyLatchChrBank(0xfd, mChrLatch[0] == 0xfd);
        break;
    }
    case 0xc000: {
        unsigned long value = mRegisters[6];
        mRegisters[2] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[1][0].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][1].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][2].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][3].readOffset = offset;
        _ApplyLatchChrBank(0xfe, mChrLatch[0] == 0xfe);
        break;
    }
    case 0xd000: {
        unsigned long value = mRegisters[6];
        mRegisters[3] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[0][4].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][5].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][6].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[0][7].readOffset = offset;
        _ApplyLatchChrBank(0x1fd, mChrLatch[1] == 0x1fd);
        break;
    }
    case 0xe000: {
        unsigned long value = mRegisters[6];
        mRegisters[4] = value;
        unsigned long bank = value & ((mpEntity->chrBankCount << 1) - 1) & 0x1f;
        unsigned long offset = 0x200000 + (bank << 12);
        mpEntity->latchBanks[1][4].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][5].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][6].readOffset = offset;
        offset += 0x400;
        mpEntity->latchBanks[1][7].readOffset = offset;
        _ApplyLatchChrBank(0x1fe, mChrLatch[1] == 0x1fe);
        break;
    }
    case 0xf000: {
        unsigned char value = mRegisters[6];
        mRegisters[5] = value;
        mpEntity->ppuBanks[8].writeOffset = 0x302800;
        mpEntity->ppuBanks[8].readOffset = 0x302800;
        if (value & 1) {
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
        } else {
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
        }
        mpEntity->ppuBanks[11].writeOffset = 0x302c00;
        mpEntity->ppuBanks[11].readOffset = 0x302c00;
        break;
    }
    }
}
/** @brief Copies both latch tables and restores initial character selectors on reset.
 * @param sender Unused sender. @param category Unused category.
 * @param operation Unused operation. @param args Unused arguments. @return Zero.
 */
unsigned int CHvcMmc4::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* resetEntity = mpEntity;
    if ((1u << _18) & resetEntity->order.words[0]) {
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][0].readOffset = entity->ppuBanks[0].readOffset;
            entity->latchBanks[0][0]._00 = entity->ppuBanks[0]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][0].writeOffset = entity->ppuBanks[0].writeOffset;
            entity->latchBanks[0][0]._10 = entity->ppuBanks[0]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][0].readOffset = entity->ppuBanks[0].readOffset;
            entity->latchBanks[1][0]._00 = entity->ppuBanks[0]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][0].writeOffset = entity->ppuBanks[0].writeOffset;
            entity->latchBanks[1][0]._10 = entity->ppuBanks[0]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][1].readOffset = entity->ppuBanks[1].readOffset;
            entity->latchBanks[0][1]._00 = entity->ppuBanks[1]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][1].writeOffset = entity->ppuBanks[1].writeOffset;
            entity->latchBanks[0][1]._10 = entity->ppuBanks[1]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][1].readOffset = entity->ppuBanks[1].readOffset;
            entity->latchBanks[1][1]._00 = entity->ppuBanks[1]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][1].writeOffset = entity->ppuBanks[1].writeOffset;
            entity->latchBanks[1][1]._10 = entity->ppuBanks[1]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][2].readOffset = entity->ppuBanks[2].readOffset;
            entity->latchBanks[0][2]._00 = entity->ppuBanks[2]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][2]._10 = entity->ppuBanks[2]._10;
            entity->latchBanks[0][2].writeOffset = entity->ppuBanks[2].writeOffset;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][2]._00 = entity->ppuBanks[2]._00;
            entity->latchBanks[1][2].readOffset = entity->ppuBanks[2].readOffset;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][2].writeOffset = entity->ppuBanks[2].writeOffset;
            entity->latchBanks[1][2]._10 = entity->ppuBanks[2]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][3].readOffset = entity->ppuBanks[3].readOffset;
            entity->latchBanks[0][3]._00 = entity->ppuBanks[3]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][3].writeOffset = entity->ppuBanks[3].writeOffset;
            entity->latchBanks[0][3]._10 = entity->ppuBanks[3]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][3].readOffset = entity->ppuBanks[3].readOffset;
            entity->latchBanks[1][3]._00 = entity->ppuBanks[3]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][3].writeOffset = entity->ppuBanks[3].writeOffset;
            entity->latchBanks[1][3]._10 = entity->ppuBanks[3]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][4].readOffset = entity->ppuBanks[4].readOffset;
            entity->latchBanks[0][4]._00 = entity->ppuBanks[4]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][4].writeOffset = entity->ppuBanks[4].writeOffset;
            entity->latchBanks[0][4]._10 = entity->ppuBanks[4]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][4].readOffset = entity->ppuBanks[4].readOffset;
            entity->latchBanks[1][4]._00 = entity->ppuBanks[4]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][4].writeOffset = entity->ppuBanks[4].writeOffset;
            entity->latchBanks[1][4]._10 = entity->ppuBanks[4]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][5].readOffset = entity->ppuBanks[5].readOffset;
            entity->latchBanks[0][5]._00 = entity->ppuBanks[5]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][5].writeOffset = entity->ppuBanks[5].writeOffset;
            entity->latchBanks[0][5]._10 = entity->ppuBanks[5]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][5].readOffset = entity->ppuBanks[5].readOffset;
            entity->latchBanks[1][5]._00 = entity->ppuBanks[5]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][5].writeOffset = entity->ppuBanks[5].writeOffset;
            entity->latchBanks[1][5]._10 = entity->ppuBanks[5]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][6].readOffset = entity->ppuBanks[6].readOffset;
            entity->latchBanks[0][6]._00 = entity->ppuBanks[6]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][6].writeOffset = entity->ppuBanks[6].writeOffset;
            entity->latchBanks[0][6]._10 = entity->ppuBanks[6]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][6].readOffset = entity->ppuBanks[6].readOffset;
            entity->latchBanks[1][6]._00 = entity->ppuBanks[6]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][6].writeOffset = entity->ppuBanks[6].writeOffset;
            entity->latchBanks[1][6]._10 = entity->ppuBanks[6]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][7].readOffset = entity->ppuBanks[7].readOffset;
            entity->latchBanks[0][7]._00 = entity->ppuBanks[7]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][7].writeOffset = entity->ppuBanks[7].writeOffset;
            entity->latchBanks[0][7]._10 = entity->ppuBanks[7]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][7].readOffset = entity->ppuBanks[7].readOffset;
            entity->latchBanks[1][7]._00 = entity->ppuBanks[7]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][7].writeOffset = entity->ppuBanks[7].writeOffset;
            entity->latchBanks[1][7]._10 = entity->ppuBanks[7]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][8].readOffset = entity->ppuBanks[8].readOffset;
            entity->latchBanks[0][8]._00 = entity->ppuBanks[8]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][8].writeOffset = entity->ppuBanks[8].writeOffset;
            entity->latchBanks[0][8]._10 = entity->ppuBanks[8]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][8].readOffset = entity->ppuBanks[8].readOffset;
            entity->latchBanks[1][8]._00 = entity->ppuBanks[8]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][8].writeOffset = entity->ppuBanks[8].writeOffset;
            entity->latchBanks[1][8]._10 = entity->ppuBanks[8]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][9].readOffset = entity->ppuBanks[9].readOffset;
            entity->latchBanks[0][9]._00 = entity->ppuBanks[9]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][9].writeOffset = entity->ppuBanks[9].writeOffset;
            entity->latchBanks[0][9]._10 = entity->ppuBanks[9]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][9].readOffset = entity->ppuBanks[9].readOffset;
            entity->latchBanks[1][9]._00 = entity->ppuBanks[9]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][9].writeOffset = entity->ppuBanks[9].writeOffset;
            entity->latchBanks[1][9]._10 = entity->ppuBanks[9]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][10].readOffset = entity->ppuBanks[10].readOffset;
            entity->latchBanks[0][10]._00 = entity->ppuBanks[10]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][10].writeOffset = entity->ppuBanks[10].writeOffset;
            entity->latchBanks[0][10]._10 = entity->ppuBanks[10]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][10].readOffset = entity->ppuBanks[10].readOffset;
            entity->latchBanks[1][10]._00 = entity->ppuBanks[10]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][10].writeOffset = entity->ppuBanks[10].writeOffset;
            entity->latchBanks[1][10]._10 = entity->ppuBanks[10]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][11].readOffset = entity->ppuBanks[11].readOffset;
            entity->latchBanks[0][11]._00 = entity->ppuBanks[11]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][11].writeOffset = entity->ppuBanks[11].writeOffset;
            entity->latchBanks[0][11]._10 = entity->ppuBanks[11]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][11].readOffset = entity->ppuBanks[11].readOffset;
            entity->latchBanks[1][11]._00 = entity->ppuBanks[11]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][11].writeOffset = entity->ppuBanks[11].writeOffset;
            entity->latchBanks[1][11]._10 = entity->ppuBanks[11]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][12].readOffset = entity->ppuBanks[12].readOffset;
            entity->latchBanks[0][12]._00 = entity->ppuBanks[12]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][12].writeOffset = entity->ppuBanks[12].writeOffset;
            entity->latchBanks[0][12]._10 = entity->ppuBanks[12]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][12].readOffset = entity->ppuBanks[12].readOffset;
            entity->latchBanks[1][12]._00 = entity->ppuBanks[12]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][12].writeOffset = entity->ppuBanks[12].writeOffset;
            entity->latchBanks[1][12]._10 = entity->ppuBanks[12]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][13].readOffset = entity->ppuBanks[13].readOffset;
            entity->latchBanks[0][13]._00 = entity->ppuBanks[13]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][13].writeOffset = entity->ppuBanks[13].writeOffset;
            entity->latchBanks[0][13]._10 = entity->ppuBanks[13]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][13].readOffset = entity->ppuBanks[13].readOffset;
            entity->latchBanks[1][13]._00 = entity->ppuBanks[13]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][13].writeOffset = entity->ppuBanks[13].writeOffset;
            entity->latchBanks[1][13]._10 = entity->ppuBanks[13]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][14].readOffset = entity->ppuBanks[14].readOffset;
            entity->latchBanks[0][14]._00 = entity->ppuBanks[14]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][14].writeOffset = entity->ppuBanks[14].writeOffset;
            entity->latchBanks[0][14]._10 = entity->ppuBanks[14]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][14].readOffset = entity->ppuBanks[14].readOffset;
            entity->latchBanks[1][14]._00 = entity->ppuBanks[14]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][14].writeOffset = entity->ppuBanks[14].writeOffset;
            entity->latchBanks[1][14]._10 = entity->ppuBanks[14]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][15].readOffset = entity->ppuBanks[15].readOffset;
            entity->latchBanks[0][15]._00 = entity->ppuBanks[15]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[0][15].writeOffset = entity->ppuBanks[15].writeOffset;
            entity->latchBanks[0][15]._10 = entity->ppuBanks[15]._10;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][15].readOffset = entity->ppuBanks[15].readOffset;
            entity->latchBanks[1][15]._00 = entity->ppuBanks[15]._00;
        }
        {
            _SEntity* entity = mpEntity;
            entity->latchBanks[1][15].writeOffset = entity->ppuBanks[15].writeOffset;
            entity->latchBanks[1][15]._10 = entity->ppuBanks[15]._10;
        }
        mChrLatch[0] = 0xfe;
        mChrLatch[1] = 0x1fe;
        _SEntity* entity = mpEntity;
        mRegisters[1] = entity->chrBankCount - 8;
        mRegisters[2] = entity->chrBankCount - 8;
        mRegisters[3] = entity->chrBankCount - 4;
        mRegisters[4] = entity->chrBankCount - 4;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Initializes the original MMC5 identity. */
CHvcMmc5::CHvcMmc5() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMmc5::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMmc5::_Epilogue() { return 0; }
/** @brief Transfers MMC5 state in the original serialized order.
 * @param buffer State storage or null to measure. @param reading Import selector.
 * @return Bytes transferred.
 */
unsigned long CHvcMmc5::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mRegisters[2]);
    stream.Entry(mRegisters[3]);
    stream.Entry(mRegisters[4]);
    stream.Entry(mRegisters[5]);
    stream.Entry(mRegisters[6]);
    stream.Entry(mWriteAddress);
    stream.Entry(mState[0]);
    stream.Entry(mState[1]);
    stream.Entry(mState[2]);
    stream.Entry(mState[3]);
    stream.Entry(mState[4]);
    stream.Entry(mState[5]);
    stream.Entry(mState[6]);
    stream.Entry(mState[7]);
    stream.Entry(mState[8]);
    stream.Entry(mState[9]);
    stream.Entry(mState[10]);
    stream.Entry(mState[11]);
    stream.Entry(mState[12]);
    stream.Entry(mState[13]);
    stream.Entry(mState[14]);
    stream.Entry(mState[15]);
    stream.Entry(mState[16]);
    stream.Entry(mState[17]);
    stream.Entry(mState[18]);
    stream.Entry(mState[19]);
    stream.Entry(mState[20]);
    stream.Entry(mState[21]);
    stream.Entry(mState[22]);
    stream.Entry(mState[23]);
    stream.Entry(mState[24]);
    stream.Entry(mState[25]);
    return stream.GetSize();
}
/** @brief Exports MMC5 state. @param buffer State output. */
void CHvcMmc5::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports MMC5 state. @param buffer State input. */
void CHvcMmc5::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned state storage. @return Aligned state size. */
unsigned long CHvcMmc5::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies MMC5 state. @return Eight. */
unsigned char CHvcMmc5::_SerializeTagSpecies() { return 8; }
/** @brief Provides the MMC5 state label. @return Mapper identifier. */
const unsigned char* CHvcMmc5::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-5  ");
}
/** @brief Initializes MMC5 bus tables and routes device accesses. @return Zero. */
unsigned int CHvcMmc5::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, mmc5CpuBankDefaults, sizeof(mmc5CpuBankDefaults));
        memcpy(mpEntity->ppuBanks, mmc5PpuBankDefaults, sizeof(mmc5PpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            case 5:
                if (address < 0x5016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Handles MMC5 reset and scanline interrupts. @return Zero. */
unsigned int CHvcMmc5::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        unsigned int banks = entity->prgBankCount << 1;
        mState[24] = 0;
        mState[25] = 0;
        mState[7] = banks - 1;
        mState[6] = banks - 2;
        mState[5] = banks - 3;
        mState[4] = banks - 4;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    if (mState[25]) {
        _SEntity* entity = mpEntity;
        if ((entity->cycle.line >> 12) < 15 && (args->values[0] & 0x10)) {
            if (mState[24])
                --mState[24];
            else {
                entity->order.words[4] |= 1;
                mState[24] = 0xff;
            }
        }
    }
    return 0;
}
/** @brief Routes MMC5 bus accesses, extended RAM, fill registers, and multiplication.
 * @param sender Requesting unit. @param category Unused category.
 * @param operation Read/write selector. @param args Bus arguments.
 * @return Original bus status, including six for mapper-register reads.
 */
unsigned int CHvcMmc5::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    unsigned int result = 6;
    if (sender->_18 == 3) {
        unsigned long address = args->values[2];
        if ((address >> 10) == 0x17) {
            unsigned char* memory = mpEntity->pMemory + 0x303000;
            if (operation == 1)
                memory[address - 0x5c00] = *static_cast<unsigned char*>(args->pointers[4]);
            else
                *static_cast<unsigned char*>(args->pointers[4]) = memory[address - 0x5c00];
            return 0;
        }
        if (mpEntity->cpuBanks[address >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[6] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            result = 0;
        }
        unsigned char* memory = mpEntity->pMemory;
        switch (mWriteAddress) {
        case 0x5106:
            if (operation == 1) {
                unsigned char value = mRegisters[6];
                unsigned char* fill = memory + 0x303400;
                memory[0x304116] = value;
                memory[0x303d16] = value;
                memset(fill, value, 0x3c0);
            } else
                *static_cast<unsigned char*>(args->pointers[4]) = memory[0x303d16];
            break;
        case 0x5107:
            if (operation == 1) {
                unsigned char value = mRegisters[6];
                memory[0x304117] = value;
                memory[0x303d17] = value;
                memset(memory + 0x3037c0, value, 0x40);
            } else
                *static_cast<unsigned char*>(args->pointers[4]) = memory[0x303d17];
            break;
        case 0x5205:
            if (operation == 1) {
                unsigned long value = mRegisters[6];
                memory[0x304215] = value;
                unsigned long product = memory[0x304216] * value;
                memory[0x303e15] = product;
                memory[0x303e16] = product >> 8;
            } else
                *static_cast<unsigned char*>(args->pointers[4]) = memory[0x303e15];
            break;
        case 0x5206:
            if (operation == 1) {
                unsigned long value = mRegisters[6];
                memory[0x304216] = value;
                unsigned long product = memory[0x304215] * value;
                memory[0x303e15] = product;
                memory[0x303e16] = product >> 8;
            } else
                *static_cast<unsigned char*>(args->pointers[4]) = memory[0x303e16];
            break;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return result;
}
/** @brief Applies MMC5 program, character, nametable, and interrupt register writes. */
void CHvcMmc5::_AdjustBusMap() {
    switch (mWriteAddress) {
    case 0x5100: {
        mState[0] = mRegisters[6] & 3;
        {
            unsigned long bank = mState[4];
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[8].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[9].readOffset = offset;
        }
        {
            unsigned long bank = mState[5];
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[10].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[11].readOffset = offset;
        }
        {
            unsigned long bank = mState[6];
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[12].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[13].readOffset = offset;
        }
        {
            unsigned long bank = mState[7];
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[14].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[15].readOffset = offset;
        }
        break;
    }
    case 0x5101: mState[1] = mRegisters[6] & 3; break;
    case 0x5102: {
        unsigned char flags = mState[2] & 1;
        if (mRegisters[6]) flags |= 2;
        mState[2] = flags;
        break;
    }
    case 0x5103: {
        unsigned char flags = mState[2] & 2;
        if (mRegisters[6]) flags |= 1;
        mState[2] = flags;
        break;
    }
    case 0x5104: mState[23] = mRegisters[6]; break;
    case 0x5105: {
        mState[20] = mRegisters[6];
        {
            unsigned long offset = 0x302800 + ((static_cast<unsigned long>(mState[20]) << 10) & 0xc00);
            mpEntity->ppuBanks[8].writeOffset = offset;
            mpEntity->ppuBanks[8].readOffset = offset;
        }
        {
            unsigned long offset = 0x302800 + ((static_cast<unsigned long>(mState[20]) << 8) & 0xc00);
            mpEntity->ppuBanks[9].writeOffset = offset;
            mpEntity->ppuBanks[9].readOffset = offset;
        }
        {
            unsigned long offset = 0x302800 + ((static_cast<unsigned long>(mState[20]) << 6) & 0xc00);
            mpEntity->ppuBanks[10].writeOffset = offset;
            mpEntity->ppuBanks[10].readOffset = offset;
        }
        {
            unsigned long offset = 0x302800 + ((static_cast<unsigned long>(mState[20]) << 4) & 0xc00);
            mpEntity->ppuBanks[11].writeOffset = offset;
            mpEntity->ppuBanks[11].readOffset = offset;
        }
        break;
    }
    case 0x5113: {
        unsigned long bank = mRegisters[6] & 3;
        mState[3] = bank;
        unsigned long offset = (bank << 13);
        mpEntity->cpuBanks[6].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[7].readOffset = offset;
        break;
    }
    case 0x5114: {
        unsigned long bank = static_cast<signed char>(mRegisters[6]);
        if (static_cast<signed char>(bank) < 0) {
            if (mState[0] == 3) {
                bank &= 0x7f;
                mState[4] = bank;
                unsigned long offset = (bank << 13);
                mpEntity->cpuBanks[8].readOffset = offset;
                offset += 0x1000;
                mpEntity->cpuBanks[9].readOffset = offset;
            }
        } else {
            mState[3] = bank & 3;
        }
        {
            unsigned long bank = mState[3];
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[6].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[7].readOffset = offset;
        }
        break;
    }
    case 0x5115: {
        unsigned long bank = static_cast<signed char>(mRegisters[6]);
        if (static_cast<signed char>(bank) < 0) {
            if (mState[0] == 1 || mState[0] == 2) {
                bank &= 0x7e;
                mState[4] = bank;
                mState[5] = bank + 1;
                unsigned long offset = (bank << 14);
                mpEntity->cpuBanks[8].readOffset = offset;
                offset += 0x1000;
                mpEntity->cpuBanks[9].readOffset = offset;
                offset += 0x1000;
                mpEntity->cpuBanks[10].readOffset = offset;
                offset += 0x1000;
                mpEntity->cpuBanks[11].readOffset = offset;
            } else if (mState[0] == 3) {
                bank &= 0x7f;
                mState[5] = bank;
                unsigned long offset = (bank << 13);
                mpEntity->cpuBanks[10].readOffset = offset;
                offset += 0x1000;
                mpEntity->cpuBanks[11].readOffset = offset;
            }
        } else {
            mState[3] = bank & 3;
        }
        {
            unsigned long bank = mState[3];
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[6].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[7].readOffset = offset;
        }
        break;
    }
    case 0x5116: {
        unsigned long bank = static_cast<signed char>(mRegisters[6]);
        if (static_cast<signed char>(bank) < 0) {
            if (mState[0] >= 2) {
                bank &= 0x7f;
                mState[6] = bank;
                unsigned long offset = (bank << 13);
                mpEntity->cpuBanks[12].readOffset = offset;
                offset += 0x1000;
                mpEntity->cpuBanks[13].readOffset = offset;
            }
        } else {
            mState[3] = bank & 3;
        }
        {
            unsigned long bank = mState[3];
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[6].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[7].readOffset = offset;
        }
        break;
    }
    case 0x5117:
        switch (mState[0]) {
        case 0: {
            unsigned long bank = mRegisters[6] & 0x7c;
            mState[4] = bank + 0;
            mState[5] = bank + 1;
            mState[6] = bank + 2;
            mState[7] = bank + 3;
            unsigned long offset = (bank << 15);
            mpEntity->cpuBanks[8].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[9].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[10].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[11].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[12].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[13].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[14].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[15].readOffset = offset;
            break;
        }
        case 1: {
            unsigned long bank = mRegisters[6] & 0x7e;
            mState[6] = bank + 0;
            mState[7] = bank + 1;
            unsigned long offset = (bank << 14);
            mpEntity->cpuBanks[12].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[13].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[14].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[15].readOffset = offset;
            break;
        }
        case 2: case 3: {
            unsigned long bank = mRegisters[6] & 0x7f;
            mState[7] = bank + 0;
            unsigned long offset = (bank << 13);
            mpEntity->cpuBanks[14].readOffset = offset;
            offset += 0x1000;
            mpEntity->cpuBanks[15].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5120:
        switch (mState[1]) {
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[8] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[0].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5121:
        switch (mState[1]) {
        case 2: {
            unsigned long bank = mRegisters[6];
            mState[8] = bank + 0;
            mState[9] = bank + 1;
            unsigned long offset = 0x200000 + (bank << 11);
            mpEntity->ppuBanks[0].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[1].readOffset = offset;
            break;
        }
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[9] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[1].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5122:
        switch (mState[1]) {
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[10] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[2].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5123:
        switch (mState[1]) {
        case 1: {
            unsigned long bank = mRegisters[6];
            mState[8] = bank + 0;
            mState[9] = bank + 1;
            mState[10] = bank + 2;
            mState[11] = bank + 3;
            unsigned long offset = 0x200000 + (bank << 12);
            mpEntity->ppuBanks[0].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[1].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[2].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[3].readOffset = offset;
            break;
        }
        case 2: {
            unsigned long bank = mRegisters[6];
            mState[10] = bank + 0;
            mState[11] = bank + 1;
            unsigned long offset = 0x200000 + (bank << 11);
            mpEntity->ppuBanks[2].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[3].readOffset = offset;
            break;
        }
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[11] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[3].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5124:
        switch (mState[1]) {
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[12] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[4].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5125:
        switch (mState[1]) {
        case 2: {
            unsigned long bank = mRegisters[6];
            mState[12] = bank + 0;
            mState[13] = bank + 1;
            unsigned long offset = 0x200000 + (bank << 11);
            mpEntity->ppuBanks[4].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[5].readOffset = offset;
            break;
        }
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[13] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[5].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5126:
        switch (mState[1]) {
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[14] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[6].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5127:
        switch (mState[1]) {
        case 0: {
            unsigned long bank = mRegisters[6];
            mState[8] = bank + 0;
            mState[9] = bank + 1;
            mState[10] = bank + 2;
            mState[11] = bank + 3;
            mState[12] = bank + 4;
            mState[13] = bank + 5;
            mState[14] = bank + 6;
            mState[15] = bank + 7;
            unsigned long offset = 0x200000 + (bank << 13);
            mpEntity->ppuBanks[0].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[1].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[2].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[3].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[4].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[5].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[6].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        case 1: {
            unsigned long bank = mRegisters[6];
            mState[12] = bank + 0;
            mState[13] = bank + 1;
            mState[14] = bank + 2;
            mState[15] = bank + 3;
            unsigned long offset = 0x200000 + (bank << 12);
            mpEntity->ppuBanks[4].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[5].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[6].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        case 2: {
            unsigned long bank = mRegisters[6];
            mState[14] = bank + 0;
            mState[15] = bank + 1;
            unsigned long offset = 0x200000 + (bank << 11);
            mpEntity->ppuBanks[6].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[15] = bank + 0;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5128:
        switch (mState[1]) {
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[16] = bank + 0;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[4].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5129:
        switch (mState[1]) {
        case 2: {
            unsigned long bank = mRegisters[6];
            mState[16] = bank + 0;
            mState[17] = bank + 1;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 11);
            mpEntity->ppuBanks[4].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[5].readOffset = offset;
            break;
        }
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[17] = bank + 0;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[5].readOffset = offset;
            break;
        }
        }
        break;
    case 0x512a:
        switch (mState[1]) {
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[18] = bank + 0;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[6].readOffset = offset;
            break;
        }
        }
        break;
    case 0x512b:
        switch (mState[1]) {
        case 0: {
            unsigned long bank = mRegisters[6];
            mState[8] = bank + 0;
            mState[9] = bank + 1;
            mState[10] = bank + 2;
            mState[11] = bank + 3;
            mState[12] = bank + 4;
            mState[13] = bank + 5;
            mState[14] = bank + 6;
            mState[15] = bank + 7;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 13);
            mpEntity->ppuBanks[0].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[1].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[2].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[3].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[4].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[5].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[6].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        case 1: {
            unsigned long bank = mRegisters[6];
            mState[16] = bank + 0;
            mState[17] = bank + 1;
            mState[18] = bank + 2;
            mState[19] = bank + 3;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 12);
            mpEntity->ppuBanks[4].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[5].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[6].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        case 2: {
            unsigned long bank = mRegisters[6];
            mState[18] = bank + 0;
            mState[19] = bank + 1;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 11);
            mpEntity->ppuBanks[6].readOffset = offset;
            offset += 0x400;
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        case 3: {
            unsigned long bank = mRegisters[6];
            mState[19] = bank + 0;
            if (mState[23] == 1) break;
            unsigned long offset = 0x200000 + (bank << 10);
            mpEntity->ppuBanks[7].readOffset = offset;
            break;
        }
        }
        break;
    case 0x5203: mState[24] = mRegisters[6]; break;
    case 0x5204: mState[25] = (mRegisters[6] >> 6) & 2; break;
    }
}
/** @brief Initializes mapper 45 identity. */
CHvcMapper45::CHvcMapper45() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - Chr  ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMapper45::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMapper45::_Epilogue() { return 0; }
/** @brief Initializes mapper 45 bus tables and resolves devices. @return Zero. */
unsigned int CHvcMapper45::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, chrCpuBankDefaults, sizeof(chrCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);

    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a mapper 45 write. @return Bus status. */
unsigned int CHvcMapper45::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers mapper 45 state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcMapper45::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    stream.Entry(mBanks[0]);
    stream.Entry(mBanks[1]);
    stream.Entry(mBanks[2]);
    stream.Entry(mBanks[3]);
    stream.Entry(mBanks[4]);
    stream.Entry(mBanks[5]);
    stream.Entry(mBanks[6]);
    stream.Entry(mBanks[7]);
    stream.Entry(mBanks[8]);
    stream.Entry(mBanks[9]);
    stream.Entry(mBanks[10]);
    stream.Entry(mBanks[11]);
    stream.Entry(mBanks[12]);
    stream.Entry(mBanks[13]);
    stream.Entry(mControl);
    stream.Entry(mEnabled);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcMapper45::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcMapper45::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcMapper45::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Nine. */
unsigned char CHvcMapper45::_SerializeTagSpecies() { return 9; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcMapper45::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-x45");
}
/** @brief Applies mapper 45 bank, mirroring, and interrupt writes. */
void CHvcMapper45::_AdjustBusMap() {
    unsigned int upper, lower;
    switch (mWriteAddress) {
    case 0x8000:
        mSelectedBank = mRegisters[1] & 15;
        break;
    case 0xa000:
        switch (mSelectedBank) {
        case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
            mChrBanks[mSelectedBank] = mRegisters[1];
            mpEntity->ppuBanks[mSelectedBank].readOffset = 0x200000 + (mChrBanks[mSelectedBank] << 10);
            break;
        case 8:
            if (!(mRegisters[1] & 0x40)) {
                unsigned long bank = mRegisters[1] & 0x3f;
                mBanks[0] = bank;
                unsigned long offset = bank << 13;
                mpEntity->cpuBanks[6].readOffset = offset;
                mpEntity->cpuBanks[7].readOffset = offset | 0x1000;
            }
            break;
        case 9: {
            unsigned long bank = mRegisters[1];
            mBanks[1] = bank;
            unsigned long offset = (bank & 15) << 13;
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            break;
        }
        case 10: {
            unsigned long bank = mRegisters[1];
            mBanks[2] = bank;
            unsigned long offset = (bank & 15) << 13;
            mpEntity->cpuBanks[10].readOffset = offset;
            mpEntity->cpuBanks[11].readOffset = offset | 0x1000;
            break;
        }
        case 11: {
            unsigned long bank = mRegisters[1];
            mBanks[3] = bank;
            unsigned long offset = (bank & 15) << 13;
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            break;
        }
        case 12:
            switch (mRegisters[1] & 3) {
            case 0:
                mpEntity->ppuBanks[8].writeOffset = 0x302800;
                mpEntity->ppuBanks[8].readOffset = 0x302800;
                mpEntity->ppuBanks[9].writeOffset = 0x302c00;
                mpEntity->ppuBanks[9].readOffset = 0x302c00;
                mpEntity->ppuBanks[10].writeOffset = 0x302800;
                mpEntity->ppuBanks[10].readOffset = 0x302800;
                mpEntity->ppuBanks[11].writeOffset = 0x302c00;
                mpEntity->ppuBanks[11].readOffset = 0x302c00;
                break;
            case 1:
                mpEntity->ppuBanks[8].writeOffset = 0x302800;
                mpEntity->ppuBanks[8].readOffset = 0x302800;
                mpEntity->ppuBanks[9].writeOffset = 0x302800;
                mpEntity->ppuBanks[9].readOffset = 0x302800;
                mpEntity->ppuBanks[10].writeOffset = 0x302c00;
                mpEntity->ppuBanks[10].readOffset = 0x302c00;
                mpEntity->ppuBanks[11].writeOffset = 0x302c00;
                mpEntity->ppuBanks[11].readOffset = 0x302c00;
                break;
            case 2:
                mpEntity->ppuBanks[8].writeOffset = 0x302800;
                mpEntity->ppuBanks[8].readOffset = 0x302800;
                mpEntity->ppuBanks[9].writeOffset = 0x302800;
                mpEntity->ppuBanks[9].readOffset = 0x302800;
                mpEntity->ppuBanks[10].writeOffset = 0x302800;
                mpEntity->ppuBanks[10].readOffset = 0x302800;
                mpEntity->ppuBanks[11].writeOffset = 0x302800;
                mpEntity->ppuBanks[11].readOffset = 0x302800;
                break;
            case 3:
                mpEntity->ppuBanks[8].writeOffset = 0x302c00;
                mpEntity->ppuBanks[8].readOffset = 0x302c00;
                mpEntity->ppuBanks[9].writeOffset = 0x302c00;
                mpEntity->ppuBanks[9].readOffset = 0x302c00;
                mpEntity->ppuBanks[10].writeOffset = 0x302c00;
                mpEntity->ppuBanks[10].readOffset = 0x302c00;
                mpEntity->ppuBanks[11].writeOffset = 0x302c00;
                mpEntity->ppuBanks[11].readOffset = 0x302c00;
                break;
            }
            break;
        case 13:
            switch (mRegisters[1]) {
            case 0: mEnabled = 0; break;
            case 0x81: mEnabled = 1; break;
            }
            break;
        case 14:
            upper = (mControl >> 8) & 0xff;
            lower = mRegisters[1];
            goto storeCounter;
        case 15:
            upper = mRegisters[1];
            lower = mControl & 0xff;
            goto storeCounter;
        }
        break;
    }
    return;
storeCounter:
    mControl = (upper << 8) | lower;
}
/** @brief Resets mapper 45 and advances its cycle-based interrupt countdown. @return Zero. */
unsigned int CHvcMapper45::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mControl = 0;
        mEnabled = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    if (mEnabled && (args->values[0] & 0x10)) {
        mControl -= 114;
        if (static_cast<int>(mControl) <= 0) {
            mControl += 0x10000;
            mEnabled = 0;
            mpEntity->order.words[4] |= 1;
        }
    }
    return 0;
}
/** @brief Initializes mapper 57 identity. */
CHvcMapper57::CHvcMapper57() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - Chr  ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMapper57::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMapper57::_Epilogue() { return 0; }
/** @brief Initializes mapper 57 bus tables and resolves devices. @return Zero. */
unsigned int CHvcMapper57::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, chrCpuBankDefaults, sizeof(chrCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);

    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a mapper 57 write. @return Bus status. */
unsigned int CHvcMapper57::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers mapper 57 state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcMapper57::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcMapper57::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcMapper57::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcMapper57::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Ten. */
unsigned char CHvcMapper57::_SerializeTagSpecies() { return 10; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcMapper57::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-x57");
}
/** @brief Applies the pending mapper 57 character-bank write. */
void CHvcMapper57::_AdjustBusMap() {
    if ((mWriteAddress & 0xe000) == 0x6000) {
        mRegisters[0] = mRegisters[1];
        unsigned long bank = (mRegisters[0] >> 1) & 1;
        unsigned long offset = 0x200000 + (bank << 13);
        mpEntity->ppuBanks[0].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[1].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[2].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[3].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[4].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[5].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[6].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[7].readOffset = offset;
    }
}
/** @brief Applies mapper 57 reset requests. @return Zero. */
unsigned int CHvcMapper57::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if (entity->order.words[0] & (1u << _18)) {
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Initializes mapper 5F identity. */
CHvcMapper5F::CHvcMapper5F() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMapper5F::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMapper5F::_Epilogue() { return 0; }
/** @brief Initializes mapper 5F bus tables and resolves devices. @return Zero. */
unsigned int CHvcMapper5F::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a mapper 5F write. @return Bus status. */
unsigned int CHvcMapper5F::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[5] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers mapper 5F state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcMapper5F::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mRegisters[2]);
    stream.Entry(mRegisters[3]);
    stream.Entry(mRegisters[4]);
    stream.Entry(mRegisters[5]);
    stream.Entry(mRegisters[6]);
    stream.Entry(mRegisters[7]);
    stream.Entry(mBanks[0]);
    stream.Entry(mBanks[1]);
    stream.Entry(mBanks[2]);
    stream.Entry(mBanks[3]);
    stream.Entry(mBanks[4]);
    stream.Entry(mBanks[5]);
    stream.Entry(mBanks[6]);
    stream.Entry(mBanks[7]);
    stream.Entry(mWriteAddress);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcMapper5F::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcMapper5F::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcMapper5F::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Eleven. */
unsigned char CHvcMapper5F::_SerializeTagSpecies() { return 11; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcMapper5F::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-x5F");
}
/** @brief Applies mapper 5F reset requests. @return Zero. */
unsigned int CHvcMapper5F::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mRegisters[6] = 0;
        mRegisters[7] = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Applies mapper 5F program, character, and nametable bank writes. */
void CHvcMapper5F::_AdjustBusMap() {
    switch (static_cast<unsigned int>(mWriteAddress) & 0xe001) {
    case 0x8000: {
        unsigned char value = mRegisters[5];
        mRegisters[0] = value;
        mRegisters[2] = value & 7;
        mRegisters[3] = (value >> 6) & 1;
        mRegisters[4] = value >> 7;
        unsigned long bank = static_cast<unsigned long>(mRegisters[6]) & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
        unsigned long offset = bank << 13;
        if (mRegisters[3]) {
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
        } else {
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
        }
        break;
    }
    case 0x8001: {
        unsigned char value = mRegisters[5];
        mRegisters[1] = value;
        if (mRegisters[0] <= 5) {
            _SEntity* entity = mpEntity;
            unsigned long offset = (value & 0x20) ? 0x302c00 : 0x302800;
            value &= 0x1f;
            entity->ppuBanks[8].writeOffset = offset;
            mpEntity->ppuBanks[8].readOffset = offset;
            mpEntity->ppuBanks[9].writeOffset = offset;
            mpEntity->ppuBanks[9].readOffset = offset;
            mpEntity->ppuBanks[10].writeOffset = offset;
            mpEntity->ppuBanks[10].readOffset = offset;
            mpEntity->ppuBanks[11].writeOffset = offset;
            mpEntity->ppuBanks[11].readOffset = offset;
        }
        switch (mRegisters[2]) {
        case 0: {
            mBanks[0] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[4]) {
                mpEntity->ppuBanks[4].readOffset = offset;
                mpEntity->ppuBanks[5].readOffset = offset | 0x400;
            } else {
                mpEntity->ppuBanks[0].readOffset = offset;
                mpEntity->ppuBanks[1].readOffset = offset | 0x400;
            }
            break;
        }
        case 1: {
            mBanks[2] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xfe;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[4]) {
                mpEntity->ppuBanks[6].readOffset = offset;
                mpEntity->ppuBanks[7].readOffset = offset | 0x400;
            } else {
                mpEntity->ppuBanks[2].readOffset = offset;
                mpEntity->ppuBanks[3].readOffset = offset | 0x400;
            }
            break;
        }
        case 2: {
            mBanks[4] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[4]) {
                mpEntity->ppuBanks[0].readOffset = offset;
            } else {
                mpEntity->ppuBanks[4].readOffset = offset;
            }
            break;
        }
        case 3: {
            mBanks[5] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[4]) {
                mpEntity->ppuBanks[1].readOffset = offset;
            } else {
                mpEntity->ppuBanks[5].readOffset = offset;
            }
            break;
        }
        case 4: {
            mBanks[6] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[4]) {
                mpEntity->ppuBanks[2].readOffset = offset;
            } else {
                mpEntity->ppuBanks[6].readOffset = offset;
            }
            break;
        }
        case 5: {
            mBanks[7] = value;
            unsigned long bank = value & ((mpEntity->chrBankCount << 3) - 1) & 0xff;
            unsigned long offset = 0x200000 | (bank << 10);
            if (mRegisters[4]) {
                mpEntity->ppuBanks[3].readOffset = offset;
            } else {
                mpEntity->ppuBanks[7].readOffset = offset;
            }
            break;
        }
        case 6: {
            mRegisters[6] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            unsigned long offset = bank << 13;
            if (mRegisters[3]) {
                mpEntity->cpuBanks[12].readOffset = offset;
                mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
            } else {
                mpEntity->cpuBanks[8].readOffset = offset;
                mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
            }
            break;
        }
        case 7: {
            mRegisters[7] = value;
            unsigned long bank = value & ((mpEntity->prgBankCount << 1) - 1) & 0x3f;
            mpEntity->cpuBanks[10].readOffset = bank << 13;
            mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
            break;
        }
        }
        break;
    }
    }
}
/** @brief Initializes mapper B8 identity. */
CHvcMapperB8::CHvcMapperB8() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - Chr  ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcMapperB8::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcMapperB8::_Epilogue() { return 0; }
/** @brief Initializes mapper B8 bus tables and resolves devices. @return Zero. */
unsigned int CHvcMapperB8::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, chrCpuBankDefaults, sizeof(chrCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);

    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a mapper B8 write. @return Bus status. */
unsigned int CHvcMapperB8::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers mapper B8 state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcMapperB8::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcMapperB8::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcMapperB8::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcMapperB8::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Twelve. */
unsigned char CHvcMapperB8::_SerializeTagSpecies() { return 12; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcMapperB8::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-xB8");
}
/** @brief Applies the pending mapper B8 character-bank write. */
void CHvcMapperB8::_AdjustBusMap() {
    if ((mWriteAddress & 0xe000) == 0x6000) {
        unsigned long mask = ((mpEntity->chrBankCount << 3) - 1) & 0x3c;
        unsigned long value = mRegisters[1];
        unsigned long lower = (value << 2) & mask;
        unsigned long upper = (value >> 2) & mask;
        mpEntity->ppuBanks[0].readOffset = 0x200000 + (lower << 10);
        mpEntity->ppuBanks[1].readOffset = 0x200400 + (lower << 10);
        mpEntity->ppuBanks[2].readOffset = 0x200800 + (lower << 10);
        mpEntity->ppuBanks[3].readOffset = 0x200c00 + (lower << 10);
        mpEntity->ppuBanks[4].readOffset = 0x200000 + (upper << 10);
        mpEntity->ppuBanks[5].readOffset = 0x200400 + (upper << 10);
        mpEntity->ppuBanks[6].readOffset = 0x200800 + (upper << 10);
        mpEntity->ppuBanks[7].readOffset = 0x200c00 + (upper << 10);
    }
}
/** @brief Applies mapper B8 reset requests. @return Zero. */
unsigned int CHvcMapperB8::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if (entity->order.words[0] & (1u << _18)) {
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Initializes VRC1 identity. */
CHvcVrc1::CHvcVrc1() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcVrc1::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcVrc1::_Epilogue() { return 0; }
/** @brief Initializes VRC1 bus tables and resolves devices. @return Zero. */
unsigned int CHvcVrc1::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a VRC1 write. @return Bus status. */
unsigned int CHvcVrc1::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers VRC1 state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcVrc1::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    stream.Entry(mState[0]);
    stream.Entry(mState[1]);
    stream.Entry(mState[2]);
    stream.Entry(mState[3]);
    stream.Entry(mState[4]);
    stream.Entry(mState[5]);
    stream.Entry(mState[6]);
    stream.Entry(mState[7]);
    stream.Entry(mState[8]);
    stream.Entry(mState[9]);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcVrc1::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcVrc1::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcVrc1::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Thirteen. */
unsigned char CHvcVrc1::_SerializeTagSpecies() { return 13; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcVrc1::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-V1 ");
}
/** @brief Applies VRC1 reset requests. @return Zero. */
unsigned int CHvcVrc1::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mState[2] = 0;
        mState[3] = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Applies VRC1 program, character, and nametable bank writes. */
void CHvcVrc1::_AdjustBusMap() {
    switch (mWriteAddress >> 12) {
    case 8: {
        unsigned long bank = mRegisters[1] & 0xf;
        mpEntity->cpuBanks[8].readOffset = bank << 13;
        mpEntity->cpuBanks[9].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 9: {
        _SEntity* entity = mpEntity;
        unsigned char control = mRegisters[1];
        mState[2] = (mState[2] & 0x3c) | ((control << 5) & 0x40);
        mState[3] = (mState[3] & 0x3c) | ((control << 4) & 0x40);
        unsigned long offset0 = 0x200000 + (static_cast<unsigned long>(mState[2]) << 10);
        entity->ppuBanks[0].readOffset = offset0 + 0x0;
        mpEntity->ppuBanks[1].readOffset = offset0 + 0x400;
        mpEntity->ppuBanks[2].readOffset = offset0 + 0x800;
        mpEntity->ppuBanks[3].readOffset = offset0 + 0xc00;
        unsigned long offset4 = 0x200000 + (static_cast<unsigned long>(mState[3]) << 10);
        mpEntity->ppuBanks[4].readOffset = offset4 + 0x0;
        mpEntity->ppuBanks[5].readOffset = offset4 + 0x400;
        mpEntity->ppuBanks[6].readOffset = offset4 + 0x800;
        mpEntity->ppuBanks[7].readOffset = offset4 + 0xc00;
        if (mRegisters[1] & 1) {
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
        } else {
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
        }
        break;
    }
    case 10: {
        unsigned long bank = mRegisters[1] & 0xf;
        mpEntity->cpuBanks[10].readOffset = bank << 13;
        mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 15: {
        mState[3] = (mState[3] & 0x40) | ((mRegisters[1] << 2) & 0x3c);
        unsigned long offset = 0x200000 + (static_cast<unsigned long>(mState[3]) << 10);
        mpEntity->ppuBanks[4].readOffset = offset + 0x0;
        offset += 0x400;
        mpEntity->ppuBanks[5].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[6].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[7].readOffset = offset;
        break;
    }
    case 12: {
        unsigned long bank = mRegisters[1] & 0xf;
        mpEntity->cpuBanks[12].readOffset = bank << 13;
        mpEntity->cpuBanks[13].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 14: {
        mState[2] = (mState[2] & 0x40) | ((mRegisters[1] << 2) & 0x3c);
        unsigned long offset = 0x200000 + (static_cast<unsigned long>(mState[2]) << 10);
        mpEntity->ppuBanks[0].readOffset = offset + 0x0;
        offset += 0x400;
        mpEntity->ppuBanks[1].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[2].readOffset = offset;
        offset += 0x400;
        mpEntity->ppuBanks[3].readOffset = offset;
        break;
    }
    }
}
/** @brief Initializes VRC2b identity. */
CHvcVrc2b::CHvcVrc2b() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcVrc2b::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcVrc2b::_Epilogue() { return 0; }
/** @brief Initializes VRC2b bus tables and resolves devices. @return Zero. */
unsigned int CHvcVrc2b::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a VRC2b write. @return Bus status. */
unsigned int CHvcVrc2b::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers VRC2b state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcVrc2b::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    stream.Entry(mState[0]);
    stream.Entry(mState[1]);
    stream.Entry(mState[2]);
    stream.Entry(mState[3]);
    stream.Entry(mState[4]);
    stream.Entry(mState[5]);
    stream.Entry(mState[6]);
    stream.Entry(mState[7]);
    stream.Entry(mState[8]);
    stream.Entry(mState[9]);
    stream.Entry(mState[10]);
    stream.Entry(mState[11]);
    stream.Entry(mState[12]);
    stream.Entry(mState[13]);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcVrc2b::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcVrc2b::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcVrc2b::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Fourteen. */
unsigned char CHvcVrc2b::_SerializeTagSpecies() { return 14; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcVrc2b::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-V2b");
}
/** @brief Applies VRC2b reset requests. @return Zero. */
unsigned int CHvcVrc2b::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mState[3] = 0;
        mState[4] = 0;
        mState[5] = 0;
        mState[6] = 0;
        mState[7] = 0;
        mState[8] = 0;
        mState[9] = 0;
        mState[10] = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Applies VRC2b program, character, and nametable writes. */
void CHvcVrc2b::_AdjustBusMap() {
    switch (mWriteAddress >> 12) {
    case 11: case 12: case 13: case 14: {
        unsigned int index = ((mWriteAddress - 0xb000) >> 11) | ((mWriteAddress >> 1) & 1);
        if (mWriteAddress & 1)
            mState[3 + index] = (mState[3 + index] & 0xf) | ((mRegisters[1] << 4) & 0x70);
        else
            mState[3 + index] = (mState[3 + index] & 0x70) | (mRegisters[1] & 0xf);
        mpEntity->ppuBanks[index].readOffset = 0x200000 + (static_cast<unsigned long>(mState[3 + index]) << 10);
        break;
    }
    case 8: {
        unsigned long bank = mRegisters[1] & 0xf;
        mpEntity->cpuBanks[8].readOffset = bank << 13;
        mpEntity->cpuBanks[9].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 10: {
        unsigned long bank = mRegisters[1] & 0xf;
        mpEntity->cpuBanks[10].readOffset = bank << 13;
        mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 9:
        switch (mRegisters[1] & 3) {
        case 0:
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
            break;
        case 1:
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
            break;
        case 2:
            mpEntity->ppuBanks[8].writeOffset = 0x302c00;
            mpEntity->ppuBanks[8].readOffset = 0x302c00;
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
            break;
        case 3:
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
            mpEntity->ppuBanks[11].writeOffset = 0x302800;
            mpEntity->ppuBanks[11].readOffset = 0x302800;
            break;
        }
        break;
    }
}
/** @brief Initializes VRC3 identity. */
CHvcVrc3::CHvcVrc3() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcVrc3::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcVrc3::_Epilogue() { return 0; }
/** @brief Initializes VRC3 bus tables and resolves devices. @return Zero. */
unsigned int CHvcVrc3::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a VRC3 write. @return Bus status. */
unsigned int CHvcVrc3::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers VRC3 state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcVrc3::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    stream.Entry(mBank);
    stream.Entry(mControl);
    stream.Entry(mEnabled);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcVrc3::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcVrc3::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcVrc3::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Fifteen. */
unsigned char CHvcVrc3::_SerializeTagSpecies() { return 15; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcVrc3::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-V3 ");
}
/** @brief Handles VRC3 reset requests and interrupt countdowns. @return Zero. */
unsigned int CHvcVrc3::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mControl = 0;
        mEnabled = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    if (mEnabled && (args->values[0] & 0x10)) {
        mControl -= 114;
        if (static_cast<int>(mControl) <= 0) {
            mControl += 0x10000;
            mEnabled = 0;
            mpEntity->order.words[4] |= 1;
        }
    }
    return 0;
}
/** @brief Applies VRC3 counter and program-bank writes. */
void CHvcVrc3::_AdjustBusMap() {
    switch (mWriteAddress >> 12) {
    case 8:
        mControl = (mControl & 0xfff0) | ((~mRegisters[1] & 0xf) << 0);
        break;
    case 9:
        mControl = (mControl & 0xff0f) | ((~mRegisters[1] & 0xf) << 4);
        break;
    case 10:
        mControl = (mControl & 0xf0ff) | ((~mRegisters[1] & 0xf) << 8);
        break;
    case 11:
        mControl = (mControl & 0xfff) | ((~mRegisters[1] & 0xf) << 12);
        break;
    case 12:
        mEnabled = (mRegisters[1] >> 1) & 1;
        break;
    case 15: {
        unsigned long bank = mRegisters[1] & 7;
        mBank = bank;
        unsigned long offset = bank << 14;
        mpEntity->cpuBanks[8].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[9].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[10].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[11].readOffset = offset;
        break;
    }
    }
}
/** @brief Initializes VRC4 identity. */
CHvcVrc4::CHvcVrc4() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcVrc4::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcVrc4::_Epilogue() { return 0; }
/** @brief Initializes VRC4 bus tables and resolves devices. @return Zero. */
unsigned int CHvcVrc4::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a VRC4 write. @return Bus status. */
unsigned int CHvcVrc4::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers VRC4 state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcVrc4::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    stream.Entry(mState[0]);
    stream.Entry(mState[1]);
    stream.Entry(mState[2]);
    stream.Entry(mState[3]);
    stream.Entry(mState[4]);
    stream.Entry(mState[5]);
    stream.Entry(mState[6]);
    stream.Entry(mState[7]);
    stream.Entry(mState[8]);
    stream.Entry(mState[9]);
    stream.Entry(mState[10]);
    stream.Entry(mState[11]);
    stream.Entry(mState[12]);
    stream.Entry(mIrqCounter);
    stream.Entry(mIrqEnabled);
    stream.Entry(mIrqControl);
    stream.Entry(mRamControl);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcVrc4::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcVrc4::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcVrc4::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Sixteen. */
unsigned char CHvcVrc4::_SerializeTagSpecies() { return 16; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcVrc4::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-V4 ");
}
/** @brief Handles VRC4 reset requests and interrupt countdowns. @return Zero. */
unsigned int CHvcVrc4::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mIrqEnabled = 0;
        mState[0] = 0;
        mState[1] = 0;
        mState[2] = 0;
        mState[3] = 0;
        mState[4] = 0;
        mState[5] = 0;
        mState[6] = 0;
        mState[7] = 0;
        mState[8] = 0;
        mState[9] = 0;
        mState[10] = 0;
        mState[11] = 0;
        mState[12] = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    if (mIrqEnabled && (args->values[0] & 0x10)) {
        ++mIrqCounter;
        if (mIrqCounter >= 0x100) {
            mIrqCounter = mState[12];
            mpEntity->order.words[4] |= 1;
        }
    }
    return 0;
}
/** @brief Applies VRC4 bank, mirroring, and interrupt-control writes. */
void CHvcVrc4::_AdjustBusMap() {
    unsigned int page = mWriteAddress >> 12;
    unsigned int sub = ((mWriteAddress & 1) << 1) | ((mWriteAddress >> 1) & 1);
    switch (page) {
    case 11: case 12: case 13: case 14: {
        unsigned int index = ((page - 11) << 1) | (sub >> 1);
        unsigned char value = mRegisters[1];
        if (sub & 1) {
            mChrBanks[index] &= 0xf;
            mChrBanks[index] |= (value << 4) & 0x70;
        } else {
            mChrBanks[index] &= 0x70;
            mChrBanks[index] |= value & 0xf;
        }
        mpEntity->ppuBanks[index].readOffset = 0x200000 + (static_cast<unsigned long>(mChrBanks[index]) << 10);
        break;
    }
    case 8: {
        unsigned long selected = mState[0];
        unsigned long value = mRegisters[1];
        mPrgBanks[selected] = value;
        unsigned long offset = (value & 0xf) << 13;
        if (selected) {
            mpEntity->cpuBanks[12].readOffset = offset;
            mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
        } else {
            mpEntity->cpuBanks[8].readOffset = offset;
            mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
        }
        break;
    }
    case 9: {
        unsigned char value = mRegisters[1];
        if (sub == 0) {
        switch (value & 3) {
        case 0:
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
            break;
        case 1:
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
            break;
        case 2:
            mpEntity->ppuBanks[8].writeOffset = 0x302c00;
            mpEntity->ppuBanks[8].readOffset = 0x302c00;
            mpEntity->ppuBanks[9].writeOffset = 0x302c00;
            mpEntity->ppuBanks[9].readOffset = 0x302c00;
            mpEntity->ppuBanks[10].writeOffset = 0x302c00;
            mpEntity->ppuBanks[10].readOffset = 0x302c00;
            mpEntity->ppuBanks[11].writeOffset = 0x302c00;
            mpEntity->ppuBanks[11].readOffset = 0x302c00;
            break;
        case 3:
            mpEntity->ppuBanks[8].writeOffset = 0x302800;
            mpEntity->ppuBanks[8].readOffset = 0x302800;
            mpEntity->ppuBanks[9].writeOffset = 0x302800;
            mpEntity->ppuBanks[9].readOffset = 0x302800;
            mpEntity->ppuBanks[10].writeOffset = 0x302800;
            mpEntity->ppuBanks[10].readOffset = 0x302800;
            mpEntity->ppuBanks[11].writeOffset = 0x302800;
            mpEntity->ppuBanks[11].readOffset = 0x302800;
            break;
        }
        } else {
            mRamControl = value & 1;
            unsigned int selected = value & 2;
            if (mState[0] != selected) {
                unsigned long offset = static_cast<unsigned long>(mPrgBanks[selected]) << 13;
                if (selected) {
                    mpEntity->cpuBanks[8].readOffset = offset;
                    mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
                } else {
                    mpEntity->cpuBanks[12].readOffset = offset;
                    mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
                }
                offset = (static_cast<unsigned long>(mpEntity->prgBankCount) << 14) - 0x4000;
                if (mState[0]) {
                    mpEntity->cpuBanks[8].readOffset = offset;
                    mpEntity->cpuBanks[9].readOffset = offset | 0x1000;
                } else {
                    mpEntity->cpuBanks[12].readOffset = offset;
                    mpEntity->cpuBanks[13].readOffset = offset | 0x1000;
                }
                mState[0] = selected;
            }
        }
        break;
    }
    case 10: {
        unsigned long value = mRegisters[1];
        mState[2] = value;
        unsigned long bank = value & 0xf;
        mpEntity->cpuBanks[10].readOffset = bank << 13;
        mpEntity->cpuBanks[11].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 15:
        switch (sub) {
        case 0:
            mState[12] = (mState[12] & 0xf0) | (mRegisters[1] & 0xf);
            break;
        case 1:
            mState[12] = (mState[12] & 0xf) | (mRegisters[1] << 4);
            break;
        case 2:
            mIrqCounter = mState[12];
            mIrqControl = mRegisters[1] & 3;
            mIrqEnabled = (mRegisters[1] & 2) ? 3 : 0;
            break;
        case 3:
            mIrqEnabled = (mIrqControl & 1) ? 3 : 0;
            break;
        }
        break;
    }
}
/** @brief Initializes disk mapper identity. */
CHvcDisk::CHvcDisk() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcDisk::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcDisk::_Epilogue() { return 0; }
/** @brief Initializes disk mapper bus tables and resolves devices. @return Zero. */
unsigned int CHvcDisk::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, diskCpuBankDefaults, sizeof(diskCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xffc0);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);

    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address < 0x40a0) device = diskDevices[address & 0xff];
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a disk mapper write. @return Bus status. */
unsigned int CHvcDisk::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[0] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
        mWriteAddress = args->values[2];
        unsigned char* data = static_cast<unsigned char*>(args->pointers[4]);
        *data = mRegisters[0];
        return 0;
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers disk mapper state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcDisk::_SerializeCore(void* buffer, bool reading) {
    return 0;
}
/** @brief Exports state. @param buffer State output. */
void CHvcDisk::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcDisk::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcDisk::_SerializeCoreInferSize() {
    return 16;
}
/** @brief Identifies the state species. @return Nineteen. */
unsigned char CHvcDisk::_SerializeTagSpecies() { return 19; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcDisk::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-DISK");
}
/** @brief Applies the pending disk mapper character-bank write. */
void CHvcDisk::_AdjustBusMap() {}
/** @brief Applies disk mapper reset requests. @return Zero. */
unsigned int CHvcDisk::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if (entity->order.words[0] & (1u << _18)) {
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    return 0;
}
/** @brief Initializes VRC6 identity. */
CHvcVrc6::CHvcVrc6() {
    _18 = 1;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    strncpy(mName, "HVC MMC - 1    ", 15);
}
/** @brief Provides the empty initialization hook. @return Zero. */
unsigned int CHvcVrc6::_Prologue() { return 0; }
/** @brief Provides the empty finalization hook. @return Zero. */
unsigned int CHvcVrc6::_Epilogue() { return 0; }
/** @brief Initializes VRC6 bus tables and resolves devices. @return Zero. */
unsigned int CHvcVrc6::System(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (operation == 0) {
        memcpy(mpEntity->cpuBanks, prgCpuBankDefaults, sizeof(prgCpuBankDefaults));
        memcpy(mpEntity->ppuBanks, prgPpuBankDefaults, sizeof(prgPpuBankDefaults));
        AdjustEntityPageTableForPrg(mpEntity, 0xff00);
        AdjustEntityPageTableForChr(mpEntity, 0xff);
        AdjustEntityPageTableForVRam(mpEntity, 0xf00);
        if (mpEntity->flags & 2) {
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[6].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x300800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[0];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
            {
                _SBankAccess& access = mpEntity->cpuBanks[7].access[1];
                access.type = 1;
                access.mask = 0xfff;
                access.offset = 0x301800;
            }
        }
    } else if (operation == 2) {
        unsigned char device = 1;
        if (sender->_18 == 3) {
            unsigned long address = args->values[2];
            switch (address >> 12) {
            case 2: case 3: device = 4; break;
            case 4:
                if (address == 0x4016 || address == 0x4017) device = 2;
                else if (address == 0x4014) device = 4;
                else if (address < 0x4016) device = 5;
                break;
            case 9: case 10:
                if (args->values[0] == 1) device = 5;
                break;
            case 11:
                if (args->values[0] == 1 && (mWriteAddress & 3) != 3) device = 5;
                break;
            }
        }
        *static_cast<unsigned char*>(args->pointers[4]) = device;
    }
    return 0;
}
/** @brief Routes bus accesses or captures a VRC6 write. @return Bus status. */
unsigned int CHvcVrc6::Access(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    if (sender->_18 == 3) {
        if (mpEntity->cpuBanks[args->values[2] >> 12].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 3, operation, args);
        if (operation == 1) {
            mRegisters[1] = *static_cast<unsigned char*>(args->pointers[4]);
            mWriteAddress = args->values[2];
            _AdjustBusMap();
            return 0;
        }
    } else if (sender->_18 == 4) {
        if (mpEntity->ppuBanks[args->values[2] >> 10].access[operation].type != 2)
            return _MapBasicOperation(mpEntity, 4, operation, args);
    }
    return 6;
}
/** @brief Transfers VRC6 state. @param buffer State storage. @param reading Import selector. @return Bytes transferred. */
unsigned long CHvcVrc6::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mRegisters[0]);
    stream.Entry(mRegisters[1]);
    stream.Entry(mWriteAddress);
    stream.Entry(mState[0]);
    stream.Entry(mState[1]);
    stream.Entry(mState[2]);
    stream.Entry(mState[3]);
    stream.Entry(mState[4]);
    stream.Entry(mState[5]);
    stream.Entry(mState[6]);
    stream.Entry(mState[7]);
    stream.Entry(mState[8]);
    stream.Entry(mState[9]);
    stream.Entry(mState[10]);
    stream.Entry(mIrqCounter);
    stream.Entry(mIrqEnabled);
    stream.Entry(mIrqControl);
    stream.Entry(mRamControl);
    return stream.GetSize();
}
/** @brief Exports state. @param buffer State output. */
void CHvcVrc6::_SerializeCoreExport(void* buffer) { _SerializeCore(buffer, false); }
/** @brief Imports state. @param buffer State input. */
void CHvcVrc6::_SerializeCoreImport(const void* buffer) { _SerializeCore(const_cast<void*>(buffer), true); }
/** @brief Measures aligned storage. @return Aligned size. */
unsigned long CHvcVrc6::_SerializeCoreInferSize() {
    return (static_cast<unsigned int>(_SerializeCore(nullptr, false)) + 23) & ~15u;
}
/** @brief Identifies the state species. @return Eighteen. */
unsigned char CHvcVrc6::_SerializeTagSpecies() { return 18; }
/** @brief Supplies the mapper label. @return Mapper identifier. */
const unsigned char* CHvcVrc6::_SerializeTagComment() {
    return reinterpret_cast<const unsigned char*>("MMC-V6 ");
}
/** @brief Handles VRC6 reset and interrupt events. @return Zero. */
unsigned int CHvcVrc6::Process(CPlatformUnit* sender, unsigned long category,
    unsigned long operation, UArgument* args) {
    _SEntity* entity = mpEntity;
    if ((1u << _18) & entity->order.words[0]) {
        mState[0] = 0;
        mState[1] = 0;
        mState[10] = 0;
        mIrqCounter = 0;
        mIrqEnabled = 0;
        _AdjustBusMap();
        _SOrderController& order = mpEntity->order;
        order.words[0] &= ~(1u << _18);
    }
    if (args->values[0] & 0x20) mIrqCounter = 0;
    if (mIrqControl && (args->values[0] & 0x10)) {
        ++mIrqCounter;
        if (mIrqCounter >= 0x100) {
            mIrqCounter &= 0xff;
            if (mIrqEnabled) {
                mpEntity->order.words[4] |= 8;
                if (mState[10] != 0xff) mIrqCounter = mState[10];
            } else {
                mIrqControl = 0;
            }
        }
    }
    return 0;
}
/** @brief Applies VRC6 program, character, mirroring, and interrupt writes. */
void CHvcVrc6::_AdjustBusMap() {
    unsigned int page = mWriteAddress >> 12;
    unsigned int sub = mWriteAddress & 3;
    switch (page) {
    case 13: case 14: {
        unsigned long value = mRegisters[1];
        unsigned int index = ((page - 13) << 2) | sub;
        mChrBanks[index] = value;
        mpEntity->ppuBanks[index].readOffset = 0x200000 + (value << 10);
        break;
    }
    case 8: {
        unsigned long bank = mRegisters[1];
        mState[0] = bank;
        unsigned long offset = bank << 14;
        mpEntity->cpuBanks[8].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[9].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[10].readOffset = offset;
        offset += 0x1000;
        mpEntity->cpuBanks[11].readOffset = offset;
        break;
    }
    case 11:
        if (sub == 3) {
            unsigned long control = mRegisters[1];
            switch ((control >> 2) & 3) {
            case 0:
                mpEntity->ppuBanks[8].writeOffset = 0x302800;
                mpEntity->ppuBanks[8].readOffset = 0x302800;
                mpEntity->ppuBanks[9].writeOffset = 0x302c00;
                mpEntity->ppuBanks[9].readOffset = 0x302c00;
                mpEntity->ppuBanks[10].writeOffset = 0x302800;
                mpEntity->ppuBanks[10].readOffset = 0x302800;
                mpEntity->ppuBanks[11].writeOffset = 0x302c00;
                mpEntity->ppuBanks[11].readOffset = 0x302c00;
                break;
            case 1:
                mpEntity->ppuBanks[8].writeOffset = 0x302800;
                mpEntity->ppuBanks[8].readOffset = 0x302800;
                mpEntity->ppuBanks[9].writeOffset = 0x302800;
                mpEntity->ppuBanks[9].readOffset = 0x302800;
                mpEntity->ppuBanks[10].writeOffset = 0x302c00;
                mpEntity->ppuBanks[10].readOffset = 0x302c00;
                mpEntity->ppuBanks[11].writeOffset = 0x302c00;
                mpEntity->ppuBanks[11].readOffset = 0x302c00;
                break;
            case 2:
                if (control & 0x20) {
                mpEntity->ppuBanks[8].writeOffset = 0x302800;
                mpEntity->ppuBanks[8].readOffset = 0x302800;
                mpEntity->ppuBanks[9].writeOffset = 0x302800;
                mpEntity->ppuBanks[9].readOffset = 0x302800;
                mpEntity->ppuBanks[10].writeOffset = 0x302800;
                mpEntity->ppuBanks[10].readOffset = 0x302800;
                mpEntity->ppuBanks[11].writeOffset = 0x302800;
                mpEntity->ppuBanks[11].readOffset = 0x302800;
                } else {
                mpEntity->ppuBanks[8].writeOffset = 0x302c00;
                mpEntity->ppuBanks[8].readOffset = 0x302c00;
                mpEntity->ppuBanks[9].writeOffset = 0x302c00;
                mpEntity->ppuBanks[9].readOffset = 0x302c00;
                mpEntity->ppuBanks[10].writeOffset = 0x302c00;
                mpEntity->ppuBanks[10].readOffset = 0x302c00;
                mpEntity->ppuBanks[11].writeOffset = 0x302c00;
                mpEntity->ppuBanks[11].readOffset = 0x302c00;
                }
                break;
            case 3:
                if (control & 0x20) {
                mpEntity->ppuBanks[8].writeOffset = 0x302c00;
                mpEntity->ppuBanks[8].readOffset = 0x302c00;
                mpEntity->ppuBanks[9].writeOffset = 0x302c00;
                mpEntity->ppuBanks[9].readOffset = 0x302c00;
                mpEntity->ppuBanks[10].writeOffset = 0x302c00;
                mpEntity->ppuBanks[10].readOffset = 0x302c00;
                mpEntity->ppuBanks[11].writeOffset = 0x302c00;
                mpEntity->ppuBanks[11].readOffset = 0x302c00;
                } else {
                mpEntity->ppuBanks[8].writeOffset = 0x302800;
                mpEntity->ppuBanks[8].readOffset = 0x302800;
                mpEntity->ppuBanks[9].writeOffset = 0x302800;
                mpEntity->ppuBanks[9].readOffset = 0x302800;
                mpEntity->ppuBanks[10].writeOffset = 0x302800;
                mpEntity->ppuBanks[10].readOffset = 0x302800;
                mpEntity->ppuBanks[11].writeOffset = 0x302800;
                mpEntity->ppuBanks[11].readOffset = 0x302800;
                }
                break;
            }
            mRamControl = mRegisters[1] >> 7;
        }
        break;
    case 12: {
        unsigned long bank = mRegisters[1];
        mState[1] = bank;
        mpEntity->cpuBanks[12].readOffset = bank << 13;
        mpEntity->cpuBanks[13].readOffset = (bank << 13) | 0x1000;
        break;
    }
    case 15:
        switch (sub) {
        case 0: mState[10] = mRegisters[1]; break;
        case 1:
            mIrqCounter = mState[10];
            mIrqEnabled = mRegisters[1];
            if (!mIrqEnabled) mpEntity->order.words[4] &= ~8;
            mIrqControl = (mRegisters[1] & 3) ? 3 : 0;
            break;
        case 2:
            if (mState[10] == 0xff && mIrqEnabled == 2) mIrqCounter = 0;
            break;
        }
        break;
    }
}
/** @brief Releases mapper state through the platform base. */
CHvcMmcPrg::~CHvcMmcPrg() {}
/** @brief Releases mapper state through the platform base. */
CHvcMmcChr::~CHvcMmcChr() {}
/** @brief Releases mapper state through the platform base. */
CHvcMmc1::~CHvcMmc1() {}
/** @brief Releases mapper state through the platform base. */
CHvcMmc2::~CHvcMmc2() {}
/** @brief Releases mapper state through the platform base. */
CHvcMmc3::~CHvcMmc3() {}
/** @brief Releases mapper state through the platform base. */
CHvcMmc3TLS::~CHvcMmc3TLS() {}
/** @brief Releases mapper state through the platform base. */
CHvcMmc4::~CHvcMmc4() {}
/** @brief Releases mapper state through the platform base. */
CHvcMmc5::~CHvcMmc5() {}
/** @brief Releases mapper state through the platform base. */
CHvcMapper45::~CHvcMapper45() {}
/** @brief Releases mapper state through the platform base. */
CHvcMapper57::~CHvcMapper57() {}
/** @brief Releases mapper state through the platform base. */
CHvcDisk::~CHvcDisk() {}
/** @brief Releases mapper state through the platform base. */
CHvcVrc1::~CHvcVrc1() {}
/** @brief Releases mapper state through the platform base. */
CHvcVrc4::~CHvcVrc4() {}
/** @brief Releases mapper state through the platform base. */
CHvcVrc6::~CHvcVrc6() {}
/** @brief Releases mapper state through the platform base. */
CHvcVrc3::~CHvcVrc3() {}
/** @brief Releases mapper state through the platform base. */
CHvcVrc2b::~CHvcVrc2b() {}
/** @brief Releases mapper state through the platform base. */
CHvcMapperB8::~CHvcMapperB8() {}
/** @brief Releases mapper state through the platform base. */
CHvcMapper5F::~CHvcMapper5F() {}
} } } }
