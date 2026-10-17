#include "LuigiBros/Emulator/Hvc.hpp"
#include "LuigiBros/Emulator/HvcPatch.hpp"
#include "LuigiBros/Emulator/HvcVideoPatchData.hpp"
#include "LuigiBros/Utility/Serialize.hpp"
#include <cstring>

/**
 * @brief The Luigi Bros. emulator front end.
 */
struct HVCEmu {
    u8 _00[0x58];
    u8* mDiskImage;
    u8 _60[0x38];
};

/**
 * @brief State of the controllers read by the emulator front end.
 */
struct InputDeviceController {
    u8 _00[0x82];
    u8 mTrigger;
    u8 _83[0x39];
};

extern void* g_pDiskSystem;
extern u8 g_nDiskSideMax;
extern HVCEmu m_HVCEmu;
extern InputDeviceController m_inputdevice_controller;

namespace Vessel {
void DebugDumpMemory(u8* pMemory, u64 size, u64 width, const u8* pName);
}  // namespace Vessel

namespace Vessel::Emulator::Virtual::PlatformHvc {

void AnalyzeVariation(const void* pBase, const void* pData, u64 size, u64& rOffset, u64& rSize);

namespace {

using Serializer = Common::Serialize::LittleEndian;

/// Offset of the work RAM inside the machine memory.
constexpr u64 cMemory_WorkRam = 0x300000;

/// Offset of the battery backed RAM inside the machine memory.
constexpr u64 cMemory_BackupRam = 0x300800;

/// Offset of the disk images inside the machine memory.
constexpr u64 cMemory_Disk = 0x100000;

/// Offset of the disk program RAM inside the machine memory.
constexpr u64 cMemory_DiskProgram = 0x2000;

/// Offset of the video RAM inside the machine memory.
constexpr u64 cMemory_VideoRam = 0x302800;

/// Offset of the backup memory area used by the title patches.
constexpr u64 cMemory_PatchBackup = 0x303000;

/// Offset of the output palette inside the machine memory.
constexpr u64 cMemory_OutputPalette = 0x304530;

/// Size of a serialize tag.
constexpr u64 cSerializeTagSize = 0x10;

/// Size of the disk program block of a saved state.
constexpr u64 cDiskProgramContentSize = 0x8010;

/**
 * @brief Round a size up to a multiple of 16 bytes.
 * @param size The size.
 * @return The rounded size.
 */
u64 alignSize(u32 size) {
    return (size + 15) & ~0xfu;
}

/**
 * @brief Get the size of one block of a saved state.
 * @param pUnit The units of the emulator.
 * @param index The content number.
 * @param rSize Receives the size.
 */
ALWAYS_INLINE void inferContentSize(CPlatformUnit* const* pUnit, u64 index, u64& rSize) {
    const CHvc::SSerializeInformation& rInfo = CHvc::c_aSerializeInformationTable[index];
    switch (index) {
    case CHvc::cContent_DiskSide:
        rSize = cSerializeTagSize | (g_nDiskSideMax << 14);
        break;
    case CHvc::cContent_DiskProgram:
        rSize = cDiskProgramContentSize;
        break;
    default:
        if (index - 6 <= 18) {
            rSize =
                static_cast<u32>(rInfo.mSize + rInfo.mExtraSize + cSerializeTagSize + 15) & ~0xfu;
        } else {
            pUnit[rInfo.mUnit]->InferContentSize(rSize);
        }
        break;
    }
}

/// Content numbers whose block contains a main memory range (one bit per content number).
constexpr u32 cMainRangeContentMask = 0x10ffc00;

/**
 * @brief Check whether a block of a saved state contains a main memory range.
 * @param index The content number.
 */
bool hasMainRange(u64 index) {
    return (cMainRangeContentMask >> index) & 1;
}

/**
 * @brief Check whether a block of a saved state contains an extra memory range.
 * @param index The content number.
 */
bool hasExtraRange(u64 index) {
    return index - 14 <= 2;
}

/**
 * @brief Convert one output palette color to a gray level.
 * @param color The color (red, green and blue in bits 24-31, 16-23 and 8-15).
 * @return The gray level.
 */
ALWAYS_INLINE u64 toGray(u64 color) {
    u64 red = (color >> 24) & 0xff;
    u64 green = (color >> 16) & 0xff;
    u64 blue = (color >> 8) & 0xff;
    red = red > 24 ? red : 24;
    green = green > 24 ? green : 24;
    blue = blue > 24 ? blue : 24;
    red = red < 232 ? red : 232;
    green = green < 232 ? green : 232;
    blue = blue < 232 ? blue : 232;

    u64 gray = static_cast<u64>(red * 0.299) + static_cast<u64>(green * 0.587) +
               static_cast<u64>(blue * 0.114);
    return gray < 255 ? gray : 255;
}

/**
 * @brief Clamp a mixing setting.
 * @param rValue The setting.
 * @param min The lowest value.
 * @param max The highest value.
 */
ALWAYS_INLINE void clampSetting(s8& rValue, s32 min, s32 max) {
    if (rValue < min) {
        rValue = min;
    } else if (rValue > max) {
        rValue = max;
    }
}

/**
 * @brief Initialize a unit through its virtual interface.
 * @param pUnit The unit.
 * @param pParent The parent unit.
 */
ALWAYS_INLINE void initializeUnit(CPlatformUnit* pUnit, CPlatformUnit* pParent) {
    pUnit->Initialize(pParent);
}

/**
 * @brief Finalize a unit through its virtual interface.
 * @param pUnit The unit.
 */
ALWAYS_INLINE void finalizeUnit(CPlatformUnit* pUnit) {
    pUnit->Finalize();
}

/**
 * @brief Let a unit process the cycles of one step.
 * @param pUnit The unit.
 * @param pCaller The calling unit.
 * @param channel The channel.
 * @param operation The operation.
 * @param rArgument The argument to fill.
 * @param events The events of the step.
 * @param clock The clock of the step.
 * @param pCycles Receives the cycles used by the unit.
 * @param pInput The state of the controllers, or nullptr.
 */
ALWAYS_INLINE void processUnit(CPlatformUnit* pUnit, CPlatformUnit* pCaller, u64 channel,
                               u64 operation, CPlatformUnit::UArgument& rArgument, u64 events,
                               u64 clock, u64* pCycles, void* pInput) {
    rArgument.mProcess.mEvents = events;
    rArgument.mProcess.mClock = clock;
    rArgument.mProcess.mCycles = pCycles;
    rArgument.mProcess.mInput = pInput;
    pUnit->Process(pCaller, channel, operation, &rArgument);
}

}  // namespace

/**
 * @brief Construct the emulator and all of its units.
 */
CHvc::CHvc() : CPlatformHvcUnit() {
    std::strncpy(mName, "HVC            ", sizeof(mName) - 1);
    _18 = 0;
    mScale = 1;
    std::memset(&mEntityData, 0, sizeof(mEntityData));
}

/**
 * @brief Destroy the emulator and all of its units.
 */
CHvc::~CHvc() {}

/**
 * @brief Start emulating a title.
 * @param pImage The cartridge image, or nullptr for a disk title.
 * @param pDiskImage The disk image.
 * @param p3 Passed to the units.
 * @param p4 Passed to the units.
 * @param titleId The title number.
 * @param p6 Passed to the units.
 * @return The result of attaching the units.
 */
int CHvc::Commence(void* pImage, void* pDiskImage, void* p3, void* p4, u64 titleId, u64 p6) {
    int result;
    if (pImage != nullptr || (pDiskImage != nullptr && g_pDiskSystem != nullptr)) {
        UArgument argument;
        _STitleElement element;
        element.mImage = pImage;
        element.mDiskImage = pDiskImage;
        element._10 = p3;
        element.mDiskSystem = g_pDiskSystem;
        element._compat20 = p4;
        element._28 = 0;
        element.mTitleId = titleId;
        element._38 = p6;

        argument.mRaw[0] = reinterpret_cast<u64>(&element);
        argument.mRaw[1] = 0;
        argument.mRaw[2] = 0;
        argument.mRaw[3] = 0;
        result = SystemAttach(this, 0, 0, &argument);
        mSkipFrameIndex = mSkipFrameNum;

        const u64* pPalette = reinterpret_cast<const u64*>(GetMemory() + cMemory_OutputPalette);
        for (s32 i = 0; i < 64; i++) {
            mGrayPalette[i] = toGray(pPalette[i]) * 0x01010100 | 0xff;
        }
    } else {
        _SCycleController& rCycle = mEntityData.mCycleController;
        rCycle.lineCycle = 0;
        rCycle.frameCycle = 0;
        rCycle.line = 0;
        rCycle.frame = 0;
        rCycle.sequenceMode = 0;
        rCycle.sequenceIndex = 0;
        rCycle.sequenceCycle = 0;
        rCycle._38 = 0;

        _SOrderController& rOrder = mEntityData.mOrderController;
        rOrder.words[0] = 0x3e;
        rOrder.words[2] = 0;
        rOrder.words[4] = 0;
        rOrder.words[6] = 0;

        mEntityData.mCycleRemain = 0;
        mEntityData.mCycleDebt = 0;
        result = 0;
        mSkipFrameIndex = mSkipFrameNum;
    }

    CHvcPatch::GetInstance().PostCommence();
    return result;
}

/**
 * @brief Emulate one frame.
 * @param frame Unused.
 * @param pInput The state of the controllers.
 * @param flags Unused.
 * @return The result of processing the units, or 0 when the frame is skipped.
 */
int CHvc::Progress(u64 frame, u64* pInput, u64 flags) {
    if (mSkipFrameNum > 0 && mSkipFrameIndex > 0) {
        mSkipFrameIndex--;
        if (mSkipFrame[mSkipFrameIndex] != 0) {
            return 0;
        }
    }

    std::memcpy(mBackup.mMemory, GetMemory() + cMemory_BackupRam, sizeof(mBackup.mMemory));
    CHvcPatch::GetInstance().AnteProgress();

    UArgument argument;
    argument.mProcess.mEvents = 0;
    argument.mProcess.mClock = 0;
    argument.mProcess.mCycles = nullptr;
    argument.mProcess.mInput = pInput;
    int result = Process(this, 2, 2, &argument);

    CHvcPatch::GetInstance().PostProgress();
    if (std::memcmp(mBackup.mMemory, GetMemory() + cMemory_BackupRam, sizeof(mBackup.mMemory)) !=
        0) {
        mBackup.mIsDirty = true;
    }

    return result;
}

/**
 * @brief Convert the rendered frame to 32-bit colors.
 * @param pOutput The output picture (cOutputPitch pixels per line), or nullptr.
 * @param size The size of the output picture.
 */
void CHvc::ExtractVideoSignal(u64* pOutput, u64 size) {
    if (pOutput == nullptr) {
        return;
    }

    u32* pLine = reinterpret_cast<u32*>(pOutput);
    const u8* pFrame = mEntityData.mFrameBuffer;
    const u32* pPalette = reinterpret_cast<const u32*>(GetMemory() + cMemory_OutputPalette);
    u32 y = 0;
    for (; y < mEntityData.mTitleRender->mVisibleTop; y++) {
        for (s32 x = 0; x < cScreenWidth; x++) {
            pLine[x] = 0xff;
        }

        pLine += cOutputPitch;
    }

    const u8* pPixel = pFrame + y * cScreenWidth * 4;
    for (; y <= mEntityData.mTitleRender->mVisibleBottom; y++) {
        for (s32 x = 0; x < cScreenWidth; x++, pPixel += 4) {
            s8 color = pPixel[0];
            if (color < 0 && static_cast<u8>(color) < 0xc0) {
                pLine[x] = mSpecialColor[pPixel[2] & 0x1f];
            } else if (static_cast<u8>(color) >= 0xc0) {
                pLine[x] = mDirectColor[(color & 0x3f) | ((pPixel[3] & 0x3f) << 6)];
            } else if (pPixel[1] & 1) {
                pLine[x] = mGrayPalette[color & 0x3f];
            } else {
                pLine[x] = pPalette[color & 0x3f];
                if (pPixel[1] & 0x20) {
                    pLine[x] |= 0xc00000ff;
                }

                if (pPixel[1] & 0x40) {
                    pLine[x] |= 0x00c000ff;
                }

                if (pPixel[1] & 0x80) {
                    pLine[x] |= 0x0000c0ff;
                }
            }
        }

        pLine += cOutputPitch;
    }

    for (; y < cScreenHeight; y++) {
        for (s32 x = 0; x < cScreenWidth; x++) {
            pLine[x] = 0xff;
        }

        pLine += cOutputPitch;
    }
}

/**
 * @brief Replace the title and logo screens of title 354 with their remastered pictures.
 *
 * This title renders with 64-bit pixels (palette index in bits 8-15, direct color in bits 24-31).
 */
void CHvc::PatchVideoSignal() {
    if (mTitleElement.mTitleId != 354) {
        return;
    }

    u64* pFrame = reinterpret_cast<u64*>(mEntityData.mFrameBuffer);
    u8* pMemory = mpEntity->mMemory;
    u8* pBackup = GetMemory() + cMemory_PatchBackup;

    bool isTitle = true;
    for (s32 y = 0; y < 8; y++) {
        for (s32 x = 104; x < 216; x++) {
            bool isSame =
                static_cast<u8>(pFrame[(104 + y) * 256 + x] >> 8) == cTitleCheck[y][x - 104];
            isTitle &= isSame;
            if (!isSame) {
                break;
            }
        }
    }

    const u8* pPalette = pMemory + cMemory_Palette;
    if (isTitle) {
        for (s32 i = 0; i < 6; i++) {
            bool isVariant = true;
            for (s32 j = 0; j < 32; j++) {
                if (pBackup[j] != cTitleSignature[i][j]) {
                    isVariant = false;
                    break;
                }
            }

            if (!isVariant) {
                continue;
            }

            for (s32 y = 16; y < 112; y++) {
                for (s32 x = 25; x < 225; x++) {
                    u8 index = cTitleImage[y - 16][x - 25];
                    u64& rPixel = pFrame[y * 256 + x];
                    rPixel = (index << 8 | static_cast<u64>(cTitleColor[i][index]) << 24) |
                             (rPixel & 0xff00ff);
                }
            }

            for (s32 y = 112; y < 200; y++) {
                for (s32 x = 64; x < 184; x++) {
                    pFrame[(y - 8) * 256 + x] = static_cast<u32>(pFrame[y * 256 + x - 8]);
                }
            }

            for (s32 x = 101; x < 107; x++) {
                pFrame[190 * 256 + x] = static_cast<u32>(pFrame[192 * 256 + x]);
            }

            for (s32 x = 101; x < 107; x++) {
                pFrame[191 * 256 + x] = static_cast<u32>(pFrame[193 * 256 + x]);
            }

            break;
        }
    }

    bool isLogo = true;
    for (s32 y = 0; y < 5; y++) {
        for (s32 x = 185; x < 199; x++) {
            bool isSame =
                static_cast<u8>(pFrame[(26 + y) * 256 + x] >> 8) == cLogoCheck[y][x - 185];
            isLogo &= isSame;
            if (!isSame) {
                break;
            }
        }
    }

    if (isLogo) {
        for (s32 y = 25; y < 63; y++) {
            for (s32 x = 65; x < 201; x++) {
                u8 index = cLogoImage[y - 25][x - 65];
                u64& rPixel = pFrame[y * 256 + x];
                rPixel =
                    (index << 8 | static_cast<u64>(pBackup[index]) << 24) | (rPixel & 0xff00ff);
            }
        }
    }

    for (s32 i = 0; i < 32; i++) {
        pBackup[i] = pPalette[i];
    }
}

/**
 * @brief Convert the generated sound (not used, see TakeAudioSample).
 * @param pOutput The output samples.
 * @param size The number of samples.
 */
void CHvc::ExtractAudioSignal(s16* pOutput, u64 size) {}

/**
 * @brief Take the samples generated since the last call.
 * @param rpSample Receives the samples.
 * @param rSampleNum Receives the number of samples.
 */
void CHvc::TakeAudioSample(s16*& rpSample, u64& rSampleNum) {
    rSampleNum = mEntityData.mAudioSampleNum;
    rpSample = mEntityData.mAudioBuffer;
    mEntityData.mAudioSampleNum = 0;
}

/**
 * @brief Get the size of a saved state.
 * @param mask The content blocks to save (one bit per content number).
 * @return The size in bytes.
 */
unsigned long CHvc::ContentSize(unsigned long mask) {
    unsigned long i = 0;
    unsigned long total = 0;
    for (; i < 25; ++i) {
        unsigned long size = 0;
        if (mask & static_cast<long>(1 << i)) {
            if (i == 8) {
                size = (static_cast<unsigned long>(g_nDiskSideMax) << 14) + 16;
            } else if (i == 24) {
                size = 0x8010;
            } else if (i >= 6 && i <= 24) {
                size = (static_cast<unsigned int>(c_aSerializeInformationTable[i].mSize) +
                        static_cast<unsigned int>(c_aSerializeInformationTable[i].mExtraSize) + 31) & ~15U;
            } else {
                mUnit[c_aSerializeInformationTable[i].mUnit]->InferContentSize(size);
            }
            total += size;
        }
    }
    return total;
}

/**
 * @brief Save the state of the emulator.
 * @param mask The content blocks to save (one bit per content number).
 * @param pBuffer The destination buffer.
 * @param bufferSize The size of the destination buffer.
 * @return The number of bytes written, or 0 if the buffer is too small.
 */
u64 CHvc::ContentSave(u64 mask, u8* pBuffer, u64 bufferSize) {
    CHvcPatch::GetInstance().Export();
    if (ContentSize(mask) > bufferSize) {
        return 0;
    }

    u8* pCurrent = pBuffer;
    u64 rest = bufferSize;
    for (u64 i = 0; i < cContent_Num; i++) {
        u64 size = 0;
        if (!((1 << i) & mask)) {
            continue;
        }

        const SSerializeInformation& rInfo = c_aSerializeInformationTable[i];
        if (i == cContent_DiskSide) {
            size = cSerializeTagSize | (g_nDiskSideMax << 14);
            GenerateSerializeTag(pCurrent, rInfo.mContent, 0, 0,
                                 reinterpret_cast<const u8*>("-DSK   "), size);

            u8* pDisk = GetMemory() + cMemory_Disk;
            u8* pStart = pCurrent + cSerializeTagSize;
            u8* pSide = pStart;
            u64 diffSize;
            u64 offset;
            u64 reserved = 0;
            u8 reserved0 = 0;
            u8 reserved1 = 0;
            u8 reserved2 = 0;
            for (u64 side = 0; side < g_nDiskSideMax; side++) {
                u64 base = side * cDiskSideSize;
                AnalyzeVariation(m_HVCEmu.mDiskImage + base, pDisk + base, cDiskSideSize, offset,
                                 diffSize);
                if (diffSize == 0) {
                    continue;
                }

                offset += base;

                Serializer serializer(pSide, false);
                u8 version = 1;
                serializer.Entry(version);
                serializer.Entry(reserved0);
                serializer.Entry(reserved1);
                serializer.Entry(reserved2);
                serializer.Entry(diffSize);
                serializer.Entry(offset);
                serializer.Entry(reserved);
                pSide += alignSize(serializer.GetOffset());
                std::memcpy(pSide, pDisk + offset, diffSize);
                pSide += alignSize(diffSize);
            }

            u64 used = pSide - pStart;
            u64 total = g_nDiskSideMax << 14;
            if (total < used) {
                return 0;
            }

            if (total != used) {
                std::memset(pSide, 0, total - used);
            }

            pCurrent = pSide + (total - used);
        } else if (i == cContent_DiskProgram) {
            size = cDiskProgramContentSize;
            GenerateSerializeTag(pCurrent, rInfo.mContent, 0, 0,
                                 reinterpret_cast<const u8*>("-DSKPRG"), size);
            std::memcpy(pCurrent + cSerializeTagSize, GetMemory() + cMemory_DiskProgram, 0x8000);
            pCurrent += size;
        } else if (i - 6 <= 18) {
            u64 mainSize = rInfo.mSize;
            u64 extraSize = rInfo.mExtraSize;
            size = mainSize + extraSize + cSerializeTagSize;
            GenerateSerializeTag(pCurrent, rInfo.mContent, rInfo.mSpecies, rInfo.mVersion,
                                 rInfo.mComment, size);
            pCurrent += cSerializeTagSize;
            if (hasMainRange(i)) {
                std::memcpy(pCurrent, GetMemory() + rInfo.mOffset, mainSize);
                pCurrent += mainSize;
            }

            if (hasExtraRange(i)) {
                std::memcpy(pCurrent, GetMemory() + rInfo.mExtraOffset, extraSize);
                pCurrent += extraSize;
            }
        } else {
            mUnit[rInfo.mUnit]->ExportContent(pCurrent, rest, size);
            pCurrent += size;
        }

        rest -= size;
    }

    return pCurrent - pBuffer;
}

/**
 * @brief Load the state of the emulator.
 * @param mask The content blocks to load (one bit per content number).
 * @param pBuffer The source buffer.
 * @param bufferSize The size of the source buffer.
 * @return The number of bytes read, or 0 if the buffer is too small.
 */
u64 CHvc::ContentLoad(u64 mask, const u8* pBuffer, u64 bufferSize) {
    if (ContentSize(mask) > bufferSize) {
        return 0;
    }

    const u8* pCurrent = pBuffer;
    u64 rest = bufferSize;
    for (u64 i = 0; i < cContent_Num; i++) {
        u64 size = 0;
        if (!((1 << i) & mask)) {
            continue;
        }

        const SSerializeInformation& rInfo = c_aSerializeInformationTable[i];
        if (i == cContent_DiskSide) {
            size = cSerializeTagSize | (g_nDiskSideMax << 14);
            GenerateSerializeTag(const_cast<u8*>(pCurrent), rInfo.mContent, 0, 0,
                                 reinterpret_cast<const u8*>("-DSK   "), size);

            const u8* pStart = pCurrent + cSerializeTagSize;
            u8* pDisk = GetMemory() + cMemory_Disk;
            std::memcpy(pDisk, mTitleElement.mDiskImage, 0x100000);

            const u8* pSide = pStart;
            u8 version;
            for (u32 side = 0; side < g_nDiskSideMax; side++) {
                Serializer serializer(const_cast<u8*>(pSide), true);
                u8 reserved0;
                u8 reserved1;
                u8 reserved2;
                u64 diffSize;
                u64 offset;
                u64 reserved;
                serializer.Entry(version);
                serializer.Entry(reserved0);
                serializer.Entry(reserved1);
                serializer.Entry(reserved2);
                serializer.Entry(diffSize);
                serializer.Entry(offset);
                serializer.Entry(reserved);
                if (version != 1 || diffSize == 0) {
                    break;
                }

                pSide += alignSize(serializer.GetOffset());
                std::memcpy(pDisk + offset, pSide, diffSize);
                pSide += alignSize(diffSize);
            }

            u64 used = pSide - pStart;
            u64 total = g_nDiskSideMax << 14;
            if (total < used) {
                return 0;
            }

            pCurrent = pSide + (total - used);
        } else if (i == cContent_DiskProgram) {
            size = cDiskProgramContentSize;
            ValidateSerializeTag(pCurrent, rInfo.mContent, 0, 0,
                                 reinterpret_cast<const u8*>("-DSKPRG"), size);
            std::memcpy(GetMemory() + cMemory_DiskProgram, pCurrent + cSerializeTagSize, 0x8000);
            pCurrent += size;
        } else if (i - 6 <= 18) {
            u64 mainSize = rInfo.mSize;
            u64 extraSize = rInfo.mExtraSize;
            size = mainSize + extraSize + cSerializeTagSize;
            ValidateSerializeTag(pCurrent, rInfo.mContent, rInfo.mSpecies, rInfo.mVersion,
                                 rInfo.mComment, size);
            pCurrent += cSerializeTagSize;
            if (hasMainRange(i)) {
                std::memcpy(GetMemory() + rInfo.mOffset, pCurrent, mainSize);
                pCurrent += mainSize;
            }

            if (hasExtraRange(i)) {
                std::memcpy(GetMemory() + rInfo.mExtraOffset, pCurrent, extraSize);
                pCurrent += extraSize;
            }
        } else {
            mUnit[rInfo.mUnit]->ImportContent(pCurrent, rest, size);
            pCurrent += size;
        }

        rest -= size;
    }

    CHvcPatch::GetInstance().Import();
    return pCurrent - pBuffer;
}

/**
 * @brief Clamp the custom parameters to their valid ranges and update the volume scale.
 */
void CHvc::AdjustCustomParameter() {
    _SCustomParameter& rParameter = mCustomParameter;
    rParameter._30 = 0;
    if (rParameter.mSpriteLimit > 64) {
        rParameter.mSpriteLimit = 64;
    }

    for (s32 i = 0; i < cSoundChannelNum; i++) {
        clampSetting(rParameter.mVolume[i], 0, 64);
        clampSetting(rParameter.mRate[i], 1, 64);
        clampSetting(rParameter.mPan[i], -16, 16);
    }

    if (rParameter.mPan[0] <= 0) {
        mCustomParameter.mPan[0] = 1;
    }

    clampSetting(rParameter.mPanMaster, -16, 16);

    s64 total = 0;
    for (s32 i = 0; i <= cSoundChannelNum; i++) {
        total += rParameter.mVolume[i];
    }

    if (total != 0) {
        rParameter.mVolumeScale = 0x10000 / total;
    }
}

/**
 * @brief Dump the memories of the machine.
 * @param flags Dump even when the debug button is not held if nonzero.
 */
void CHvc::Debug(unsigned long mode) {
    if (mode || !(reinterpret_cast<unsigned char*>(&m_inputdevice_controller)[0x82] & 8)) return;
    Vessel::DebugDumpMemory(mEntityData.pMemory + 0x300000, 0x800, 16, reinterpret_cast<const unsigned char*>("WRam"));
    Vessel::DebugDumpMemory(mEntityData.pMemory + 0x300800, 0x2000, 16, reinterpret_cast<const unsigned char*>("SRam"));
    Vessel::DebugDumpMemory(mEntityData.pMemory + 0x304410, 0x100, 16, reinterpret_cast<const unsigned char*>("Oam "));
    Vessel::DebugDumpMemory(mEntityData.pMemory + 0x304510, 0x20, 16, reinterpret_cast<const unsigned char*>("Cgm "));
    Vessel::DebugDumpMemory(mEntityData.pMemory + 0x302800, 0x800, 32, reinterpret_cast<const unsigned char*>("VRam"));
}

/**
 * @brief Initialize every unit.
 * @return 0.
 */
unsigned int CHvc::_Prologue() {
    mUnit[cUnit_Cpu] = &mCpu;
    mUnit[cUnit_Ppu] = &mPpu;
    mUnit[cUnit_Spu] = &mSpu;
    mUnit[cUnit_Gio] = &mGio;
    mUnit[cUnit_Self] = this;
    mUnit[cUnit_Mapper] = &mMmc;
    for (s32 i = cUnit_Mapper; i < cUnit_Num; i++) {
        CPlatformUnit* pUnit = mUnit[i];
        if (pUnit != this && pUnit != nullptr) {
            pUnit->Initialize(this);
        }
    }

    initializeUnit(&mMmc1, this);
    initializeUnit(&mMmcPrg, this);
    initializeUnit(&mMmcChr, this);
    initializeUnit(&mMmc2, this);
    initializeUnit(&mMmc3, this);
    initializeUnit(&mMmc3TLS, this);
    initializeUnit(&mMmc4, this);
    initializeUnit(&mMmc5, this);
    initializeUnit(&mMapper45, this);
    initializeUnit(&mMapper57, this);
    initializeUnit(&mMapper5F, this);
    initializeUnit(&mMapperB8, this);
    initializeUnit(&mVrc1, this);
    initializeUnit(&mVrc2b, this);
    initializeUnit(&mVrc3, this);
    initializeUnit(&mVrc4, this);
    initializeUnit(&mVrc6, this);
    initializeUnit(&mDisk, this);

    std::memset(mGrayPalette, 0, sizeof(mGrayPalette));
    std::memset(mDirectColor, 0, sizeof(mDirectColor));
    std::memset(&mBackup, 0, sizeof(mBackup));
    CHvcPatch::GetInstance().Reset(this);
    mInitialized = true;
    return 0;
}

/**
 * @brief Finalize every unit.
 * @return 0.
 */
unsigned int CHvc::_Epilogue() {
    CHvcPatch::GetInstance();

    finalizeUnit(&mDisk);
    finalizeUnit(&mVrc6);
    finalizeUnit(&mVrc4);
    finalizeUnit(&mVrc3);
    finalizeUnit(&mVrc2b);
    finalizeUnit(&mVrc1);
    finalizeUnit(&mMapperB8);
    finalizeUnit(&mMapper5F);
    finalizeUnit(&mMapper57);
    finalizeUnit(&mMapper45);
    finalizeUnit(&mMmc5);
    finalizeUnit(&mMmc4);
    finalizeUnit(&mMmc3);
    finalizeUnit(&mMmc3TLS);
    finalizeUnit(&mMmc2);
    finalizeUnit(&mMmcChr);
    finalizeUnit(&mMmcPrg);
    finalizeUnit(&mMmc1);

    for (s32 i = cUnit_Num - 1; i > cUnit_Self; i--) {
        if (mUnit[i] != nullptr) {
            mUnit[i]->Finalize();
        }
    }

    mInitialized = false;
    return 0;
}

/**
 * @brief Attach a title (operation 0), detach it (operation 1) or forward a system command.
 * @param pUnit The calling unit.
 * @param channel The channel.
 * @param operation The operation.
 * @param pArgument The argument; for operation 0, mRaw[0] points to the _STitleElement of the
 * title and mRaw[3] receives the machine state.
 * @return 0.
 */
unsigned int CHvc::System(CPlatformUnit* pUnit, u64 channel, u64 operation, UArgument* pArgument) {
    if (operation == 0) {
        std::memset(&mMachine, 0, sizeof(mMachine));

        mTitleElement.mImage = reinterpret_cast<const _STitleElement*>(pArgument->mRaw[0])->mImage;
        mTitleElement.mDiskImage =
            reinterpret_cast<const _STitleElement*>(pArgument->mRaw[0])->mDiskImage;
        mTitleElement._10 = reinterpret_cast<const _STitleElement*>(pArgument->mRaw[0])->_10;
        mTitleElement.mDiskSystem = g_pDiskSystem;
        mTitleElement._compat20 = reinterpret_cast<const _STitleElement*>(pArgument->mRaw[0])->_compat20;
        mTitleElement.mTitleId =
            reinterpret_cast<const _STitleElement*>(pArgument->mRaw[0])->mTitleId;
        mTitleElement._38 = reinterpret_cast<const _STitleElement*>(pArgument->mRaw[0])->_38;

        _SCustomParameter& rParameter = mCustomParameter;
        u64 titleId = mTitleElement.mTitleId;
        for (s32 i = 0; i < 6; i++) {
            rParameter._00[i] = 0;
        }

        rParameter._30 = 0;
        rParameter.mVisibleTop = 9;
        rParameter.mVisibleBottom = 236;
        mSkipFrameNum = 0;
        mSkipFrameIndex = 0;
        std::memset(mSkipFrame, 0, sizeof(mSkipFrame));
        rParameter.mSpriteLimit = 8;
        rParameter.mIsSprite0HitAlways = titleId == 139;
        for (s32 i = 0; i < cLineScrollNum; i++) {
            rParameter.mLineScroll[i].mX = 0x200;
            rParameter.mLineScroll[i].mY = 0x200;
        }

        rParameter._51e = 8;
        rParameter._51f = 0;
        rParameter._520 = 0xff;
        rParameter._521 = 7;
        rParameter._522 = 0;
        rParameter.mVolumeScale = 0x10000;

        static const s8 cVolume[cSoundChannelNum] = {0x1b, 0x1b, 0x20, 0x1e, 0x20, 0x20, 0, 0, 0};
        static const s8 cPan[cSoundChannelNum] = {0x0b, 0x0b, 0x09, 0x0a, 0x09, 0x08, 0, 0, 0};
        for (s32 i = 0; i < cSoundChannelNum; i++) {
            rParameter.mVolume[i] = cVolume[i];
            rParameter.mRate[i] = 8;
            rParameter.mPan[i] = cPan[i];
        }

        rParameter.mPanMaster = 0;
        for (s32 i = 0; i < 4; i++) {
            rParameter._54c[i] = 0;
        }

        rParameter._550 = 0;

        switch (titleId) {
        case 204:
        case 205:
        case 206:
        case 212:
        case 213:
        case 214:
        case 215: {
            for (s32 i = 0; i < 6; i++) {
                rParameter._00[i] = 0;
            }

            static const s8 cVolumeWrecking[cSoundChannelNum] = {0x20, 0x20, 0x20, 0x20, 0x20,
                                                                 0x20, 0,    0,    0};
            static const s8 cPanWrecking[cSoundChannelNum] = {0x0a, 0x0a, 0x08, 0x09, 0x08,
                                                              0x08, 0,    0,    0};
            for (s32 i = 0; i < cSoundChannelNum; i++) {
                rParameter.mVolume[i] = cVolumeWrecking[i];
                rParameter.mPan[i] = cPanWrecking[i];
            }

            rParameter.mPanMaster = 1;
            for (s32 i = 0; i < 4; i++) {
                rParameter._54c[i] = 1;
            }

            rParameter._550 = 0;
            break;
        }
        }

        AdjustCustomParameter();
        mSurveyParameter._00 = 0;
        ConfigureEntity(mEntityData, mMachine.mMemory, mMachine.mFrameBuffer, mMachine.mAudioBuffer,
                        mAudioWork, &mTitleElement, &mCustomParameter, &mSurveyParameter);
        mEntityData.mCycleController.Initialize(mTitleElement);
        mEntityData.mOrderController.Initialize(mTitleElement);

        u8* pWorkRam = &mMachine.mMemory[cMemory_WorkRam];
        for (u64 i = 0; i < 0x800; i += 4) {
            *pWorkRam++ = 0x0f;
            *pWorkRam++ = 0xef;
            *pWorkRam++ = 0xfe;
            *pWorkRam++ = 0x7d;
        }

        u8* pPixel = mEntityData.mFrameBuffer;
        for (s32 y = 0; y < cScreenHeight; y++) {
            for (s32 x = 0; x < cScreenWidth; x++, pPixel += 4) {
                pPixel[0] = 0x0f;
                pPixel[1] = 0;
                pPixel[2] = 0x0f;
                pPixel[3] = 0;
            }
        }

        mEntityData.mCycleRemain = 0;
        mEntityData.mCycleDebt = 0;
        mpEntity = &mEntityData;
        pArgument->mRaw[3] = reinterpret_cast<u64>(&mEntityData);

        CHvcPatch::GetInstance().Configure();
        CHvcPatch::GetInstance().Attach();
    } else if (operation == 1) {
        CHvcPatch::GetInstance().Detach();
    }

    switch (mpEntity->mMapper) {
    case 1:
        mUnit[cUnit_Mapper] = &mMmc1;
        break;
    case 2:
        mUnit[cUnit_Mapper] = &mMmcPrg;
        break;
    case 3:
        mUnit[cUnit_Mapper] = &mMmcChr;
        break;
    case 4:
        mUnit[cUnit_Mapper] = &mMmc3;
        break;
    case 5:
        mUnit[cUnit_Mapper] = &mMmc5;
        break;
    case 9:
        mUnit[cUnit_Mapper] = &mMmc2;
        break;
    case 10:
        mUnit[cUnit_Mapper] = &mMmc4;
        break;
    case 23:
        mUnit[cUnit_Mapper] = &mVrc2b;
        break;
    case 24:
        mUnit[cUnit_Mapper] = &mVrc6;
        break;
    case 25:
        mUnit[cUnit_Mapper] = &mVrc4;
        break;
    case 69:
        mUnit[cUnit_Mapper] = &mMapper45;
        break;
    case 73:
        mUnit[cUnit_Mapper] = &mVrc3;
        break;
    case 75:
        mUnit[cUnit_Mapper] = &mVrc1;
        break;
    case 87:
        mUnit[cUnit_Mapper] = &mMapper57;
        break;
    case 95:
        mUnit[cUnit_Mapper] = &mMapper5F;
        break;
    case 118:
        mUnit[cUnit_Mapper] = &mMmc3TLS;
        break;
    case 184:
        mUnit[cUnit_Mapper] = &mMapperB8;
        break;
    case 255:
        mUnit[cUnit_Mapper] = &mDisk;
        break;
    }

    for (s32 i = cUnit_Mapper; i < cUnit_Num; i++) {
        if (operation == 0) {
            mUnit[i]->SystemAttach(this, channel, operation, pArgument);
        } else {
            mUnit[i]->System(this, channel, operation, pArgument);
        }
    }

    return 0;
}

/**
 * @brief Route a bus access to the unit that owns the address.
 * @param pUnit The calling unit.
 * @param channel The channel.
 * @param operation cOperation_Read or cOperation_Write.
 * @param pArgument The bus transfer.
 * @return The result of the owning unit.
 */
unsigned int CHvc::Access(CPlatformUnit* sender, unsigned long category,
                         unsigned long operation, UArgument* arguments) {
    unsigned long address = arguments->values[2];
    CPlatformUnit* mapper = mUnit[1];
    unsigned char unit;
    UArgument query;
    query.values[0] = operation;
    query.values[2] = address;
    query.pointers[4] = &unit;
    mapper->System(sender, 0, 2, &query);
    return mUnit[unit]->Access(sender, category, operation, arguments);
}

/**
 * @brief Run the units until the requested event happens.
 * @param pUnit The calling unit.
 * @param channel The channel.
 * @param operation The events to wait for (1 or 2), or another value to run a single step.
 * @param pArgument The argument; mProcess.mInput points to the state of the controllers.
 * @return 0.
 */
unsigned int CHvc::Process(CPlatformUnit*, unsigned long category,
                          unsigned long operation, UArgument* arguments) {
    UArgument gio, mapper, cpu, video, audio;
    unsigned long input[12];
    if (arguments && arguments->pointers[4]) {
        unsigned char* source = static_cast<unsigned char*>(arguments->pointers[4]);
        input[0] = source[0];
        input[1] = source[8];
        input[2] = source[16];
        input[3] = source[24];
        input[7] = *reinterpret_cast<unsigned long*>(source + 32);
        input[8] = source[40];
    }
    unsigned short& cycleRemainder = *reinterpret_cast<unsigned short*>(mEntityData._408 + 0x800);
    unsigned short& cpuDelay = *reinterpret_cast<unsigned short*>(mEntityData._408 + 0x802);
    unsigned long events;
    do {
        unsigned long stepCycles = 0, inputCycles = 0, mapperCycles = 0;
        unsigned long cpuCycles = 0, videoCycles = 0, audioCycles = 0;
        if (cpuDelay >= 4) {
            cpuCycles = stepCycles = 0x400;
        } else {
            unsigned int unitId = _18;
            unsigned short pending = mEntityData.order.words[2];
            unsigned int mask = 1U << unitId;
            if (pending & mask) mEntityData.order.words[2] = (pending | 8) & ~mask;
            if (mEntityData.order.words[4] & mask) mEntityData.order.words[4] = (mEntityData.order.words[4] | 8) & ~mask;
            if (mEntityData.order.words[6] & mask) mEntityData.order.words[6] = (mEntityData.order.words[6] | 8) & ~mask;
            CPlatformUnit* cpuUnit = mUnit[3];
            cpu.values[0] = 0;
            cpu.values[1] = 0;
            cpu.pointers[2] = &stepCycles;
            cpu.pointers[4] = nullptr;
            cpuUnit->Process(this, category, 3, &cpu);
        }
        events = mEntityData.cycle.Progress(stepCycles + cycleRemainder);
        CPlatformUnit* mapperUnit = mUnit[1];
        mapper.values[0] = events;
        mapper.values[1] = stepCycles + cycleRemainder;
        mapper.pointers[2] = &mapperCycles;
        mapper.pointers[4] = nullptr;
        mapperUnit->Process(this, category, operation, &mapper);
        CPlatformUnit* gioUnit = mUnit[2];
        gio.values[0] = events;
        gio.values[1] = stepCycles + cycleRemainder;
        gio.pointers[2] = &inputCycles;
        gio.pointers[4] = input;
        gioUnit->Process(this, category, operation, &gio);
        CPlatformUnit* videoUnit = mUnit[4];
        video.values[0] = events;
        video.values[1] = stepCycles + cycleRemainder;
        video.pointers[2] = &videoCycles;
        video.pointers[4] = nullptr;
        videoUnit->Process(this, category, operation, &video);
        CPlatformUnit* audioUnit = mUnit[5];
        audio.values[0] = events;
        audio.values[1] = stepCycles + cycleRemainder;
        audio.pointers[2] = &audioCycles;
        audio.pointers[4] = nullptr;
        audioUnit->Process(this, category, operation, &audio);
        unsigned int remainingDelay;
        if (stepCycles > (static_cast<unsigned long>(cpuDelay) << 8)) {
            CPlatformUnit* cpuUnit = mUnit[3];
            cpu.values[0] = events;
            cpu.values[1] = stepCycles;
            cpu.pointers[2] = &cpuCycles;
            cpu.pointers[4] = nullptr;
            cpuUnit->Process(this, category, operation, &cpu);
            remainingDelay = 0;
        } else {
            remainingDelay = cpuDelay - (stepCycles >> 8);
        }
        cpuDelay = remainingDelay + ((static_cast<unsigned int>(videoCycles) + static_cast<unsigned int>(audioCycles)) >> 8);
        cycleRemainder = cpuCycles - stepCycles;
        if (operation - 1 > 1) break;
    } while (!(events & operation));
    return 0;
}

/**
 * @brief Read or write the machine state.
 * @param pBuffer The buffer, or nullptr to only measure.
 * @param isImport Whether the state is read from the buffer.
 * @return The size of the serialized state.
 */
unsigned long CHvc::_SerializeCore(void* buffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(buffer), reading);
    stream.Entry(mEntityData.mapper);
    stream.Entry(mEntityData.flags);
    stream.Entry(mEntityData.prgBankCount);
    stream.Entry(mEntityData.chrBankCount);
    for (unsigned long i = 0; i < 16; ++i) {
        stream.Entry(mEntityData.cpuBanks[i].readType);
        stream.Entry(mEntityData.cpuBanks[i]._01);
        stream.Entry(mEntityData.cpuBanks[i].readMask);
        stream.Entry(mEntityData.cpuBanks[i].readOffset);
        stream.Entry(mEntityData.cpuBanks[i].writeType);
        stream.Entry(mEntityData.cpuBanks[i]._11);
        stream.Entry(mEntityData.cpuBanks[i].writeMask);
        stream.Entry(mEntityData.cpuBanks[i].writeOffset);
    }
    for (unsigned long i = 0; i < 16; ++i) {
        stream.Entry(mEntityData.ppuBanks[i].readType);
        stream.Entry(mEntityData.ppuBanks[i]._01);
        stream.Entry(mEntityData.ppuBanks[i].readMask);
        stream.Entry(mEntityData.ppuBanks[i].readOffset);
        stream.Entry(mEntityData.ppuBanks[i].writeType);
        stream.Entry(mEntityData.ppuBanks[i]._11);
        stream.Entry(mEntityData.ppuBanks[i].writeMask);
        stream.Entry(mEntityData.ppuBanks[i].writeOffset);
    }
    for (unsigned long i = 0; i < 64; ++i) {
        stream.Entry(mEntityData.chrHistory[i].readType);
        stream.Entry(mEntityData.chrHistory[i]._01);
        stream.Entry(mEntityData.chrHistory[i].readMask);
        stream.Entry(mEntityData.chrHistory[i].readOffset);
        stream.Entry(mEntityData.chrHistory[i].writeType);
        stream.Entry(mEntityData.chrHistory[i]._11);
        stream.Entry(mEntityData.chrHistory[i].writeMask);
        stream.Entry(mEntityData.chrHistory[i].writeOffset);
    }
    stream.Entry(*reinterpret_cast<unsigned short*>(mEntityData._408 + 0x800));
    stream.Entry(*reinterpret_cast<unsigned short*>(mEntityData._408 + 0x802));
    stream.Entry(mEntityData.cycle.lineCycle);
    stream.Entry(mEntityData.cycle.frameCycle);
    stream.Entry(mEntityData.cycle.line);
    stream.Entry(mEntityData.cycle.frame);
    stream.Entry(mEntityData.cycle.sequenceMode);
    stream.Entry(mEntityData.cycle.sequenceIndex);
    stream.Entry(mEntityData.cycle.sequenceCycle);
    stream.Entry(reinterpret_cast<long&>(mEntityData.cycle._38));
    stream.Entry(mEntityData.order.words[0]);
    stream.Entry(mEntityData.order.words[1]);
    stream.Entry(mEntityData.order.words[2]);
    stream.Entry(mEntityData.order.words[3]);
    stream.Entry(mEntityData.order.words[4]);
    stream.Entry(mEntityData.order.words[5]);
    stream.Entry(mEntityData.order.words[6]);
    stream.Entry(mEntityData.order.words[7]);
    return stream.GetSize();
}

/**
 * @brief Write the machine state.
 * @param pBuffer The destination buffer.
 */
void CHvc::_SerializeCoreExport(void* pBuffer) {
    _SerializeCore(pBuffer, false);
}

/**
 * @brief Read the machine state.
 * @param pBuffer The source buffer.
 */
void CHvc::_SerializeCoreImport(const void* pBuffer) {
    _SerializeCore(const_cast<void*>(pBuffer), true);
}

/**
 * @brief Get the size of the serialized machine state including its tag.
 * @return The size.
 */
u64 CHvc::_SerializeCoreInferSize() {
    return static_cast<u32>(_SerializeCore(nullptr, false) + cSerializeTagSize + 31) & ~0xfu;
}

}  // namespace Vessel::Emulator::Virtual::PlatformHvc
