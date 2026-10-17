#include "Scene/TitleDemoInfo.hpp"

TitleDemoInfo::TitleDemoInfo(s32 demoId, const s32* pPlayerIds, const char* pStageName)
    : mDemoId(demoId), mStageName(pStageName) {
    for (s32 i = 0; i < 4; ++i) {
        mPlayerIds[i] = pPlayerIds[i];
    }
}
