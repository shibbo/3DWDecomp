#pragma once

#include <basis/seadTypes.h>

/// The player's particle effects (implemented by PlayerEffect).
class IUsePlayerEffect {
public:
    virtual void emitEffect(const char* pName) = 0;
    virtual void stopEffect(const char* pName) = 0;
    virtual void killEffect(const char* pName) = 0;
    virtual bool isEmitting(const char* pName) const = 0;
    virtual bool isEmittingIdx(const char* pName, s32 index) const = 0;
    virtual void emitCommonEffect(const char* pName) = 0;
    virtual void stopCommonEffect(const char* pName) = 0;
    virtual void tryUpdateMaterial(const char* pMaterialName) = 0;
    virtual bool isValidWater() const = 0;
    virtual void emitEffectIdx(const char* pName, s32 index) = 0;
    virtual void stopEffectIdx(const char* pName, s32 index) = 0;
    virtual void emitEffectSubActor(const char* pName, s32 index, const char* pSubActorName) = 0;
    virtual void stopEffectSubActor(const char* pName, s32 index, const char* pSubActorName) = 0;
};
