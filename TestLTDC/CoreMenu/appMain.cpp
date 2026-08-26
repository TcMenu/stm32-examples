/**
 * An example TcMenu application built with StmCube and web designer in initializer mode. You can read mode about web
 * designer and try it out yourself: https://designer.thecoderscorner.com/
 *
 * This application is designed to operate on the STM32F429-DISC1 board, it uses the touch screen controller and the
 * LTDC frame buffer based display controller too.
 */

#include "appMain.h"
#include "main.h"
#include <TaskManagerIO.h>
#include <IoLogging.h>
#include "FrameBufferDrawable.h"
#include "TestLTDC_menu.h"
#include "graphics/TcThemeBuilder.h"
#include "bitmapSources.h"
#include <stockIcons/wifiAndConnectionIcons16x12.h>

//
// We've used enum menu items below, these require upfront choices be defined in an array of const char* as follows
//
const char* strStatusEnumEntries[] = { "Standby", "Starting", "Warm-up", "Running", "Protect" };

// We added these menu items ourself instead of from designer, so it will not create
// the ID and get helper method, but they are trivial so we introduce them here.
constexpr int menuEngineId = 100;
constexpr int freeRamId = 101;
constexpr int minRamId = 102;
static ActionMenuItem& getMenuEngine() { return getActionItemById(menuEngineId); }
static AnalogMenuItem& getMenuFreeRam() { return getAnalogItemById(freeRamId); }
static AnalogMenuItem& getMenuMinRam() { return getAnalogItemById(minRamId); }

// This is the callback for the engine menu item
static void onEngineSel(int menuId) {
    serlogF(SER_DEBUG, "Engine selected");
}

//
// Below is the menu build out. It generates all the items during setupMenu(). Once this call is made, the entire menu
// is initialised and ready to use. You can read about menu builder:
// https://www.thecoderscorner.com/products/arduino-libraries/tc-menu/menu-item-types/fluent-menu-builder-intro/
//
void buildMenu(TcMenuBuilder& builder) {
    builder.usingDynamicEEPROMStorage()
        .analogBuilder(MENU_LINE_VOLTS_ID, "Line Volts", DONT_SAVE, NoMenuFlags, 0, nullptr)
            .offset(-100).divisor(10).step(1).maxValue(255).unit("V").endItem()
        .enumItem(MENU_STATUS_ID, "Status", DONT_SAVE, strStatusEnumEntries, 5, NoMenuFlags, 0, nullptr)
        .analogBuilder(MENU_COUNT_ID, "Count", DONT_SAVE, MenuFlags().readOnly(), 0, nullptr)
            .offset(0).divisor(1).step(1).maxValue(255).unit("tms").endItem()
        .actionItem(MENU_ADD_ID, "Add", NoMenuFlags, tcAdded_onAddToCount)
        .actionItem(menuEngineId, "Engine", NoMenuFlags, onEngineSel)
        .analogBuilder(freeRamId, "Free Heap", DONT_SAVE, MenuFlags().readOnly(), 0, nullptr)
            .offset(0).divisor(1000).step(1).maxValue(40000).unit("K").endItem()
        .analogBuilder(minRamId, "Min Heap", DONT_SAVE, MenuFlags().readOnly(), 0, nullptr)
            .offset(0).divisor(1000).step(1).maxValue(40000).unit("K").endItem()
        .endSub();
}

constexpr color_t colorPaletteButton[] = { RGB(46, 58, 69), RGB(247, 249, 251), RGB(90, 160, 243), RGB(208, 232, 255) };
static TitleWidget connectionWidget(iconsEthernetConnection, 2, 16, 12);

static void extendTheme() {
    TcThemeBuilder theme(renderer);
    theme.addingTitleWidget(connectionWidget);

    theme.menuItemOverride(getMenuEngine())
        .onRowCol(3, 1, 2)
        .withDrawingMode(GridPosition::DRAW_AS_ICON_ONLY)
        .withPalette(colorPaletteButton)
        .withImage4bpp(Coord(64,64), engineBitmap_palette0, engineBitmap0)
        .apply();

    theme.menuItemOverride(getMenuAdd())
        .onRowCol(3, 2, 2)
        .withDrawingMode(GridPosition::DRAW_AS_ICON_ONLY)
        .withPalette(colorPaletteButton)
        .withImage4bpp(Coord(64,64), plusIconBitmap_palette0, plusIconBitmap0)
        .apply();

    theme.apply();
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
    extendTheme();

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
    });

    taskManager.schedule(repeatSeconds(1), [] {
        connectionWidget.setCurrentState(connectionWidget.getCurrentState() == 0 ? 1 : 0);
        getMenuStatus().setCurrentValue(rand()%5);
        getMenuFreeRam().setFromFloatingPointValue(static_cast<float>(xPortGetFreeHeapSize()) / 1024.0F);
        getMenuMinRam().setFromFloatingPointValue(static_cast<float>(xPortGetMinimumEverFreeHeapSize()) / 1024.0F);
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
