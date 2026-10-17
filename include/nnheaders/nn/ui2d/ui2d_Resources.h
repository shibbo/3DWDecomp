#pragma once

#include "nn/ui2d/ui2d_Types.h"
#include "nn/util/util_MathTypes.h"

namespace nn {
namespace ui2d {
/** @brief Common header of every pane block in a layout resource. */
struct ResPane {
    u32 signature;
    u32 blockSize;
    u8 flag;
    u8 basePosition;
    u8 alpha;
    u8 flagEx;
    char name[24];
    char userData[8];
    nn::util::Float3 translate;
    nn::util::Float3 rotate;
    nn::util::Float2 scale;
    nn::util::Float2 size;
};

enum BnvgShapePathType {
    BnvgShapePathType_Path,
    BnvgShapePathType_Ellipse,
    BnvgShapePathType_Rect,
    BnvgShapePathType_Star,
    BnvgShapePathType_Max
};

class ResBlendMode {
public:
    ResBlendMode() {}

    void Set(BlendOp aBlendOp, BlendFactor aSrcFactor, BlendFactor aDstFactor, LogicOp aLogicOp) {
        m_BlendOperation = uint8_t(aBlendOp);
        m_SrcBlendFactor = uint8_t(aSrcFactor);
        m_DestBlendFactor = uint8_t(aDstFactor);
        m_LogicOp = uint8_t(aLogicOp);
    }

    uint8_t m_BlendOperation;
    uint8_t m_SrcBlendFactor;
    uint8_t m_DestBlendFactor;
    uint8_t m_LogicOp;
};
};  // namespace ui2d
};  // namespace nn