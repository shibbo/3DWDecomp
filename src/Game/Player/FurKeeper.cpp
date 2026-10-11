#include "Player/FurKeeper.hpp"

#include <gfx/seadGraphics.h>
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Player/FurShape.hpp"
#include "Project/Base/StringUtil.hpp"

/**
 * Creates an empty fur keeper; the shapes are created by init().
 */
FurKeeper::FurKeeper() = default;

/**
 * Reads the "InitFur" init file of the actor's model and creates one fur shape per entry.
 * @param pActor the actor the fur grows on
 * @param pName the name of the fur setting in the init file
 */
void FurKeeper::init(al::LiveActor* pActor, const char* pName) {
    al::ByamlIter rootIter;
    if (!al::tryGetActorInitFileIter(&rootIter, al::getModelResource(pActor), "InitFur", pName)) {
        return;
    }

    u32 shapeNum = rootIter.getSize();
    if (shapeNum == 0) {
        return;
    }

    mParams.allocBuffer(shapeNum, nullptr);
    mShapes.allocBuffer(shapeNum, nullptr);

    for (u32 i = 0; i < shapeNum; i++) {
        al::ByamlIter shapeIter;
        rootIter.tryGetIterByIndex(&shapeIter, i);

        mParams.emplaceBack();
        FurParam* param = mParams.back();
        shapeIter.tryGetStringByKey(&param->shapeName, "Shape");
        shapeIter.tryGetStringByKey(&param->diameterTextureName, "DiameterTexture");
        shapeIter.tryGetIntByKey(&param->layerNum, "LayerNum");
        shapeIter.tryGetFloatByKey(&param->height, "Height");
        shapeIter.tryGetFloatByKey(&param->startAlpha, "StartAlpha");
        shapeIter.tryGetFloatByKey(&param->endAlpha, "EndAlpha");

        sead::Graphics::instance()->lockDrawContext();
        auto* shape = new FurShape(pActor, param->shapeName, param->diameterTextureName);
        shape->setHeight(param->height);
        shape->setAlpha(param->startAlpha, param->endAlpha);
        shape->setLayerNum(param->layerNum);
        shape->setUnk14(false);
        mShapes.pushBack(shape);
        sead::Graphics::instance()->unlockDrawContext();
    }
}

/**
 * Updates the uniform buffers of every fur shape.
 */
void FurKeeper::updateUbo() {
    for (s32 i = 0; i < mShapes.size(); i++) {
        mShapes.at(i)->updateUbo();
    }
}

/**
 * Draws every fur shape.
 */
void FurKeeper::draw() const {
    for (s32 i = 0; i < mShapes.size(); i++) {
        mShapes.at(i)->draw();
    }
}

/**
 * Finds the fur shape created from the given shape name.
 * @param pShapeName the shape name to look for
 * @return the matching fur shape, or nullptr if there is none
 */
FurShape* FurKeeper::find(const char* pShapeName) const {
    for (s32 i = 0; i < mParams.size(); i++) {
        if (al::isEqualString(mParams.unsafeAt(i)->shapeName, pShapeName)) {
            return mShapes.at(i);
        }
    }

    return nullptr;
}

/**
 * @return the number of fur shapes
 */
s32 FurKeeper::getShapeNum() const {
    return mShapes.size();
}

/**
 * @param index the index of the fur shape
 * @return the fur shape at the given index, or nullptr if the index is out of range
 */
FurShape* FurKeeper::getShape(u32 index) const {
    return mShapes.at(index);
}
