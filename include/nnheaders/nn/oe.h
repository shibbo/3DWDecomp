/**
 * @file oe.h
 * @brief Extenstions to OS functions.
 */

#pragma once

#include <nn/album/album_types.h>
#include <nn/settings.h>
#include <nn/types.h>

namespace nn {
class TimeSpan;
}

namespace nn::oe {

enum OperationMode {
    OperationMode_Handheld,
    OperationMode_Docked,
};

enum PerformanceMode {
    PerformanceMode_Normal = 0,
    PerformanceMode_Boost = 1,
    PerformanceMode_Invalid = -1,
};

enum FocusHandlingMode {
    FocusHandlingMode_SuspendHomeSleep,
    FocusHandlingMode_NoSuspend,
    FocusHandlingMode_SuspendHomeSleepNotify,
    FocusHandlingMode_AlwaysSuspend
};

enum FocusState {
    FocusState_InFocus = 1,
    FocusState_OutOfFocus = 2,
    FocusState_Background = 3,
};

struct DisplayVersion {
    char name[16];
};

typedef s32 PerformanceConfiguration;

enum CpuBoostMode {
    CpuBoostMode_Disabled = 0,
    CpuBoostMode_Enabled = 1,
};

void Initialize();
[[noreturn]] void ExitApplicationAndGoBackToInStoreDemoMenu();
void FinishStartupLogo();
void EnableGamePlayRecording(void*, u64);
void SetExpectedVolumeBalance(f32, f32);
void SetPerformanceConfiguration(nn::oe::PerformanceMode, nn::oe::PerformanceConfiguration);
nn::oe::PerformanceConfiguration GetPerformanceConfiguration(nn::oe::PerformanceMode);
void SetCpuBoostMode(nn::oe::CpuBoostMode);
void SetResumeNotificationEnabled(bool);
void SetOperationModeChangedNotificationEnabled(bool);
void SetPerformanceModeChangedNotificationEnabled(bool);
void SetFocusHandlingMode(nn::oe::FocusHandlingMode);
void setScreenShotImageOrientation(nn::album::ImageOrientation);
void SetAlbumImageOrientation(nn::album::ImageOrientation);
void SetUserInactivityDetectionTimeExtended(bool);
bool IsUserInactivityDetectionTimeExtended();
bool TryPopNotificationMessage(u32*);
bool TryPopLaunchParameter(size_t*, void*, size_t);
void GetExpectedVolumeBalance(f32*, f32*);
void GetDisplayVersion(DisplayVersion*);
FocusState GetCurrentFocusState();
OperationMode GetOperationMode();
PerformanceMode GetPerformanceMode();
nn::settings::LanguageCode GetDesiredLanguage();
nn::TimeSpan GetProgramTotalActiveTime();

}  // namespace nn::oe
