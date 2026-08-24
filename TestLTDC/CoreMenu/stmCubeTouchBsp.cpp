
#include "stmCubeTouchBsp.h"
#include "stm32f429i_discovery_ts.h"

iotouch::TouchState StBspTouchInterrogator::internalProcessTouch(float *ptrX, float *ptrY, const iotouch::TouchOrientationSettings& rotation,
                                                                 const iotouch::CalibrationHandler& calibrationHandler) {
    TS_StateTypeDef tsState;
    BSP_TS_GetState(&tsState);
    if(!tsState.TouchDetected) return iotouch::NOT_TOUCHED;

    *ptrX = calibrationHandler.calibrateX(static_cast<float>(tsState.X) / static_cast<float>(width), rotation.isXInverted());
    *ptrY = calibrationHandler.calibrateY(static_cast<float>(height - tsState.Y) / static_cast<float>(height), rotation.isYInverted());
    return iotouch::TOUCHED;
}

StBspTouchInterrogator::StBspTouchInterrogator(const int wid, const int hei) {
    width = wid;
    height = hei;
}

void StBspTouchInterrogator::init() {
    BSP_TS_Init(width, height);
}


