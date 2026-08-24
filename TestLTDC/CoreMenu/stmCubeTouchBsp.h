#ifndef STM_CUBE_TOUCH_BSP_H
#define STM_CUBE_TOUCH_BSP_H
#include "ResistiveTouchScreen.h"

/**
 * A touch screen interface for tcMenu mapping in the BSP STM32Cube HAL library functions.
 */
class StBspTouchInterrogator : public iotouch::TouchInterrogator {
private:
    int width, height;
public:
    StBspTouchInterrogator(int wid, int hei);
    void init();
    ~StBspTouchInterrogator() override = default;
    iotouch::TouchState internalProcessTouch(float *ptrX, float *ptrY, const iotouch::TouchOrientationSettings& rotation,
                                             const iotouch::CalibrationHandler& calib) override;
};

#endif


