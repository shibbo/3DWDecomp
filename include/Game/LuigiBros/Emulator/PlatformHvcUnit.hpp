#pragma once

#include "LuigiBros/Emulator/PlatformUnit.hpp"

namespace Vessel::Emulator::Virtual::PlatformHvc {

/**
 * @brief Base of the units of the emulated Famicom (HVC) platform.
 */
class CPlatformHvcUnit : public CPlatformUnit {
public:
    /// Mapper numbers stored in _SEntity::mMapper.
    enum Mapper {
        cMapper_Mmc2 = 9,
        cMapper_Mmc4 = 10,
    };

    /// Offsets of the PPU and I/O registers inside _SEntity::mMemory.
    enum MemoryOffset {
        cMemory_PpuRegister = 0x303800,
        cMemory_PpuLatch = 0x303808,
        cMemory_IoRegister = 0x303810,
        cMemory_IoShadow = 0x303a10,
        cMemory_Oam = 0x304410,
        cMemory_Palette = 0x304510,
    };

    /**
     * @brief One entry of a bus page table.
     */
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

    static_assert(sizeof(_SPage) == 0x20, "_SPage size");

    /// Scroll position forced on one scanline (values above 0x1ff mean "no override").
    struct _SLineScroll {
        u16 mX;
        u16 mY;
    };

    /**
     * @brief Per-title settings of the emulated cartridge.
     */
    struct _STitleElement {
        const void* mImage;
        const void* mDiskImage;
        void* _10;
        void* mDiskSystem;
        void* _20;
        u64 _28;
        u64 mTitleId;
        u64 _38;
    };

    static_assert(sizeof(_STitleElement) == 0x40, "_STitleElement size");

    using _STitleSetting = _STitleElement;

    /// Number of entries of _SCustomParameter::mLineScroll.
    static constexpr s32 cLineScrollNum = 314;

    /// Number of sound channels with a mixing setting.
    static constexpr s32 cSoundChannelNum = 9;

    /**
     * @brief Per-title rendering and mixing parameters.
     */
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

    static_assert(__builtin_offsetof(_SCustomParameter, mSweepPeriodMax) == 0x520,
                  "_SCustomParameter::mSweepPeriodMax");
    static_assert(sizeof(_SCustomParameter) == 0x558, "_SCustomParameter size");

    using _STitleRender = _SCustomParameter;

    /**
     * @brief Statistics gathered while emulating.
     */
    struct _SSurveyParameter {
        u64 _00;
    };

    /**
     * @brief Divides the master clock into frames and scanlines.
     */
    struct _SCycleController {
        void Initialize(_STitleElement& rElement);
        void AdjustParameter(s32, s32, s32, s32, s32, s32);
        void ChangeSequenceMode(s32 mode);
        void Finalize();
        u64 Progress(u64 cycles);

        u64 mFrameClock;
        u64 _08;
        u64 mScanlineClock;
        u64 _18;
        u64 _20;
        u64 _28;
        s64 _30;
        s64 _38;
    };

    /**
     * @brief Pending reset and interrupt requests.
     */
    struct _SOrderController {
        void Initialize(_STitleElement& rElement);
        void Finalize();

        u16 mResetRequest;
        u16 _02;
        u16 mInterruptRequest;
        u16 _06;
        u16 _08;
        u16 _0a;
        u16 _0c;
        u16 _0e;
    };

    /**
     * @brief State of the emulated machine shared by all of its units.
     */
    struct _SEntity {
        /// Whether the cartridge uses the MMC2 or MMC4 character latch.
        bool IsMmc2orMmc4() const {
            return mMapper == cMapper_Mmc2 || mMapper == cMapper_Mmc4;
        }

        /**
         * @brief Get a pointer into PPU address space.
         * @param address The PPU address.
         * @return The backing memory of that address.
         */
        u8* GetPpuPointer(u64 address) const {
            return mMemory + mPpuPages[address >> 10].mOffset + (address & 0x3ff);
        }

        u8 mMapper;
        u8 _01;
        u8 _02;
        u8 _03;
        u8 _04[4];
        _SPage mCpuPages[16];
        _SPage mPpuPages[80];
        u16 mCycleRemain;
        u16 mCycleDebt;
        u8 _c0c[4];

        union {
            _SCycleController mCycleController;

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
    };

    static_assert(__builtin_offsetof(_SEntity, mPpuPages) == 0x208, "_SEntity::mPpuPages");
    static_assert(__builtin_offsetof(_SEntity, mScanlineClock) == 0xc20, "_SEntity::mScanlineClock");
    static_assert(__builtin_offsetof(_SEntity, mTitleRender) == 0x1760, "_SEntity::mTitleRender");
    static_assert(__builtin_offsetof(_SEntity, mMemory) == 0x1728, "_SEntity::mMemory");
    static_assert(__builtin_offsetof(_SEntity, mSampleClock) == 0x1658, "_SEntity::mSampleClock");
    static_assert(__builtin_offsetof(_SEntity, mHalfFrameEvents) == 0x1668,
                  "_SEntity::mHalfFrameEvents");
    static_assert(sizeof(_SEntity) == 0x1770, "_SEntity size");

    /**
     * @brief Construct a unit of the given kind.
     * @param kind The unit kind.
     * @param index The unit index.
     * @param flags The unit flags.
     */
    CPlatformHvcUnit(u8 kind, u64 index, u64 flags) : CPlatformUnit(index, flags) {
        mIsActive = false;
        mParent = nullptr;
        mDescriptor.mKind = kind;
        mEntity = nullptr;
    }

    ~CPlatformHvcUnit() override {}

    /**
     * @brief Attach the unit to its parent and run its prologue.
     * @param pParent The parent unit.
     * @return The result of the prologue (0 on success), or 1 without a parent.
     */
    int Initialize(CPlatformUnit* pParent) override {
        if (mIsActive) {
            return 0;
        }

        if (pParent == nullptr) {
            return 1;
        }

        mParent = pParent;
        if (pParent != this) {
            mDescriptor.mFlags *= pParent->mDescriptor.mFlags;
        }

        mEntity = nullptr;
        int result = _Prologue();
        mIsActive = result == 0;
        return result;
    }

    /**
     * @brief Run the epilogue of an active unit.
     * @return The result of the epilogue (0 on success).
     */
    int Finalize() override {
        if (!mIsActive) {
            return 0;
        }

        int result = _Epilogue();
        mIsActive = result == 0;
        return result;
    }

    int Notify(CPlatformUnit* pUnit, u64 channel, u64 operation, void* pArgument) override;
    int SystemAttach(CPlatformUnit* pUnit, u64 channel, u64 operation,
                     UArgument* pArgument) override;
    int SystemDetach(CPlatformUnit* pUnit, u64 channel, u64 operation,
                     UArgument* pArgument) override;
    int System(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    int Access(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    int Process(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) override;
    int ExportContent(void* pBuffer, u64 bufferSize, u64& rSize) override;
    int ImportContent(const void* pBuffer, u64 bufferSize, u64& rSize) override;
    int InferContentSize(u64& rSize) override;

    static void ConfigureEntity(_SEntity& rEntity, void* pMemory, u8* pFrameBuffer,
                                s16* pAudioBuffer, u64* pAudioWork, _STitleElement* pElement,
                                _SCustomParameter* pCustom, _SSurveyParameter* pSurvey);
    static void GenerateSerializeTag(u8* pBuffer, u8 content, u8 species, u16 version,
                                     const u8* pComment, u64 size);
    static void ValidateSerializeTag(const u8* pBuffer, u8 content, u8 species, u16 version,
                                     const u8* pComment, u64 size);

    virtual int _Prologue() = 0;
    virtual int _Epilogue() = 0;
    virtual void _SerializeCoreExport(void* pBuffer) = 0;
    virtual void _SerializeCoreImport(const void* pBuffer) = 0;
    virtual u64 _SerializeCoreInferSize() = 0;
    virtual u8 _SerializeTagContent() = 0;
    virtual u8 _SerializeTagSpecies() = 0;
    virtual u16 _SerializeTagVersion() = 0;
    virtual const char* _SerializeTagComment() = 0;

    CPlatformUnit* mParent;
    bool mIsActive;
    _SEntity* mEntity;
};

static_assert(sizeof(CPlatformHvcUnit) == 0x48, "CPlatformHvcUnit size");

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
