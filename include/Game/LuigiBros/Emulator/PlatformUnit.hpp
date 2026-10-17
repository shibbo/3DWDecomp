#pragma once

namespace Vessel { namespace Emulator { namespace Virtual {

class CPlatformUnit {
public:
    union UArgument {
        unsigned long values[8];
        void* pointers[8];
    };

    /** @brief Initializes identity, timing, and status fields. */
    CPlatformUnit() : _08(0), _10(0), _18(0), mScale(0), _28(0) {
    }

    /** @brief Clears the base platform state. */
    virtual ~CPlatformUnit() {
        _08 = _10 = _18 = mScale = _28 = 0;
    }
    virtual unsigned int Initialize(CPlatformUnit* pParent) = 0;
    virtual unsigned int Finalize() = 0;
    virtual unsigned int Notify(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, void* pArguments) = 0;
    virtual unsigned int SystemAttach(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) = 0;
    virtual unsigned int SystemDetach(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) = 0;
    virtual unsigned int System(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) = 0;
    virtual unsigned int Access(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) = 0;
    virtual unsigned int Process(CPlatformUnit* pSender, unsigned long category,
        unsigned long operation, UArgument* pArguments) = 0;
    /**
     * @brief Reports unsupported content serialization for the base platform.
     * @param pBuffer Unused content buffer.
     * @param size Unused buffer size.
     * @param rSize Receives zero bytes processed.
     * @return Unsupported-operation code six.
     */
    virtual unsigned int ExportContent(void* pBuffer, unsigned long size, unsigned long& rSize) {
        rSize = 0;
        return 6;
    }
    /**
     * @brief Reports unsupported content serialization for the base platform.
     * @param pBuffer Unused content buffer.
     * @param size Unused buffer size.
     * @param rSize Receives zero bytes processed.
     * @return Unsupported-operation code six.
     */
    virtual unsigned int ImportContent(const void* pBuffer, unsigned long size, unsigned long& rSize) {
        rSize = 0;
        return 6;
    }
    /**
     * @brief Reports unsupported content sizing for the base platform.
     * @param rSize Receives zero bytes.
     * @return Unsupported-operation code six.
     */
    virtual unsigned int InferContentSize(unsigned long& rSize) {
        rSize = 0;
        return 6;
    }

    union {
        char mName[16];
        struct { unsigned long _08; unsigned long _10; };
    };
    unsigned long _18;
    unsigned long mScale;
    unsigned long _28;
};
static_assert(sizeof(CPlatformUnit) == 0x30, "Platform base layout");

} } }
