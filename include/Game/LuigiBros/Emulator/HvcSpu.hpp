#pragma once

#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Sound processing unit of the emulated Famicom.
 */
class CHvcSpu : public CPlatformHvcUnit {
public:
    CHvcSpu();
    ~CHvcSpu() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0x330 - 0x48];
};

static_assert(sizeof(CHvcSpu) == 0x330, "CHvcSpu size");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
