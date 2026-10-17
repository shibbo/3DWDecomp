#include "Scene/SceneDrawFunction.hpp"

#include <common/aglRenderBuffer.h>
#include "Library/Framework/GameFrameworkNx.hpp"
#include "System/Application.hpp"

namespace SceneDrawFunction {
agl::RenderTargetColor* getRenderTargetColor() {
    auto* framework = sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
    return (framework->mIsDocked ? framework->mDockedRenderBuffer : framework->mHandheldRenderBuffer)->getRenderTargetColor();
}

agl::RenderTargetColor* getRenderTargetColorDRC() {
    auto* framework = sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
    return (framework->mIsDocked ? framework->mDockedRenderBuffer : framework->mHandheldRenderBuffer)->getRenderTargetColor();
}

agl::RenderTargetDepth* getRenderTargetDepth() {
    auto* framework = sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
    return (framework->mIsDocked ? framework->mDockedRenderBuffer : framework->mHandheldRenderBuffer)->getRenderTargetDepth();
}

agl::RenderTargetDepth* getRenderTargetDepthDRC() {
    auto* framework = sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
    return (framework->mIsDocked ? framework->mDockedRenderBuffer : framework->mHandheldRenderBuffer)->getRenderTargetDepth();
}

agl::RenderBuffer* getRenderBuffer() {
    auto* framework = sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
    return (framework->mIsDocked ? framework->mDockedRenderBuffer : framework->mHandheldRenderBuffer);
}

agl::RenderBuffer* getRenderBufferDRC() {
    auto* framework = sead::DynamicCast<al::GameFrameworkNx>(Application::instance()->getFramework());
    return (framework->mIsDocked ? framework->mDockedRenderBuffer : framework->mHandheldRenderBuffer);
}
}
