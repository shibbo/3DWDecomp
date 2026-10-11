#pragma once

#include "Project/AreaObj/AreaObj.hpp"

/**
 * @brief Area in which Bowser's fireballs are extinguished.
 * @note Only the members used by reconstructed code are declared.
 */
class FireBallSafeArea : public al::AreaObj {
public:
    bool isIgnoreGiantFireballs();
};
