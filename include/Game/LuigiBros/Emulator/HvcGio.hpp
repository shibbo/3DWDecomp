#pragma once

#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Game controller input/output of the emulated Famicom.
 */
class CHvcGio : public CPlatformHvcUnit {
public:
    CHvcGio();
    ~CHvcGio() override;

    int _Prologue() override;
    int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;
    u8 _SerializeTagContent() override;
    u8 _SerializeTagSpecies() override;
    u16 _SerializeTagVersion() override;
    const char* _SerializeTagComment() override;

    u8 _48[0xc0 - 0x48];
};

static_assert(sizeof(CHvcGio) == 0xc0, "CHvcGio size");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
