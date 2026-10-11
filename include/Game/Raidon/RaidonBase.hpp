#pragma once

#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Raidon/RaidonActor.hpp"

/// Interface shared by the rideable Plessie actors (land Plessie and surfing Plessie).
class RaidonBase : public RaidonActor {
public:
    /** @param pName Actor name. */
    RaidonBase(const char* pName) : RaidonActor(pName, nullptr) {}

    void startPuppetActionAll(const char* pActionName) override = 0;
    void setPuppetInputBlendAnimWeight() override = 0;
    void setInputBlendAnimWeight() override = 0;
    bool isOnGroundRaidon() const override = 0;
    bool isInWater() const override = 0;

    /** @return Whether Plessie stands on ground (water does not count here). */
    bool isOnGroundOrWaterRaidon() const override { return isOnGroundRaidon(); }

    virtual const sead::Vector3f& getDashBlurCenter() const = 0;
    virtual const sead::Vector3f& getBaseFrontDir() const = 0;
    virtual const sead::Vector3f& getGroundUpVec() const = 0;
    virtual const sead::Quatf& getBaseQuat() const = 0;
    virtual const sead::Vector3f& getGoalPosition() const = 0;
    virtual bool isEnableGoalPosition() const = 0;
    virtual f32 getHandle() const = 0;
    virtual f32 getAccel() const = 0;
    virtual f32 getRotateY() const = 0;
    virtual void updatePuppetInput() = 0;
    virtual void updateHandleAndAccel() = 0;
    virtual void updateGroundUpVec() = 0;
    virtual void updateOnGround() = 0;
    virtual void updateMatrialCode() = 0;
    virtual void updateStart() = 0;
    virtual void updateRide() = 0;
    virtual void clearGroundCount() = 0;
    virtual bool isAllGetOffPlayer() const = 0;
    virtual void startPuppetSe(const char* pName) = 0;
    virtual bool isNotChangeBgm() const = 0;

    /** @return Name of the floor material Plessie stands on; none by default. */
    virtual const char* getMaterialCode() { return nullptr; }

    /** @return Vertical stick input of the riding players; none by default. */
    virtual f32 getPuppetInputStickY() { return 0.0f; }
};
