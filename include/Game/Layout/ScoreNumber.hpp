#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <math/seadVector.h>
namespace al { class LayoutInitInfo; }
class ScoreNumber : public al::LayoutActor {
public:
    explicit ScoreNumber(const al::LayoutInitInfo&);
    void startScore(int, unsigned int, const sead::Vector3f*, const sead::Vector3f&, int);
    void startScore(int, unsigned int, const sead::Vector3f&, int);
    void startPlayerUp(int, const sead::Vector3f*, const sead::Vector3f&, int);
    void startPlayerUp(int, const sead::Vector3f&, int);
private:
    u8 _121[0x3f];
};
static_assert(sizeof(ScoreNumber) == 0x160);
