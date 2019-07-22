/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_common.h"
#include "pwm.h"
#include "fsl_ctimer.h"

/************************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
************************************************************************************/
typedef struct _hal_pwm_handle_struct_t
{
    uint32_t pwmClock_Hz;
    uint8_t instance;
    uint8_t pwmLevelSelect;
} hal_pwm_handle_struct_t;

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/
hal_pwm_status_t HAL_PwmInit(hal_pwm_handle_t halPwmHandle, uint8_t instance, uint32_t srcClock_Hz)
{
    ctimer_config_t ctimerInfo;
    CTIMER_Type *mCtimerBase[]           = CTIMER_BASE_PTRS;
    hal_pwm_handle_struct_t *halPwmState = halPwmHandle;

    assert(halPwmHandle);
    assert(instance <= kCTIMER_Capture_2);
    assert(sizeof(hal_pwm_handle_struct_t) == HAL_PWM_HANDLE_SIZE);
    halPwmState->pwmClock_Hz = srcClock_Hz;
    halPwmState->instance    = instance;
    CTIMER_GetDefaultConfig(&ctimerInfo);
    /* Initialize CTIMER module */
    CTIMER_Init(mCtimerBase[instance], &ctimerInfo);
    return kStatus_HAL_PwmSuccess;
}

void HAL_PwmDeinit(hal_pwm_handle_t halPwmHandle)
{
    CTIMER_Type *mCtimerBase[]           = CTIMER_BASE_PTRS;
    hal_pwm_handle_struct_t *halPwmState = halPwmHandle;

    assert(halPwmHandle);
    assert(halPwmState->instance <= kCTIMER_Capture_2);
    /* DeInitialize ctimer module */
    CTIMER_Deinit(mCtimerBase[halPwmState->instance]);
}

/*! -------------------------------------------------------------------------
 * \brief  Pwm setup.
 *---------------------------------------------------------------------------*/
hal_pwm_status_t HAL_PwmSetupPwm(hal_pwm_handle_t halPwmHandle, uint8_t channel, hal_pwm_setup_config_t *setupConfig)
{
    CTIMER_Type *mCtimerBase[]           = CTIMER_BASE_PTRS;
    hal_pwm_handle_struct_t *halPwmState = halPwmHandle;

    assert(halPwmHandle);
    assert(halPwmState->instance <= kCTIMER_Capture_2);
    assert(setupConfig);
    halPwmState->pwmLevelSelect = (uint8_t)(setupConfig->level);
    if (kStatus_Success != CTIMER_SetupPwm(mCtimerBase[halPwmState->instance], (ctimer_match_t)channel,
                                           setupConfig->dutyCyclePercent, setupConfig->pwmFreq_Hz,
                                           halPwmState->pwmClock_Hz, false))
        return kStatus_HAL_PwmFail;
    CTIMER_StartTimer(mCtimerBase[halPwmState->instance]);
    return kStatus_HAL_PwmSuccess;
}

/*! -------------------------------------------------------------------------
 * \brief   update the Duty cycle of pwm
 *---------------------------------------------------------------------------*/
hal_pwm_status_t HAL_PwmUpdateDutycycle(hal_pwm_handle_t halPwmHandle,
                                        uint8_t channel,
                                        hal_pwm_mode_t mode,
                                        uint8_t dutyCyclePercent)
{
    CTIMER_Type *mCtimerBase[]           = CTIMER_BASE_PTRS;
    hal_pwm_handle_struct_t *halPwmState = halPwmHandle;
    uint8_t duty                         = 0;

    assert(halPwmHandle);
    assert(halPwmState->instance <= kCTIMER_Capture_2);
    if (halPwmState->pwmLevelSelect == kHAL_PwmHighTrue)
    {
        duty = dutyCyclePercent;
    }
    else
    {
        duty = 100 - dutyCyclePercent;
    }
    CTIMER_UpdatePwmDutycycle(mCtimerBase[halPwmState->instance], (ctimer_match_t)channel, duty);
    return kStatus_HAL_PwmSuccess;
}
