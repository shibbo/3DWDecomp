#pragma once

#include <container/seadPtrArray.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>
#include "Library/Execute/IUseExecutor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class IUseSceneObjHolder;
class LiveActor;
struct ActorInitInfo;
class UniformBlock;
}

/**
 * @brief Scene object holding the echo emitters and their uniform blocks.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class EchoEmitterHolder : public al::IUseExecutor, public al::ISceneObj {
public:
    using UniformBlockArray = sead::PtrArray<al::UniformBlock>;

    void execute() override;
    const char* getSceneObjName() const override;
    void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) override;
    void setToUbo(s32 viewIndex, const sead::Matrix34f& rViewMtx,
                  const sead::Matrix44f& rProjMtx) const;

    /**
     * Gets the uniform blocks of the emitters, one per view.
     * @return The uniform blocks.
     */
    const UniformBlockArray* getUboArray() const { return &mUboArray; }

private:
    UniformBlockArray mUboArray;  // 0x10
};

namespace rc {
EchoEmitterHolder* tryGetEmitterHolder(const al::IUseSceneObjHolder* pHolder);
void initEchoEmitterHolder(const al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
bool emitEcho(const al::LiveActor* pActor, const sead::Vector3f& rPos, f32 radius, s32 frame,
              bool isForce);
void emitKeepEcho(const al::LiveActor* pActor, const sead::Vector3f& rPos, f32 radius, s32 frame);
void killAllEcho(const al::LiveActor* pActor);
}
