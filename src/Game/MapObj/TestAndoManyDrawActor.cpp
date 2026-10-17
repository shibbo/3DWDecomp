#include "MapObj/TestAndoManyDrawActor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Project/Model/SimpleModelG3D.hpp"
#include "Project/Model/ModelAdditionalInfo.hpp"
#include "Library/Shader/DeferredRendering/DeferredRenderingShpUbo.hpp"
#include <nn/g3d/g3d_ModelObj.h>
#include <nn/g3d/g3d_ResModel.h>
TestAndoManyDrawActor::TestAndoManyDrawActor(const char* name) : al::LiveActor(name) {}
void TestAndoManyDrawActor::draw() const {
    auto* model = mModelKeeper->getModelCafe()->getModelG3D();
    al::ModelAdditionalInfo info(getSceneInfo()->graphicsSystemInfo, false);
    model->setModelAdditionalInfo(info);
}
void TestAndoManyDrawActor::createShpUbo(unsigned) {
    mShapeUbos = new al::DeferredRenderingShpUbo**[mModelKeeper->getModelCafe()->getResModel()->GetShapeCount()];
}
void TestAndoManyDrawActor::updateShpUbo() {
    auto* model = mModelKeeper->getModelCafe()->getModelG3D()->getModelObj();
    sead::Matrix34f base;
    al::makeMtxRT(&base, this);
    int shapeCount = model->GetNumShapes();
    int count = getNum();
    if (mUseLocalPositions) {
        sead::Matrix34f translation;
        translation.makeIdentity();
        for (int shape = 0; shape < shapeCount; ++shape) {
            for (int i = 0; i < count; ++i) {
                sead::Vector3f pos;
                getPos(&pos, i);
                translation.setTranslation(pos);
                sead::Matrix34f matrix;
                matrix.setMul(translation, base);
                mShapeUbos[shape][i]->setMtx(&matrix);
                mShapeUbos[shape][i]->swap();
            }
        }
    } else {
        for (int shape = 0; shape < shapeCount; ++shape) {
            for (int i = 0; i < count; ++i) {
                sead::Vector3f pos;
                getPos(&pos, i);
                base.setTranslation(pos);
                mShapeUbos[shape][i]->setMtx(&base);
                mShapeUbos[shape][i]->swap();
            }
        }
    }
}
void TestAndoManyDrawActor::initModelAdditionalInfo(al::ModelAdditionalInfo* info) const {
    info->setGraphicsSystemInfo(getSceneInfo()->graphicsSystemInfo);
}
