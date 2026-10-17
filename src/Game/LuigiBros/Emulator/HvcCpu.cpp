#include "LuigiBros/Emulator/HvcCpu.hpp"
#include "LuigiBros/Emulator/Hvc.hpp"
#include "LuigiBros/Common/Serialize.hpp"
#include "LuigiBros/Emulator/HVCEmu.hpp"
#include <cstring>

extern unsigned char g_nDiskSideMax;

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
/** @brief Default encoded descriptors indexed by the fetched 6502 opcode. */
const CHvcCpu::_S6502Mnemonic CHvcCpu::m_a6502MnemonicTableDefault[256] = {
    {{0x0807100bu}, "BRK         "}, // 00
    {{0x41062b22u}, "ORA (%02X,x)  "}, // 01
    {{0x00081e39u}, "undef cop_02"}, // 02
    {{0x00041e39u}, "undef ora_03"}, // 03
    {{0x00051e39u}, "undef tsb_04"}, // 04
    {{0x41032422u}, "ORA %02X      "}, // 05
    {{0xc1052403u}, "ASL %02X      "}, // 06
    {{0x00061e39u}, "undef ora_07"}, // 07
    {{0x00031024u}, "PHP         "}, // 08
    {{0x41022322u}, "ORA #%02X     "}, // 09
    {{0xc1021103u}, "ASL A       "}, // 0A
    {{0x00041e39u}, "undef phd_0b"}, // 0B
    {{0x00061e39u}, "undef tsb_0c"}, // 0C
    {{0x41043722u}, "ORA %02X%02X    "}, // 0D
    {{0xc1063703u}, "ASL %02X%02X    "}, // 0E
    {{0x00051e39u}, "undef ora_0f"}, // 0F
    {{0x0012220au}, "BPL %02X      "}, // 10
    {{0x41152c22u}, "ORA (%02X),y  "}, // 11
    {{0x00051e39u}, "undef ora_12"}, // 12
    {{0x00071e39u}, "undef ora_13"}, // 13
    {{0x00051e39u}, "undef trb_14"}, // 14
    {{0x41042522u}, "ORA %02X,x    "}, // 15
    {{0xc1062503u}, "ASL %02X,x    "}, // 16
    {{0x00061e39u}, "undef ora_17"}, // 17
    {{0x0002100eu}, "CLC         "}, // 18
    {{0x41143922u}, "ORA %02X%02X,y  "}, // 19
    {{0x00021e39u}, "undef inc_1a"}, // 1A
    {{0x00021e39u}, "undef tcs_1b"}, // 1B
    {{0x00061e39u}, "undef trb_1c"}, // 1C
    {{0x41143822u}, "ORA %02X%02X,x  "}, // 1D
    {{0xc1073803u}, "ASL %02X%02X,x  "}, // 1E
    {{0x00051e39u}, "undef ora_1f"}, // 1F
    {{0x00063d1du}, "JSR %02X%02X    "}, // 20
    {{0x41062b02u}, "AND (%02X,x)  "}, // 21
    {{0x00081e39u}, "undef jsl_22"}, // 22
    {{0x00041e39u}, "undef and_23"}, // 23
    {{0x43032407u}, "BIT %02X      "}, // 24
    {{0x41032402u}, "AND %02X      "}, // 25
    {{0xc1052427u}, "ROL %02X      "}, // 26
    {{0x00061e39u}, "undef and_27"}, // 27
    {{0xff041026u}, "PLP         "}, // 28
    {{0x41022302u}, "AND #%02X     "}, // 29
    {{0xc1021127u}, "ROL A       "}, // 2A
    {{0x00051e39u}, "undef pld_2b"}, // 2B
    {{0x43043707u}, "BIT %02X%02X    "}, // 2C
    {{0x41043702u}, "AND %02X%02X    "}, // 2D
    {{0xc1063727u}, "ROL %02X%02X    "}, // 2E
    {{0x00051e39u}, "undef and_2f"}, // 2F
    {{0x00122208u}, "BMI %02X      "}, // 30
    {{0x41152c02u}, "AND (%02X),y  "}, // 31
    {{0x00051e39u}, "undef and_32"}, // 32
    {{0x00071e39u}, "undef and_33"}, // 33
    {{0x00041e39u}, "undef bit_34"}, // 34
    {{0x41042502u}, "AND %02X,x    "}, // 35
    {{0xc1062527u}, "ROL %02X,x    "}, // 36
    {{0x00061e39u}, "undef and_37"}, // 37
    {{0x0002102cu}, "SEC         "}, // 38
    {{0x41143902u}, "AND %02X%02X,y  "}, // 39
    {{0x00021e39u}, "undef dec_3a"}, // 3A
    {{0x00021e39u}, "undef tsc_3b"}, // 3B
    {{0x00041e39u}, "undef bit_3c"}, // 3C
    {{0x41143802u}, "AND %02X%02X,x  "}, // 3D
    {{0xc1073827u}, "ROL %02X%02X,x  "}, // 3E
    {{0x00051e39u}, "undef and_3f"}, // 3F
    {{0xff061029u}, "RTI         "}, // 40
    {{0x41062b18u}, "EOR (%02X,x)  "}, // 41
    {{0x00022338u}, "J42 #%02X%"}, // 42
    {{0x00041e39u}, "undef eor_43"}, // 43
    {{0x00071e39u}, "undef mvp_44"}, // 44
    {{0x41032418u}, "EOR %02X      "}, // 45
    {{0xc1052421u}, "LSR %02X      "}, // 46
    {{0x00061e39u}, "undef eor_47"}, // 47
    {{0x00031023u}, "PHA         "}, // 48
    {{0x41022318u}, "EOR #%02X     "}, // 49
    {{0xc1021121u}, "LSR A       "}, // 4A
    {{0x00031e39u}, "undef phk_4b"}, // 4B
    {{0x00033d1cu}, "JMP %02X%02X    "}, // 4C
    {{0x41043718u}, "EOR %02X%02X    "}, // 4D
    {{0xc1063721u}, "LSR %02X%02X    "}, // 4E
    {{0x00051e39u}, "undef eor_4f"}, // 4F
    {{0x0012220cu}, "BVC %02X      "}, // 50
    {{0x41152c18u}, "EOR (%02X),y  "}, // 51
    {{0x00051e39u}, "undef eor_52"}, // 52
    {{0x00071e39u}, "undef eor_53"}, // 53
    {{0x00071e39u}, "undef mvn_54"}, // 54
    {{0x41042518u}, "EOR %02X,x    "}, // 55
    {{0xc1062521u}, "LSR %02X,x    "}, // 56
    {{0x00061e39u}, "undef eor_57"}, // 57
    {{0x00021010u}, "CLI         "}, // 58
    {{0x41143918u}, "EOR %02X%02X,y  "}, // 59
    {{0x00031e39u}, "undef phy_5a"}, // 5A
    {{0x00021e39u}, "undef tcd_5b"}, // 5B
    {{0x00041e39u}, "undef jmp_5c"}, // 5C
    {{0x41143818u}, "EOR %02X%02X,x  "}, // 5D
    {{0xc1073821u}, "LSR %02X%02X,x  "}, // 5E
    {{0x00051e39u}, "undef eor_5f"}, // 5F
    {{0x0006102au}, "RTS         "}, // 60
    {{0xc3062b01u}, "ADC (%02X,x)  "}, // 61
    {{0x00061e39u}, "undef per_62"}, // 62
    {{0x00041e39u}, "undef adc_63"}, // 63
    {{0x00031e39u}, "undef stz_64"}, // 64
    {{0xc3032401u}, "ADC %02X      "}, // 65
    {{0xc1052428u}, "ROR %02X      "}, // 66
    {{0x00061e39u}, "undef adc_67"}, // 67
    {{0x41041025u}, "PLA         "}, // 68
    {{0xc3022301u}, "ADC #%02X     "}, // 69
    {{0xc1021128u}, "ROR A       "}, // 6A
    {{0x00061e39u}, "undef rtl_6b"}, // 6B
    {{0x00053a1cu}, "JMP (%02X%02X)  "}, // 6C
    {{0xc3043701u}, "ADC %02X%02X    "}, // 6D
    {{0xc1063728u}, "ROR %02X%02X    "}, // 6E
    {{0x00051e39u}, "undef adc_6f"}, // 6F
    {{0x0012220du}, "BVS %02X      "}, // 70
    {{0xc3152c01u}, "ADC (%02X),y  "}, // 71
    {{0x00051e39u}, "undef adc_72"}, // 72
    {{0x00071e39u}, "undef adc_73"}, // 73
    {{0x00041e39u}, "undef stz_74"}, // 74
    {{0xc3042501u}, "ADC %02X,x    "}, // 75
    {{0xc1062528u}, "ROR %02X,x    "}, // 76
    {{0x00061e39u}, "undef adc_77"}, // 77
    {{0x0002102eu}, "SEI         "}, // 78
    {{0xc3143901u}, "ADC %02X%02X,y  "}, // 79
    {{0x00041e39u}, "undef ply_7a"}, // 7A
    {{0x00021e39u}, "undef tdc_7b"}, // 7B
    {{0x00061e39u}, "undef jmp_7c"}, // 7C
    {{0xc3143801u}, "ADC %02X%02X,x  "}, // 7D
    {{0xc1073828u}, "ROR %02X%02X,x  "}, // 7E
    {{0x00051e39u}, "undef adc_7f"}, // 7F
    {{0x00021e39u}, "undef bra_80"}, // 80
    {{0x00062b2fu}, "STA (%02X,x)  "}, // 81
    {{0x00041e39u}, "undef brl_82"}, // 82
    {{0x00041e39u}, "undef sta_83"}, // 83
    {{0x00032431u}, "STY %02X      "}, // 84
    {{0x0003242fu}, "STA %02X      "}, // 85
    {{0x00032430u}, "STX %02X      "}, // 86
    {{0x00061e39u}, "undef sta_87"}, // 87
    {{0x41021017u}, "DEY         "}, // 88
    {{0x00021e39u}, "undef bit_89"}, // 89
    {{0x41021035u}, "TXA         "}, // 8A
    {{0x00031e39u}, "undef phb_8b"}, // 8B
    {{0x00043731u}, "STY %02X%02X    "}, // 8C
    {{0x0004372fu}, "STA %02X%02X    "}, // 8D
    {{0x00043730u}, "STX %02X%02X    "}, // 8E
    {{0x00051e39u}, "undef sta_8f"}, // 8F
    {{0x00122204u}, "BCC %02X      "}, // 90
    {{0x00162c2fu}, "STA (%02X),y  "}, // 91
    {{0x00051e39u}, "undef sta_92"}, // 92
    {{0x00071e39u}, "undef sta_93"}, // 93
    {{0x00042531u}, "STY %02X,x    "}, // 94
    {{0x0004252fu}, "STA %02X,x    "}, // 95
    {{0x00042630u}, "STX %02X,y    "}, // 96
    {{0x00061e39u}, "undef sta_97"}, // 97
    {{0x41021037u}, "TYA         "}, // 98
    {{0x0005392fu}, "STA %02X%02X,y  "}, // 99
    {{0x00021036u}, "TXS         "}, // 9A
    {{0x00021e39u}, "undef txy_9b"}, // 9B
    {{0x00041e39u}, "undef stz_9c"}, // 9C
    {{0x0005382fu}, "STA %02X%02X,x  "}, // 9D
    {{0x00051e39u}, "undef stz_9e"}, // 9E
    {{0x00051e39u}, "undef sta_9f"}, // 9F
    {{0x41022320u}, "LDY #%02X     "}, // A0
    {{0x41062b1eu}, "LDA (%02X,x)  "}, // A1
    {{0x4102231fu}, "LDX #%02X     "}, // A2
    {{0x00041e39u}, "undef lda_a3"}, // A3
    {{0x41032420u}, "LDY %02X      "}, // A4
    {{0x4103241eu}, "LDA %02X      "}, // A5
    {{0x4103241fu}, "LDX %02X      "}, // A6
    {{0x00061e39u}, "undef lda_a7"}, // A7
    {{0x41021033u}, "TAY         "}, // A8
    {{0x4102231eu}, "LDA #%02X     "}, // A9
    {{0x41021032u}, "TAX         "}, // AA
    {{0x00041e39u}, "undef plb_ab"}, // AB
    {{0x41043720u}, "LDY %02X%02X    "}, // AC
    {{0x4104371eu}, "LDA %02X%02X    "}, // AD
    {{0x4104371fu}, "LDX %02X%02X    "}, // AE
    {{0x00051e39u}, "undef lda_af"}, // AF
    {{0x00122205u}, "BCS %02X      "}, // B0
    {{0x41152c1eu}, "LDA (%02X),y  "}, // B1
    {{0x00051e39u}, "undef lda_b2"}, // B2
    {{0x00071e39u}, "undef lda_b3"}, // B3
    {{0x41042520u}, "LDY %02X,x    "}, // B4
    {{0x4104251eu}, "LDA %02X,x    "}, // B5
    {{0x4104261fu}, "LDX %02X,y    "}, // B6
    {{0x00061e39u}, "undef lda_b7"}, // B7
    {{0x00021011u}, "CLV         "}, // B8
    {{0x4114391eu}, "LDA %02X%02X,y  "}, // B9
    {{0x41021034u}, "TSX         "}, // BA
    {{0x00021e39u}, "undef tyx_bb"}, // BB
    {{0x41143820u}, "LDY %02X%02X,x  "}, // BC
    {{0x4114381eu}, "LDA %02X%02X,x  "}, // BD
    {{0x4114391fu}, "LDX %02X%02X,y  "}, // BE
    {{0x00051e39u}, "undef lda_bf"}, // BF
    {{0xc1022314u}, "CPY #%02X     "}, // C0
    {{0xc1062b12u}, "CMP (%02X,x)  "}, // C1
    {{0x00031e39u}, "undef rep_c2"}, // C2
    {{0x00041e39u}, "undef cmp_c3"}, // C3
    {{0xc1032414u}, "CPY %02X      "}, // C4
    {{0xc1032412u}, "CMP %02X      "}, // C5
    {{0x41052415u}, "DEC %02X      "}, // C6
    {{0x00061e39u}, "undef cmp_c7"}, // C7
    {{0x4102101bu}, "INY         "}, // C8
    {{0xc1022312u}, "CMP #%02X     "}, // C9
    {{0x41021016u}, "DEX         "}, // CA
    {{0x00031e39u}, "undef wai_cb"}, // CB
    {{0xc1043714u}, "CPY %02X%02X    "}, // CC
    {{0xc1043712u}, "CMP %02X%02X    "}, // CD
    {{0x41063715u}, "DEC %02X%02X    "}, // CE
    {{0x00051e39u}, "undef cmp_cf"}, // CF
    {{0x00122209u}, "BNE %02X      "}, // D0
    {{0xc1152c12u}, "CMP (%02X),y  "}, // D1
    {{0x00051e39u}, "undef cmp_d2"}, // D2
    {{0x00071e39u}, "undef cmp_d3"}, // D3
    {{0x00061e39u}, "undef pei_d4"}, // D4
    {{0xc1042512u}, "CMP %02X,x    "}, // D5
    {{0x41062515u}, "DEC %02X,x    "}, // D6
    {{0x00061e39u}, "undef cmp_d7"}, // D7
    {{0x0002100fu}, "CLD         "}, // D8
    {{0xc1143912u}, "CMP %02X%02X,y  "}, // D9
    {{0x00031e39u}, "undef phx_da"}, // DA
    {{0x00031e39u}, "undef stp_db"}, // DB
    {{0x00061e39u}, "undef jml_dc"}, // DC
    {{0xc1143812u}, "CMP %02X%02X,x  "}, // DD
    {{0x41073815u}, "DEC %02X%02X,x  "}, // DE
    {{0x00051e39u}, "undef cmp_df"}, // DF
    {{0xc1022313u}, "CPX #%02X     "}, // E0
    {{0xc3062b2bu}, "SBC (%02X,x)  "}, // E1
    {{0x00031e39u}, "undef sep_e2"}, // E2
    {{0x00041e39u}, "undef sbc_e3"}, // E3
    {{0xc1032413u}, "CPX %02X      "}, // E4
    {{0xc303242bu}, "SBC %02X      "}, // E5
    {{0x41052419u}, "INC %02X      "}, // E6
    {{0x00061e39u}, "undef sbc_e7"}, // E7
    {{0x4102101au}, "INX         "}, // E8
    {{0xc302232bu}, "SBC #%02X     "}, // E9
    {{0x00021000u}, "NOP         "}, // EA
    {{0x00031e39u}, "undef xba_eb"}, // EB
    {{0xc1043713u}, "CPX %02X%02X    "}, // EC
    {{0xc304372bu}, "SBC %02X%02X    "}, // ED
    {{0x41063719u}, "INC %02X%02X    "}, // EE
    {{0x00051e39u}, "undef sbc_ef"}, // EF
    {{0x00122206u}, "BEQ %02X      "}, // F0
    {{0xc3152c2bu}, "SBC (%02X),y  "}, // F1
    {{0x00051e39u}, "undef sbc_f2"}, // F2
    {{0x00071e39u}, "undef sbc_f3"}, // F3
    {{0x00051e39u}, "undef pea_f4"}, // F4
    {{0xc304252bu}, "SBC %02X,x    "}, // F5
    {{0x41062519u}, "INC %02X,x    "}, // F6
    {{0x00061e39u}, "undef sbc_f7"}, // F7
    {{0x0002102du}, "SED         "}, // F8
    {{0xc314392bu}, "SBC %02X%02X,y  "}, // F9
    {{0x00041e39u}, "undef plx_fa"}, // FA
    {{0x00021e39u}, "undef xce_fb"}, // FB
    {{0x00061e39u}, "undef jsr_fc"}, // FC
    {{0xc314382bu}, "SBC %02X%02X,x  "}, // FD
    {{0x41073819u}, "INC %02X%02X,x  "}, // FE
    {{0x00021e39u}, "undef sbc_ff"}, // FF
};
/** @brief Initializes the CPU unit's identity and timing scale. */
CHvcCpu::CHvcCpu() {
    _18 = 3;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    memset(mName, ' ', 15);
    strncpy(mName, "HVC CPU        ", 15);

}
/** @brief Releases disk support and clears inherited platform state. */
CHvcCpu::~CHvcCpu() {}
/** @brief Sets the stack-page and interrupt vector addresses. @return Zero. */
unsigned int CHvcCpu::_Prologue() {
    mVectors[0] = 0x100;
    mVectors[1] = 0xfffc;
    mVectors[2] = 0xfffa;
    mVectors[3] = 0xfffe;
    return 0;
}
/** @brief Provides the CPU's empty shutdown hook. @return Zero. */
unsigned int CHvcCpu::_Epilogue() { return 0; }
/**
 * @brief Restores the default opcode descriptors and applies the platform-specific patch.
 * @param pSender Sending platform unit.
 * @param category Notification category.
 * @param operation Zero to initialize the instruction table.
 * @param pArguments Optional notification arguments.
 * @return Zero.
 */
unsigned int CHvcCpu::System(CPlatformUnit* pSender, unsigned long category,
    unsigned long operation, UArgument* pArguments) {
    if (operation == 0) {
        memcpy(mInstructions, m_a6502MnemonicTableDefault, sizeof(mInstructions));
        if (GetCHvc()->mTitleElement.mTitleId - 0xbf <= 1) {
            mInstructions[0xcf].encoding = (mInstructions[0xcf].encoding & ~(0xffu << 12)) | (0x63u << 12);
        }
    }
    return 0;
}
/**
 * @brief Reads CPU bus bytes through the parent platform.
 * @param pParent Bus provider.
 * @param pSender CPU issuing the read.
 * @param address Effective bus address.
 * @param pBuffer Destination bytes.
 * @param size Number of bytes requested.
 * @return Parent access result.
 */
static unsigned int ReadCpuBus(CPlatformUnit* pParent, CPlatformUnit* pSender,
    unsigned long address, unsigned char* pBuffer, unsigned long size) {
    CPlatformUnit::UArgument arguments;
    arguments.values[0] = size;
    arguments.pointers[4] = pBuffer;
    arguments.values[2] = address;
    return pParent->Access(pSender, 1, 0, &arguments);
}
/**
 * @brief Writes CPU bus bytes through the parent platform.
 * @param pParent Bus provider.
 * @param pSender CPU issuing the write.
 * @param address Effective bus address.
 * @param pBuffer Source bytes.
 * @param size Number of bytes requested.
 * @return Parent access result.
 */
static unsigned int WriteCpuBus(CPlatformUnit* pParent, CPlatformUnit* pSender,
    unsigned long address, unsigned char* pBuffer, unsigned long size) {
    CPlatformUnit::UArgument arguments;
    arguments.values[0] = size;
    arguments.pointers[4] = pBuffer;
    arguments.values[2] = address;
    return pParent->Access(pSender, 1, 1, &arguments);
}
/** @brief Updates one processor flag. @param status Status byte. @param mask Flag mask. @param set New flag state. */
static void SetCpuFlag(unsigned char& status, unsigned int mask, bool set) {
    if (set) status |= mask;
    else status &= ~mask;
}
/** @brief Updates negative and zero flags. @param status Status byte. @param value Byte result. */
static void SetCpuNZ(unsigned char& status, unsigned char value) {
    SetCpuFlag(status, 0x80, static_cast<signed char>(value) < 0);
    SetCpuFlag(status, 2, value == 0);
}
/** @brief Maps a CPU address directly. @param entity Platform bus mapping. @param address CPU address. @return Backing memory pointer. */
static unsigned char* MapCpuAddress(CPlatformHvcUnit::_SEntity* entity, unsigned long address) {
    return entity->pMemory + entity->cpuBanks[(address >> 12) & 15].readOffset + (address & 0xfff);
}
/**
 * @brief Handles pending CPU events, fetches opcodes, or executes the fetched instruction.
 * @param pSender Sending platform unit.
 * @param category Notification category.
 * @param operation Three for the opcode-fetch phase.
 * @param pArguments Arguments containing the optional fixed-point cycle result pointer.
 * @return Most recent bus-access result.
 */
unsigned int CHvcCpu::Process(CPlatformUnit* pSender, unsigned long category,
    unsigned long operation, UArgument* pArguments) {
    unsigned int result = 0;
    unsigned char byte;
    unsigned short vector;
    if (mpEntity->order.words[0] & (1 << _18)) {
        if (operation != 3) {
            result = ReadCpuBus(mpParent, this, mVectors[1], reinterpret_cast<unsigned char*>(&vector), 2);
            if (mpEntity->mapper == 0xff) {
                unsigned char* memory = mpEntity->pMemory + 0x300000;
                for (unsigned long i = 0; i < 0x800;) {
                    memory[i++] = 0x0f;
                    memory[i++] = 0xef;
                    memory[i++] = 0xfe;
                    memory[i++] = 0x7d;
                }
            }
            mRegisters[0].pc = vector;
            mRegisters[0].status = 0x24;
            mRegisters[0].stack = 0xfd;
            mRegisters[0].y = 0;
            mRegisters[0].x = 0;
            mRegisters[0].accumulator = 0;
            mpEntity->order.words[0] &= ~(1 << _18);
        }
    } else if (mpEntity->order.words[2] & (1 << _18)) {
        if (operation != 3) {
            mRegisters[0].status &= ~0x10;
            unsigned short pc = mRegisters[0].pc;
            byte = pc >> 8;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            byte = pc;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            byte = mRegisters[0].status;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            mRegisters[0].status |= 4;
            result = ReadCpuBus(mpParent, this, mVectors[2], reinterpret_cast<unsigned char*>(&vector), 2);
            mRegisters[0].pc = vector;
            mpEntity->order.words[2] &= ~(1 << _18);
        }
    } else if ((mpEntity->order.words[4] & (1 << _18)) && !(mRegisters[0].status & 4)) {
        if (operation != 3) {
            mRegisters[0].status &= ~0x10;
            unsigned short pc = mRegisters[0].pc;
            byte = pc >> 8;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            byte = pc;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            byte = mRegisters[0].status;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            mRegisters[0].status |= 4;
            result = ReadCpuBus(mpParent, this, mVectors[3], reinterpret_cast<unsigned char*>(&vector), 2);
            mRegisters[0].pc = vector;
            mpEntity->order.words[4] &= ~(1 << _18);
        }
    } else if (!(mpEntity->order.words[4] & (1 << _18)) &&
        (mpEntity->order.words[6] & (1 << _18)) && !(mRegisters[0].status & 4)) {
        if (operation != 3) {
            mRegisters[0].status |= 0x10;
            unsigned short pc = mRegisters[0].pc;
            byte = pc >> 8;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            byte = pc;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            byte = mRegisters[0].status;
            WriteCpuBus(mpParent, this, mRegisters[0].stack + mVectors[0], &byte, 1);
            --mRegisters[0].stack;
            mRegisters[0].status |= 4;
            result = ReadCpuBus(mpParent, this, mVectors[3], reinterpret_cast<unsigned char*>(&vector), 2);
            mRegisters[0].pc = vector;
            mpEntity->order.words[6] &= ~(1 << _18);
        }
    } else {
        unsigned long cycles;
        mRegisters[1] = mRegisters[0];
        unsigned long pc = mRegisters[1].pc;
        if (operation == 3) {
            mInstructionAddress = pc;
            mFlags[0] = *(mpEntity->pMemory + mpEntity->banks[(pc >> 12) & 31].readOffset + (pc & 0xfff));
            ++pc;
            mFlags[1] = *(mpEntity->pMemory + mpEntity->banks[(pc >> 12) & 31].readOffset + (pc & 0xfff));
            ++pc;
            mFlags[2] = *(mpEntity->pMemory + mpEntity->banks[(pc >> 12) & 31].readOffset + (pc & 0xfff));
            mOpcode = mFlags[0];
            if (pArguments->pointers[2])
                *static_cast<unsigned long*>(pArguments->pointers[2]) = mInstructions[mOpcode].cycles << 8;
            return 0;
        } else {
            cycles = 0;
            const _S6502Mnemonic* instruction = &mInstructions[mOpcode];
            _S6502Register& r = mRegisters[1];
            r.pc = pc + instruction->length;
            result = _ResolveParameter(mEffectiveAddress, mOperand, cycles, instruction, &r);
            unsigned long operand = mOperand;
            unsigned long address = mEffectiveAddress;
            byte = operand;
            signed char operandByte = operand;
            switch (instruction->operation) {
            case 1: { // ADC
                unsigned int value = r.accumulator + static_cast<unsigned char>(operand) + (r.status & 1);
                SetCpuFlag(r.status, 1, value > 255);
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 0x40, (~(static_cast<unsigned int>(operand) ^ r.accumulator) & (value ^ r.accumulator)) & 0x80);
                SetCpuFlag(r.status, 2, (value & 255) == 0);
                r.accumulator = value;
                break;
            }
            case 2: {
                unsigned long value = operand & r.accumulator;
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 2, value == 0);
                r.accumulator = value; break;
            }
            case 3: {
                int value = operandByte * 2;
                SetCpuFlag(r.status, 1, static_cast<unsigned int>(operand) & 0x80);
                SetCpuFlag(r.status, 0x80, static_cast<unsigned int>(operand) & 0x40);
                SetCpuFlag(r.status, 2, (static_cast<unsigned int>(operand) & 0x7f) == 0);
                if (instruction->addressing == 1) r.accumulator = value;
                else { byte = value; result = WriteCpuBus(mpParent, this, address, &byte, 1); }
                break;
            }
            case 4: _BranchRelative(&r, operand, cycles, !(r.status & 1)); break;
            case 5: _BranchRelative(&r, operand, cycles, r.status & 1); break;
            case 6: _BranchRelative(&r, operand, cycles, r.status & 2); break;
            case 7: {
                unsigned int accumulator = r.accumulator;
                SetCpuFlag(r.status, 0x80, operand & 0x80);
                SetCpuFlag(r.status, 0x40, operandByte & 0x40);
                SetCpuFlag(r.status, 2, (operand & accumulator) == 0); break;
            }
            case 8: _BranchRelative(&r, operand, cycles, static_cast<signed char>(r.status) < 0); break;
            case 9: _BranchRelative(&r, operand, cycles, !(r.status & 2)); break;
            case 10: _BranchRelative(&r, operand, cycles, static_cast<signed char>(r.status) >= 0); break;
            case 11: mpEntity->order.words[6] |= 1; break;
            case 12: _BranchRelative(&r, operand, cycles, !(r.status & 0x40)); break;
            case 13: _BranchRelative(&r, operand, cycles, r.status & 0x40); break;
            case 14: r.status &= ~1; break;
            case 15: r.status &= ~8; break;
            case 16: r.status &= ~4; break;
            case 17: r.status &= ~0x40; break;
            case 18: {
                unsigned int value = r.accumulator;
                SetCpuFlag(r.status, 1, value >= static_cast<unsigned char>(operand));
                SetCpuFlag(r.status, 0x80, (value - static_cast<unsigned int>(operand)) & 0x80);
                SetCpuFlag(r.status, 2, ((value - static_cast<unsigned int>(operandByte)) & 255) == 0); break;
            }
            case 19: {
                unsigned int value = r.x;
                SetCpuFlag(r.status, 1, value >= static_cast<unsigned char>(operand));
                SetCpuFlag(r.status, 0x80, (value - static_cast<unsigned int>(operand)) & 0x80);
                SetCpuFlag(r.status, 2, ((value - static_cast<unsigned int>(operandByte)) & 255) == 0); break;
            }
            case 20: {
                unsigned int value = r.y;
                SetCpuFlag(r.status, 1, value >= static_cast<unsigned char>(operand));
                SetCpuFlag(r.status, 0x80, (value - static_cast<unsigned int>(operand)) & 0x80);
                SetCpuFlag(r.status, 2, ((value - static_cast<unsigned int>(operandByte)) & 255) == 0); break;
            }
            case 21: {
                unsigned long value = operand + 255;
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 2, (value & 255) == 0);
                if (instruction->addressing == 1) r.accumulator = value;
                else { byte = value; result = WriteCpuBus(mpParent, this, address, &byte, 1); }
                break;
            }
            case 25: {
                unsigned long value = operand + 1;
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 2, (value & 255) == 0);
                if (instruction->addressing == 1) r.accumulator = value;
                else { byte = value; result = WriteCpuBus(mpParent, this, address, &byte, 1); }
                break;
            }
            case 22: --r.x; SetCpuNZ(r.status, r.x); break;
            case 23: --r.y; SetCpuNZ(r.status, r.y); break;
            case 24: {
                unsigned long value = r.accumulator ^ (operand & 255);
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 2, value == 0);
                r.accumulator = value; break;
            }
            case 26: ++r.x; SetCpuNZ(r.status, r.x); break;
            case 27: ++r.y; SetCpuNZ(r.status, r.y); break;
            case 28: _BranchAbsolute(&r, operand, cycles, true); break;
            case 29: {
                unsigned int pc = r.pc - 1;
                byte = pc >> 8;
                WriteCpuBus(mpParent, this, r.stack + mVectors[0], &byte, 1); --r.stack;
                byte = pc;
                result = WriteCpuBus(mpParent, this, r.stack + mVectors[0], &byte, 1); --r.stack;
                _BranchAbsolute(&r, operand, cycles, true); break;
            }
            case 30: r.accumulator = operandByte; SetCpuFlag(r.status, 0x80, operand & 0x80); SetCpuFlag(r.status, 2, operandByte == 0); break;
            case 31: r.x = operandByte; SetCpuFlag(r.status, 0x80, operand & 0x80); SetCpuFlag(r.status, 2, operandByte == 0); break;
            case 32: r.y = operandByte; SetCpuFlag(r.status, 0x80, operand & 0x80); SetCpuFlag(r.status, 2, operandByte == 0); break;
            case 33: {
                unsigned long value = (operand >> 1) & 127;
                SetCpuFlag(r.status, 1, operand & 1);
                r.status &= ~0x80;
                SetCpuFlag(r.status, 2, value == 0);
                if (instruction->addressing == 1) r.accumulator = value;
                else { byte = value; result = WriteCpuBus(mpParent, this, address, &byte, 1); }
                break;
            }
            case 34: {
                unsigned long value = r.accumulator | (operand & 255);
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 2, value == 0);
                r.accumulator = value; break;
            }
            case 35: {
                unsigned long stackAddress = r.stack + mVectors[0];
                byte = r.accumulator;
                result = WriteCpuBus(mpParent, this, stackAddress, &byte, 1); --r.stack; break;
            }
            case 36: {
                unsigned long stackAddress = r.stack + mVectors[0];
                byte = r.status;
                result = WriteCpuBus(mpParent, this, stackAddress, &byte, 1); --r.stack; break;
            }
            case 37: {
                ++r.stack;
                unsigned long stackAddress = r.stack + mVectors[0];
                byte = 0; result = ReadCpuBus(mpParent, this, stackAddress, &byte, 1);
                int value = static_cast<signed char>(byte);
                r.accumulator = value;
                SetCpuFlag(r.status, 0x80, value < 0);
                SetCpuFlag(r.status, 2, (value & 255) == 0);
                break;
            }
            case 38: {
                ++r.stack;
                unsigned long stackAddress = r.stack + mVectors[0];
                byte = 0; result = ReadCpuBus(mpParent, this, stackAddress, &byte, 1);
                r.status = byte; break;
            }
            case 39: {
                unsigned int value = ((operand << 1) & 255) | (r.status & 1);
                SetCpuFlag(r.status, 1, static_cast<signed char>(operand) < 0);
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 2, (value & 255) == 0);
                if (instruction->addressing == 1) r.accumulator = value;
                else { byte = value; result = WriteCpuBus(mpParent, this, address, &byte, 1); }
                break;
            }
            case 40: {
                unsigned int carry = r.status & 1;
                unsigned long value = ((operand >> 1) & 127) | (carry << 7);
                SetCpuFlag(r.status, 1, operandByte & 1);
                SetCpuFlag(r.status, 0x80, carry != 0);
                SetCpuFlag(r.status, 2, value == 0);
                if (instruction->addressing == 1) r.accumulator = value;
                else { byte = value; result = WriteCpuBus(mpParent, this, address, &byte, 1); }
                break;
            }
            case 41: {
                ++r.stack; ReadCpuBus(mpParent, this, r.stack + mVectors[0], &byte, 1); r.status = byte;
                ++r.stack; ReadCpuBus(mpParent, this, r.stack + mVectors[0], &byte, 1);
                unsigned int pc = byte;
                ++r.stack; result = ReadCpuBus(mpParent, this, r.stack + mVectors[0], &byte, 1);
                pc |= static_cast<unsigned int>(byte) << 8;
                r.pc = pc + 0; break;
            }
            case 42: {
                ++r.stack; ReadCpuBus(mpParent, this, r.stack + mVectors[0], &byte, 1);
                unsigned int pc = byte;
                ++r.stack; result = ReadCpuBus(mpParent, this, r.stack + mVectors[0], &byte, 1);
                pc |= static_cast<unsigned int>(byte) << 8;
                r.pc = pc + 1; break;
            }
            case 43: {
                int value = r.accumulator - static_cast<unsigned char>(operand) - ((~r.status) & 1);
                SetCpuFlag(r.status, 1, value >= 0);
                SetCpuFlag(r.status, 0x80, value & 0x80);
                SetCpuFlag(r.status, 0x40, ((r.accumulator ^ static_cast<unsigned int>(operand)) & (value ^ r.accumulator)) & 0x80);
                SetCpuFlag(r.status, 2, (value & 255) == 0);
                r.accumulator = value; break;
            }
            case 44: r.status |= 1; break;
            case 45: r.status |= 8; break;
            case 46: r.status |= 4; break;
            case 47: byte = r.accumulator; result = WriteCpuBus(mpParent, this, address, &byte, 1); break;
            case 48: byte = r.x; result = WriteCpuBus(mpParent, this, address, &byte, 1); break;
            case 49: byte = r.y; result = WriteCpuBus(mpParent, this, address, &byte, 1); break;
            case 50: r.x = r.accumulator; SetCpuNZ(r.status, r.x); break;
            case 51: r.y = r.accumulator; SetCpuNZ(r.status, r.y); break;
            case 52: r.x = r.stack; SetCpuNZ(r.status, r.x); break;
            case 53: r.accumulator = r.x; SetCpuNZ(r.status, r.accumulator); break;
            case 54: r.stack = r.x; SetCpuNZ(r.status, r.stack); break;
            case 55: r.accumulator = r.y; SetCpuNZ(r.status, r.accumulator); break;
            case 56:
                if ((m_HVCEmu.mState7a & 0xfff0) == 0xf0f0) {
                    switch (r.pc) {
                    case 0xeef6:
                        r.accumulator = mpEntity->pMemory[0x300090];
                        GetCHvc()->mSpu.mDisk.status = 0x46; break;
                    case 0xe408: {
                        unsigned char* memory = mpEntity->pMemory;
                        memory[0x300001] = r.accumulator;
                        unsigned char* data = MapCpuAddress(mpEntity, mpEntity->pMemory[0x300000] | (static_cast<unsigned long>(r.accumulator) << 8));
                        if (*data != 0xff) {
                            unsigned char* diskBase = mpEntity->pMemory + 0x100000;
                            unsigned char* disk = diskBase;
                            for (unsigned int i = 0; i < g_nDiskSideMax; ++i) {
                                if (memcmp(data + 1, disk + 16, 8) == 0) {
                                    g_cHvc.mSpu.mDisk.SetQdOffset(disk - diskBase); break;
                                }
                                disk += 0x10000;
                            }
                        }
                        break;
                    }
                    case 0xe7a6: r.pc = 0xe7a4; break;
                    case 0xeebf:
                        if (mDiskUtility.LoadOnColdStart(mpEntity) == 0) {
                            mpEntity->pMemory[0x303842] = 0x46;
                            g_cHvc.mSpu.mDisk.status = 0x46;
                            unsigned char* memory = mpEntity->pMemory;
                            *reinterpret_cast<unsigned short*>(memory + 0x3000f8) = 0;
                            for (int i = 0xf0; i >= 0; i -= 8) *reinterpret_cast<unsigned long*>(memory + 0x300000 + i) = 0;
                            memcpy(mpEntity->pMemory + 0x3000f9, "\x00\x2f\x00\x00\x00\x06\x10\xc0\x80\x35\xac", 11);
                            r.pc -= 0x24;
                        }
                        break;
                    case 0xe23b: {
                        unsigned char* memory = mpEntity->pMemory;
                        unsigned char file = r.accumulator;
                        memory[0x30000e] = file;
                        if (file == 0xff) break;
                        unsigned char* stack = memory + 0x300000 + r.stack;
                        unsigned long pc = stack[0x101] | (static_cast<unsigned long>(stack[0x102]) << 8);
                        unsigned char* args = MapCpuAddress(mpEntity, pc + 1);
                        const unsigned char* id = MapCpuAddress(mpEntity, args[0] | (static_cast<unsigned long>(args[1]) << 8));
                        const unsigned char* header = MapCpuAddress(mpEntity, args[2] | (static_cast<unsigned long>(args[3]) << 8));
                        long saved = mDiskUtility.PurgeSave(mpEntity, id, file, header);
                        r.status &= 0x7f;
                        r.accumulator = 0;
                        r.status |= 2;
                        if (saved == 0) { r.stack += 2; r.pc = pc + 5; }
                        break;
                    }
                    }
                }
                break;
            }
            mRegisters[0] = mRegisters[1];
        }
        if (pArguments->pointers[2]) *static_cast<unsigned long*>(pArguments->pointers[2]) = cycles << 8;
        return result;
    }
    if (pArguments->pointers[2]) *static_cast<unsigned long*>(pArguments->pointers[2]) = 0x700;
    return result;
}
/**
 * @brief Resolves an instruction's address, operand, and base cycle count.
 * @param rAddress Resolved effective address.
 * @param rValue Resolved operand value.
 * @param rCycles Instruction cycle count including page crossings.
 * @param pInstruction Decoded instruction descriptor.
 * @param pRegisters Active CPU registers.
 * @return Bus access result, or four for an unsupported addressing mode.
 */
unsigned int CHvcCpu::_ResolveParameter(unsigned long& rAddress, unsigned long& rValue,
    unsigned long& rCycles, const _S6502Mnemonic* pInstruction, _S6502Register* pRegisters) {
    unsigned int result = 0;
    rCycles = pInstruction->cycles;
    if ((pInstruction->addressing) == 0) goto finished;
    rAddress = 0;
    rValue = 0;
    unsigned char buffer[4];
    switch (pInstruction->addressing) {
    case 1: rAddress = 0; break;
    case 2: case 3: case 13: rAddress = mInstructionAddress + 1; break;
    case 4: rAddress = mFlags[1]; break;
    case 5: rAddress = mFlags[1]; rAddress = (rAddress + pRegisters->x) & 255; break;
    case 6: rAddress = mFlags[1]; rAddress = (rAddress + pRegisters->y) & 255; break;
    case 7: rAddress = mFlags[1] | (static_cast<unsigned long>(mFlags[2]) << 8); break;
    case 8:
        rAddress = mFlags[1] | (static_cast<unsigned long>(mFlags[2]) << 8);
        rAddress = (rAddress + pRegisters->x) & 65535;
        if (pInstruction->pageCrossing && (rAddress & 255) < mFlags[1]) ++rCycles;
        break;
    case 9:
        rAddress = mFlags[1] | (static_cast<unsigned long>(mFlags[2]) << 8);
        rAddress = (rAddress + pRegisters->y) & 65535;
        if (pInstruction->pageCrossing && (rAddress & 255) < mFlags[1]) ++rCycles;
        break;
    case 10:
        rAddress = mFlags[1] | (static_cast<unsigned long>(mFlags[2]) << 8);
        result = ReadCpuBus(mpParent, this, rAddress, buffer, 2);
        rAddress = buffer[0] | (static_cast<unsigned long>(buffer[1]) << 8);
        break;
    case 11:
        rAddress = mFlags[1];
        rAddress = (rAddress + pRegisters->x) & 255;
        result = ReadCpuBus(mpParent, this, rAddress, buffer, 2);
        rAddress = buffer[0] | (static_cast<unsigned long>(buffer[1]) << 8);
        break;
    case 12:
        rAddress = mFlags[1];
        result = ReadCpuBus(mpParent, this, rAddress, buffer, 2);
        rAddress = buffer[0] | (static_cast<unsigned long>(buffer[1]) << 8);
        rAddress = (rAddress + pRegisters->y) & 65535;
        if ((rAddress & 255) < buffer[0]) ++rCycles;
        break;
    default: rAddress = 0; result = 4; break;
    }
    {
        unsigned int operation = pInstruction->operation;
        if (operation >= 0x2f && operation <= 0x31) {
            switch (operation) {
            case 0x2f: rValue = pRegisters->accumulator; break;
            case 0x30: rValue = pRegisters->x; break;
            case 0x31: rValue = pRegisters->y; break;
            }
        } else {
            switch (pInstruction->addressing) {
            case 1: rValue = pRegisters->accumulator; break;
            case 2: case 3: rValue = mFlags[1]; break;
            case 4: case 5: case 6: case 7: case 8: case 9: case 11: case 12:
                result = ReadCpuBus(mpParent, this, rAddress, buffer, 1);
                rValue = buffer[0];
                break;
            case 10: rValue = rAddress; break;
            case 13: rValue = mFlags[1] | (static_cast<unsigned long>(mFlags[2]) << 8); break;
            default: rValue = 0; break;
            }
        }
    }
finished:
    return result;
}
/**
 * @brief Applies a signed relative branch and its taken/page-crossing cycle costs.
 * @param pRegisters Active CPU registers.
 * @param address Encoded relative offset.
 * @param rCycles Accumulated instruction cycles.
 * @param condition Whether the branch is taken.
 */
void CHvcCpu::_BranchRelative(_S6502Register* pRegisters, unsigned long address,
    unsigned long& rCycles, bool condition) {
    if (condition) {
        unsigned long next = pRegisters->pc + static_cast<long>(static_cast<signed char>(address));
        unsigned int previousPage = pRegisters->pc & 0xff00;
        pRegisters->pc = next;
        rCycles += (static_cast<unsigned int>(next) & 0xff00) == previousPage ? 1 : 2;
    }
}
/**
 * @brief Replaces the program counter with an absolute address.
 * @param pRegisters Active CPU registers.
 * @param address New program counter.
 * @param rCycles Unchanged cycle counter.
 * @param condition Unused condition parameter.
 */
void CHvcCpu::_BranchAbsolute(_S6502Register* pRegisters, unsigned long address,
    unsigned long& rCycles, bool condition) { pRegisters->pc = address; }
/**
 * @brief Provides the port's empty program-counter notification hook.
 * @param previous Previous address.
 * @param current Current address.
 */
void CHvcCpu::_ReportChangePC(unsigned long previous, unsigned long current) {}
/**
 * @brief Serializes CPU registers, execution state, and vectors.
 * @param pBuffer Save-state buffer, or null to measure the state.
 * @param reading True to restore state.
 * @return Bytes processed.
 */
unsigned long CHvcCpu::_SerializeCore(void* pBuffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(pBuffer), reading);
    stream.Entry(mRegisters[0].status);
    stream.Entry(mRegisters[0].accumulator);
    stream.Entry(mRegisters[0].x);
    stream.Entry(mRegisters[0].y);
    stream.Entry(mRegisters[0].stack);
    stream.Entry(mRegisters[0]._05);
    stream.Entry(mRegisters[0].pc);
    stream.Entry(mRegisters[0].pc);
    stream.Entry(mRegisters[1].status);
    stream.Entry(mRegisters[1].accumulator);
    stream.Entry(mRegisters[1].x);
    stream.Entry(mRegisters[1].y);
    stream.Entry(mRegisters[1].stack);
    stream.Entry(mRegisters[1]._05);
    stream.Entry(mRegisters[1].pc);
    stream.Entry(mRegisters[1].pc);
    stream.Entry(mOpcode);
    stream.Entry(mInstructionAddress);
    stream.Entry(mFlags[0]);
    stream.Entry(mFlags[1]);
    stream.Entry(mFlags[2]);
    stream.Entry(mFlags[3]);
    stream.Entry(mState1070);
    stream.Entry(mEffectiveAddress);
    stream.Entry(mOperand);
    stream.Entry(mVectors[0]);
    stream.Entry(mVectors[1]);
    stream.Entry(mVectors[2]);
    stream.Entry(mVectors[3]);
    return stream.GetSize();
}
/** @brief Exports CPU state. @param pBuffer Output buffer. */
void CHvcCpu::_SerializeCoreExport(void* pBuffer) { _SerializeCore(pBuffer, false); }
/** @brief Imports CPU state. @param pBuffer Input buffer. */
void CHvcCpu::_SerializeCoreImport(const void* pBuffer) { _SerializeCore(const_cast<void*>(pBuffer), true); }
/** @brief Measures aligned CPU state including its tag. @return Storage size. */
unsigned long CHvcCpu::_SerializeCoreInferSize() { return (unsigned(_SerializeCore(nullptr, false)) + 31) & ~15u; }
/** @brief Returns the CPU content identifier. @return Three. */
unsigned char CHvcCpu::_SerializeTagContent() { return 3; }
/** @brief Returns the CPU state species. @return Zero. */
unsigned char CHvcCpu::_SerializeTagSpecies() { return 0; }
/** @brief Returns the state format version. @return Version 0x100. */
unsigned short CHvcCpu::_SerializeTagVersion() { return 0x100; }
/** @brief Returns the CPU state comment. @return CPU identifier text. */
const unsigned char* CHvcCpu::_SerializeTagComment() { return reinterpret_cast<const unsigned char*>("CPU    "); }
} } } }
