#pragma once
#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"
#include "LuigiBros/Emulator/HvcDiskUtility.hpp"

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
class CHvcCpu : public CPlatformHvcUnit {
public:
    struct _S6502Register {
        unsigned char status;
        unsigned char accumulator;
        unsigned char x;
        unsigned char y;
        unsigned char stack;
        unsigned char _05;
        unsigned short pc;
    };
    struct _S6502Mnemonic {
        union {
        unsigned int encoding;
        struct {
        unsigned int operation : 8;
        unsigned int addressing : 4;
        unsigned int length : 4;
        unsigned int cycles : 4;
        unsigned int pageCrossing : 1;
        unsigned int reserved : 11;
        };
        };
        const char* pFormat;
    };
    static const _S6502Mnemonic m_a6502MnemonicTableDefault[256];
    CHvcCpu();
    ~CHvcCpu() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override;
    unsigned int Process(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override;
    unsigned int _ResolveParameter(unsigned long& rAddress, unsigned long& rValue,
        unsigned long& rCycles, const _S6502Mnemonic* pInstruction, _S6502Register* pRegisters);
    void _BranchRelative(_S6502Register* pRegisters, unsigned long address,
        unsigned long& rCycles, bool condition);
    void _BranchAbsolute(_S6502Register* pRegisters, unsigned long address,
        unsigned long& rCycles, bool condition);
    void _ReportChangePC(unsigned long previous, unsigned long current);
    unsigned long _SerializeCore(void* pBuffer, bool reading);
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagContent() override;
    unsigned char _SerializeTagSpecies() override;
    unsigned short _SerializeTagVersion() override;
    const unsigned char* _SerializeTagComment() override;
private:
    _S6502Mnemonic mInstructions[256];
    _S6502Register mRegisters[2];
    unsigned long mOpcode;
    unsigned long mInstructionAddress;
    unsigned char mFlags[4];
    unsigned char _106c[4];
    unsigned long mState1070;
    unsigned long mEffectiveAddress;
    unsigned long mOperand;
    unsigned short mVectors[4];
    CHvcDiskUtility mDiskUtility;
    unsigned char _1098[0x2000];
};
static_assert(sizeof(CHvcCpu::_S6502Mnemonic) == 16, "6502 mnemonic layout");
static_assert(sizeof(CHvcCpu::_S6502Register) == 8, "6502 register layout");
static_assert(sizeof(CHvcCpu) == 0x3098, "HVC CPU layout");
} } } }
