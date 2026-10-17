#pragma once

class IUsePlayerSubAction {
public:
    virtual void validateAll() = 0;
    virtual void invalidateAll() = 0;
    virtual void validate(unsigned int) = 0;
    virtual void invalidate(unsigned int) = 0;
    virtual bool isValid(unsigned int) const = 0;
    virtual void setThrowAnimCancel(bool) = 0;
    virtual void setMainAnimAfterThrow(const char*) = 0;
    virtual void setMainAnimAfterTailAttack(const char*) = 0;
    virtual void setIgnoreFloorCondition(bool) = 0;
    virtual bool isRunning() const = 0;
    virtual void forceEnd() = 0;
};
