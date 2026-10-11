#pragma once

#include "heap/seadHeap.h"

namespace sead
{
class FrameHeap : public Heap
{
    SEAD_RTTI_OVERRIDE(FrameHeap, Heap)
    friend class PrintFormatter;

public:
    struct State
    {
        void* mHeadPtr;
        void* mTailPtr;
    };

    static FrameHeap* tryCreate(size_t size, const SafeString& rName, Heap* pParent,
                                s32 alignment = sizeof(void*),
                                HeapDirection direction = cHeapDirection_Forward,
                                bool enableLock = false);
    static FrameHeap* create(size_t size, const SafeString& rName, Heap* pParent,
                             s32 alignment = sizeof(void*),
                             HeapDirection direction = cHeapDirection_Forward,
                             bool enableLock = false);

    static size_t getManagementAreaSize(s32 alignment);

    void restoreState(const State& rState);
    const State& getState() const { return mState; }
    void freeHead();
    void freeTail();

    void destroy() override;
    size_t adjust() override;
    void* tryAlloc(size_t size, s32 alignment) override;
    void free(void* pPtr) override;
    void* resizeFront(void* pPtr, size_t size) override;
    void* resizeBack(void* pPtr, size_t size) override;
    void freeAll() override;
    uintptr_t getStartAddress() const override;
    uintptr_t getEndAddress() const override;
    size_t getSize() const override;
    size_t getFreeSize() const override;
    size_t getMaxAllocatableSize(int alignment) const override;
    bool isInclude(const void* pPtr) const override;
    bool isEmpty() const override;
    bool isFreeable() const override;
    bool isResizable() const override;
    bool isAdjustable() const override;
    void dump() const override;
    void dumpYAML(WriteStream& rStream, int indent) const override;
    void genInformation_(hostio::Context* pContext) override;

protected:
    FrameHeap(const SafeString& rName, Heap* pParent, void* pAddress, size_t size,
              HeapDirection direction, bool enableLock);
    ~FrameHeap() override;

    void initialize_();
    void* getAreaStart_() const;
    void* getAreaEnd_() const;
    size_t adjustBack_();
    size_t adjustFront_();

    State mState;
};
}  // namespace sead
