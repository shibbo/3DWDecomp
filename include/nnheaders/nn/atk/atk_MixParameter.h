#pragma once

#include <nn/atk/atk_Global.h>

namespace nn::atk {
enum PanMode {
    PanMode_Dual,
    PanMode_Balance,
    PanMode_Invalid,
};

enum PanCurve {
    PanCurve_Sqrt,
    PanCurve_Sqrt0Db,
    PanCurve_Sqrt0DbClamp,
    PanCurve_SinCos,
    PanCurve_SinCos0Db,
    PanCurve_SinCos0DbClamp,
    PanCurve_Linear,
    PanCurve_Linear0Db,
    PanCurve_Linear0DbClamp,
    PanCurve_Invalid,
};

enum MixMode {
    MixMode_Pan,
    MixMode_MixParameter,
};

/** @brief Gains of one source channel into every output channel. */
struct MixParameter {
    f32 ch[ChannelIndex_Count];
};
}  // namespace nn::atk
