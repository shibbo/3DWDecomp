#include "Player/FurShape.hpp"

#include "Library/LiveActor/LiveActor.hpp"
#include "Player/FurEnv.hpp"

/**
 * Creates a fur shape with the default layer setup (two layers, alpha fading from 1.0 to 0.7).
 * @param pActor actor whose model the fur is drawn on
 * @param pShapeName name of the model shape covered with fur (unused in this build)
 * @param pDiameterTextureName name of the fur diameter texture (unused in this build)
 */
FurShape::FurShape(al::LiveActor* pActor, const char* pShapeName,
                   const char* pDiameterTextureName)
    : mActor(pActor), _8(nullptr), _10(0), _14(false), mHeight(1.0f), mStartAlpha(1.0f),
      mEndAlpha(0.7f), mLayerNum(2) {
    rc::getFurEnv(pActor)->getShaderProgram(1);
}

/**
 * Draws the fur shells. Stripped in this build.
 */
void FurShape::draw() const {}

/**
 * Updates the fur uniform block. Stripped in this build.
 */
void FurShape::updateUbo() {}
