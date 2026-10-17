#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutActor;

class LayoutPartsActorKeeper {
public:
    LayoutPartsActorKeeper(s32 maxActors);
    ~LayoutPartsActorKeeper();

    void resisterPartsActor(LayoutActor* pActor);
    void appear();
    void calcAnim(bool isRecursive);
    void update();
    void kill();
    LayoutActor* getPartsActor(const char* pName) const;

    /** @brief Returns the registered parts array; use getPartsActorNum() for its length. */
    LayoutActor* const* getPartsActors() const { return mPartsActors; }

    s32 getPartsActorNum() const { return mNumActors; }

    LayoutActor* getPartsActor(s32 index) const {
        if (!mPartsActors || index >= mMaxActors) {
            return nullptr;
        }

        return mPartsActors[index];
    }

private:
    LayoutActor** mPartsActors = nullptr;
    s32 mNumActors = 0;
    s32 mMaxActors = 0;
};
}  // namespace al
