#pragma once

namespace Vessel { namespace Resource { namespace File {
enum FileIndex : unsigned int {
    FILE_DATA, FILE_MENU, FILE_DIALOG, FILE_KEY_CONFIG, FILE_SYSTEM,
    FILE_FONT_SHADER, FILE_LAYOUT_SHADER, FILE_COUNT
};

struct ResourceFile {
    void Set(const char* pName, int index);
    const unsigned char* mpData;
    unsigned int mSize;
    unsigned int mOriginalSize;
};
static_assert(sizeof(ResourceFile) == 16, "Resource descriptor layout");
void Initialize();
ResourceFile* Get(FileIndex index);
void Finalize();
} } }
