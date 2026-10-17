#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class StaffRollName : public al::LayoutActor {
public:
    explicit StaffRollName(const al::LayoutInitInfo& rInfo);

    void appear() override;
    void exeAppear();
    void exeWait();
    void exeEnd();
    void exeHide();
    bool isEnd() const;
    void setString(const char* pString);
    void setStringW(const char16_t* pString);
};
