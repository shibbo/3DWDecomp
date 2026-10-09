#pragma once

#include <nn/ui2d/ui2d_Pane.h>

namespace eui {
/** @brief Empty grouping pane; only its runtime type is used by the alignment code. */
class NullPane : public nn::ui2d::Pane {
public:
    NN_RUNTIME_TYPEINFO(nn::ui2d::Pane);
};
}  // namespace eui
