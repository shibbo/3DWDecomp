#pragma once

#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Cartridge without a memory mapper (NROM).
 */
class CHvcMmc : public CPlatformHvcUnit {
public:
    CHvcMmc();
    ~CHvcMmc() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;
};

static_assert(sizeof(CHvcMmc) == 0x48, "CHvcMmc size");

/**
 * @brief MMC1 memory mapper.
 */
class CHvcMmc1 : public CPlatformHvcUnit {
public:
    CHvcMmc1();
    ~CHvcMmc1() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x50 - 0x48];
};

static_assert(sizeof(CHvcMmc1) == 0x50, "CHvcMmc1 size");

/**
 * @brief Mapper that only switches program banks.
 */
class CHvcMmcPrg : public CPlatformHvcUnit {
public:
    CHvcMmcPrg();
    ~CHvcMmcPrg() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x50 - 0x48];
};

static_assert(sizeof(CHvcMmcPrg) == 0x50, "CHvcMmcPrg size");

/**
 * @brief Mapper that only switches character banks.
 */
class CHvcMmcChr : public CPlatformHvcUnit {
public:
    CHvcMmcChr();
    ~CHvcMmcChr() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x50 - 0x48];
};

static_assert(sizeof(CHvcMmcChr) == 0x50, "CHvcMmcChr size");

/**
 * @brief MMC2 memory mapper.
 */
class CHvcMmc2 : public CPlatformHvcUnit {
public:
    CHvcMmc2();
    ~CHvcMmc2() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x60 - 0x48];
};

static_assert(sizeof(CHvcMmc2) == 0x60, "CHvcMmc2 size");

/**
 * @brief MMC3 memory mapper.
 */
class CHvcMmc3 : public CPlatformHvcUnit {
public:
    CHvcMmc3();
    ~CHvcMmc3() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0xb8 - 0x48];
};

static_assert(sizeof(CHvcMmc3) == 0xb8, "CHvcMmc3 size");

/**
 * @brief MMC3 variant with the TLS name table layout.
 */
class CHvcMmc3TLS : public CPlatformHvcUnit {
public:
    CHvcMmc3TLS();
    ~CHvcMmc3TLS() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0xa8 - 0x48];
};

static_assert(sizeof(CHvcMmc3TLS) == 0xa8, "CHvcMmc3TLS size");

/**
 * @brief MMC4 memory mapper.
 */
class CHvcMmc4 : public CPlatformHvcUnit {
public:
    CHvcMmc4();
    ~CHvcMmc4() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x60 - 0x48];
};

static_assert(sizeof(CHvcMmc4) == 0x60, "CHvcMmc4 size");

/**
 * @brief MMC5 memory mapper.
 */
class CHvcMmc5 : public CPlatformHvcUnit {
public:
    CHvcMmc5();
    ~CHvcMmc5() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x78 - 0x48];
};

static_assert(sizeof(CHvcMmc5) == 0x78, "CHvcMmc5 size");

/**
 * @brief Mapper 69 (Sunsoft FME-7).
 */
class CHvcMapper45 : public CPlatformHvcUnit {
public:
    CHvcMapper45();
    ~CHvcMapper45() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0xd0 - 0x48];
};

static_assert(sizeof(CHvcMapper45) == 0xd0, "CHvcMapper45 size");

/**
 * @brief Mapper 87.
 */
class CHvcMapper57 : public CPlatformHvcUnit {
public:
    CHvcMapper57();
    ~CHvcMapper57() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x60 - 0x48];
};

static_assert(sizeof(CHvcMapper57) == 0x60, "CHvcMapper57 size");

/**
 * @brief Mapper 95 (Namco 118 variant).
 */
class CHvcMapper5F : public CPlatformHvcUnit {
public:
    CHvcMapper5F();
    ~CHvcMapper5F() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x98 - 0x48];
};

static_assert(sizeof(CHvcMapper5F) == 0x98, "CHvcMapper5F size");

/**
 * @brief Mapper 184 (Sunsoft-1).
 */
class CHvcMapperB8 : public CPlatformHvcUnit {
public:
    CHvcMapperB8();
    ~CHvcMapperB8() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x58 - 0x48];
};

static_assert(sizeof(CHvcMapperB8) == 0x58, "CHvcMapperB8 size");

/**
 * @brief Konami VRC1 memory mapper.
 */
class CHvcVrc1 : public CPlatformHvcUnit {
public:
    CHvcVrc1();
    ~CHvcVrc1() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x68 - 0x48];
};

static_assert(sizeof(CHvcVrc1) == 0x68, "CHvcVrc1 size");

/**
 * @brief Konami VRC2b memory mapper.
 */
class CHvcVrc2b : public CPlatformHvcUnit {
public:
    CHvcVrc2b();
    ~CHvcVrc2b() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x68 - 0x48];
};

static_assert(sizeof(CHvcVrc2b) == 0x68, "CHvcVrc2b size");

/**
 * @brief Konami VRC3 memory mapper.
 */
class CHvcVrc3 : public CPlatformHvcUnit {
public:
    CHvcVrc3();
    ~CHvcVrc3() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x68 - 0x48];
};

static_assert(sizeof(CHvcVrc3) == 0x68, "CHvcVrc3 size");

/**
 * @brief Konami VRC4 memory mapper.
 */
class CHvcVrc4 : public CPlatformHvcUnit {
public:
    CHvcVrc4();
    ~CHvcVrc4() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x78 - 0x48];
};

static_assert(sizeof(CHvcVrc4) == 0x78, "CHvcVrc4 size");

/**
 * @brief Konami VRC6 memory mapper.
 */
class CHvcVrc6 : public CPlatformHvcUnit {
public:
    CHvcVrc6();
    ~CHvcVrc6() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x78 - 0x48];
};

static_assert(sizeof(CHvcVrc6) == 0x78, "CHvcVrc6 size");

/**
 * @brief Famicom Disk System adapter.
 */
class CHvcDisk : public CPlatformHvcUnit {
public:
    CHvcDisk();
    ~CHvcDisk() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;
};

static_assert(sizeof(CHvcDisk) == 0x48, "CHvcDisk size");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
