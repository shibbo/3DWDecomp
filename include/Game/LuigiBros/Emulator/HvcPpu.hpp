#pragma once
#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"
namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
/** @brief PPU interface; rendering state is still being recovered. */
class CHvcPpu : public CPlatformHvcUnit {
public:
    CHvcPpu();
    ~CHvcPpu() override;
    unsigned int Access(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int Process(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned int System(CPlatformUnit*, unsigned long, unsigned long, UArgument*) override;
    unsigned long _SerializeCore(void*, bool);
    void _SerializeCoreExport(void*) override;
    void _SerializeCoreImport(const void*) override;
    static const unsigned long _PatternDotData[256];
    static const unsigned long _PatternDotDataFlip[256];
    unsigned long _PatternDot(unsigned char*, bool);
    unsigned long _PatternDotNoFlip(unsigned char*);
    void _RasterizePhase0(unsigned long);
    void _RasterizePhase1(unsigned long);
    void _RasterizePhase2(unsigned long);
    void _RasterizePhase3(unsigned long);
    void _RasterizePhase4(unsigned long);
    void _RasterizePhase1_Mother(unsigned long);
    void _RasterizePhase4_Mother(unsigned long);
    void _LatchCharacter(unsigned short);
    void _LatchCharacter_Mmc2orMmc4(unsigned short);
    unsigned int _Prologue() override;
    unsigned int _Epilogue() override;
    unsigned long _SerializeCoreInferSize() override;
    unsigned char _SerializeTagContent() override;
    unsigned char _SerializeTagSpecies() override;
    unsigned short _SerializeTagVersion() override;
    const unsigned char* _SerializeTagComment() override;
private:
    unsigned char mRegisters[8];
    unsigned short mScrollX;
    unsigned short mScrollY;
    unsigned short mVramAddress;
    unsigned char mOamAddress;
    unsigned char mNmiLatched;
    unsigned short mRasterScrollX;
    unsigned short mRasterScrollY;
    unsigned char mStatus;
    unsigned char mReadBuffer;
    unsigned short mRawScrollY;
    unsigned short mAddress60;
    unsigned short mDmaCount;
    typedef void (CHvcPpu::*RasterPhase)(unsigned long);
    RasterPhase mRasterPhase0;
    RasterPhase mRasterPhase1;
    RasterPhase mRasterPhase2;
    RasterPhase mRasterPhase3;
    RasterPhase mRasterPhase4;
    void (CHvcPpu::*mCharacterLatch)(unsigned short);
    struct RasterPixel {
        unsigned char color;
        unsigned char flags;
    };
    union {
        unsigned char mRasterBuffer[0x400];
        RasterPixel mRasterPixels[512];
    };
};
} } } }
