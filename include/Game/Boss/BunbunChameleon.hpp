#pragma once
#include "Library/LiveActor/LiveActor.hpp"
struct RenderMaterialIndirectParam;
/** @brief Controls Bunbun's distance-based chameleon material and shadows. */
class BunbunChameleon : public al::LiveActor {
public:
    explicit BunbunChameleon(const char* pName);
    ~BunbunChameleon() override;
    void control() override;
private:
    RenderMaterialIndirectParam* mParam;
};
static_assert(sizeof(BunbunChameleon) == 0x150);
