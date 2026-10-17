#include <common/aglTextureSampler.h>

namespace agl::detail {

/**
 * @brief Initializes sampler filtering, wrapping, LOD limits, and an opaque black border.
 */
SamplerObject::SamplerObject()
    : mBorderColor{0.0f, 0.0f, 0.0f, 1.0f}, mMinLod(0.0f), mMaxLod(14.0f), mLodBias(0.0f), mMagFilter(1),
      mMinFilter(1), mMipFilter(2), mWrapX(7), mWrapY(7), mWrapZ(7), mMaxAnisotropy(1), mCompareFunc(1) {}

} // namespace agl::detail
