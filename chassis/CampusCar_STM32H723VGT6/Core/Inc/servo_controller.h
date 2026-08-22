/**
 ******************************************************************************
 * @file    servo_controller.h
 * @brief   MG996R 舵机控制模块
 * @note    用于控制摄像头云台俯仰角度（Pitch）
 ******************************************************************************
 */

#ifndef SERVO_CONTROLLER_H
#define SERVO_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 舵机初始化
 * @note  启动 PWM 输出，舵机复位到 90° 中位
 * @retval 0=成功, -1=失败
 */
int Servo_Init(void);

/**
 * @brief 设置舵机角度
 * @param angle 目标角度 (0~180°)
 * @note  自动限幅到安全范围 (20°~160°)，避免机械结构顶死
 * @retval 实际设置的角度（限幅后）
 */
uint8_t Servo_SetAngle(uint8_t angle);

/**
 * @brief 获取当前舵机角度
 * @retval 当前角度 (0~180°)
 */
uint8_t Servo_GetAngle(void);

/**
 * @brief 舵机复位到中位 (90°)
 */
void Servo_Reset(void);

/**
 * @brief 舵机测试序列（上电自检）
 * @note  执行 6 阶段测试：0°→45°→90°→135°→180°→90°
 * @param led_callback 可选的 LED 指示回调函数（每阶段调用一次）
 */
void Servo_TestSequence(void (*led_callback)(uint8_t stage));

#ifdef __cplusplus
}
#endif

#endif /* SERVO_CONTROLLER_H */
