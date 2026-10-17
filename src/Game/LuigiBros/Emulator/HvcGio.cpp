#include "LuigiBros/Emulator/HvcGio.hpp"
#include "LuigiBros/Common/Serialize.hpp"
#include <cstring>

namespace Vessel { namespace Emulator { namespace Virtual { namespace PlatformHvc {
/** @brief Initializes platform identity and lifecycle state. */
CHvcGio::CHvcGio() {
    _18 = 2;
    mScale = 1;
    *reinterpret_cast<unsigned char*>(&_28) = 0;
    memset(mName, ' ', 15);
    strncpy(mName, "HVC GIO        ", 15);

}
/** @brief Destroys the I/O unit and clears inherited platform state. */
CHvcGio::~CHvcGio() {}
/** @brief Resets controller shifts and frame flags. @return Zero. */
unsigned int CHvcGio::_Prologue() {
    memset(mReadIndex, 0, sizeof(mReadIndex));
    memset(mPorts, 0, sizeof(mPorts));
    mFrameFlags[0] = 0x40;
    memset(mFrameFlags + 1, 0, sizeof(mFrameFlags) - 1);
    return 0;
}
/** @brief Provides the empty shutdown hook. @return Zero. */
unsigned int CHvcGio::_Epilogue() { return 0; }

/**
 * @brief Reads controller bits or writes controller and frame-control registers.
 * @param pSender Unused sending unit.
 * @param category Unused access category.
 * @param operation One for writing, otherwise reading.
 * @param pArguments Register address and byte-buffer arguments.
 * @return Zero.
 */
unsigned int CHvcGio::Access(CPlatformUnit* pSender, unsigned long category,
    unsigned long operation, UArgument* pArguments) {
    unsigned char* pMemory = mpEntity->pMemory;
    unsigned long address = pArguments->values[2];
    if (operation == 1) {
        switch (address) {
        case 0x4016: {
            unsigned char previous = pMemory[0x303a26];
            unsigned char value = *static_cast<unsigned char*>(pArguments->pointers[4]);
            pMemory[0x303a26] = value;
            if ((previous & 1) && !(value & 1)) {
                mReadIndex[0] = mReadIndex[1] = 0;
            }
            break;
        }
        case 0x4017: {
            pMemory[0x303a27] = *static_cast<unsigned char*>(pArguments->pointers[4]);
            mFrameFlags[0] = pMemory[0x303a27] & 0x40;
            mFrameFlags[4] = pMemory[0x303a27] & 0x80;
            mpEntity->cycle.ChangeSequenceMode(mFrameFlags[4]);
            break;
        }
        }
    } else {
        switch (address) {
        case 0x4016: {
            unsigned long mask = 1 << mReadIndex[0];
            pMemory[0x303826] = 0x40;
            pMemory[0x303826] |= (mPorts[0] & mask ? 1 : 0) << 0;
            pMemory[0x303826] |= (mPorts[2] & mask ? 1 : 0) << 1;
            pMemory[0x303826] |= (mPorts[7] & mask ? 1 : 0) << 2;
            pMemory[0x303826] |= (mPorts[8] & mask ? 1 : 0) << 4;
            *static_cast<unsigned char*>(pArguments->pointers[4]) = pMemory[0x303826];
            ++mReadIndex[0];
            break;
        }
        case 0x4017: {
            unsigned long mask = 1 << mReadIndex[1];
            pMemory[0x303827] = 0x40;
            pMemory[0x303827] |= (mPorts[1] & mask ? 1 : 0) << 0;
            pMemory[0x303827] |= (mPorts[3] & mask ? 1 : 0) << 1;
            pMemory[0x303827] |= (mPorts[4] & mask ? 1 : 0) << 2;
            pMemory[0x303827] |= (mPorts[5] & mask ? 1 : 0) << 3;
            pMemory[0x303827] |= (mPorts[6] & mask ? 1 : 0) << 4;
            *static_cast<unsigned char*>(pArguments->pointers[4]) = pMemory[0x303827];
            ++mReadIndex[1];
            break;
        }
        }
    }
    return 0;
}

/**
 * @brief Applies reset orders, raises frame interrupts, and latches controller input.
 * @param pSender Unused sending unit.
 * @param category Unused processing category.
 * @param operation Unused operation identifier.
 * @param pArguments Optional timing and controller-input arguments.
 * @return Zero.
 */
unsigned int CHvcGio::Process(CPlatformUnit* pSender, unsigned long category,
    unsigned long operation, UArgument* pArguments) {
    if (mpEntity->order.words[0] & (1 << _18)) {
        mReadIndex[0] = mReadIndex[1] = 0;
        mpEntity->order.words[0] &= ~(1 << _18);
    }
    if (pArguments) {
        if (!(mFrameFlags[0] & 0x40) && !(mFrameFlags[4] & 0x80) &&
            (pArguments->values[0] & mpEntity->cycle._a50)) {
            mpEntity->pMemory[0x303825] |= 0x40;
            mpEntity->order.words[4] |= 1;
        }
        unsigned long* pInput = static_cast<unsigned long*>(pArguments->pointers[4]);
        if (pInput) {
            mPorts[0] = pInput[0];
            mPorts[1] = pInput[1];
            mPorts[2] = pInput[2];
            mPorts[3] = pInput[3];
            mPorts[7] = pInput[7];
            mPorts[8] = pInput[8];
        }
    }
    return 0;
}

/**
 * @brief Serializes controller and frame-control state in little-endian order.
 * @param pBuffer Save-state buffer, or null to measure the state.
 * @param reading True to restore state.
 * @return Number of bytes processed.
 */
unsigned long CHvcGio::_SerializeCore(void* pBuffer, bool reading) {
    Common::Serialize::LittleEndian stream(static_cast<unsigned char*>(pBuffer), reading);
    stream.Entry(mReadIndex[0]);
    stream.Entry(mReadIndex[1]);
    stream.Entry(mPorts[0]);
    stream.Entry(mPorts[1]);
    stream.Entry(mPorts[2]);
    stream.Entry(mPorts[3]);
    stream.Entry(mPorts[4]);
    stream.Entry(mPorts[5]);
    stream.Entry(mPorts[6]);
    stream.Entry(mPorts[7]);
    stream.Entry(mPorts[8]);
    stream.Entry(mPorts[9]);
    stream.Entry(mPorts[10]);
    stream.Entry(mPorts[11]);
    stream.Entry(mFrameFlags[0]);
    stream.Entry(mFrameFlags[1]);
    stream.Entry(mFrameFlags[2]);
    stream.Entry(mFrameFlags[3]);
    stream.Entry(mFrameFlags[4]);
    stream.Entry(mFrameFlags[5]);
    stream.Entry(mFrameFlags[6]);
    stream.Entry(mFrameFlags[7]);
    return stream.GetSize();
}
/** @brief Exports core state. @param pBuffer Output buffer. */
void CHvcGio::_SerializeCoreExport(void* pBuffer) { _SerializeCore(pBuffer, false); }
/** @brief Imports core state. @param pBuffer Input buffer. */
void CHvcGio::_SerializeCoreImport(const void* pBuffer) { _SerializeCore(const_cast<void*>(pBuffer), true); }
/** @brief Measures aligned state storage including its tag. @return Storage size. */
unsigned long CHvcGio::_SerializeCoreInferSize() { return (unsigned(_SerializeCore(nullptr, false)) + 31) & ~15u; }
/** @brief Returns the I/O content identifier. @return Two. */
unsigned char CHvcGio::_SerializeTagContent() { return 2; }
/** @brief Returns the state species. @return Zero. */
unsigned char CHvcGio::_SerializeTagSpecies() { return 0; }
/** @brief Returns the state format version. @return Version 0x100. */
unsigned short CHvcGio::_SerializeTagVersion() { return 0x100; }
/** @brief Returns the state tag comment. @return I/O identifier text. */
const unsigned char* CHvcGio::_SerializeTagComment() { return reinterpret_cast<const unsigned char*>("GIO    "); }
} } } }
