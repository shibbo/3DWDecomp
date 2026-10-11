#pragma once
#include <nn/os.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::atk {
class TaskProfileLogger;
namespace detail {
class Task {
public:
    /** @brief Life-cycle states stored in mState. */
    enum Status {
        Status_Free,
        Status_Append,
        Status_Execute,
        Status_Done,
        Status_Cancel,
    };

    Task();
    virtual ~Task();
    virtual void Execute(TaskProfileLogger& logger) = 0;
    /**
     * @brief Marks the task terminal and wakes callers waiting for it.
     * @param state Completion state: 3 for executed tasks or 4 for cancelled tasks.
     */
    void Complete(u32 state) {
        mState = state;
        nn::os::SignalEvent(&mCompletionEvent);
    }

    /** @brief Blocks until the task has executed or been cancelled. */
    void Wait() { nn::os::WaitEvent(&mCompletionEvent); }

    /** @brief Checks without blocking whether the task is finished. @return True when finished. */
    bool TryWait() { return nn::os::TryWaitEvent(&mCompletionEvent); }

    /** @brief Gets the life-cycle state. @return One of Status. */
    u32 GetStatus() const { return mState; }

    /**
     * @brief Sets the identifier TaskManager::CancelTaskById matches.
     * @param id Identifier, usually the owner's address.
     */
    void SetId(u32 id) { mId = id; }

    nn::util::IntrusiveListNode mNode;
    nn::os::EventType mCompletionEvent;
    u32 mState;
    u32 mId;
};
static_assert(sizeof(Task) == 0x48, "Task size");
}
}
