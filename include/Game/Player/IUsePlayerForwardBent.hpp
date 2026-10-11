#pragma once

/// The player bending forward while running (implemented by PlayerForwardBent).
class IUsePlayerForwardBent {
public:
    virtual void invalidateForwardBent() = 0;
    virtual void validateForwardBent() = 0;
    virtual void forceClearForwardBend() = 0;
    virtual void clearForwardBent() = 0;
    virtual bool isBent() const = 0;
};
