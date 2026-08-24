
#include "frameBufferSetup.h"
#include "main.h"

#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200)


void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef* hsdram) {
    FMC_SDRAM_CommandTypeDef command = {0};

    /*
     * Step 1: Configure a clock configuration enable command.
     */
    command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = 0;

    if (HAL_SDRAM_SendCommand(hsdram, &command, HAL_MAX_DELAY) != HAL_OK) {
        Error_Handler();
    }

    /*
     * Step 2: Insert delay.
     * SDRAM requires at least 100 us after clock enable.
     */
    HAL_Delay(1);

    /*
     * Step 3: Precharge all command.
     */
    command.CommandMode = FMC_SDRAM_CMD_PALL;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = 0;

    if (HAL_SDRAM_SendCommand(hsdram, &command, HAL_MAX_DELAY) != HAL_OK) {
        Error_Handler();
    }

    /*
     * Step 4: Auto-refresh command.
     */
    command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
    command.AutoRefreshNumber = 8;
    command.ModeRegisterDefinition = 0;

    if (HAL_SDRAM_SendCommand(hsdram, &command, HAL_MAX_DELAY) != HAL_OK) {
        Error_Handler();
    }

    /*
     * Step 5: Program the external memory mode register.
     */
    uint32_t modeRegister = SDRAM_MODEREG_BURST_LENGTH_1 |
        SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
        SDRAM_MODEREG_CAS_LATENCY_3 |
        SDRAM_MODEREG_OPERATING_MODE_STANDARD |
        SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK2;
    command.AutoRefreshNumber = 1;
    command.ModeRegisterDefinition = modeRegister;

    if (HAL_SDRAM_SendCommand(hsdram, &command, HAL_MAX_DELAY) != HAL_OK) {
        Error_Handler();
    }

    /*
     * Step 6: Program refresh rate.
     *
     * Your current clock setup gives:
     * SYSCLK = 72 MHz
     * FMC SDRAM clock = HCLK / 2 = 36 MHz
     *
     * Refresh count = 64 ms / 4096 rows * 36 MHz - 20
     *               ~= 542
     */
    if (HAL_SDRAM_ProgramRefreshRate(hsdram, 542) != HAL_OK) {
        Error_Handler();
    }
}

void FrameBuffer_EnableLTDC(LTDC_HandleTypeDef* hltdc)
{
    /*
     * Layer 0 in HAL_LTDC_ConfigLayer(..., 0) is LTDC_LAYER_1.
     * Enable the layer, enable the LTDC peripheral, then reload shadow registers.
     */
    __HAL_LTDC_LAYER_ENABLE(hltdc, LTDC_LAYER_1);
    __HAL_LTDC_ENABLE(hltdc);

    if (HAL_LTDC_Reload(hltdc, LTDC_RELOAD_IMMEDIATE) != HAL_OK) {
        Error_Handler();
    }
}

static void LCD_Select(void)
{
    HAL_GPIO_WritePin(CSX_GPIO_Port, CSX_Pin, GPIO_PIN_RESET);
}

static void LCD_Unselect(void)
{
    HAL_GPIO_WritePin(CSX_GPIO_Port, CSX_Pin, GPIO_PIN_SET);
}

static void LCD_WriteCommand(SPI_HandleTypeDef* hspi, uint8_t command)
{
    HAL_GPIO_WritePin(WRX_DCX_GPIO_Port, WRX_DCX_Pin, GPIO_PIN_RESET);
    LCD_Select();
    HAL_SPI_Transmit(hspi, &command, 1, HAL_MAX_DELAY);
    LCD_Unselect();
}

static void LCD_WriteData(SPI_HandleTypeDef* hspi, const uint8_t* data, uint16_t size)
{
    HAL_GPIO_WritePin(WRX_DCX_GPIO_Port, WRX_DCX_Pin, GPIO_PIN_SET);
    LCD_Select();
    HAL_SPI_Transmit(hspi, (uint8_t*)data, size, HAL_MAX_DELAY);
    LCD_Unselect();
}

static void LCD_WriteData8(SPI_HandleTypeDef* hspi, uint8_t data)
{
    LCD_WriteData(hspi, &data, 1);
}

void LCD_ILI9341_Init(SPI_HandleTypeDef* hspi)
{
    /*
     * Keep LCD read disabled.
     */
    HAL_GPIO_WritePin(RDX_GPIO_Port, RDX_Pin, GPIO_PIN_SET);

    /*
     * Deselect LCD initially.
     */
    HAL_GPIO_WritePin(CSX_GPIO_Port, CSX_Pin, GPIO_PIN_SET);

    /*
     * Reset LCD controller.
     */
    HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(120);

    /*
     * Software reset.
     */
    LCD_WriteCommand(hspi, 0x01);
    HAL_Delay(120);

    /* Power control A (0xCB) */
    LCD_WriteCommand(hspi, 0xCB);
    { uint8_t d[] = {0x39, 0x2C, 0x00, 0x34, 0x02}; LCD_WriteData(hspi, d, sizeof(d)); }

    /* Power control B (0xCF) */
    LCD_WriteCommand(hspi, 0xCF);
    { uint8_t d[] = {0x00, 0xC1, 0x30}; LCD_WriteData(hspi, d, sizeof(d)); }

    /* Driver timing control A (0xE8) */
    LCD_WriteCommand(hspi, 0xE8);
    { uint8_t d[] = {0x85, 0x00, 0x78}; LCD_WriteData(hspi, d, sizeof(d)); }

    /* Driver timing control B (0xEA) */
    LCD_WriteCommand(hspi, 0xEA);
    { uint8_t d[] = {0x00, 0x00}; LCD_WriteData(hspi, d, sizeof(d)); }

    /* Power on sequence control (0xED) */
    LCD_WriteCommand(hspi, 0xED);
    { uint8_t d[] = {0x64, 0x03, 0x12, 0x81}; LCD_WriteData(hspi, d, sizeof(d)); }

    /* Pump ratio control (0xF7) */
    LCD_WriteCommand(hspi, 0xF7);
    LCD_WriteData8(hspi, 0x20);

    /* Power control 1 (0xC0) */
    LCD_WriteCommand(hspi, 0xC0);
    LCD_WriteData8(hspi, 0x23);

    /* Power control 2 (0xC1) */
    LCD_WriteCommand(hspi, 0xC1);
    LCD_WriteData8(hspi, 0x10);

    /* VCOM control 1 (0xC5) */
    LCD_WriteCommand(hspi, 0xC5);
    { uint8_t d[] = {0x3E, 0x28}; LCD_WriteData(hspi, d, sizeof(d)); }

    /* VCOM control 2 (0xC7) */
    LCD_WriteCommand(hspi, 0xC7);
    LCD_WriteData8(hspi, 0x86);

    /*
     * Memory access control (0x36).
     * Orientation to match the 240x320 LTDC layer.
     */
    LCD_WriteCommand(hspi, 0x36);
    LCD_WriteData8(hspi, 0x48);

    /*
     * Pixel format: 16 bits/pixel (RGB565).
     */
    LCD_WriteCommand(hspi, 0x3A);
    LCD_WriteData8(hspi, 0x55);

    /*
     * RGB interface signal control (0xB0).
     * Bypass GRAM, take pixels from the RGB (LTDC) interface, use DE mode.
     */
    LCD_WriteCommand(hspi, 0xB0);
    LCD_WriteData8(hspi, 0xC2);

    /*
     * Frame rate control (0xB1).
     */
    LCD_WriteCommand(hspi, 0xB1);
    { uint8_t d[] = {0x00, 0x18}; LCD_WriteData(hspi, d, sizeof(d)); }

    /*
     * Display function control (0xB6).
     * Configure for RGB interface + DE mode.
     */
    LCD_WriteCommand(hspi, 0xB6);
    { uint8_t d[] = {0x0A, 0xA7, 0x27, 0x04}; LCD_WriteData(hspi, d, sizeof(d)); }

    /*
     * Column / Page address sets (0x2A / 0x2B): full 240x320.
     */
    LCD_WriteCommand(hspi, 0x2A);
    { uint8_t d[] = {0x00, 0x00, 0x00, 0xEF}; LCD_WriteData(hspi, d, sizeof(d)); }

    LCD_WriteCommand(hspi, 0x2B);
    { uint8_t d[] = {0x00, 0x00, 0x01, 0x3F}; LCD_WriteData(hspi, d, sizeof(d)); }

    /*
     * Interface control (0xF6).
     * This is the critical register that selects the RGB interface data path.
     * Byte2 = 0x06 -> use system interface for commands, RGB interface for pixels.
     */
    LCD_WriteCommand(hspi, 0xF6);
    { uint8_t d[] = {0x01, 0x00, 0x06}; LCD_WriteData(hspi, d, sizeof(d)); }

    /*
     * Set image function (0xE9): disable 24-bit data bus.
     */
    LCD_WriteCommand(hspi, 0xE9);
    LCD_WriteData8(hspi, 0x00);

    /* Enable 3-gamma (0xF2) */
    LCD_WriteCommand(hspi, 0xF2);
    LCD_WriteData8(hspi, 0x00);

    /* Gamma set (0x26) */
    LCD_WriteCommand(hspi, 0x26);
    LCD_WriteData8(hspi, 0x01);

    /* Positive gamma correction (0xE0) */
    LCD_WriteCommand(hspi, 0xE0);
    {
        uint8_t d[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                       0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
        LCD_WriteData(hspi, d, sizeof(d));
    }

    /* Negative gamma correction (0xE1) */
    LCD_WriteCommand(hspi, 0xE1);
    {
        uint8_t d[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                       0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
        LCD_WriteData(hspi, d, sizeof(d));
    }

    /*
     * Sleep out.
     */
    LCD_WriteCommand(hspi, 0x11);
    HAL_Delay(120);

    /*
     * Display ON.
     */
    LCD_WriteCommand(hspi, 0x29);
    HAL_Delay(20);
}