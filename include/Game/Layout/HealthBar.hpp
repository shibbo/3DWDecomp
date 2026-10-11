#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace rc {
/** @brief Boss health bar layout. */
class HealthBar : public al::LayoutActor {
public:
    void startAppear(bool isInstant);
    void show();
    void hide();
    void end();
    void setEndAfterDamage();
};
}  // namespace rc
