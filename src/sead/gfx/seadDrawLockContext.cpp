#include "gfx/seadDrawLockContext.h"

namespace sead
{
/** @brief Constructs the drawing lock and its critical section. */
DrawLockContext::DrawLockContext() = default;

/**
 * @brief Performs platform-specific lock initialization, which is unnecessary here.
 * @param pHeap Allocator supplied by the caller; unused on this platform.
 */
void DrawLockContext::initialize(Heap* pHeap) {}

/**
 * @brief Locks the critical section guarding draw operations.
 */
void DrawLockContext::lock()
{
    mCriticalSection.lock();
}

/**
 * @brief Releases the critical section after draw operations.
 */
void DrawLockContext::unlock()
{
    mCriticalSection.unlock();
}

/**
 * @brief Generates host IO messages, which are omitted in release builds.
 * @param pContext Host IO message context; unused in this release implementation.
 */
void DrawLockContext::genMessage(hostio::Context* pContext) {}

}  // namespace sead
