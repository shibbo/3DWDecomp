#pragma once
#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
class CHvcMmc : public CPlatformHvcUnit {
public:
    CHvcMmc();
    ~CHvcMmc() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    /** @brief Provides the mapper base's empty export hook. @param buffer Unused buffer. */
    void _SerializeCoreExport(void* buffer) override {}
    /** @brief Provides the mapper base's empty import hook. @param buffer Unused buffer. */
    void _SerializeCoreImport(const void* buffer) override {}
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagContent() override;
    unsigned char _SerializeTagSpecies() override;
    unsigned short _SerializeTagVersion() override;
    const unsigned char* _SerializeTagComment() override;
};
static_assert(sizeof(CHvcMmc) == 0x48, "HVC mapper base layout");
class CHvcMmcPrg : public CHvcMmc {
public:
    CHvcMmcPrg();
    /** @brief Releases the program-bank controller. */
    ~CHvcMmcPrg() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
private:
    unsigned char mBank;
};
static_assert(sizeof(CHvcMmcPrg) == 0x50, "Program mapper layout");
class CHvcMmcChr : public CHvcMmc {
public:
    CHvcMmcChr();
    /** @brief Releases the character-bank controller. */
    ~CHvcMmcChr() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
private:
    unsigned char mBank;
};
static_assert(sizeof(CHvcMmcChr) == 0x50, "Character mapper layout");
class CHvcMmc1 : public CHvcMmc {
public:
    CHvcMmc1();
    /** @brief Releases the program-bank controller. */
    ~CHvcMmc1() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mControl;
    unsigned char mChrBank0;
    unsigned char mChrBank1;
    unsigned char mPrgBank;
    unsigned char mShiftCount;
    unsigned char mShiftValue;
};
static_assert(sizeof(CHvcMmc1) == 0x50, "Serial mapper layout");
class CHvcMmc2 : public CHvcMmc {
public:
    CHvcMmc2();
    /** @brief Releases the program-bank controller. */
    ~CHvcMmc2() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
    __attribute__((always_inline)) void _ApplyLatchChrBank(unsigned short, bool);
private:
    unsigned char mRegisters[7];
    unsigned long mWriteAddress;
    unsigned short mChrLatch[2];
};
static_assert(sizeof(CHvcMmc2) == 0x60, "Latched mapper layout");
class CHvcMmc3 : public CHvcMmc {
public:
    CHvcMmc3();
    /** @brief Releases the scanline mapper. */
    ~CHvcMmc3() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    void _AdjustBusMap();
    void _AdjustBusMap_Mother();
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
private:
    unsigned char mRegisters[14];
    unsigned long mChrBanks[8];
    unsigned long mWriteAddress;
    unsigned char mIrqReload;
    unsigned char mIrqEnabled;
    unsigned char mIrqCounter;
    void (CHvcMmc3::*mAdjustBusMap)();
};
static_assert(sizeof(CHvcMmc3) == 0xb8, "Scanline mapper layout");
class CHvcMmc3TLS : public CHvcMmc {
public:
    CHvcMmc3TLS();
    /** @brief Releases the scanline mapper. */
    ~CHvcMmc3TLS() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    void _AdjustBusMap();
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
private:
    unsigned char mRegisters[14];
    unsigned long mChrBanks[8];
    unsigned long mWriteAddress;
    unsigned char mIrqReload;
    unsigned char mIrqEnabled;
    unsigned char mIrqCounter;
};
static_assert(sizeof(CHvcMmc3TLS) == 0xa8, "Scanline mapper layout");
class CHvcMmc4 : public CHvcMmc {
public:
    CHvcMmc4();
    /** @brief Releases the program-bank controller. */
    ~CHvcMmc4() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
    __attribute__((always_inline)) void _ApplyLatchChrBank(unsigned short, bool);
private:
    unsigned char mRegisters[7];
    unsigned long mWriteAddress;
    unsigned short mChrLatch[2];
};
static_assert(sizeof(CHvcMmc4) == 0x60, "Latched mapper layout");
class CHvcMmc5 : public CHvcMmc {
public:
    CHvcMmc5();
    /** @brief Releases the program-bank controller. */
    ~CHvcMmc5() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[7];
    unsigned long mWriteAddress;
    unsigned char mState[26];
};
static_assert(sizeof(CHvcMmc5) == 0x78, "MMC5 state layout");
class CHvcMapper45 : public CHvcMmc {
public:
    CHvcMapper45();
    /** @brief Releases the program-bank controller. */
    ~CHvcMapper45() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;
    union {
        unsigned long mBanks[14];
        struct {
            unsigned long mPrgBanks[4];
            unsigned long mChrBanks[8];
            unsigned long mSelectedBank;
            unsigned long mStateC0;
        };
    };
    unsigned int mControl;
    unsigned char mEnabled;
};
static_assert(sizeof(CHvcMapper45) == 0xd0, "Mapper 45 state layout");
class CHvcMapper57 : public CHvcMmc {
public:
    CHvcMapper57();
    /** @brief Releases the program-bank controller. */
    ~CHvcMapper57() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    __attribute__((always_inline)) void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;

    unsigned long _58;
};
static_assert(sizeof(CHvcMapper57) == 0x60, "Mapper 57 state layout");
class CHvcDisk : public CHvcMmc {
public:
    CHvcDisk();
    /** @brief Releases the program-bank controller. */
    ~CHvcDisk() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    __attribute__((always_inline)) void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;

};
static_assert(sizeof(CHvcDisk) == 0x58, "Disk mapper state layout");
class CHvcVrc1 : public CHvcMmc {
public:
    CHvcVrc1();
    /** @brief Releases the program-bank controller. */
    ~CHvcVrc1() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;
    unsigned char mState[10];

};
static_assert(sizeof(CHvcVrc1) == 0x68, "VRC1 state layout");
class CHvcVrc4 : public CHvcMmc {
public:
    CHvcVrc4();
    /** @brief Releases the program-bank controller. */
    ~CHvcVrc4() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;
    union {
        unsigned char mState[13];
        struct {
            unsigned char mPrgMode;
            unsigned char mPrgBanks[3];
            unsigned char mChrBanks[8];
            unsigned char mIrqReload;
        };
    };
    unsigned long mIrqCounter;
    unsigned char mIrqEnabled;
    unsigned char mIrqControl;
    unsigned char mRamControl;

};
static_assert(sizeof(CHvcVrc4) == 0x78, "VRC4 state layout");
class CHvcVrc6 : public CHvcMmc {
public:
    CHvcVrc6();
    /** @brief Releases the program-bank controller. */
    ~CHvcVrc6() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;
    union {
        unsigned char mState[11];
        struct {
            unsigned char mPrgBanks[2];
            unsigned char mChrBanks[8];
            unsigned char mIrqReload;
        };
    };
    unsigned long mIrqCounter;
    unsigned char mIrqEnabled;
    unsigned char mIrqControl;
    unsigned char mRamControl;

};
static_assert(sizeof(CHvcVrc6) == 0x78, "VRC6 state layout");
class CHvcVrc3 : public CHvcMmc {
public:
    CHvcVrc3();
    /** @brief Releases the program-bank controller. */
    ~CHvcVrc3() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;
    unsigned char mBank;
    unsigned int mControl;
    unsigned char mEnabled;

};
static_assert(sizeof(CHvcVrc3) == 0x68, "VRC3 state layout");
class CHvcVrc2b : public CHvcMmc {
public:
    CHvcVrc2b();
    /** @brief Releases the program-bank controller. */
    ~CHvcVrc2b() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;
    unsigned char mState[14];

};
static_assert(sizeof(CHvcVrc2b) == 0x68, "VRC2b state layout");
class CHvcMapperB8 : public CHvcMmc {
public:
    CHvcMapperB8();
    /** @brief Releases the program-bank controller. */
    ~CHvcMapperB8() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    __attribute__((always_inline)) void _AdjustBusMap();
private:
    unsigned char mRegisters[2];
    unsigned long mWriteAddress;

};
static_assert(sizeof(CHvcMapperB8) == 0x58, "Mapper B8 state layout");
class CHvcMapper5F : public CHvcMmc {
public:
    CHvcMapper5F();
    /** @brief Releases the program-bank controller. */
    ~CHvcMapper5F() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagSpecies() override;
    const unsigned char* _SerializeTagComment() override;
    void _AdjustBusMap();
private:
    unsigned char mRegisters[8];
    unsigned long mBanks[8];
    unsigned long mWriteAddress;

};
static_assert(sizeof(CHvcMapper5F) == 0x98, "Mapper 5F state layout");
} } } }
