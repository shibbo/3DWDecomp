#pragma once
#include "LuigiBros/Vessel/VesselMemory.hpp"

class FlashingFilter {
public:
    /** @brief Clears the filter's pending buffer references. */
    FlashingFilter() { mpBuffer390 = nullptr; mpBuffer388 = nullptr; }

    /** @brief Releases the filter's allocated image and work buffers. */
    ~FlashingFilter() {
        VESSEL_free(mpBuffer60);
        VESSEL_free(mpBuffer68);
        VESSEL_free(mpBuffer88);
        VESSEL_free(mpBuffer90);
        VESSEL_free(mpBufferA8);
        VESSEL_free(mpBufferB0);
        VESSEL_free(mpBufferD8);
    }

    void SetDiv();
    void SetDataRGB();
    void SetInitConstant();

private:
    friend class screenfilterCpu;
    bool _00;
    bool _01;
    bool _02;
    float _04;
    float _08;
    float _0c;
    float _10;
    unsigned int _14;
    float mRedWeight;
    float mGreenWeight;
    float mBlueWeight;
    unsigned char _24[0x3c];
    void* mpBuffer60;
    void* mpBuffer68;
    unsigned char _70[0x18];
    void* mpBuffer88;
    void* mpBuffer90;
    unsigned char _98[0x10];
    void* mpBufferA8;
    void* mpBufferB0;
    unsigned char _b8[0x20];
    void* mpBufferD8;
    unsigned char _e0[0x2a8];
    void* mpBuffer388;
    void* mpBuffer390;
    const unsigned char* mpDiv8;
    const unsigned char* mpDiv4;
    const unsigned short* mpRed565;
    const unsigned short* mpGreen565;
    const unsigned short* mpBlue565;
    const unsigned int* mpRed8888;
    const unsigned int* mpGreen8888;
    const unsigned int* mpBlue8888;
    unsigned char _3d8[8];
};
static_assert(sizeof(FlashingFilter) == 0x3e0, "Flashing filter layout");
extern FlashingFilter m_FlashingFilter;
