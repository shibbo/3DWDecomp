#pragma once

#include <prim/seadSafeString.h>

class GameDataHolder;

class ProductStageStartParam {
public:
    explicit ProductStageStartParam(GameDataHolder* pGameDataHolder);
    void init();
    const char* getStageDataName() const;
    int getWorldId() const;
    int getStageId() const;
    void setWorldId(int worldId);
    void setStageId(int stageId);
    void createCommandString(sead::BufferedSafeString* pCommand);

private:
    int mWorldId = 0;
    int mStageId = 0;
    GameDataHolder* mpGameDataHolder;
};

static_assert(sizeof(ProductStageStartParam) == 0x10);
