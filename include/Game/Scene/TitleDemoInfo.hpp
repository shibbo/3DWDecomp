#pragma once

#include <basis/seadTypes.h>

class TitleDemoInfo {
public:
    TitleDemoInfo(s32 demoId, const s32* pPlayerIds, const char* pStageName);

    s32 mDemoId;
    s32 mPlayerIds[4];
    const char* mStageName;
};

static_assert(sizeof(TitleDemoInfo) == 0x20);
