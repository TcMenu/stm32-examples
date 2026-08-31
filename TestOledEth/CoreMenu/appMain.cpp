
#include "appMain.h"
#include "main.h"
#include "TaskManagerIO.h"
#include "stmCube/StmCubeDigital.h"
#include "TestOledEth_menu.h"
//                        0123456789 0123456789 0123456789 0123456789 0123456789
static char fixedArrayFoods[] = "Pizza\0    Pasta\0    Salad\0    Curry\0    Soup\0     ";

// Declaring as extern any custom RtCalls and scroll variables

void buildMenu(TcMenuBuilder& builder) {
    builder.usingDynamicEEPROMStorage()
        .dateItem(MENU_DATE_ID, "Date", DONT_SAVE, NoMenuFlags, DateStorage(1, 1, 2020), nullptr)
        .timeItem(MENU_TIME_NOW_ID, "Time now", DONT_SAVE, NoMenuFlags, EDITMODE_TIME_24H, TimeStorage(0, 0, 0, 0), nullptr)
        .analogBuilder(MENU_COUNT_ID, "Count", DONT_SAVE, MenuFlags().readOnly(), 0, nullptr)
            .offset(0).divisor(1).step(1).maxValue(255).unit("tms").endItem()
        .actionItem(MENU_ADD_ID, "Add", NoMenuFlags, tcAdded_onAddToCount)
        .subMenu(MENU_EXTRAS_ID, "Extras", NoMenuFlags, nullptr)
            .scrollChoiceBuilder(MENU_FOODS_ID, "Foods", DONT_SAVE, NoMenuFlags, 0, nullptr).fromRamChoices(fixedArrayFoods, 5, 10).endItem()
            .largeNumberItem(MENU_LARGE_ID, "Large", ROM_SAVE, LargeFixedNumber(6, 3, 0U, 0U, false), true, NoMenuFlags, onLargeChange)
            .rgb32Item(MENU_RGB_ID, "RGB", DONT_SAVE, false, NoMenuFlags, RgbColor32(255, 255, 255), nullptr)
            .floatItem(MENU_FLOAT_ID, "Float", DONT_SAVE, 1, NoMenuFlags, 0.0, nullptr)
            .listItemRtCustom(MENU_COUNT_LIST_ID, "Count List", 10, fnCountListRtCall, NoMenuFlags, nullptr)
            .endSub();
}


static void setup() {
    // CS = 1/PF13, DC=3/PD15, RST=2/PF12
    appendIoaPin(StmGpioDesc(GPIOF, 13, 1));
    appendIoaPin(StmGpioDesc(GPIOF, 12, 2));
    appendIoaPin(StmGpioDesc(GPIOD, 15, 3));
    // rotary encoder mappings A=8, B=10, OK=9
    appendIoaPin(StmGpioDesc(GPIOC, 8, 8));
    appendIoaPin(StmGpioDesc(GPIOC, 9, 9));
    appendIoaPin(StmGpioDesc(GPIOC, 10, 10));

    setupMenu();
}

void runMenuApp() {
    setup();

    for (;;) {
        taskManager.runLoop();
    }
}


void CALLBACK_FUNCTION tcAdded_onAddToCount(int id) {
    getMenuCount().setCurrentValue(getMenuCount().getCurrentValue() + 1); // Added by generator for the count example
}


void CALLBACK_FUNCTION onLargeChange(int id) {
    auto lgeVal = getMenuLarge().getLargeNumber()->getAsFloat();
    serlogF2(SER_DEBUG, "Large num", lgeVal);
}

// This callback needs to be implemented by you, see the below docs:
//  1. List Docs - https://www.thecoderscorner.com/products/arduino-libraries/tc-menu/menu-item-types/list-menu-item/
//  2. ScrollChoice Docs - https://www.thecoderscorner.com/products/arduino-libraries/tc-menu/menu-item-types/scrollchoice-menu-item/
int CALLBACK_FUNCTION fnCountListRtCall(RuntimeMenuItem* item, uint8_t row, RenderFnMode mode, char* buffer, int bufferSize) {
    switch(mode) {
    default:
        return defaultRtListCallback(item, row, mode, buffer, bufferSize);
    }
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
extern UART_HandleTypeDef huart3;
UART_HandleTypeDef* loggingUart = &huart3;
