#pragma once
#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
class CHvcGio : public CPlatformHvcUnit {
public:
    CHvcGio();
    ~CHvcGio() override;
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned int Access(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override;
    unsigned int Process(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override;
    unsigned long _SerializeCore(void* pBuffer, bool reading);
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagContent() override;
    unsigned char _SerializeTagSpecies() override;
    unsigned short _SerializeTagVersion() override;
    const unsigned char* _SerializeTagComment() override;
private:
    unsigned long mReadIndex[2];
    unsigned long mPorts[12];
    unsigned char mFrameFlags[8];
};
static_assert(sizeof(CHvcGio) == 0xc0, "HVC I/O layout");
} } } }
