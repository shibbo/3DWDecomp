#pragma once

#include <nn/types.h>

namespace nn::atk {
R_DEFINE_NAMESPACE_RESULT_MODULE(209);
R_DEFINE_ERROR_RESULT(Unknown, 1);
R_DEFINE_ERROR_RESULT(InvalidSoundId, 101);
R_DEFINE_ERROR_RESULT(InvalidLabelString, 102);
R_DEFINE_ERROR_RESULT(InvalidFileFormat, 103);
R_DEFINE_ERROR_RESULT(FileNotLoaded, 104);
R_DEFINE_ERROR_RESULT(StreamSoundPlaying, 105);
R_DEFINE_ERROR_RESULT(StreamFilePathNotFound, 106);
R_DEFINE_ERROR_RESULT(StreamFileHeaderLoadFailed, 107);
R_DEFINE_ERROR_RESULT(FileAccessFailed, 108);
R_DEFINE_ERROR_RESULT(RegionNotFound, 109);
R_DEFINE_ERROR_RESULT(RegionNameNotFound, 110);
}  // namespace nn::atk
