#pragma once

#include <basis/seadTypes.h>

#include "Library/Scene/ISceneObj.hpp"

namespace rc {
class StampDirector;
}  // namespace rc

class DrcAssistDirector;
class PlayerAliveWatcher;

namespace al {
class IUseSceneObjHolder;
class PlayerHolder;
}  // namespace al

/**
 * @brief Scene object holding the touch-screen (DRC) assist director of every pad port.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class DrcAssistDirectorList : public al::ISceneObj {
public:
    explicit DrcAssistDirectorList(s32 maxNum);

    void addTouchAssist(s32 port, bool isEnable);
    void setEnable(bool isEnable);
    DrcAssistDirector* getDrcAssist(s32 port);
    void setStampDirector(rc::StampDirector* pStampDirector);
    void setKinopioBrigadeFlag(bool isKinopioBrigade);
    void setPlayerHolder(al::PlayerHolder* pPlayerHolder);
    void setPlayerAliveWatcher(PlayerAliveWatcher* pPlayerAliveWatcher);
    void disappearTouchPointer();
    void disappearTouchPointerEffect();
    void disappearTouchPointerImmediately();

private:
    u8 _8[0x40 - 0x8];
};

static_assert(sizeof(DrcAssistDirectorList) == 0x40);

namespace rc {
void updateDrcAssistDirector(const al::IUseSceneObjHolder* pHolder);
}  // namespace rc
