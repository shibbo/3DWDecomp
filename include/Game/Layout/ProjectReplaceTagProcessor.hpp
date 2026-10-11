#pragma once

#include "Library/Message/ReplaceTagProcessorBase.hpp"

/**
 * @brief Tag processor that resolves the project-specific picture font tags.
 */
class ProjectReplaceTagProcessor : public al::ReplaceTagProcessorBase {
public:
    ProjectReplaceTagProcessor() = default;

    s32 replacePictureGroup(char16_t* pDst, const al::MessageTag& rTag) const override;
};
