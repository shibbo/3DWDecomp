#pragma once

class controller {
public:
    enum TYPE : unsigned int { TYPE_DRC = 0, TYPE_UNKNOWN = 10, TYPE_NONE = 11 };
    enum BUTTON : unsigned int {};
    enum ANALOGSTICK : unsigned int { STICK_LEFT, STICK_RIGHT };
    enum COMBINATION : unsigned int {};
};
