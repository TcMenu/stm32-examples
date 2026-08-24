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
int count = 0;

void intHasTriggered() {
    count++;
}

/**
 * Here I have overridden the default interrupt handler for the GPIO pin. I just call into the IoAbstraction
 * intHasTriggered function that will work out if IoAbstraction has anything to do.
 * @param pin
 */
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    count++;
    stmIntHasTriggered(pin);
}

static void setup() {

    serlogF(SER_DEBUG, "Starting up LTDC demo");
    appendIoaPin(StmGpioDesc(LD3_GPIO_Port, LD3_Pin, 1));
    appendIoaPin(StmGpioDesc(B1_GPIO_Port, B1_Pin, 2));
    internalDigitalDevice().digitalWriteS(1, HIGH);
    internalDigitalDevice().attachInterrupt(2, intHasTriggered, CHANGE);

    setupMenu();

    taskManager.schedule(repeatMillis(200), [] {
        //HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
        internalDigitalDevice().digitalWriteS(1, pinState);
        pinState = !pinState;
    });

    taskManager.schedule(repeatMillis(100), [] {
        auto& voltsMenu = getMenuLineVolts();
        voltsMenu.setCurrentValue((voltsMenu.getCurrentValue() + 1)%voltsMenu.getMaximumValue());
        getMenuCount().setCurrentValue(count);
        getMenuStatus().setCurrentValue(rand()%5);
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