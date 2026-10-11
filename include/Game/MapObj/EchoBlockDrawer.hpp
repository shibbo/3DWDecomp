#pragma once

#include <container/seadSafeArray.h>
#include <math/seadVector.h>

namespace al { class ShaderHolder; class UniformBlock; }
namespace nn::g3d { class ResShaderProgram; }
namespace sead { class LookAtCamera; class PerspectiveProjection; }

class EchoBlockDrawer {
public:
    EchoBlockDrawer();
    void init(al::ShaderHolder* shaderHolder);
    void draw(int viewIndex, const sead::LookAtCamera* camera,
              const sead::PerspectiveProjection* projection) const;

private:
    nn::g3d::ResShaderProgram* mShader;
    sead::Vector4f _8;
    sead::SafeArray<al::UniformBlock*, 2> mUniformBlocks;
    void* _28;
};
static_assert(sizeof(EchoBlockDrawer) == 0x30);
