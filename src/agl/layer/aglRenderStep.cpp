#include "layer/aglRenderStep.h"

#include "detail/aglRootNode.h"
#include "layer/aglDrawMethod.h"

namespace agl::lyr {

/**
 * @brief Constructs an enabled render step without draw methods.
 */
RenderStep::RenderStep() : mFlag(1) { detail::RootNode::setNodeMeta(this, "Icon = IN"); }

/**
 * @brief Sorts the draw methods by priority.
 */
void RenderStep::calc() {
    if (mDrawMethod.size() < 2) {
        return;
    }
    mDrawMethod.shakerSort_<DrawMethod>(DrawMethod::compare);
}

/**
 * @brief Adds a draw method if it is not registered yet.
 * @param pMethod draw method to add
 * @return false if the draw method was already registered
 */
bool RenderStep::pushBack(DrawMethod* pMethod) {
    for (auto& rMethod : mDrawMethod) {
        if (&rMethod == pMethod) {
            return false;
        }
    }

    mDrawMethod.pushBack(pMethod);
    return true;
}

/**
 * @brief Removes every draw method bound to an object.
 * @param pObject object the draw methods are bound to
 * @return number of removed draw methods
 */
s32 RenderStep::removeByObject(const void* pObject) {
    s32 count = 0;
    s32 index = 0;

    for (auto it = mDrawMethod.begin(); it != mDrawMethod.end();) {
        if (it->getObject() == pObject) {
            mDrawMethod.erase(index);
            count++;
        } else {
            ++index;
            ++it;
        }
    }

    return count;
}

/**
 * @brief Removes a draw method.
 * @param pMethod draw method to remove
 * @return number of removed draw methods
 */
s32 RenderStep::remove(const DrawMethod* pMethod) {
    s32 count = 0;
    s32 index = 0;

    for (auto it = mDrawMethod.begin(); it != mDrawMethod.end();) {
        if (&*it == pMethod) {
            mDrawMethod.erase(index);
            count++;
        } else {
            ++index;
            ++it;
        }
    }

    return count;
}

/**
 * @brief Removes all draw methods.
 */
void RenderStep::clear() { mDrawMethod.clear(); }

/**
 * @brief Generates the host I/O messages of every draw method.
 * @param pContext host I/O context
 */
void RenderStep::genMessage(sead::hostio::Context* pContext) {
    if (mDrawMethod.size() < 1) {
        return;
    }
    for (auto& rMethod : mDrawMethod) {
        rMethod.genMessage(pContext);
    }
}

/**
 * @brief Handles a host I/O property event (no-op in release builds).
 * @param pEvent Property event; unused in this release build.
 */
void RenderStep::listenPropertyEvent(const sead::hostio::PropertyEvent* pEvent) {}

/**
 * @brief Handles a host I/O node event (no-op in release builds).
 * @param pEvent Node event; unused in this release build.
 */
void RenderStep::listenNodeEvent(const sead::hostio::NodeEvent* pEvent) {}

} // namespace agl::lyr
