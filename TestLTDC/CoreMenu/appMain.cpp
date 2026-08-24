#include "appMain.h"
#include "main.h"
#include <TaskManagerIO.h>
#include <IoLogging.h>
#include "FrameBufferDrawable.h"
#include <Fonts/RobotoMedium18.h>
#include "TestLTDC_menu.h"

StmDMA2dAdafruitFrameBuffer16 frameBuffer(reinterpret_cast<uint16_t*>(0xD0000000), 240, 320);

// Declaring any arrays used by enum/list items
const char* strStatusEnumEntries[] = { "Standby", "Starting", "Warm-up", "Running", "Protect" };

void buildMenu(TcMenuBuilder& builder) {
    builder.usingDynamicEEPROMStorage()
        .analogBuilder(MENU_LINE_VOLTS_ID, "Line Volts", DONT_SAVE, NoMenuFlags, 0, nullptr)
            .offset(-100).divisor(10).step(1).maxValue(255).unit("V").endItem()
        .enumItem(MENU_STATUS_ID, "Status", DONT_SAVE, strStatusEnumEntries, 5, NoMenuFlags, 0, nullptr)
        .analogBuilder(MENU_COUNT_ID, "Count", DONT_SAVE, MenuFlags().readOnly(), 0, nullptr)
            .offset(0).divisor(1).step(1).maxValue(255).unit("tms").endItem()
        .actionItem(MENU_ADD_ID, "Add", NoMenuFlags, tcAdded_onAddToCount);
}

bool pinState = false;

static void bumpTheCount(pinid_t which, bool held) {
    getMenuCount().setCurrentValue(getMenuCount().getCurrentValue() + 1); // Added by generator for the count example

}

static void setup() {

    serlogF(SER_DEBUG, "Starting up LTDC demo");
    appendIoaPin(StmGpioDesc(LD3_GPIO_Port, LD3_Pin, 1));
    appendIoaPin(StmGpioDesc(B1_GPIO_Port, B1_Pin, 2));
    internalDigitalDevice().digitalWriteS(1, HIGH);

    setupMenu();

    switches.initialise(internalDigitalIo(), false);
    switches.addSwitch(2, bumpTheCount);

    taskManager.schedule(repeatMillis(200), [] {
        //HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        internalDigitalDevice().digitalWriteS(1, pinState);
        pinState = !pinState;
    });

    taskManager.schedule(repeatMillis(100), [] {
        auto& voltsMenu = getMenuLineVolts();
        voltsMenu.setCurrentValue((voltsMenu.getCurrentValue() + 1)%voltsMenu.getMaximumValue());
        getMenuStatus().setCurrentValue(rand()%5);
    });

    setTitlePressedCallback([](int id) {
       showVersionDialog(&applicationInfo);
    });

    serlogF(SER_DEBUG, "Started up");
}

void appMain() {
    setup();
    for (;;) {
        taskManager.runLoop();
    }
}

void CALLBACK_FUNCTION tcAdded_onAddToCount(int id) {
    getMenuCount().setCurrentValue(getMenuCount().getCurrentValue() + 1); // Added by generator for the count example
}

// ---------------------- IMPORTANT ---------------------
// Some required functions you need to implement.

//
// TaskManagerIO needs to know the milliseconds since start, this is the easiest method.
//
uint32_t millis(void) {
    return HAL_GetTick();
}

//
// TaskManagerIO also needs the microseconds since start, this is one such method to achieve that.
// In this method start timer 2 on a 1 microsecond prescaler. Then when it reaches maximum value,
// reset it. The reset code is further down, the timer setup is in STM code.
//
extern TIM_HandleTypeDef htim2;
uint32_t micros(void) {
    return __HAL_TIM_GET_COUNTER(&htim2);
}

//
// TaskManagerIO needs a way of reliably yielding in spin loops, up to you you could maybe leave empty.
//
void yield(void) {
    osDelay(0);
}

//
// TcMenuLog needs a UART that it can log to, you can disable logging if not required.
//
extern UART_HandleTypeDef huart1;
UART_HandleTypeDef* loggingUart = &huart1;

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM6)
    {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}


/**
 * Here I have overridden the default interrupt handler for the GPIO pin. I just call into the IoAbstraction
 * intHasTriggered function that will work out if IoAbstraction has anything to do.
 * @param pin
 */
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    stmIntHasTriggered(pin);
}
