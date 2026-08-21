/**
 ******************************************************************************
 * @file    esc_pwm_controller.h
 * @brief   电调 PWM 控制模块（备用方案）
 * @note    当前主推串口直控方案，此模块保留用于快速回滚
 ******************************************************************************
 */

#ifndef ESC_PWM_CONTROLLER_H
#define ESC_PWM_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief PWM 控制初始化
 * @note  启动 TIM1 CH1/CH3 PWM 输出，复位到中位 (1500us)
 * @retval 0=成功, -1=失败
 */
int ESC_PWM_Init(void);

/**
 * @brief 执行电调手势初始化
 * @note  9 步手势序列：中位预备 → 上 → 下 → 左 → 右 → 回中
 * @note  用于将电调从 USART 模式切换到 PWM 模式
 */
void ESC_PWM_GestureInit(void);

/**
 * @brief 应用运动命令到 PWM
 * @param steer 转向值 (-1000~1000)
 * @param speed 速度值 (-1000~1000)
 * @note  自动映射到 1000~2000us 脉宽
 */
void ESC_PWM_ApplyCommand(int16_t steer, int16_t speed);

/**
 * @brief 停止电调（输出中位 1500us）
 */
void ESC_PWM_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* ESC_PWM_CONTROLLER_H */
