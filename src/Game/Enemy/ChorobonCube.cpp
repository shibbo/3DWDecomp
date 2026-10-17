#include "Enemy/ChorobonCube.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_DECL(ChorobonCube, Follow)
NERVES_MAKE_NOSTRUCT(ChorobonCube, Follow)
}

/** @brief Constructs the Fuzzy cube actor.
 * @param pName Actor name.
 */
ChorobonCube::ChorobonCube(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the cube as a map part and activates its follow state.
 * @param rInfo Actor placement and scene information.
 */
void ChorobonCube::init(const al::ActorInitInfo& rInfo) {
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvChorobonCubeFollow, 0);
    makeActorAppeared();
}

/** @brief Leaves movement to the cube's external controller. */
void ChorobonCube::exeFollow() {}
