#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
}  // namespace al

class CourseSelectScene;

/**
 * @brief Holds the message windows shown on the course select map.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class CourseSelectWindowHolder {
public:
    explicit CourseSelectWindowHolder(const CourseSelectScene* pScene);

    virtual void init(const al::LayoutInitInfo& rInfo);

    void startFadeWhite(s32 frame);
    void endFadeWhite(s32 frame);
    bool isCloseFadeWhite() const;
    void appearMessage(const char* pLabel);
    bool isMessageActive() const;

private:
    u8 _8[0x40 - 0x8];
};

static_assert(sizeof(CourseSelectWindowHolder) == 0x40);
