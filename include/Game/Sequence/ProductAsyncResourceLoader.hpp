#pragma once

#include <basis/seadTypes.h>

class GameDataHolder;

namespace al {
class AudioSystemInfo;
}  // namespace al

/**
 * @brief Loads the resources of the next sequence state in the background.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductAsyncResourceLoader {
public:
    ProductAsyncResourceLoader(const al::AudioSystemInfo* pAudioSystemInfo,
                               GameDataHolder* pHolder);
    ~ProductAsyncResourceLoader();

private:
    u8 _0[0x38];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductAsyncResourceLoader) == 0x38);
