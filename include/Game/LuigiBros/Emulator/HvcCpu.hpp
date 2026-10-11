#pragma once

#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Central processing unit (6502) of the emulated Famicom.
 */
class CHvcCpu : public CPlatformHvcUnit {
public:
    CHvcCpu();
    ~CHvcCpu() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x3098 - 0x48];
};

static_assert(sizeof(CHvcCpu) == 0x3098, "CHvcCpu size");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
