
#include "appMain.h"
#include "main.h"
#include "TaskManagerIO.h"
#include "stmCube/StmCubeDigital.h"
#include "TestOledEth_menu.h"
#include "NTPTimeEvent.h"
#include <time.h>

#include "lwip.h"
#include "ScreenSaverCustomDrawing.h"

extern RTC_HandleTypeDef hrtc;

//
// Here we declare a screen saver custom drawing class that we'll later attach to
// the renderer to use whenever the system is idle. See ScreenSaverCustomDrawing.h
// https://www.thecoderscorner.com/products/arduino-libraries/tc-menu/renderer-take-over-display/
//
static ScreenSaverCustomDrawing menuScreenSaver;

// We've used a scroll choice item that is defined as fixed width in RAM. Here's the
// actual values defined that we'll use. Notice they are not const, so can be changed
// at runtime.
// https://www.thecoderscorner.com/products/arduino-libraries/tc-menu/menu-item-types/scrollchoice-menu-item/
//
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

class ClockNtpTimeEvent : public NTPTimeEvent {
public:

    ClockNtpTimeEvent() : NTPTimeEvent("2.pool.ntp.org", 123) {}

    void exec() override {
        // Convert Unix timestamp to RTC date/time
        time_t timestamp = _presentValue;
        tm* timeinfo = gmtime(&timestamp);

        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};

        sTime.Hours = timeinfo->tm_hour;
        sTime.Minutes = timeinfo->tm_min;
        sTime.Seconds = timeinfo->tm_sec;
        sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sTime.StoreOperation = RTC_STOREOPERATION_RESET;

        sDate.Year = timeinfo->tm_year - 100; // Years since 2000
        sDate.Month = timeinfo->tm_mon + 1;
        sDate.Date = timeinfo->tm_mday;
        sDate.WeekDay = timeinfo->tm_wday == 0 ? RTC_WEEKDAY_SUNDAY : timeinfo->tm_wday;

        HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

        taskManager.schedule(repeatSeconds(1), [] {
            RTC_TimeTypeDef lTime = {0};
            RTC_DateTypeDef lDate = {0};

            HAL_RTC_GetDate(&hrtc, &lDate, RTC_FORMAT_BIN);
            HAL_RTC_GetTime(&hrtc, &lTime, RTC_FORMAT_BIN);
            const TimeStorage theTime(lTime.Hours, lTime.Minutes, lTime.Seconds);
            const DateStorage theDate(lDate.Date, lDate.Month, lDate.Year);
            auto& dateItem = getMenuDate();
            auto& timeItem = getMenuTimeNow();
            dateItem.setDate(theDate);
            dateItem.setChanged(true);
            timeItem.setTime(theTime);
            timeItem.setChanged(true);
        });

        setCompleted(true);
    }
};


static void setup() {
    MX_LWIP_Init();

    // CS = 1/PF13, DC=3/PD15, RST=2/PF12
    appendIoaPin(StmGpioDesc(GPIOF, GPIO_PIN_13, 1));
    appendIoaPin(StmGpioDesc(GPIOF, GPIO_PIN_12, 2));
    appendIoaPin(StmGpioDesc(GPIOD, GPIO_PIN_15, 3));
    // rotary encoder mappings A=8, B=10, OK=9
    appendIoaPin(StmGpioDesc(GPIOC, GPIO_PIN_8, 8));
    appendIoaPin(StmGpioDesc(GPIOC, GPIO_PIN_9, 9));
    appendIoaPin(StmGpioDesc(GPIOC, GPIO_PIN_10, 10));

    setupMenu();

    // Here we register a custom drawing handler that will draw the screen saver
    // when the screen does a "reset". A screen reset takes place after any editing
    // to put he display back into the normal main menu start state.
    renderer.setCustomDrawingHandler(&menuScreenSaver);

    // When the main title is pressed or touched, we can register a callback to be executed.
    // Here we just present a simple dialog.
    setTitlePressedCallback([](int id) {
        showVersionDialog(&applicationInfo);
    });

    taskManager.registerEvent(new ClockNtpTimeEvent(), true);
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
