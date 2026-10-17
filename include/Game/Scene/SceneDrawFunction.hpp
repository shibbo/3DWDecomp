#pragma once

namespace agl {
class RenderBuffer;
class RenderTargetColor;
class RenderTargetDepth;
}

namespace SceneDrawFunction {
agl::RenderTargetColor* getRenderTargetColor();
agl::RenderTargetColor* getRenderTargetColorDRC();
agl::RenderTargetDepth* getRenderTargetDepth();
agl::RenderTargetDepth* getRenderTargetDepthDRC();
agl::RenderBuffer* getRenderBuffer();
agl::RenderBuffer* getRenderBufferDRC();
}
