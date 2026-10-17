/**
 * @file vfx_StripeUtility.h
 * @brief Helpers shared by the stripe emitter plugins.
 */

#pragma once

#include <attributes.h>
#include <cstring>
#include <nn/gfx/gfx_CommandBuffer.h>
#include <nn/gfx/gfx_Types.h>
#include <nn/types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/vfx/Callback.h>
#include <nn/vfx/EmitterRes.h>
#include <nn/vfx/EmitterSet.h>
#include <nn/vfx/System.h>
#include <nn/vfx/vfx_EmitterCalc.h>

namespace nn {
namespace vfx {
namespace detail {

void OutputWarning(const char* pFormat, ...);

bool DummyRenderStateSetCallback(RenderStateSetArg& rArg);

void VectorRotateArbitraryAxis(util::Vector3fType* pOut, const util::Vector3fType& rVector,
                               const util::Vector3fType& rAxis, f32 angle);

void HermiteInterpolationOnCubic(util::Vector3fType* pOut, const util::Vector3fType& rStartPos,
                                 const util::Vector3fType& rStartTangent,
                                 const util::Vector3fType& rEndPos,
                                 const util::Vector3fType& rEndTangent, f32 t);

/**
 * Interpolates linearly between two vectors.
 * @param pOut the interpolated vector
 * @param rStart the vector at t = 0
 * @param rEnd the vector at t = 1
 * @param t the position between the two vectors
 */
inline void VectorLerp(util::Vector3fType* pOut, const util::Vector3fType& rStart,
                       const util::Vector3fType& rEnd, f32 t) {
    f32 s = 1.0f - t;
    pOut->_v = vaddq_f32(vmulq_n_f32(rStart._v, s), vmulq_n_f32(rEnd._v, t));
}

/**
 * Code shared by the stripe systems, written for any of them.
 */
class StripeSystemUtility {
public:
    static int CalculateDelayStripeCount(const Emitter* pEmitter, int historyNum);
    static bool SetupDefaultShaderSetting(gfx::CommandBuffer* pCommandBuffer, Emitter* pEmitter,
                                          ShaderType shaderType, Shader* pShader,
                                          void* pUserParam, DrawParameterArg* pDrawParameterArg);

    /**
     * Takes a free stripe instance and gives it a ring of history records.
     * @param pSystem the stripe system
     * @param pEmitter the emitter of the stripe
     * @return the stripe instance, or nullptr when none could be taken
     */
    template <typename T>
    static typename T::StripeInstance* AllocStripe(T* pSystem, Emitter* pEmitter) {
        typedef typename T::History History;
        typedef typename T::StripeInstance StripeInstance;

        for (int i = 0; i < pSystem->m_StripeNum; i++) {
            StripeInstance* pInstance = &pSystem->m_pStripeArray[pSystem->m_StripeIndex];

            if (!pInstance->isUsed) {
                const typename T::ResourceType* pRes =
                    static_cast<const typename T::ResourceType*>(
                        pEmitter->m_pEmitterRes->m_pEmitterPluginData);
                u32 historyNum = static_cast<u32>(pRes->historyNum);
                pInstance->pHistoryBuffer = static_cast<History*>(
                    pEmitter->GetDynamicHeap()->Alloc(historyNum * sizeof(History), 0x80));

                if (pInstance->pHistoryBuffer == nullptr) {
                    Warning(pEmitter, RuntimeWarningId_StripeHistoryAllocationFailed);
                    OutputWarning("Buffer for stripe history is not enough\n");
                    return nullptr;
                }

                for (u32 j = 0; j < historyNum - 1; j++) {
                    pInstance->pHistoryBuffer[j].pNext = &pInstance->pHistoryBuffer[j + 1];
                }

                for (u32 j = 1; j < historyNum; j++) {
                    pInstance->pHistoryBuffer[j].pPrev = &pInstance->pHistoryBuffer[j - 1];
                }

                pInstance->pHistoryBuffer[historyNum - 1].pNext = &pInstance->pHistoryBuffer[0];
                pInstance->pHistoryBuffer[0].pPrev = &pInstance->pHistoryBuffer[historyNum - 1];
                pInstance->pHistoryHead = pInstance->pHistoryBuffer;
                pInstance->pHistoryTail = pInstance->pHistoryBuffer;
                pInstance->isUsed = true;
                pInstance->time = 0.0f;
                pInstance->historyNum = 0;
                pInstance->vertexNum = 0;
                pInstance->pNext = nullptr;
                pSystem->m_ProcessingStripeCount++;
                return pInstance;
            }

            pSystem->m_StripeIndex = pSystem->m_StripeIndex + 1 >= pSystem->m_StripeNum ?
                                         0 :
                                         pSystem->m_StripeIndex + 1;
        }

        Warning(pEmitter, RuntimeWarningId_NoAvailableStripeInstance);
        OutputWarning("There is no available Stripe instance.\n");
        return nullptr;
    }

    /**
     * Interpolates the position and direction of a stripe between two history records.
     * @param pOutPos the interpolated position
     * @param pOutDir the interpolated direction
     * @param pHistory the record the interpolation starts from
     * @param index the index of the record in the stripe
     * @param historyNum the number of records of the stripe
     * @param t the position between the record and the next one, from 0 to 1
     */
    template <typename T>
    static void CalculateHermiteInterpolatedCurveVec(util::Vector3fType* pOutPos,
                                                     util::Vector3fType* pOutDir,
                                                     const typename T::History* pHistory,
                                                     int index, int historyNum, f32 t) {
        typedef typename T::History History;

        util::Vector3fType startPos;
        util::Vector3fType endPos;
        util::Vector3fType startTangent;
        util::Vector3fType endTangent;

        if (index == 0) {
            const History* pNext = pHistory->pNext;
            const History* pNextNext = pNext->pNext;
            startPos = pHistory->pos;
            endPos = pNext->pos;
            endTangent._v = vmulq_n_f32(vsubq_f32(pNextNext->pos._v, pHistory->pos._v), 0.5f);
            startTangent._v =
                vmulq_n_f32(vaddq_f32(vsubq_f32(pNext->pos._v, pHistory->pos._v),
                                      vsubq_f32(pNext->pos._v, pNextNext->pos._v)),
                            0.25f);
            VectorLerp(pOutDir, pHistory->dir, pNext->dir, t);
        } else if (index == historyNum - 1) {
            const History* pPrev = pHistory->pPrev;
            const History* pPrevPrev = pPrev->pPrev;
            startPos = pPrev->pos;
            endPos = pHistory->pos;
            startTangent._v = vmulq_n_f32(vsubq_f32(pHistory->pos._v, pPrevPrev->pos._v), 0.5f);
            endTangent._v =
                vmulq_n_f32(vaddq_f32(vsubq_f32(pPrev->pos._v, pHistory->pos._v),
                                      vsubq_f32(pPrev->pos._v, pPrevPrev->pos._v)),
                            -0.25f);
            t = 1.0f;
            VectorLerp(pOutDir, pPrev->dir, pHistory->dir, t);
        } else if (index == historyNum - 2) {
            const History* pNext = pHistory->pNext;
            const History* pCurrent = pNext->pPrev;
            const History* pPrev = pCurrent->pPrev;
            startPos = pCurrent->pos;
            endPos = pNext->pos;
            startTangent._v = vmulq_n_f32(vsubq_f32(pNext->pos._v, pPrev->pos._v), 0.5f);
            endTangent._v =
                vmulq_n_f32(vaddq_f32(vsubq_f32(pCurrent->pos._v, pNext->pos._v),
                                      vsubq_f32(pCurrent->pos._v, pPrev->pos._v)),
                            -0.25f);
            VectorLerp(pOutDir, pCurrent->dir, pNext->dir, t);
        } else {
            const History* pNext = pHistory->pNext;
            const History* pNextNext = pNext->pNext;
            const History* pPrev = pHistory->pPrev;
            startPos = pHistory->pos;
            endPos = pNext->pos;
            startTangent._v = vmulq_n_f32(vsubq_f32(pNext->pos._v, pPrev->pos._v), 0.5f);
            endTangent._v = vmulq_n_f32(vsubq_f32(pNextNext->pos._v, pHistory->pos._v), 0.5f);
            VectorLerp(pOutDir, pHistory->dir, pNext->dir, t);
        }

        HermiteInterpolationOnCubic(pOutPos, startPos, startTangent, endPos, endTangent, t);
    }

    /**
     * Draws every stripe of an emitter.
     * @param pCommandBuffer the command buffer
     * @param pSystem the effect system
     * @param pEmitter the emitter
     * @param shaderType the shader to draw with
     * @param pUserParam user parameter passed to the callbacks
     * @param pDrawParameterArg the draw parameters
     */
    template <typename T>
    static void DrawParticleStripeEmitter(gfx::CommandBuffer* pCommandBuffer, System* pSystem,
                                          Emitter* pEmitter, ShaderType shaderType,
                                          void* pUserParam, DrawParameterArg* pDrawParameterArg) {
        typedef typename T::ResourceType ResourceType;
        typedef typename T::EmitterPluginUserData EmitterPluginUserData;
        typedef typename T::StripeInstance StripeInstance;

        const EmitterResource* pEmitterRes = pEmitter->m_pEmitterRes;
        const EmitterPluginUserData* pUserData =
            static_cast<const EmitterPluginUserData*>(pEmitter->m_pEmitterPluginUserData);
        const ResourceType* pRes =
            static_cast<const ResourceType*>(pEmitterRes->m_pEmitterPluginData);
        gfx::Device* pDevice = pSystem->m_pDevice;
        TemporaryBuffer* pTemporaryBuffer = pDrawParameterArg->m_pTemporaryBuffer;
        Shader* pShader = shaderType == ShaderType_Compute ?
                              nullptr :
                              pEmitter->m_pEmitterRes->m_Shader[shaderType];

        if (!SetupDefaultShaderSetting(pCommandBuffer, pEmitter, shaderType, pShader, pUserParam,
                                       pDrawParameterArg)) {
            return;
        }

        pCommandBuffer->SetVertexBuffer(
            pEmitter->m_pEmitterRes->m_CustomAttributeBufferSlot,
            *pEmitter->m_Attribute.GetGpuAddress(pUserData->bufferSide),
            sizeof(typename T::VertexAttribute), pEmitter->m_Attribute.GetSize());

        for (const StripeInstance* pInstance = pUserData->pDelayedStripeHead;
             pInstance != nullptr; pInstance = pInstance->pNext) {
            if (pInstance->vertexNum > 0) {
                DrawParticleStripe<T>(pCommandBuffer, pDevice, pTemporaryBuffer, pEmitter,
                                      pShader, pRes, pUserData, pInstance);
            }
        }

        for (int i = 0; i < pEmitter->m_MaxParticleNum; i++) {
            const StripeInstance* pInstance =
                static_cast<const StripeInstance*>(pEmitter->GetParticleData()[i].pUserData2);

            if (pInstance != nullptr && pInstance->isUsed && pInstance->vertexNum > 0) {
                DrawParticleStripe<T>(pCommandBuffer, pDevice, pTemporaryBuffer, pEmitter,
                                      pShader, pRes, pUserData, pInstance);
            }
        }
    }

    /**
     * Binds the constant buffer of one stripe and draws it.
     * @param pCommandBuffer the command buffer
     * @param pDevice the graphics device
     * @param pTemporaryBuffer the buffer the constant buffer is written to
     * @param pEmitter the emitter of the stripe
     * @param pShader the shader to draw with
     * @param pRes the plugin data of the emitter
     * @param pUserData the plugin state of the emitter
     * @param pInstance the stripe
     */
    template <typename T>
    static void DrawParticleStripe(gfx::CommandBuffer* pCommandBuffer, gfx::Device* pDevice,
                                   TemporaryBuffer* pTemporaryBuffer, Emitter* pEmitter,
                                   Shader* pShader, const typename T::ResourceType* pRes,
                                   const typename T::EmitterPluginUserData* pUserData,
                                   const typename T::StripeInstance* pInstance) {
        DrawStripeMesh<T>(pCommandBuffer, pTemporaryBuffer, pEmitter, pShader, pRes, pUserData,
                          pInstance, StripeMeshType_Normal);

        if (pRes->meshType == StripeMeshType_Cross) {
            DrawStripeMesh<T>(pCommandBuffer, pTemporaryBuffer, pEmitter, pShader, pRes,
                              pUserData, pInstance, StripeMeshType_Cross);
        }
    }

private:
    /**
     * Binds the constant buffer of one mesh of a stripe and draws it.
     * @param pCommandBuffer the command buffer
     * @param pTemporaryBuffer the buffer the constant buffer is written to
     * @param pEmitter the emitter of the stripe
     * @param pShader the shader to draw with
     * @param pRes the plugin data of the emitter
     * @param pUserData the plugin state of the emitter
     * @param pInstance the stripe
     * @param meshType the mesh to draw
     */
    template <typename T>
    ALWAYS_INLINE static void DrawStripeMesh(gfx::CommandBuffer* pCommandBuffer,
                               TemporaryBuffer* pTemporaryBuffer, Emitter* pEmitter,
                               Shader* pShader, const typename T::ResourceType* pRes,
                               const typename T::EmitterPluginUserData* pUserData,
                               const typename T::StripeInstance* pInstance,
                               StripeMeshType meshType) {
        typedef typename T::ConstantBufferObject ConstantBufferObject;

        ConstantBufferObject constantBuffer;
        T::MakeConstantBufferObject(&constantBuffer, pRes, pUserData, pInstance, pEmitter,
                                    meshType);
        gfx::GpuAddress address;
        address.ToData()->value = 0;
        address.ToData()->impl = 0;
        ConstantBufferObject* pConstantBuffer = static_cast<ConstantBufferObject*>(
            pTemporaryBuffer->Map(&address, sizeof(ConstantBufferObject)));

        if (pConstantBuffer == nullptr) {
            Warning(nullptr, RuntimeWarningId_TemporaryBufferAllocationFailed);
            return;
        }

        *pConstantBuffer = constantBuffer;

        s32 vertexLocation = pShader->m_VertexEmitterPluginConstantBufferLocation;
        s32 pixelLocation = pShader->m_PixelEmitterPluginConstantBufferLocation;

        if (vertexLocation != -1) {
            pCommandBuffer->SetConstantBuffer(vertexLocation, gfx::ShaderStage_Vertex, address,
                                              sizeof(ConstantBufferObject));
        }

        if (pixelLocation != -1) {
            pCommandBuffer->SetConstantBuffer(pixelLocation, gfx::ShaderStage_Pixel, address,
                                              sizeof(ConstantBufferObject));
        }

        pTemporaryBuffer->Unmap();
        pCommandBuffer->Draw(gfx::PrimitiveTopology_TriangleStrip, pInstance->vertexNum,
                             pInstance->index * pUserData->vertexNumPerStripe);
    }
};

}  // namespace detail
}  // namespace vfx
}  // namespace nn
