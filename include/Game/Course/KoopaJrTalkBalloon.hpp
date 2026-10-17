#pragma once

#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"

class KoopaJrTalkBalloon : public al::SimpleLayoutAppearWaitEnd {
public:
    KoopaJrTalkBalloon(const char* pName, const char* pLayoutName,
                       const al::LayoutInitInfo& rInfo);

    void exeAppear() override;
    void exeWait() override;
};
