#pragma once
#include "Demo/DemoObjBase.hpp"
namespace alSeFunction { enum DemoType : s32; }
// Partial declaration of the cutscene layout.
class DemoAnimatic : public DemoObjBase {
public:
    explicit DemoAnimatic(const char*, alSeFunction::DemoType = static_cast<alSeFunction::DemoType>(5));

    /**
     * @brief Set the flag at 0x300.
     * @param isSet The flag.
     */
    void setUnk300(bool isSet) { _300 = isSet; }

    /**
     * @brief Set the flag at 0x32a.
     * @param isSet The flag.
     */
    void setUnk32a(bool isSet) { _32a = isSet; }

private:
    bool _300;
    u8 _301[0x32a - 0x301];
    bool _32a;
    u8 _32b[0x368 - 0x32b];
};
static_assert(sizeof(DemoAnimatic) == 0x368);
