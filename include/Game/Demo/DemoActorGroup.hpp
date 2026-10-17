#pragma once

namespace al {
class LiveActor;
}

/** @brief A linked collection of actors participating in a shared demo. */
class DemoActorGroup {
public:
    explicit DemoActorGroup(const char* pName = nullptr);
    void addActor(al::LiveActor* pActor);

private:
    friend class ProjectDemoDirector;
    struct Entry {
        al::LiveActor* actor;
        Entry* next;
    };
    const char* mName;
    Entry* mActors;
};
