#pragma once

#include <basis/seadTypes.h>
#include <nn/oe.h>

namespace al {
class ApplicationMessageReceiver;
class AudioSystem;
class EffectSystem;
class FontHolder;
class NfpDirector;
class GamePadSystem;
class LayoutSystem;
class MessageSystem;
class NetworkSystem;
class WaveVibrationHolder;
struct DrawSystemInfo;

struct GameSystemInfo {
    GameSystemInfo();

    EffectSystem* getEffectSystem() const { return static_cast<EffectSystem*>(_8); }
    LayoutSystem* getLayoutSystem() const { return static_cast<LayoutSystem*>(_10); }
    MessageSystem* getMessageSystem() const { return static_cast<MessageSystem*>(_18); }
    NetworkSystem* getNetworkSystem() const { return static_cast<NetworkSystem*>(_20); }
    GamePadSystem* getGamePadSystem() const { return static_cast<GamePadSystem*>(_30); }
    DrawSystemInfo* getDrawSystemInfo() const { return static_cast<DrawSystemInfo*>(_38); }
    FontHolder* getFontHolder() const { return static_cast<FontHolder*>(_40); }
    NfpDirector* getNfpDirector() const { return static_cast<NfpDirector*>(_48); }
    ApplicationMessageReceiver* getApplicationMessageReceiver() const {
        return static_cast<ApplicationMessageReceiver*>(_50);
    }
    WaveVibrationHolder* getWaveVibrationHolder() const {
        return static_cast<WaveVibrationHolder*>(_58);
    }

    void setAudioSystem(AudioSystem* pAudioSystem) { _0 = pAudioSystem; }
    void setEffectSystem(EffectSystem* pEffectSystem) { _8 = pEffectSystem; }
    void setLayoutSystem(LayoutSystem* pLayoutSystem) { _10 = pLayoutSystem; }
    void setMessageSystem(MessageSystem* pMessageSystem) { _18 = pMessageSystem; }
    void setGamePadSystem(GamePadSystem* pGamePadSystem) { _30 = pGamePadSystem; }
    void setDrawSystemInfo(DrawSystemInfo* pDrawSystemInfo) { _38 = pDrawSystemInfo; }
    void setNfpDirector(NfpDirector* pNfpDirector) { _48 = pNfpDirector; }
    void setApplicationMessageReceiver(ApplicationMessageReceiver* pReceiver) { _50 = pReceiver; }
    void setWaveVibrationHolder(WaveVibrationHolder* pHolder) { _58 = pHolder; }

    void* _0 = nullptr;
    void* _8 = nullptr;
    void* _10 = nullptr;
    void* _18 = nullptr;
    void* _20 = nullptr;
    void* _28 = nullptr;
    void* _30 = nullptr;
    void* _38 = nullptr;
    void* _40 = nullptr;
    void* _48 = nullptr;
    void* _50 = nullptr;
    void* _58 = nullptr;
};

static_assert(sizeof(GameSystemInfo) == 0x60);

class ApplicationMessageReceiver;
class LiveActor;

enum CpuPerformance : u32 {
    CpuPerformance_1020MHz = 0,
    CpuPerformance_Invalid = 1,
};

enum GpuPerformance : u32 {
    GpuPerformance_768MHz = 0,
    GpuPerformance_307MHz = 1,
    GpuPerformance_384MHz = 2,
    GpuPerformance_Boost = 3,
    GpuPerformance_Invalid = 4,
};

enum MemoryPerformance : u32 {
    MemoryPerformance_1331MHz = 0,
    MemoryPerformance_1600MHz = 1,
    MemoryPerformance_Invalid = 2,
};

bool isPerformanceNormal(const ApplicationMessageReceiver* pReceiver);
bool isPerformanceBoost(const ApplicationMessageReceiver* pReceiver);
nn::oe::PerformanceConfiguration getPerformanceConfiguration(nn::oe::PerformanceMode mode);
nn::oe::PerformanceConfiguration
getPerformanceConfiguration(const ApplicationMessageReceiver* pReceiver);
u64 getCpuPerformance(nn::oe::PerformanceMode mode);
u64 getCpuPerformance(const ApplicationMessageReceiver* pReceiver);
u64 getCpuPerformance(const LiveActor* pActor);
u64 getGpuPerformance(nn::oe::PerformanceMode mode);
u64 getGpuPerformance(const ApplicationMessageReceiver* pReceiver);
u64 getGpuPerformance(const LiveActor* pActor);
u64 getMemoryPerformance(nn::oe::PerformanceMode mode);
u64 getMemoryPerformance(const ApplicationMessageReceiver* pReceiver);
u64 getMemoryPerformance(const LiveActor* pActor);
void setCpuPerformance(CpuPerformance performance, nn::oe::PerformanceMode mode);
void setCpuPerformance(CpuPerformance performance, const ApplicationMessageReceiver* pReceiver);
void setGpuPerformance(GpuPerformance performance, nn::oe::PerformanceMode mode);
void setGpuPerformance(GpuPerformance performance, const ApplicationMessageReceiver* pReceiver);
void setMemoryPerformance(MemoryPerformance performance, nn::oe::PerformanceMode mode);
void setMemoryPerformance(MemoryPerformance performance,
                          const ApplicationMessageReceiver* pReceiver);
bool isCpuBoostOn();
void setCpuBoost(bool isBoost, bool isUnused);
void updateCpuBoost();
}  // namespace al
