#pragma once

#include <nn/gfx/gfx_Shader.h>
#include <nn/gfx/gfx_Types.h>

namespace nn::vfx::detail {

class ConstantBufferSlot {
public:
    void Initialize(gfx::Shader* pShader, const char* pName);

    int m_VertexSlot;
    int m_PixelSlot;
};

} // namespace nn::vfx::detail
