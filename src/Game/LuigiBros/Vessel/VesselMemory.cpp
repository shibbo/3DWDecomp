#include "LuigiBros/Vessel/VesselMemory.hpp"
#include <basis/seadNew.h>

#include "LuigiBros/Emulator/cafe.hpp"

/**
 * @brief Allocates an aligned byte buffer.
 * @param size Number of bytes to allocate.
 * @param alignment Required allocation alignment.
 * @return Allocated buffer.
 */
void* VESSEL_malloc(unsigned long size, int alignment) {
    return new (alignment) unsigned char[size];
}

/** @brief Releases an allocated buffer. @param pMemory Buffer to release, or nullptr. */
void VESSEL_free(void* pMemory) {
    delete static_cast<unsigned char*>(pMemory);
}

/** @brief Obtains the platform stack marker. @return Platform stack marker. */
unsigned int VESSE_STACKINIT() {
    return OSGetStackPointer();
}

/**
 * @brief Platform stub for stack diagnostics.
 * @param pFile Unused source filename.
 * @param line Unused source line number.
 * @param pLabel Unused diagnostic label.
 */
void VESSEL_STACKDISP(const char* pFile, unsigned int line, const char* pLabel) {}
