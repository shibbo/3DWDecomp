#include "LuigiBros/Vessel/Resource.hpp"

extern const unsigned char data_bin[];
extern const unsigned int data_bin_size;
extern const unsigned char vcm_arc[];
extern const unsigned int vcm_arc_size;
extern const unsigned int vcm_arc_org_size;
extern const unsigned char dialog_arc[];
extern const unsigned int dialog_arc_size;
extern const unsigned int dialog_arc_org_size;
extern const unsigned char keycon_arc[];
extern const unsigned int keycon_arc_size;
extern const unsigned int keycon_arc_org_size;
extern const unsigned char system_arc[];
extern const unsigned int system_arc_size;
extern const unsigned int system_arc_org_size;
extern const unsigned char font_buildinshader_gsh[];
extern const unsigned int font_buildinshader_gsh_size;
extern const unsigned char lyt_buildinshader_gsh[];
extern const unsigned int lyt_buildinshader_gsh_size;

namespace Vessel { namespace Resource { namespace File {
static ResourceFile sFiles[FILE_COUNT];

/**
 * @brief Clears the uncompressed size in the platform resource descriptor.
 * @param pName Unused resource name.
 * @param index Unused resource index.
 */
void ResourceFile::Set(const char* pName, int index) {
    mOriginalSize = 0;
}

/** @brief Initializes descriptors for the seven embedded resource files. */
void Initialize() {
    sFiles[0] = {data_bin, data_bin_size, 0};
    sFiles[1] = {vcm_arc, vcm_arc_size, vcm_arc_org_size};
    sFiles[2] = {dialog_arc, dialog_arc_size, dialog_arc_org_size};
    sFiles[3] = {keycon_arc, keycon_arc_size, keycon_arc_org_size};
    sFiles[4] = {system_arc, system_arc_size, system_arc_org_size};
    sFiles[5] = {font_buildinshader_gsh, font_buildinshader_gsh_size, 0};
    sFiles[6] = {lyt_buildinshader_gsh, lyt_buildinshader_gsh_size, 0};
}

/** @brief Gets an embedded resource descriptor. @param index Valid resource index. @return Resource descriptor. */
ResourceFile* Get(FileIndex index) {
    return &sFiles[index];
}

/** @brief Finalizes resources; embedded data requires no cleanup. */
void Finalize() {}
} } }
