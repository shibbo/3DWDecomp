#include "Scene/SingleModeDrawer3D.hpp"

SingleModeDrawer3D::SingleModeDrawer3D(al::GraphicsSystemInfo* pInfo)
    : al::ViewRenderer(pInfo) {
    mIsFastRendering = true;
}

SingleModeDrawer3D::~SingleModeDrawer3D() = default;
