#pragma once

namespace al {
class LiveActor;
}

/** @brief A linked collection of actors participating in a shared demo. */
class DemoActorGroup {
public:
    explicit DemoActorGroup(const char* pName);
    void addActor(al::LiveActor* pActor);

private:
    struct Entry {
        al::LiveActor* actor;
        Entry* next;
    };
    const char* mName;
    Entry* mActors;
};
