/**
 ******************************************************************************
 * @file    esc_pwm_controller.c
 * @brief   电调 PWM 控制模块实现（备用方案）
 ******************************************************************************
 */

#include "esc_pwm_controller.h"
#include "tim.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>

/* ====== PWM 参数 (TIM1: 200MHz / (PSC+1) = 1MHz, 1 count = 1μs) ====== */
#define PWM_STOP_VALUE   1500U   /* 1.5ms 中位 */
#define PWM_MIN_VALUE    1000U   /* 1.0ms 最小 */
#define PWM_MAX_VALUE    2000U   /* 2.0ms 最大 */

/* ====== 命令值范围 ====== */
#define COMMAND_MIN     -1000
#define COMMAND_MAX      1000

/* ====== 硬件配置 ====== */
#define ESC_HTIM         htim1
#define ESC_SPEED_CH     TIM_CHANNEL_1   /* PE9 → 电调 CH1 (速度) */
#define ESC_STEER_CH     TIM_CHANNEL_3   /* PE13 → 电调 CH2 (转向) */

/* ====== 私有函数 ====== */
static void debug_print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), 200);
}

/* ====== 公共函数实现 ====== */

int ESC_PWM_Init(void)
{
    /* 使能 TIM1 主输出 */
    __HAL_TIM_MOE_ENABLE(&ESC_HTIM);
    
    /* 启动 PWM 输出 */
    if (HAL_TIM_PWM_Start(&ESC_HTIM, ESC_SPEED_CH) != HAL_OK) {
        return -1;
    }
    if (HAL_TIM_PWM_Start(&ESC_HTIM, ESC_STEER_CH) != HAL_OK) {
        return -1;
    }
    
    /* 复位到中位 */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, PWM_STOP_VALUE);
    
    debug_print("[ESC-PWM] Init OK, both channels at 1500us\r\n");
    return 0;
}

void ESC_PWM_GestureInit(void)
{
    debug_print("[ESC-PWM] Starting 9-step gesture init...\r\n");
    
    /* 步骤1：中位预备 (400ms) */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, PWM_STOP_VALUE);
    debug_print("[ESC-PWM] Step 1: Center prep 400ms\r\n");
    HAL_Delay(400);
    
    /* 步骤2：速度上 (1900us, 120ms) */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, 1900);
    debug_print("[ESC-PWM] Step 2: Speed up\r\n");
    HAL_Delay(120);
    
    /* 回中 */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, PWM_STOP_VALUE);
    HAL_Delay(100);
    
    /* 步骤3：速度下 (1100us, 120ms) */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, 1100);
    debug_print("[ESC-PWM] Step 3: Speed down\r\n");
    HAL_Delay(120);
    
    /* 回中 */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, PWM_STOP_VALUE);
    HAL_Delay(100);
    
    /* 步骤4：转向左 (1100us, 120ms) */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, 1100);
    debug_print("[ESC-PWM] Step 4: Steer left\r\n");
    HAL_Delay(120);
    
    /* 回中 */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, PWM_STOP_VALUE);
    HAL_Delay(100);
    
    /* 步骤5：转向右 (1900us, 60ms) */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, 1900);
    debug_print("[ESC-PWM] Step 5: Steer right\r\n");
    HAL_Delay(60);
    
    /* 立即回中 */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, PWM_STOP_VALUE);
    debug_print("[ESC-PWM] Gesture done, back to center\r\n");
    HAL_Delay(500);
}

void ESC_PWM_ApplyCommand(int16_t steer, int16_t speed)
{
    uint32_t speed_pwm, steer_pwm;
    
    /* 限幅 */
    if (steer > COMMAND_MAX) steer = COMMAND_MAX;
    if (steer < COMMAND_MIN) steer = COMMAND_MIN;
    if (speed > COMMAND_MAX) speed = COMMAND_MAX;
    if (speed < COMMAND_MIN) speed = COMMAND_MIN;
    
    /* 映射：-1000~1000 → 1000~2000us */
    speed_pwm = PWM_STOP_VALUE + (speed * 500) / 1000;
    steer_pwm = PWM_STOP_VALUE + (steer * 500) / 1000;
    
    /* 再次限幅 */
    if (speed_pwm > PWM_MAX_VALUE) speed_pwm = PWM_MAX_VALUE;
    if (speed_pwm < PWM_MIN_VALUE) speed_pwm = PWM_MIN_VALUE;
    if (steer_pwm > PWM_MAX_VALUE) steer_pwm = PWM_MAX_VALUE;
    if (steer_pwm < PWM_MIN_VALUE) steer_pwm = PWM_MIN_VALUE;
    
    /* 应用 PWM */
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, speed_pwm);
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, steer_pwm);
}

void ESC_PWM_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_SPEED_CH, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&ESC_HTIM, ESC_STEER_CH, PWM_STOP_VALUE);
}
