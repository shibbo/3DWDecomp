#pragma once

/** @brief Identity and display information for a built-in ghost recording. */
struct GhostPlayerUserInfo {
    const char* id;
    const char* userName;
    bool isFriend;
};

namespace rc {
const GhostPlayerUserInfo* findGhostPlayerUserInfo(const char*);
const GhostPlayerUserInfo* tryFindGhostPlayerUserInfo(const char*);
}
