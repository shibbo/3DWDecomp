#pragma once
#include <container/seadPtrArray.h>
#include "Layout/ButtonCursorParts.hpp"
namespace al { class LayoutInitInfo; }
class CursorTarget;

// The trailing fields are not yet reconstructed; allocation size is verified
// against callers of the original constructor.
class ButtonGroup {
public:
    ButtonGroup(const al::LayoutInitInfo& rInfo, al::LayoutActor* pParent,
                const char* pName, const char* pSuffix, bool isLocalized);
    void reset();
    void hideCursor();
    void showCursor();
    void invalidate();
    void validate();
    void select(CursorTarget* pTarget);
    void updateAndCursorDefault(s32 port);
    bool isDecideAny() const;
    bool isDecideEndAny() const;
    s32 getDecideEndButtonIndex() const;
    ButtonCursorParts* getCursor() const { return mpCursor; }
    CursorTarget* getFirstTarget() const {
        return mTargets.isEmpty() ? nullptr : mTargets[0];
    }
private:
    ButtonCursorParts* mpCursor;
    sead::PtrArray<CursorTarget> mTargets;
    u8 mUnknown[0x238];
};
static_assert(sizeof(ButtonGroup) == 0x250);
