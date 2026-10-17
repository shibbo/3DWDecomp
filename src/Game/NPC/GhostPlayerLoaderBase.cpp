#include "NPC/GhostPlayerLoaderBase.hpp"

/**
 * @brief Stores the ghost loader's name.
 * @param pName Loader name, retained without copying.
 */
GhostPlayerLoaderBase::GhostPlayerLoaderBase(const char* pName) : mName(pName) {}

/** @brief Reports the default buffer-allocation requirement. @return True. */
bool GhostPlayerLoaderBase::needAlloc() { return true; }

/**
 * @brief Provides the base loader's empty filename-extraction implementation.
 * @param pFileName Output buffer, left unchanged.
 * @param index Recording index; unused by the base implementation.
 */
void GhostPlayerLoaderBase::extractFileName(sead::BufferedSafeString* pFileName, int index) const {}
