#pragma once

#include <basis/seadTypes.h>

namespace al {
class LiveActor;

class LiveActorGroup {
public:
    LiveActorGroup(const char*, s32);
    ~LiveActorGroup();

    virtual void registerActor(LiveActor*);

    s32 calcAliveActorNum() const;
    LiveActor* getDeadActor() const;
    LiveActor* tryFindDeadActor() const;
    void appearAll();
    void killAll();
    void makeActorAppearedAll();
    void makeActorDeadAll();

    inline LiveActor* getActor(s32 idx) const { return mActors[idx]; }
    inline s32 getActorCount() const { return mNumActors; }
    inline s32 getMaxActorCount() const { return mMaxActors; }

    const char* mGroupName;
    s32 mMaxActors;
    s32 mNumActors = 0;
    LiveActor** mActors;
};

template <class T>
class DeriveActorGroup : public LiveActorGroup {
public:
    DeriveActorGroup(const char* pName, s32 max) : LiveActorGroup(pName, max) {}

    T* getDeriveActor(s32 idx) const { return static_cast<T*>(getActor(idx)); }

    T* tryFindDeadDeriveActor() const { return static_cast<T*>(tryFindDeadActor()); }
};
}  // namespace al
