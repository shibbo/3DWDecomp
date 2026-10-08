#pragma once

#include <nn/types.h>

namespace Vessel::Emulator::Virtual {

/**
 * @brief A component of an emulated platform (CPU, PPU, sound unit, ...).
 *
 * Units talk to each other through System, Access and Process calls that carry a UArgument.
 */
class CPlatformUnit {
public:
    /// Operations passed to Access.
    enum Operation {
        cOperation_Read = 0,
        cOperation_Write = 1,
    };

    /// Results returned by System, Access and Process.
    enum Result {
        cResult_Success = 0,
        cResult_Unhandled = 4,
        cResult_Unsupported = 6,
    };

    /**
     * @brief Payload of a System, Access or Process call; which view is valid depends on the call.
     */
    union UArgument {
        /// Access: a bus transfer.
        struct {
            u64 mSize;
            u64 mCount;
            u64 mAddress;
            u64 mReserved;
            void* mData;
        } mAccess;

        /// Process: the events that happened since the last call.
        struct {
            u64 mEvents;
            u64 mClock;
            u64* mCycles;
            u64 mReserved;
            void* mInput;
        } mProcess;

        /// System: a command with a value.
        struct {
            u64 mCommand;
            u64 mCount;
            u64 mValue;
            u64 mReserved;
        } mSystem;

        u64 mRaw[8];
    };

    /**
     * @brief Identification data of a unit.
     */
    struct SDescriptor {
        char mName[16];
        u64 mIndex;
        u64 mFlags;
        u8 mKind;
    };

    /**
     * @brief Construct a unit with a cleared descriptor.
     * @param index The unit index.
     * @param flags The unit flags.
     */
    CPlatformUnit(u64 index, u64 flags) {
        mDescriptor = {};
        mDescriptor.mIndex = index;
        mDescriptor.mFlags = flags;
    }

    /// Destroy the unit, clearing its descriptor.
    virtual ~CPlatformUnit() { mDescriptor = {}; }

    virtual int Initialize(CPlatformUnit* pParent) = 0;
    virtual int Finalize() = 0;
    virtual int Notify(CPlatformUnit* pUnit, u64 channel, u64 operation, void* pArgument) = 0;
    virtual int SystemAttach(CPlatformUnit* pUnit, u64 channel, u64 operation,
                             UArgument* pArgument) = 0;
    virtual int SystemDetach(CPlatformUnit* pUnit, u64 channel, u64 operation,
                             UArgument* pArgument) = 0;
    virtual int System(CPlatformUnit* pUnit, u64 channel, u64 operation,
                       UArgument* pArgument) = 0;
    virtual int Access(CPlatformUnit* pUnit, u64 channel, u64 operation,
                       UArgument* pArgument) = 0;
    virtual int Process(CPlatformUnit* pUnit, u64 channel, u64 operation,
                        UArgument* pArgument) = 0;

    /**
     * @brief Export the state of the unit (not supported by default).
     * @param pBuffer The destination buffer.
     * @param bufferSize The size of the destination buffer.
     * @param rSize Receives the number of bytes written.
     * @return cResult_Unsupported.
     */
    virtual int ExportContent(void* pBuffer, u64 bufferSize, u64& rSize) {
        rSize = 0;
        return cResult_Unsupported;
    }

    /**
     * @brief Import the state of the unit (not supported by default).
     * @param pBuffer The source buffer.
     * @param bufferSize The size of the source buffer.
     * @param rSize Receives the number of bytes read.
     * @return cResult_Unsupported.
     */
    virtual int ImportContent(const void* pBuffer, u64 bufferSize, u64& rSize) {
        rSize = 0;
        return cResult_Unsupported;
    }

    /**
     * @brief Compute the size of the exported state (not supported by default).
     * @param rSize Receives the size.
     * @return cResult_Unsupported.
     */
    virtual int InferContentSize(u64& rSize) {
        rSize = 0;
        return cResult_Unsupported;
    }

    SDescriptor mDescriptor;
};

static_assert(sizeof(CPlatformUnit) == 0x30, "CPlatformUnit size");

}  // namespace Vessel::Emulator::Virtual
