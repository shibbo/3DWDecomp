#pragma once
#include "LuigiBros/controller.hpp"

class configSaveData {
public:
    unsigned int saveConfigHeader(unsigned char* pData);
    unsigned int loadConfigHeader(unsigned char* pData);
    void _getCombination(unsigned int player);
    bool getDRCDisp();
    unsigned int getDRCPlayer();

private:
    unsigned int mHeader;
    controller::COMBINATION mCombination[4];
};
static_assert(sizeof(configSaveData) == 0x14, "Configuration layout");
extern configSaveData m_configSaveData;
