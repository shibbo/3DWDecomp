#pragma once

#include "Library/Draw/ViewRenderer.hpp"

class SingleModeDrawer3D : public al::ViewRenderer {
public:
    explicit SingleModeDrawer3D(al::GraphicsSystemInfo* pInfo);
    ~SingleModeDrawer3D() override;
};
