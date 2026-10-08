#pragma once

#include <basis/seadTypes.h>

class GameDataHolder;

/**
 * @brief Parameters of the stage the sequence starts next.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStageStartParam {
public:
    ProductStageStartParam(GameDataHolder* pHolder);

    void init();
    void setWorldId(s32 worldId);
    void setStageId(s32 stageId);

private:
    u8 _0[0x10];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStageStartParam) == 0x10);
