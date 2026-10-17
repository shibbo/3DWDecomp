#pragma once

struct GhostPlayerUserInfo {
    const char* mFileName;
    const char* mName;
    bool mIsFlagged;
};

namespace rc {
const GhostPlayerUserInfo* findGhostPlayerUserInfo(const char* pFileName);
}
