#include "Enemy/PipePackunWaterSurfaceModel.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
NERVE_ACTION_IMPL(PipePackunWaterSurfaceModel, Wait)
NERVE_ACTION_IMPL(PipePackunWaterSurfaceModel, Move)
NERVE_ACTION_IMPL(PipePackunWaterSurfaceModel, End)
NERVE_ACTIONS_MAKE_STRUCT(PipePackunWaterSurfaceModel, Wait, Move, End)
}

/** @brief Creates the water-surface ripple model. @param pName Actor name. */
PipePackunWaterSurfaceModel::PipePackunWaterSurfaceModel(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the model and its fixed surface transform. @param rInfo Placement information. */
void PipePackunWaterSurfaceModel::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Wait", &NrvPipePackunWaterSurfaceModel.collector, 0);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::tryGetMatrixTR(&mBaseMtx, rInfo);
    makeActorDead();
}

/** @brief Sets the water-surface coordinate. @param coord Surface position along the stem. */
void PipePackunWaterSurfaceModel::setSurfaceCoord(f32 coord) {
    mSurfaceCoord = coord;
}

/** @brief Updates ripple visibility and animation from the stem position. @param coord Stem coordinate. */
void PipePackunWaterSurfaceModel::setCoord(f32 coord) {
    f32 change = coord - mCoord;
    mCoord = coord;
    s32 moveSpeed = static_cast<s32>(change > 0.0f ? change : -change);
    bool visible = coord + 100.0f >= mSurfaceCoord;
    bool dead = al::isDead(this);
    if (!visible) {
        if (!dead) {
            al::startNerveAction(this, "End");
            makeActorDead();
        }
        return;
    }
    if (dead)
        makeActorAppeared();
    if (moveSpeed >= 3) {
        if (!al::isNerve(this, NrvPipePackunWaterSurfaceModel.Move.data()))
            al::startNerveAction(this, "Move");
    } else if (!al::isNerve(this, NrvPipePackunWaterSurfaceModel.Wait.data())) {
        al::startNerveAction(this, "Wait");
    }
}

/** @brief Keeps the idle ripple action active. */
void PipePackunWaterSurfaceModel::exeWait() {}

/** @brief Keeps the moving ripple action active. */
void PipePackunWaterSurfaceModel::exeMove() {}

/** @brief Ends the ripple action. */
void PipePackunWaterSurfaceModel::exeEnd() {}

/** @brief Gets the fixed water-surface transform. @return Model transform. */
const sead::Matrix34f* PipePackunWaterSurfaceModel::getBaseMtx() const {
    return &mBaseMtx;
}
