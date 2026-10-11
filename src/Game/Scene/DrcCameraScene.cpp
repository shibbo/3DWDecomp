#include "Scene/DrcCameraScene.hpp"

#include <framework/seadFramework.h>
#include <gfx/seadFrameBuffer.h>
#include <gfx/seadViewport.h>

#include "Library/Controller/InputFunction.hpp"
#include "Library/System/SystemKit.hpp"
#include "System/Application.hpp"

DrcCameraScene::DrcCameraScene() : al::Scene("DrcCameraScene") {}

DrcCameraScene::~DrcCameraScene() {
    termDrcCamera();
}

void DrcCameraScene::termDrcCamera() {}

void DrcCameraScene::appear() {
    al::Scene::appear();
    openDrcCamera();
}

void DrcCameraScene::openDrcCamera() {}

void DrcCameraScene::kill() {
    closeDrcCamera();
    al::Scene::kill();
}

void DrcCameraScene::closeDrcCamera() {}

void DrcCameraScene::init(const al::SceneInitInfo& rInfo) {
    mMainViewport = new sead::Viewport(
        *Application::instance()->getFramework()->getMethodFrameBuffer(6));
    mSubViewport = new sead::Viewport(
        *Application::instance()->getFramework()->getMethodFrameBuffer(9));
    initDrcCamera();
}

void DrcCameraScene::initDrcCamera() {}

void DrcCameraScene::control() {
    if (al::isPadTriggerA(al::getMainControllerPort())) {
        openDrcCamera();
    } else if (al::isPadTriggerB(al::getMainControllerPort())) {
        closeDrcCamera();
    }
    updateDrcCamera();
}

void DrcCameraScene::updateDrcCamera() {}

void DrcCameraScene::drawMain_() const {
    alSystemKitFunction::applyViewportTop(*mMainViewport);
}

void DrcCameraScene::drawSub_() const {}
