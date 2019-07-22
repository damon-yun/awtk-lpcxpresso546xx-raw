/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_lcdc.h"
#include "fsl_i2c.h"
#include "fsl_ft5406.h"

#include <stdio.h>

#include "pin_mux.h"
#include "fsl_ctimer.h"
#include "fsl_sctimer.h"
#include "base/g2d.h"
#include "base/lcd.h"
#include "tkc/mem.h"


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_LCD LCD
#define LCD_PANEL_CLK 9000000
#define LCD_PPL 480
#define LCD_HSW 2
#define LCD_HFP 8
#define LCD_HBP 43
#define LCD_LPP 272
#define LCD_VSW 10
#define LCD_VFP 4
#define LCD_VBP 12
#define LCD_POL_FLAGS kLCDC_InvertVsyncPolarity | kLCDC_InvertHsyncPolarity
#define IMG_HEIGHT 272
#define IMG_WIDTH 480
#define LCD_INPUT_CLK_FREQ CLOCK_GetFreq(kCLOCK_LCD)
#define APP_LCD_IRQHandler LCD_IRQHandler
#define APP_LCD_IRQn LCD_IRQn

#define EXAMPLE_I2C_MASTER_BASE (I2C2_BASE)
#define I2C_MASTER_CLOCK_FREQUENCY (12000000)

#define EXAMPLE_I2C_MASTER ((I2C_Type *)EXAMPLE_I2C_MASTER_BASE)
#define I2C_MASTER_SLAVE_ADDR_7BIT 0x7EU
#define I2C_BAUDRATE 100000U


#define LCD_VRAM0    0xA0A00000
#define LCD_VRAM1    0xA0A3FC00


uint8_t *online_fb_addr = (uint8_t*)LCD_VRAM0;
uint8_t *offline_fb_addr = (uint8_t*)LCD_VRAM1;


extern int gui_app_start(int lcd_w, int lcd_h);
extern status_t APP_Touch_Init(void);




/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Implementation of communication with the touch controller
 ******************************************************************************/

/* Touch driver handle. */
static ft5406_handle_t touch_handle;

status_t APP_Touch_Init(void)
{
    status_t status;
    i2c_master_config_t masterConfig;
    gpio_pin_config_t pin_config = {
        kGPIO_DigitalOutput,
        0,
    };

    I2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Bps = I2C_BAUDRATE;

    /* Initialize the I2C master peripheral */
    I2C_MasterInit(EXAMPLE_I2C_MASTER, &masterConfig, I2C_MASTER_CLOCK_FREQUENCY);

    /* Enable touch panel controller */
    GPIO_PinInit(GPIO, 2, 27, &pin_config);
    GPIO_PinWrite(GPIO, 2, 27, 1);

    /* Initialize touch panel controller */
    status = FT5406_Init(&touch_handle, EXAMPLE_I2C_MASTER);
    if (status != kStatus_Success)
    {
        PRINTF("Touch panel init failed\n");
    }
    assert(status == kStatus_Success);

    return kStatus_Success;
}

int BOARD_Touch_Poll2(int *pX, int *pY, int *pPressFlg)
{
    touch_event_t touch_event;
    int touch_x = -1;
    int touch_y = -1;

    if (kStatus_Success != FT5406_GetSingleTouch(&touch_handle, &touch_event, &touch_x, &touch_y))
    {
        APP_Touch_Init();
        return 0;
    }
    else if (touch_event != kTouch_Reserved)
    {
        *pX = touch_y;
        *pY = touch_x;
        *pPressFlg = ((touch_event == kTouch_Down) || (touch_event == kTouch_Contact));
        return 1;
    }
    return 0;
}

static void BOARD_InitPWM(void)
{
    sctimer_config_t config;
    sctimer_pwm_signal_param_t pwmParam;
    uint32_t event;

    CLOCK_AttachClk(kMAIN_CLK_to_SCT_CLK);

    CLOCK_SetClkDiv(kCLOCK_DivSctClk, 2, true);

    SCTIMER_GetDefaultConfig(&config);

    SCTIMER_Init(SCT0, &config);

    pwmParam.output           = kSCTIMER_Out_5;
    pwmParam.level            = kSCTIMER_HighTrue;
    pwmParam.dutyCyclePercent = 5;

    SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 1000U, CLOCK_GetFreq(kCLOCK_Sct), &event);
}

extern int spifi_flash_init(void);

/*!
 * @brief Main function
 */
int main(void)
{
    lcdc_config_t lcdConfig;
    
    /* attach 12 MHz clock to FLEXCOMM0 (debug console) */
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* Route Main clock to LCD. */
    CLOCK_AttachClk(kMAIN_CLK_to_LCD_CLK);

    /* attach 12 MHz clock to FLEXCOMM2 (I2C master for touch controller) */
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

    CLOCK_EnableClock(kCLOCK_Gpio2);

    CLOCK_SetClkDiv(kCLOCK_DivLcdClk, 1, true);

    BOARD_InitPins();
    BOARD_SPIFIInitPins();
    BOARD_BootClockPLL180M();
    BOARD_InitDebugConsole();
    spifi_flash_init();
    BOARD_InitSDRAM();

    /* Set the back light PWM. */
    BOARD_InitPWM();
    
    APP_Touch_Init();
    
    SysTick_Config(SystemCoreClock/1000);

    LCDC_GetDefaultConfig(&lcdConfig);

    lcdConfig.panelClock_Hz  = LCD_PANEL_CLK;
    lcdConfig.ppl            = LCD_PPL;
    lcdConfig.hsw            = LCD_HSW;
    lcdConfig.hfp            = LCD_HFP;
    lcdConfig.hbp            = LCD_HBP;
    lcdConfig.lpp            = LCD_LPP;
    lcdConfig.vsw            = LCD_VSW;
    lcdConfig.vfp            = LCD_VFP;
    lcdConfig.vbp            = LCD_VBP;
    lcdConfig.polarityFlags  = LCD_POL_FLAGS;
    lcdConfig.upperPanelAddr = (uint32_t)LCD_VRAM0;
    lcdConfig.bpp            = kLCDC_16BPP565;
    lcdConfig.display        = kLCDC_DisplayTFT;
    lcdConfig.swapRedBlue    = false;
    
    LCDC_Init(APP_LCD, &lcdConfig, LCD_INPUT_CLK_FREQ);

//    LCDC_SetPalette(APP_LCD, palette, ARRAY_SIZE(palette));

//    LCDC_EnableInterrupts(APP_LCD, kLCDC_BaseAddrUpdateInterrupt);

    LCDC_Start(APP_LCD);
    LCDC_PowerUp(APP_LCD);
    
    gui_app_start(IMG_WIDTH, IMG_HEIGHT);
    while (1)
    {

    }
}
