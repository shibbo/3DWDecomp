#pragma once
#include <nn/types.h>

namespace nn::gfx {
class ResTextureFile;
}  // namespace nn::gfx

namespace nn::ui2d {
/** @brief Provides the archives that hold the layout resources of the screens. */
class ArcResourceMgr {
public:
    /** @brief Archive registered with the manager. */
    struct ArchiveData {
        u8 _00[0x68];
        void* pArchive;
        nn::gfx::ResTextureFile* pTextureFile;
    };

    // The virtual functions before FindArchiveData are not reconstructed.
    virtual void Reserved00_();
    virtual void Reserved08_();
    virtual void Reserved10_();
    virtual void Reserved18_();
    virtual void Reserved20_();
    virtual void Reserved28_();
    virtual void Reserved30_();
    virtual void Reserved38_();

    /**
     * @brief Find the archive that holds a layout.
     * @param pLayoutName Name of the layout.
     * @return Archive, or nullptr when no archive holds the layout.
     */
    virtual const ArchiveData* FindArchiveData(const char* pLayoutName) const;
};
}  // namespace nn::ui2d
