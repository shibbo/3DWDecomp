#include "Player/FurActor.hpp"
#include "Library/ActorUtil.hpp"
#include "Player/FurKeeper.hpp"

/**
 * Creates the fur actor and attaches a fur keeper for the given parent actor.
 * @param pParent the actor the fur grows on
 * @param rInfo the actor init info
 * @param pFurName the name of the fur setting to use
 */
FurActor::FurActor(al::LiveActor* pParent, const al::ActorInitInfo& rInfo, const char* pFurName)
    : al::LiveActor("FurActor") {
    al::initActorWithArchiveName(this, rInfo, "FurActor", nullptr);
    mFurKeeper = new FurKeeper();
    mFurKeeper->init(pParent, pFurName);
    makeActorAppeared();
}

/**
 * Does nothing; the fur follows its parent actor.
 */
void FurActor::movement() {}

/**
 * Updates the fur's uniform buffers.
 */
void FurActor::calcAnim() {
    mFurKeeper->updateUbo();
}

/**
 * Draws the fur.
 */
void FurActor::draw() const {
    mFurKeeper->draw();
}
