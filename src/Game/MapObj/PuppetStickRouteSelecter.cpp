#include "MapObj/PuppetStickRouteSelecter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Project/Block/BlockRailLink.hpp"
#include "Util/PlayerPuppetUtil.hpp"

PuppetStickRouteSelecter::PuppetStickRouteSelecter(int capacity) : mCapacity(capacity) {
    mPuppets = new IUsePlayerPuppet*[capacity];
    clearPuppetAll();
}

void PuppetStickRouteSelecter::clearPuppetAll() {
    for (int i = 0; i < mCapacity; ++i)
        mPuppets[i] = nullptr;
    mCount = 0;
}

void PuppetStickRouteSelecter::addPuppet(IUsePlayerPuppet* pPuppet) {
    for (int i = 0; i < mCount; ++i)
        if (mPuppets[i] == pPuppet)
            return;
    mPuppets[mCount] = pPuppet;
    ++mCount;
}

void PuppetStickRouteSelecter::clearPuppet(IUsePlayerPuppet* pPuppet) {
    for (int i = 0; i < mCount; ++i) {
        if (mPuppets[i] == pPuppet) {
            mPuppets[i] = nullptr;
            break;
        }
    }
    int count = 0;
    for (int i = 0; i < mCount; ++i) {
        if (mPuppets[i]) {
            if (i != count)
                mPuppets[count] = mPuppets[i];
            ++count;
        }
    }
    mCount = count;
}

bool PuppetStickRouteSelecter::compareBlockRailRoute(const al::BlockRailRider* pRider,
    const al::BlockRailLink* pLinkA, const al::BlockRailLink* pLinkB) const {
    const al::BlockRailLink* current = pRider->getRailLink();
    sead::Vector3f directionA(sead::Vector3f::ez);
    if (pLinkA->isPrevLink(current))
        pLinkA->calcDir(&directionA, 1.0f);
    else if (pLinkA->isNextLink(current)) {
        pLinkA->calcDir(&directionA, 0.0f);
        directionA = -directionA;
    }
    sead::Vector3f directionB(sead::Vector3f::ez);
    if (pLinkB->isPrevLink(current))
        pLinkB->calcDir(&directionB, 1.0f);
    else if (pLinkB->isNextLink(current)) {
        pLinkB->calcDir(&directionB, 0.0f);
        directionB = -directionB;
    }
    sead::Vector3f direction(sead::Vector3f::ez);
    pRider->calcDir(&direction);
    float alignmentA = direction.dot(directionA);
    float alignmentB = direction.dot(directionB);
    int votesA = 0;
    int votesB = 0;
    for (int i = 0; i < mCount; ++i) {
        float x = rc::getPuppetStickX(mPuppets[i]);
        float y = rc::getPuppetStickY(mPuppets[i]);
        if (x * x + y * y <= 0.09f)
            continue;
        auto* actor = al::getSensorHost(rc::getPuppetSensor(mPuppets[i]));
        sead::Vector3f cameraA;
        cameraA.setRotated(al::getCameraViewMtx(actor), directionA);
        sead::Vector3f cameraB;
        cameraB.setRotated(al::getCameraViewMtx(actor), directionB);
        sead::Vector3f stick(x, y, 0.0f);
        if (stick.dot(cameraA) > stick.dot(cameraB))
            ++votesA;
        else
            ++votesB;
    }
    return votesA == votesB ? alignmentA > alignmentB : votesA > votesB;
}
