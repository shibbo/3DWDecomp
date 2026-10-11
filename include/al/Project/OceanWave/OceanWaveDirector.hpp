#pragma once

#include <math/seadVector.h>

#include "Library/Execute/IUseExecutor.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/ISceneObj.hpp"

namespace al {
class ByamlIter;
class OceanWaveInfo;

class OceanWaveDirector : public ISceneObj, public LiveActor, public IUseExecutor {
public:
    OceanWaveDirector(const char* pName);

    virtual void hide();
    virtual void show();
    void execute() override;
    virtual bool createWave(const LiveActor* pActor, const OceanWaveInfo* pInfo);
    void kill() override;
    virtual f32 getY(const sead::Vector3f& rPos);
    virtual s32 getRenderType() const = 0;
    virtual bool isInInk(const sead::Vector3f& rPos);
    virtual void initFromYaml(const ByamlIter& rIter, const char* pName) = 0;
    virtual void setStageName(const char* pStageName) = 0;
    ~OceanWaveDirector() override = default;
};
}  // namespace al
