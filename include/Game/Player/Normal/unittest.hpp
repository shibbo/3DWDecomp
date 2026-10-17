#pragma once

#include <attributes.h>
#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace unittest {

    /**
     * Test triangle used by the player's collision unit tests: stores three vertices, the face
     * normal and the result of the last collision check.
     */
    class Triangle {
    public:
        // Not inlined into TestTriangleSoup (most likely a separate translation unit originally).
        NOINLINE Triangle();

        NOINLINE void set(const sead::Vector3f& rPos0, const sead::Vector3f& rPos1,
                          const sead::Vector3f& rPos2);
        bool check(const sead::Vector3f& rPos, f32 radius);
        bool isInTriangle(const sead::Vector3f& rPos) const;
        bool check(const sead::Vector3f& rPos, const sead::Vector3f& rMove, f32 radius);
        bool checkCylinderAndLine(f32* pRate, sead::Vector3f* pHitPos, sead::Vector3f* pSide,
                                  const sead::Vector3f& rStart, const sead::Vector3f& rAxis,
                                  f32 radius, const sead::Vector3f& rPos,
                                  const sead::Vector3f& rMove);
        bool checkSphereAndLine(f32* pRate, sead::Vector3f* pHitPos, const sead::Vector3f& rCenter,
                                f32 radius, const sead::Vector3f& rPos,
                                const sead::Vector3f& rMove);
        bool check(const sead::Vector3f& rPos, const sead::Vector3f& rMove);
        bool isPosInLeftSide(const sead::Vector3f& rStart, const sead::Vector3f& rEnd,
                             const sead::Vector3f& rPos) const;

        bool mIsValid;               // 0x00
        sead::Vector3f mPos0;        // 0x04
        sead::Vector3f mPos1;        // 0x10
        sead::Vector3f mPos2;        // 0x1C
        sead::Vector3f mNormal;      // 0x28
        bool mIsHit;                 // 0x34
        f32 mHitDepth;               // 0x38
        sead::Vector3f mHitPos;      // 0x3C
        f32 mHitRate;                // 0x48
    };

    static_assert(sizeof(Triangle) == 0x4C, "unittest::Triangle size");

}  // namespace unittest

/**
 * Fixed-size soup of test triangles.
 */
class TestTriangleSoup {
public:
    TestTriangleSoup();

    void appendTriangle(const sead::Vector3f& rPos0, const sead::Vector3f& rPos1,
                        const sead::Vector3f& rPos2);
    void appendQuad(const sead::Vector3f& rPos0, const sead::Vector3f& rPos1,
                    const sead::Vector3f& rPos2, const sead::Vector3f& rPos3);

    s32 mNumTriangles;                  // 0x0000
    unittest::Triangle mTriangles[64];  // 0x0004
    s32 _1304;                          // 0x1304
    s32 _1308;                          // 0x1308
    s32 _130C;                          // 0x130C
};

static_assert(sizeof(TestTriangleSoup) == 0x1310, "TestTriangleSoup size");
