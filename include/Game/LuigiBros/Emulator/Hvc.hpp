#pragma once

#include "LuigiBros/Emulator/HvcCpu.hpp"
#include "LuigiBros/Emulator/HvcGio.hpp"
#include "LuigiBros/Emulator/HvcMmc.hpp"
#include "LuigiBros/Emulator/HvcPpu.hpp"
#include "LuigiBros/Emulator/HvcSpu.hpp"
#include "LuigiBros/Emulator/PlatformHvcUnit.hpp"

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief The emulated Famicom (HVC) itself.
 *
 * Owns the machine state, the memory of the cartridge and all of the units (CPU, PPU, sound,
 * controllers and every supported mapper), and drives them one frame at a time.
 */
class CHvc : public CPlatformHvcUnit {
public:
    /// Slots of mUnit.
    enum UnitSlot {
        cUnit_Self = 0,
        cUnit_Mapper = 1,
        cUnit_Gio = 2,
        cUnit_Cpu = 3,
        cUnit_Ppu = 4,
        cUnit_Spu = 5,
        cUnit_Num = 6,
    };

    /// Content blocks of a saved state (bits of the content mask).
    enum Content {
        cContent_DiskSide = 8,
        cContent_DiskProgram = 24,
        cContent_Num = 25,
    };

    /// Width of the output picture in pixels.
    static constexpr s32 cScreenWidth = 256;

    /// Height of the output picture in lines.
    static constexpr s32 cScreenHeight = 240;

    /// Number of 32-bit pixels in one line of the output picture.
    static constexpr s32 cOutputPitch = 512;

    /// Size of one disk side.
    static constexpr u64 cDiskSideSize = 0x10000;

    /**
     * @brief Describes one block of a saved state.
     */
    struct SSerializeInformation {
        u8 mContent;
        u8 mSpecies;
        u16 mVersion;
        const u8* mComment;
        u64 mUnit;
        u64 _18;
        u64 mOffset;
        u64 mSize;
        u64 mExtraOffset;
        u64 mExtraSize;
    };

    /**
     * @brief Copy of the controller state handed to the controller unit.
     */
    struct SGioInput {
        u64 mButton[4];
        u64 _20[3];
        u64 mMicrophone;
        u64 mExpansion;
        u64 _48[3];
    };

    /**
     * @brief Backing memory of the machine.
     */
    struct SMachineMemory {
        u8 mMemory[0x304630];
        u8 mFrameBuffer[0x40000];
        s16 mAudioBuffer[0x2000];
    };

    /**
     * @brief Copy of the battery backed memory taken before a frame, to detect writes.
     */
    struct SBackupSnapshot {
        u8 mMemory[0x2000];
        bool mIsDirty;
    };

    CHvc();
    ~CHvc() override;
    int Commence(void* pImage, void* pDiskImage, void* p3, void* p4, u64 titleId, u64 p6);
    int Progress(u64 frame, u64* pInput, u64 flags);
    void ExtractVideoSignal(u64* pOutput, u64 size);
    void PatchVideoSignal();
    void ExtractAudioSignal(s16* pOutput, u64 size);
    void TakeAudioSample(s16*& rpSample, u64& rSampleNum);
    u64 ContentSize(u64 mask);
    u64 ContentSave(u64 mask, u8* pBuffer, u64 bufferSize);
    u64 ContentLoad(u64 mask, const u8* pBuffer, u64 bufferSize);
    void AdjustCustomParameter();
    void Debug(u64 flags);
    int _Prologue() override;
    int _Epilogue() override;
    int System(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    int Access(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    int Process(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    u64 _SerializeCore(void* pBuffer, bool isImport);
    void _SerializeCoreExport(void* pBuffer) override;
    void _SerializeCoreImport(const void* pBuffer) override;
    u64 _SerializeCoreInferSize() override;

    /// Content tag of the serialized state.
    u8 _SerializeTagContent() override { return 0; }

    /// Species tag of the serialized state.
    u8 _SerializeTagSpecies() override { return 0; }

    /// Version of the serialized state.
    u16 _SerializeTagVersion() override { return 0x100; }

    /// Comment stored with the serialized state.
    const char* _SerializeTagComment() override { return "Hvc    "; }

    /// The blocks of a saved state, indexed by content number.
    static SSerializeInformation c_aSerializeInformationTable[cContent_Num];

    /// Get the backing memory of the machine (bus and video memory).
    u8* GetMemory() const { return mEntityData.mMemory; }

    _SEntity mEntityData;
    _STitleElement mTitleElement;
    _SCustomParameter mCustomParameter;
    _SSurveyParameter mSurveyParameter;
    CPlatformUnit* mUnit[cUnit_Num];
    u64 mSpecialColor[32];
    u8 _1e88[0x300];

    union {
        u64 mDirectColor[0x10000];
        u8 mPatchWork[0x80000];
    };

    SMachineMemory mMachine;
    u64 mAudioWork[0x1100];
    u64 mGrayPalette[64];
    u8 mSkipFrame[0x20];
    s64 mSkipFrameIndex;
    s64 mSkipFrameNum;
    SBackupSnapshot mBackup;
    CHvcCpu mCpu;
    CHvcPpu mPpu;
    CHvcSpu mSpu;
    CHvcGio mGio;
    CHvcMmc mMmc;
    CHvcMmc1 mMmc1;
    CHvcMmcPrg mMmcPrg;
    CHvcMmcChr mMmcChr;
    CHvcMmc2 mMmc2;
    CHvcMmc3 mMmc3;
    CHvcMmc3TLS mMmc3TLS;
    CHvcMmc4 mMmc4;
    CHvcMmc5 mMmc5;
    CHvcMapper45 mMapper45;
    CHvcMapper57 mMapper57;
    CHvcMapper5F mMapper5F;
    CHvcMapperB8 mMapperB8;
    CHvcVrc1 mVrc1;
    CHvcVrc2b mVrc2b;
    CHvcVrc3 mVrc3;
    CHvcVrc4 mVrc4;
    CHvcVrc6 mVrc6;
    CHvcDisk mDisk;
};

static_assert(__builtin_offsetof(CHvc, mTitleElement) == 0x17b8, "CHvc::mTitleElement");
static_assert(__builtin_offsetof(CHvc, mUnit) == 0x1d58, "CHvc::mUnit");
static_assert(__builtin_offsetof(CHvc, mDirectColor) == 0x2188, "CHvc::mDirectColor");
static_assert(__builtin_offsetof(CHvc, mMachine) == 0x82188, "CHvc::mMachine");
static_assert(__builtin_offsetof(CHvc, mGrayPalette) == 0x3d2fb8, "CHvc::mGrayPalette");
static_assert(__builtin_offsetof(CHvc, mBackup) == 0x3d31e8, "CHvc::mBackup");
static_assert(__builtin_offsetof(CHvc, mCpu) == 0x3d51f0, "CHvc::mCpu");
static_assert(__builtin_offsetof(CHvc, mMmc) == 0x3d8b40, "CHvc::mMmc");
static_assert(__builtin_offsetof(CHvc, mDisk) == 0x3d9358, "CHvc::mDisk");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
