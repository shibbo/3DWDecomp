#pragma once
#include <container/seadPtrArray.h>
namespace al { class LiveActor; class ActorInitInfo; }
class ModelHolderActor;

/** @brief Selects one of several model actors while preserving its pose and visibility. */
class ModelHolder {
public:
    explicit ModelHolder(s32 capacity);
    void appear();
    void kill();
    ModelHolderActor* getCurrentModel() const;
    void initCurrentModel(s32 index);
    void registerModel(ModelHolderActor* pModel);
    void registerModel(al::LiveActor* pOwner, const al::ActorInitInfo& rInfo, const char* pName);
    void changeModel(s32 index);
    void startAction(const char* pName);
    bool isActionEnd() const;
    void updatePosture(const al::LiveActor* pActor);
private:
    sead::PtrArray<ModelHolderActor> mModels;
    s32 mCurrentIndex = -1;
};
static_assert(sizeof(ModelHolder) == 0x18);
