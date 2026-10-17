#pragma once
#include <cstddef>
#include "Game/LuigiBros/Emulator/PlatformUnit.hpp"

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {

class CPlatformHvcUnit : public CPlatformUnit {
public:
    enum MemoryOffset { cMemory_PpuRegister=0x303800, cMemory_PpuLatch=0x303808, cMemory_IoRegister=0x303810, cMemory_IoShadow=0x303a10, cMemory_Oam=0x304410, cMemory_Palette=0x304510 };
    /** @brief Initializes the HVC parent, lifecycle flag, and shared entity. */
    CPlatformHvcUnit() {
        mInitialized = false;
        mpParent = nullptr;
        *reinterpret_cast<unsigned char*>(&_28) = 3;
        mpEntity = nullptr;
    }

    struct _STitleElement {
 union { struct {
        const unsigned char* pHeader;
        const unsigned char* pPrg;
        const unsigned char* pChr;
        const unsigned char* pDiskBios;
        unsigned char _20[0x18];
        unsigned long frameRate;
    }; struct {
        const void* mImage;
        const void* mDiskImage;
        void* _10;
        void* mDiskSystem;
        void* _compat20;
        u64 _28;
        u64 mTitleId;
        u64 _38;
    }; };
};
    struct _SCycleController {
        struct SequenceEvent {
            long cycle;
            unsigned long flags;
        };
        unsigned long lineCycle;
        unsigned long frameCycle;
        unsigned long line;
        unsigned long frame;
        unsigned long sequenceMode;
        unsigned long sequenceIndex;
        long sequenceCycle;
        unsigned long _38;
        unsigned long region;
        unsigned long linePeriod;
        unsigned long lineBlank;
        unsigned long framePeriod;
        unsigned long frameBlank;
        unsigned long lineThresholds[314];
        long _a38;
        long _a40;
        unsigned long _a48;
        unsigned long _a50;
        unsigned long _a58;
        unsigned long _a60;
        SequenceEvent events[2][5];

        void Initialize(_STitleElement& rTitle);
        void AdjustParameter(int lineAdjust, int lineBlankAdjust, int frameAdjust,
            int frameBlankAdjust, int thresholdAdjust, int offsetAdjust);
        void ChangeSequenceMode(int mode);
        void Finalize();
        unsigned long Progress(unsigned long cycles);
    };
    struct _SOrderController {
        unsigned short words[8];
        void Initialize(_STitleElement& rTitle);
        void Finalize();
    };
    struct _SBankAccess {
        unsigned char type;
        unsigned char _01;
        unsigned short mask;
        unsigned int _04;
        unsigned long offset;
    };
    union _SBank {
        _SBankAccess access[2];
        struct {
        union {
            unsigned long _00;
            struct { unsigned char readType; unsigned char _01; unsigned short readMask; unsigned int _04; };
        };
        unsigned long readOffset;
        union {
            unsigned long _10;
            struct { unsigned char writeType; unsigned char _11; unsigned short writeMask; unsigned int _14; };
        };
        unsigned long writeOffset;
        };
    };
    struct _SLineScroll {
        u16 mX;
        u16 mY;
    };
 static constexpr int cLineScrollNum = 314;
 static constexpr int cSoundChannelNum = 9;
struct _SCustomParameter {
        u64 _00[6];
        u16 _30;
        u8 mVisibleTop;
        u8 mVisibleBottom;
        u8 mSpriteLimit;
        u8 mIsSprite0HitAlways;
        _SLineScroll mLineScroll[cLineScrollNum];
        union {
            struct {
                u8 _51e;
                u8 _51f;
                u8 _520;
                u8 _521;
            };

            /// Range of square periods that produce sound (inclusive).
            struct {
                s16 mSweepPeriodMin;
                s16 mSweepPeriodMax;
            };
        };

        u8 _522;
        u8 _523[5];
        s64 mVolumeScale;
        s8 mVolume[cSoundChannelNum];
        s8 mRate[cSoundChannelNum];
        s8 mPan[cSoundChannelNum];
        s8 mPanMaster;
        u8 _54c[4];
        u32 _550;
        u8 _554[4];
    };
    struct _SSurveyParameter { unsigned long _00; };
    struct _SPage {
        u8 mType;
        u8 mLimit;
        u16 _02;
        u8 _04[4];
        u64 mOffset;
        u8 _10;
        u8 mLine;
        u16 _12;
        u8 _14[4];
        u64 _18;
    };
    struct _SEntity {
 union { struct {
        unsigned char mapper;
        unsigned char flags;
        unsigned char prgBankCount;
        unsigned char chrBankCount;
        unsigned char _04[4];
        union {
            _SBank banks[32];
            struct {
                _SBank cpuBanks[16];
                _SBank ppuBanks[16];
            };
        };
        union {
            unsigned char _408[0x808];
            _SBank chrHistory[64];
            struct {
                _SBank latchBanks[2][16];
                unsigned char _808[0x408];
            };
        };
        union { _SCycleController cycle; _SCycleController mCycleController; };
        _SOrderController order;
        unsigned char* pMemory;
        unsigned char* pVideo;
        short* pAudio;
        unsigned long* pOutput;
        unsigned long _1748;
        unsigned long _1750;
        _STitleElement* pTitle;
        _SCustomParameter* pCustom;
        _SSurveyParameter* pSurvey;
    }; struct {        u8 mMapper;
        u8 _01;
        u8 _02;
        u8 _03;
        u8 _compat04[4];
        _SPage mCpuPages[16];
        _SPage mPpuPages[80];
        u16 mCycleRemain;
        u16 mCycleDebt;
        u8 _c0c[4];

        union {
            unsigned char _compatCycle[0x40];

            struct {
                u64 mFrameClock;
                u64 _c18;
                u64 mScanlineClock;
            };
        };

        u8 _c50[8];
        u64 mCpuClock;
        u64 mCpuClockBase;
        u8 _c68[0xd78 - 0xc68];
        u64 _d78;
        u8 _d80[0x11e8 - 0xd80];
        u64 _11e8[4];
        u8 _1208[0x1280 - 0x1208];
        u64 _1280[3];
        u8 _1298[0x1648 - 0x1298];
        u64 mDmaCycles;
        u8 _1650[8];
        u64 mSampleClock;
        u8 _1660[8];
        u64 mHalfFrameEvents;
        u64 mQuarterFrameEvents;
        u8 _1678[0x1718 - 0x1678];

        union {
            _SOrderController mOrderController;

            struct {
                u16 mResetRequest;
                u16 _171a;
                u16 mInterruptRequest;
            };
        };

        u8* mMemory;
        u8* mFrameBuffer;
        s16* mAudioBuffer;
        u8 _1740[0x10];
        u64 mAudioSampleNum;
        _STitleElement* mTitleSetting;
        _SCustomParameter* mTitleRender;
        u8 _1768[8];
    }; };
};

    /**
     * @brief Initializes the unit and inherits its parent's timing scale.
     * @param pParent Parent platform unit, or this unit for a root platform.
     * @return Prologue result, zero if already initialized, or one for a null parent.
     */
    unsigned int Initialize(CPlatformUnit* pParent) override {
        if (mInitialized) { return 0; }
        if (!pParent) { return 1; }
        mpParent = pParent;
        if (this != pParent) { mScale *= pParent->mScale; }
        mpEntity = nullptr;
        unsigned int result = _Prologue();
        mInitialized = result == 0;
        return result;
    }
    /**
     * @brief Invokes the epilogue and records its success in the lifecycle flag.
     * @return Epilogue result, or zero if the unit was not initialized.
     */
    unsigned int Finalize() override {
        if (!mInitialized) { return 0; }
        unsigned int result = _Epilogue();
        mInitialized = result == 0;
        return result;
    }
    unsigned int Notify(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, void* pArguments) override;
    unsigned int SystemAttach(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override;
    unsigned int SystemDetach(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override;
    /**
     * @brief Provides the platform's empty system hook.
     * @param pSender Unused sending unit.
     * @param category Unused notification category.
     * @param operation Unused operation identifier.
     * @param pArguments Unused operation arguments.
     * @return Zero.
     */
    unsigned int System(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override { return 0; }
    /**
     * @brief Provides the platform's empty access hook.
     * @param pSender Unused sending unit.
     * @param category Unused notification category.
     * @param operation Unused operation identifier.
     * @param pArguments Unused operation arguments.
     * @return Zero.
     */
    unsigned int Access(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override { return 0; }
    /**
     * @brief Provides the platform's empty process hook.
     * @param pSender Unused sending unit.
     * @param category Unused notification category.
     * @param operation Unused operation identifier.
     * @param pArguments Unused operation arguments.
     * @return Zero.
     */
    unsigned int Process(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) override { return 0; }
    unsigned int ExportContent(void* pBuffer, unsigned long capacity, unsigned long& rSize) override;
    unsigned int ImportContent(const void* pBuffer, unsigned long size, unsigned long& rSize) override;
    unsigned int InferContentSize(unsigned long& rSize) override;
    virtual unsigned int _Prologue() = 0;
    virtual unsigned int _Epilogue() = 0;
    virtual void _SerializeCoreExport(void* pBuffer) = 0;
    virtual void _SerializeCoreImport(const void* pBuffer) = 0;
    virtual unsigned long _SerializeCoreInferSize() = 0;
    virtual unsigned char _SerializeTagContent() = 0;
    virtual unsigned char _SerializeTagSpecies() = 0;
    virtual unsigned short _SerializeTagVersion() = 0;
    virtual const unsigned char* _SerializeTagComment() = 0;

    CPlatformUnit* mpParent;
    bool mInitialized;
    _SEntity* mpEntity;

    static _SEntity* ConfigureEntity(_SEntity& rEntity, void* pMemory,
        unsigned char* pVideo, short* pAudio, unsigned long* pOutput,
        _STitleElement* pTitle, _SCustomParameter* pCustom, _SSurveyParameter* pSurvey);
    static unsigned int _MapBasicOperation(_SEntity* pEntity, unsigned long category,
        unsigned long operation, UArgument* pArguments);

    static unsigned int GenerateSerializeTag(unsigned char* pTag, unsigned char type,
        unsigned char version, unsigned short flags, const unsigned char* pIdentifier,
        unsigned long size);
    static unsigned int ValidateSerializeTag(const unsigned char* pTag, unsigned char type,
        unsigned char version, unsigned short flags, const unsigned char* pIdentifier,
        unsigned long size);
    static void AdjustEntityPageTableForPrg(_SEntity* pEntity, unsigned long mask);
    static void AdjustEntityPageTableForChr(_SEntity* pEntity, unsigned long mask);
    static void AdjustEntityPageTableForVRam(_SEntity* pEntity, unsigned long mask);
};
void AnalyzeVariation(const void* pFirst, const void* pSecond, unsigned long size,
    unsigned long& rOffset, unsigned long& rLength);
void BusMapSwitchVRAM(CPlatformHvcUnit::_SEntity* pEntity, int mode);
static_assert(sizeof(CPlatformHvcUnit::_SCycleController) == 0xb08, "HVC cycle layout");
static_assert(offsetof(CPlatformHvcUnit::_SEntity, cycle) == 0xc10, "HVC cycle-controller offset");
static_assert(offsetof(CPlatformHvcUnit::_SEntity, order) == 0x1718, "HVC order-controller offset");
static_assert(offsetof(CPlatformHvcUnit::_SEntity, pMemory) == 0x1728, "HVC memory layout");

} } } }
