#pragma once
#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"
#include "LuigiBros/Emulator/HvcSpu.hpp"

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
/** @brief HVC platform storage; unrecovered members retain their original offsets. */
class CHvc : public CPlatformHvcUnit {
public:
    unsigned int Process(CPlatformUnit* sender, unsigned long category, unsigned long operation, UArgument* arguments) override;
    void Debug(unsigned long mode);
    unsigned long ContentSize(unsigned long mask);
    struct SerializeInformation {
        unsigned long tag;
        const unsigned char* comment;
        unsigned long unit;
        unsigned long reserved;
        unsigned long firstOffset;
        unsigned long firstSize;
        unsigned long secondOffset;
        unsigned long secondSize;
    };
    static const SerializeInformation c_aSerializeInformationTable[25];
    CHvc();
    ~CHvc() override;
    unsigned int Access(CPlatformUnit* sender, unsigned long category, unsigned long operation, UArgument* arguments) override;
    void ExtractAudioSignal(short* output, unsigned long count);
    void TakeAudioSample(short*& output, unsigned long& count);
    unsigned long _SerializeCore(void* buffer, bool reading);
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagContent() override;
    unsigned char _SerializeTagSpecies() override;
    unsigned short _SerializeTagVersion() override;
    const unsigned char* _SerializeTagComment() override;
    _SEntity mEntity;
    unsigned char _17b8[0x30];
    unsigned long mState17e8;
    unsigned char _17f0[0x1d58 - 0x17f0];
    CPlatformUnit* mUnits[6];
    unsigned long mPaletteHistory[32];
    unsigned char _1e88[0x2188 - 0x1e88];
    unsigned long mPatchState[5];
    unsigned char _21b0[0x3d8a70 - 0x21b0];
    CHvcSpu::_SQdDisk mQdDisk;
    unsigned char _3d8a80[0x3d93b0 - 0x3d8a80];
};
CHvc* GetCHvc();
static_assert(sizeof(CHvc) == 0x3d93b0, "HVC platform layout");
} } } }
extern Vessel::Emulator::Virtual::PlatformHvc::CHvc g_cHvc;
