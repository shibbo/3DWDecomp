#include "Library/Camera/CameraDistanceCurve.hpp"

#include "Library/Math/MathUtil.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace al {
namespace {
const CameraDistanceAtPoint sPoints0[] = {{1.0f, -60.0f}, {285.0f, -25.0f}, {775.0f, 0.0f}, {1100.0f, 25.0f}, {1400.0f, 60.0f}};
const CameraDistanceAtPoint sPoints1[] = {{1.0f, -60.0f}, {285.0f, -25.0f}, {780.0f, 0.0f}, {1350.0f, 25.0f}, {1800.0f, 60.0f}};
const CameraDistanceAtPoint sPoints2[] = {{400.0f, -30.0f}, {500.0f, -15.0f}, {800.0f, 0.0f}, {1100.0f, 25.0f}, {1200.0f, 60.0f}};
const CameraDistanceAtPoint sPoints3[] = {{300.0f, -60.0f}, {600.0f, -25.0f}, {900.0f, 0.0f}, {1100.0f, 25.0f}, {1400.0f, 60.0f}};
const CameraDistanceAtPoint sPoints4[] = {{300.0f, -60.0f}, {600.0f, -25.0f}, {1000.0f, 0.0f}, {1350.0f, 25.0f}, {1800.0f, 60.0f}};
const CameraDistanceAtPoint sPoints5[] = {{300.0f, -60.0f}, {600.0f, -25.0f}, {1100.0f, 0.0f}, {1600.0f, 25.0f}, {2200.0f, 60.0f}};
const CameraDistanceAtPoint sPoints6[] = {{53.0f, -60.0f}, {245.0f, -25.0f}, {450.0f, 0.0f}, {650.0f, 25.0f}, {1050.0f, 89.0f}};
const CameraDistanceAtPoint sPoints7[] = {{190.0f, -35.0f}, {450.0f, -15.0f}, {700.0f, 0.0f}, {750.0f, 20.0f}, {1350.0f, 60.0f}};
const CameraDistanceAtPoint sPoints8[] = {{1.0f, -60.0f}, {270.0f, -25.0f}, {600.0f, 0.0f}, {1000.0f, 25.0f}, {1500.0f, 60.0f}};
const CameraDistanceAtPoint sPoints9[] = {{200.0f, -35.0f}, {450.0f, -15.0f}, {800.0f, 0.0f}, {900.0f, 20.0f}, {1500.0f, 60.0f}};
const CameraDistanceAtPoint sPoints10[] = {{1.0f, -60.0f}, {285.0f, -25.0f}, {780.0f, 0.0f}, {1600.0f, 25.0f}, {2200.0f, 60.0f}};
const CameraDistanceAtPoint sPoints11[] = {{200.0f, -35.0f}, {450.0f, -15.0f}, {1100.0f, 0.0f}, {2200.0f, 20.0f}, {3000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints12[] = {{350.0f, -15.0f}, {350.0f, 60.0f}};
const CameraDistanceAtPoint sPoints13[] = {{1400.0f, -15.0f}, {1400.0f, 60.0f}};
const CameraDistanceAtPoint sPoints14[] = {{1800.0f, -15.0f}, {1800.0f, 60.0f}};
const CameraDistanceAtPoint sPoints15[] = {{2100.0f, -15.0f}, {2100.0f, 60.0f}};
const CameraDistanceAtPoint sPoints16[] = {{2500.0f, -15.0f}, {2500.0f, 60.0f}};
const CameraDistanceAtPoint sPoints17[] = {{2800.0f, -15.0f}, {2800.0f, 60.0f}};
const CameraDistanceAtPoint sPoints18[] = {{3000.0f, -15.0f}, {3000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints19[] = {{3500.0f, -15.0f}, {3500.0f, 60.0f}};
const CameraDistanceAtPoint sPoints20[] = {{4000.0f, -15.0f}, {4000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints21[] = {{8000.0f, -15.0f}, {8000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints22[] = {{600.0f, -15.0f}, {1100.0f, 10.0f}, {1600.0f, 40.0f}, {2000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints23[] = {{1100.0f, -15.0f}, {1400.0f, 0.0f}, {1800.0f, 20.0f}, {2000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints24[] = {{1600.0f, -15.0f}, {1800.0f, 0.0f}, {1800.0f, 20.0f}, {2000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints25[] = {{400.0f, -15.0f}, {400.0f, 0.0f}, {1600.0f, 20.0f}, {2300.0f, 60.0f}};
const CameraDistanceAtPoint sPoints26[] = {{500.0f, -15.0f}, {800.0f, 0.0f}, {1200.0f, 20.0f}, {3000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints27[] = {{1800.0f, -15.0f}, {2000.0f, 0.0f}, {2300.0f, 20.0f}, {2600.0f, 60.0f}};
const CameraDistanceAtPoint sPoints28[] = {{500.0f, -15.0f}, {800.0f, 0.0f}, {2100.0f, 20.0f}, {2900.0f, 60.0f}};
const CameraDistanceAtPoint sPoints29[] = {{1000.0f, -15.0f}, {1000.0f, 0.0f}, {2500.0f, 20.0f}, {3500.0f, 60.0f}};
const CameraDistanceAtPoint sPoints30[] = {{1000.0f, -15.0f}, {2000.0f, 0.0f}, {2100.0f, 20.0f}, {2900.0f, 60.0f}};
const CameraDistanceAtPoint sPoints31[] = {{1800.0f, -15.0f}, {2000.0f, 20.0f}, {2800.0f, 60.0f}};
const CameraDistanceAtPoint sPoints32[] = {{1000.0f, -15.0f}, {1400.0f, 0.0f}, {1600.0f, 20.0f}, {2400.0f, 60.0f}};
const CameraDistanceAtPoint sPoints33[] = {{1000.0f, -15.0f}, {2400.0f, 0.0f}, {3200.0f, 20.0f}, {4000.0f, 60.0f}};
const CameraDistanceAtPoint sPoints34[] = {{1000.0f, -15.0f}, {1400.0f, 0.0f}, {1600.0f, 20.0f}, {1900.0f, 60.0f}};
const CameraDistanceAtPoint sPoints35[] = {{800.0f, 0.0f}, {1200.0f, 20.0f}, {2000.0f, 60.0f}};

CameraDistanceCurve sCurves[] = {
    {"SingleMode", sPoints0, 5},
    {"MidSingleMode", sPoints1, 5},
    {"CloseSingleMode", sPoints2, 5},
    {"SingleModeOffsetY3m", sPoints3, 5},
    {"SingleModeOffsetY3mMid", sPoints4, 5},
    {"SingleModeOffsetY3mFar", sPoints5, 5},
    {"Near Distance", sPoints6, 5},
    {"Near Distance [no zoom]", sPoints7, 5},
    {"Medium Distance", sPoints8, 5},
    {"Medium Distance [no zoom]", sPoints9, 5},
    {"Far Distance", sPoints10, 5},
    {"Very Far Distance", sPoints11, 5},
    {"Super Close A[5m Fixed]", sPoints12, 2},
    {"14m Fixed", sPoints13, 2},
    {"18m Fixed", sPoints14, 2},
    {"21m Fixed", sPoints15, 2},
    {"25m Fixed", sPoints16, 2},
    {"28m Fixed", sPoints17, 2},
    {"30m Fixed", sPoints18, 2},
    {"35m Fixed", sPoints19, 2},
    {"40m Fixed", sPoints20, 2},
    {"80m Fixed", sPoints21, 2},
    {"Medium A[around 13m]", sPoints22, 4},
};

CameraDistanceCurve sExtraCurves[] = {
    {"Long A[around 18m]", sPoints23, 4},
    {"Long B[16m-20m]", sPoints24, 4},
    {"For Tutorial [4m-23m]", sPoints25, 4},
    {"福笑い用", sPoints26, 4},
    {"カエル用", sPoints27, 4},
    {"ロケットフラワー用", sPoints28, 4},
    {"ボスマグマ用[10m～35m]", sPoints29, 4},
    {"徘徊ボス用", sPoints30, 4},
    {"Tレックス用", sPoints31, 3},
    {"クッパ用", sPoints32, 4},
    {"クッパコウラ用", sPoints33, 4},
    {"崩落クッパ用", sPoints34, 4},
    {"For Video", sPoints35, 3},
};
}  // namespace

CameraDistanceCurve::CameraDistanceCurve(const char* pName, const CameraDistanceAtPoint* pPoints,
                                         s32 pointNum)
    : mName(pName), mPoints(pPoints), mPointNum(pointNum) {}

const CameraDistanceCurve* CameraDistanceCurve::getDefaultCurve() {
    return &sCurves[8];
}

const CameraDistanceCurve* CameraDistanceCurve::findOrDefaultCurve(const ByamlIter& rIter) {
    const char* name = tryGetByamlKeyStringOrNULL(rIter, "DistanceCurveName");

    if (name == nullptr) {
        return &sExtraCurves[0];
    }

    for (s32 i = 0; i < 23; i++) {
        if (isEqualString(sCurves[i].getName(), name)) {
            return &sCurves[i];
        }
    }

    for (s32 i = 0; i < 13; i++) {
        if (isEqualString(sExtraCurves[i].getName(), name)) {
            return &sExtraCurves[i];
        }
    }

    return &sExtraCurves[0];
}

const CameraDistanceCurve* CameraDistanceCurve::getRocketFlowerCurve() {
    return &sExtraCurves[5];
}

const CameraDistanceCurve* CameraDistanceCurve::getGiantWanderBossCurve() {
    return &sExtraCurves[7];
}

const CameraDistanceCurve* CameraDistanceCurve::getKoopaShellCurve() {
    return &sExtraCurves[10];
}

const CameraDistanceCurve* CameraDistanceCurve::getTRexPatrolCurve() {
    return &sCurves[10];
}

f32 CameraDistanceCurve::calcDistance(f32 angleV) const {
    if (angleV < mPoints[0].angleV) {
        return mPoints[0].distance;
    }

    for (s32 i = 1; i < mPointNum; i++) {
        if (isNearZero(mPoints[i].angleV - angleV, 0.001f)) {
            return mPoints[i].distance;
        }

        if (angleV < mPoints[i].angleV) {
            f32 rate = normalize(angleV, mPoints[i - 1].angleV, mPoints[i].angleV);
            return lerpValue(rate, mPoints[i - 1].distance, mPoints[i].distance);
        }
    }

    return mPoints[mPointNum - 1].distance;
}

}  // namespace al
