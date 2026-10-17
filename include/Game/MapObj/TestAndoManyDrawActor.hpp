#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class DeferredRenderingShpUbo; class ModelAdditionalInfo; }
class TestAndoManyDrawActor : public al::LiveActor {
public:
    explicit TestAndoManyDrawActor(const char*);
    void draw() const override;
    virtual int getNum() const = 0;
    virtual void getPos(sead::Vector3f*, unsigned) const = 0;
    void createShpUbo(unsigned);
    void updateShpUbo();
    void initModelAdditionalInfo(al::ModelAdditionalInfo*) const;
protected:
    bool mUseLocalPositions = false;
    al::DeferredRenderingShpUbo*** mShapeUbos = nullptr;
};
static_assert(sizeof(TestAndoManyDrawActor) == 0x150);
