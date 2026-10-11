#include "Demo/DemoActorGroup.hpp"

/**
 * @brief Creates an empty demo actor group.
 * @param pName Unused group name; the stored name remains null in this version.
 */
DemoActorGroup::DemoActorGroup(const char* pName) : mName(nullptr), mActors(nullptr) {}

/**
 * @brief Prepends an actor to the group's linked list.
 * @param pActor Actor to register; duplicate entries are not filtered.
 */
void DemoActorGroup::addActor(al::LiveActor* pActor) {
    mActors = new Entry{pActor, mActors};
}
