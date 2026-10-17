#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class TestRouteDokanRenderMaterialIndirect : public al::LiveActor {
public:
    TestRouteDokanRenderMaterialIndirect(const char* pName);
    ~TestRouteDokanRenderMaterialIndirect() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
};
