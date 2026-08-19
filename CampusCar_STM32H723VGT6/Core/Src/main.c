/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  * @note           : 固定 PWM 测试程序 - 上电后直接输出固定 PWM 值
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ====== 协议定义 ====== */
#define SERIAL_COMMAND_SIZE 8U       /* NUC 发来的命令帧大小 (字节) */
#define FRAME_HEADER        0xABCDU  /* hoverboard 协议帧头 */

/* ====== PWM 参数 (STM32H723: TIM1时钟=200MHz, PSC=199 → 1MHz 计数) ====== */
/* 1 个计数 = 1μs，所以 1000 计数=1.0ms, 1500 计数=1.5ms(停止), 2000 计数=2.0ms */
#define PWM_STOP_VALUE      1500U    /* 1.5ms 脉宽 = 停止/中位 */
#define PWM_MIN_VALUE       1000U    /* 1.0ms 脉宽 = 最大反向/最大右转 */
#define PWM_MAX_VALUE       2000U    /* 2.0ms 脉宽 = 最大前进/最大左转 */

/* ====== 命令值范围 (hoverboard 协议 speed/steer 的范围) ====== */
#define COMMAND_MIN        -1000
#define COMMAND_MAX         1000

/* ====== 反馈帧大小 ====== */
#define SERIAL_FEEDBACK_SIZE  16U   /* SerialFeedbackCompact = 16 字节 */

/* ====== 反馈周期: 每 200ms 发送一次保持连接 (驱动 1s 超时判离线) ====== */
#define FEEDBACK_PERIOD_MS    200U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* hoverboard 协议命令结构体 (小端序，与 NUC 侧 hoverboard_driver 一致) */
typedef struct {
    uint16_t start;     /* 固定帧头 0xABCD */
    int16_t  steer;     /* 转向值 -1000~1000 (右负左正) */
    int16_t  speed;     /* 速度值 -1000~1000 (后退负前进正) */
    uint16_t checksum;  /* 校验和 = start ^ steer ^ speed (XOR) */
} SerialCommand;

/* 反馈帧结构体 (小端序, 16 字节, 与 NUC 侧 SerialFeedbackCompact 一致) */
/* 驱动用 compact 格式, 不含编码器和电流字段 */
typedef struct {
    uint16_t start;       /* 0xABCD */
    int16_t  cmd1;        /* 回显 steer 命令 */
    int16_t  cmd2;        /* 回显 speed 命令 */
    int16_t  speedR_meas; /* 右轮转速 RPM (STM32 无传感器, 填 0) */
    int16_t  speedL_meas; /* 左轮转速 RPM (STM32 无传感器, 填 0) */
    int16_t  batVoltage;  /* 电池电压, 单位 0.01V (如 2400=24.0V) */
    int16_t  boardTemp;   /* 板温, 单位 0.1°C (如 250=25.0°C) */
    uint16_t cmdLed;      /* LED 命令 (0) */
    uint16_t checksum;    /* 校验和 = 上面所有字段 XOR */
} SerialFeedbackCompact;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void debug_print(const char *str);
static void pwm_init(void);
static void esc_gesture_init(void);
static int  parse_command(const uint8_t *data, SerialCommand *cmd);
static void apply_command_to_pwm(const SerialCommand *cmd);
static void send_feedback(const SerialCommand *cmd);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 串口调试输出函数 */
static void debug_print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), 200);
}

static void pwm_init(void)
{
    __HAL_TIM_MOE_ENABLE(&htim1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
}

/* 电调手势初始化：通过 PWM 组合手势将电调从 USART 模式切换到 PWM 模式 */
/* 顺序：中位预备 → 速度上 → 速度下 → 转向左 → 转向右 → 立即回中 */
static void esc_gesture_init(void)
{
    /* 步骤1：中位预备 - 两路 1500us，保持 400ms 完成信号稳定 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);  /* speed = 1500us */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);  /* steer = 1500us */
    debug_print("[ESC] Gesture: center prep 400ms\r\n");
    HAL_Delay(400);

    /* 步骤2：上 (speed up) - CH2 > 1800us → 1900us，保持 120ms */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1900);
    debug_print("[ESC] Gesture: up\r\n");
    HAL_Delay(120);

    /* 回中 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    HAL_Delay(100);

    /* 步骤3：下 (speed down) - CH2 < 1200us → 1100us，保持 120ms */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1100);
    debug_print("[ESC] Gesture: down\r\n");
    HAL_Delay(120);

    /* 回中 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    HAL_Delay(100);

    /* 步骤4：左 (steer left) - CH1 < 1200us → 1100us，保持 120ms */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 1100);
    debug_print("[ESC] Gesture: left\r\n");
    HAL_Delay(120);

    /* 回中 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
    HAL_Delay(100);

    /* 步骤5：右 (steer right) - CH1 > 1800us → 1900us，保持 60ms（安全要求 40-80ms） */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 1900);
    debug_print("[ESC] Gesture: right\r\n");
    HAL_Delay(60);

    /* 立即回中（安全要求） */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
    debug_print("[ESC] Gesture: done, back to center\r\n");
    HAL_Delay(500);
}

/* 解析 hoverboard 协议命令，返回值：0=成功, 1=帧头错误, 2=校验错误 */
static int parse_command(const uint8_t *data, SerialCommand *cmd)
{
    uint16_t checksum_calc;
    char buf[80];

    /* 打印接收到的原始数据 */
    snprintf(buf, sizeof(buf),
             "\r\n[RX] %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
             data[0], data[1], data[2], data[3],
             data[4], data[5], data[6], data[7]);
    debug_print(buf);

    /* 检查帧头 */
    cmd->start = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    if (cmd->start != FRAME_HEADER) {
        snprintf(buf, sizeof(buf),
                 "[ERR] Header: got 0x%04X, expect 0x%04X\r\n",
                 cmd->start, FRAME_HEADER);
        debug_print(buf);
        return 1;  /* 帧头错误 */
    }

    /* 解析数据 */
    cmd->steer = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
    cmd->speed = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));
    cmd->checksum = (uint16_t)data[6] | ((uint16_t)data[7] << 8);

    /* XOR 校验 */
    checksum_calc = cmd->start ^ (uint16_t)cmd->steer ^ (uint16_t)cmd->speed;
    if (cmd->checksum != checksum_calc) {
        snprintf(buf, sizeof(buf),
                 "[ERR] Checksum: got 0x%04X, calc 0x%04X\r\n",
                 cmd->checksum, checksum_calc);
        debug_print(buf);
        return 2;  /* 校验错误 */
    }

    /* 限制范围 */
    if (cmd->steer > COMMAND_MAX) cmd->steer = COMMAND_MAX;
    if (cmd->steer < COMMAND_MIN) cmd->steer = COMMAND_MIN;
    if (cmd->speed > COMMAND_MAX) cmd->speed = COMMAND_MAX;
    if (cmd->speed < COMMAND_MIN) cmd->speed = COMMAND_MIN;

    /* 打印解析结果 */
    snprintf(buf, sizeof(buf),
             "[OK] steer=%d speed=%d\r\n",
             cmd->steer, cmd->speed);
    debug_print(buf);

    return 0;  /* 成功 */
}

/* 将命令应用到 PWM 输出 */
static void apply_command_to_pwm(const SerialCommand *cmd)
{
    uint32_t speed_pwm;
    uint32_t steer_pwm;
    char buf[80];

    /* speed: -1000~1000 映射到 1000~2000 (PWM_MIN~PWM_MAX) */
    /* -1000 → 1000 (后退最大), 0 → 1500 (停止), 1000 → 2000 (前进最大) */
    speed_pwm = PWM_STOP_VALUE + (cmd->speed * 500) / 1000;
    if (speed_pwm > PWM_MAX_VALUE) speed_pwm = PWM_MAX_VALUE;
    if (speed_pwm < PWM_MIN_VALUE) speed_pwm = PWM_MIN_VALUE;

    /* steer: -1000~1000 映射到 1000~2000 */
    /* -1000 → 1000 (右转最大), 0 → 1500 (停止), 1000 → 2000 (左转最大) */
    steer_pwm = PWM_STOP_VALUE + (cmd->steer * 500) / 1000;
    if (steer_pwm > PWM_MAX_VALUE) steer_pwm = PWM_MAX_VALUE;
    if (steer_pwm < PWM_MIN_VALUE) steer_pwm = PWM_MIN_VALUE;

    /* CH1 = 速度 (PE9), CH3 = 转向 (PE13) */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, speed_pwm);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, steer_pwm);

    /* 打印 PWM 输出值 */
    snprintf(buf, sizeof(buf),
             "[PWM] CH1(speed)=%lu CH3(steer)=%lu\r\n",
             (unsigned long)speed_pwm, (unsigned long)steer_pwm);
    debug_print(buf);
}

/* 发送反馈帧给 NUC (SerialFeedbackCompact, 16 字节) */
/* 驱动需要收到反馈帧才会判定为 "connected"，否则显示离线 */
static void send_feedback(const SerialCommand *cmd)
{
    SerialFeedbackCompact fb;
    fb.start       = FRAME_HEADER;
    fb.cmd1        = cmd->steer;        /* 回显转向命令 */
    fb.cmd2        = cmd->speed;        /* 回显速度命令 */
    fb.speedR_meas = 0;                /* 无轮速传感器 */
    fb.speedL_meas = 0;
    fb.batVoltage  = 2400;              /* 24.0V 占位 (实际无 ADC 采集) */
    fb.boardTemp   = 250;               /* 25.0°C 占位 */
    fb.cmdLed      = 0;

    /* 校验和 = 所有字段 XOR (与 NUC 驱动一致) */
    fb.checksum = (uint16_t)(fb.start ^
                             (uint16_t)fb.cmd1 ^
                             (uint16_t)fb.cmd2 ^
                             (uint16_t)fb.speedR_meas ^
                             (uint16_t)fb.speedL_meas ^
                             (uint16_t)fb.batVoltage ^
                             (uint16_t)fb.boardTemp ^
                             fb.cmdLed);

    /* 通过 UART1 发送 16 字节反馈帧 */
    HAL_UART_Transmit(&huart1, (uint8_t *)&fb, SERIAL_FEEDBACK_SIZE, 100);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  /* 配置 PC13 和 PC14 */
  {
    GPIO_InitTypeDef GPIO_InitStruct_LED = {0};
    GPIO_InitStruct_LED.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    GPIO_InitStruct_LED.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct_LED.Pull = GPIO_NOPULL;
    GPIO_InitStruct_LED.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct_LED);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
  }

  /* PC13 闪 4 次 */
  for (int i = 0; i < 4; i++) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(200);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_Delay(200);
  }

  /* 初始化 PWM 输出 */
  pwm_init();

  /* 等待电调上电完成 */
  HAL_Delay(1000);

  /* 执行电调手势序列：自动从 USART 模式切换到 PWM 模式 */
  /* 顺序：中位预备 → 速度上 → 速度下 → 转向左 → 转向右 → 回中 */
  debug_print("[ESC] Starting gesture init...\r\n");
  esc_gesture_init();
  debug_print("[ESC] Gesture init complete. Waiting for 4+8 beeps.\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* 先执行一次测试：前进→后退→左转→右转 */
  {
    /* 前进 2 秒 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_MAX_VALUE);  /* 2.0ms 前进 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);  /* 1.5ms 直行 */
    HAL_Delay(2000);
    
    /* 停止 0.5 秒 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
    HAL_Delay(500);
    
    /* 后退 2 秒 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_MIN_VALUE);   /* 1.0ms 后退 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);  /* 1.5ms 直行 */
    HAL_Delay(2000);
    
    /* 停止 0.5 秒 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
    HAL_Delay(500);
    
    /* 左转 2 秒 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);  /* 1.5ms 停止 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_MAX_VALUE);   /* 2.0ms 左转 */
    HAL_Delay(2000);
    
    /* 停止 0.5 秒 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
    HAL_Delay(500);
    
    /* 右转 2 秒 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);  /* 1.5ms 停止 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_MIN_VALUE);   /* 1.0ms 右转 */
    HAL_Delay(2000);
    
    /* 停止位置 */
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
  }

  /* 测试完成，发送提示 */
  HAL_UART_Transmit(&huart1, (uint8_t*)"Test Done, Waiting for UART...\r\n", 31, 100);
  debug_print("Send: AB CD 00 00 E8 03 9D 9A (forward)\r\n");

  /* 主循环：PC14 闪烁 + 等待串口命令 + 发送反馈帧 */
  uint32_t last_led_tick = HAL_GetTick();
  uint32_t last_cmd_tick = HAL_GetTick();
  uint32_t last_feedback_tick = HAL_GetTick();
  uint8_t cmd_buffer[SERIAL_COMMAND_SIZE] = {0};
  SerialCommand current_cmd = {0};

  while (1)
  {
    /* PC14 持续闪烁 */
    uint32_t current_tick = HAL_GetTick();
    if ((current_tick - last_led_tick) >= 500)
    {
      last_led_tick = current_tick;
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_14);
    }

    /* 等待串口命令（超时 100ms，方便手动发送） */
    if (HAL_UART_Receive(&huart1, cmd_buffer, SERIAL_COMMAND_SIZE, 100) == HAL_OK)
    {
      /* 收到 8 字节数据，解析命令 */
      int result = parse_command(cmd_buffer, &current_cmd);
      if (result == 0)
      {
        /* 解析成功，应用到 PWM */
        apply_command_to_pwm(&current_cmd);
        last_cmd_tick = HAL_GetTick();

        /* 立即发送反馈帧给 NUC */
        send_feedback(&current_cmd);
        last_feedback_tick = HAL_GetTick();

        /* PC13 闪一下表示收到有效命令 */
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
      }
      else
      {
        /* 解析失败，打印错误类型 */
        debug_print(result == 1 ? "[FAIL] Bad header\r\n" : "[FAIL] Bad checksum\r\n");
      }
    }
    else
    {
      /* 超时无数据：如果超过 1 秒没收到命令，回到停止位置（安全） */
      if ((HAL_GetTick() - last_cmd_tick) > 1000)
      {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, PWM_STOP_VALUE);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, PWM_STOP_VALUE);
      }
    }

    /* 周期性发送反馈帧：每 200ms 发送一次，保持 NUC 驱动 "connected" 状态 */
    /* 驱动 1s 收不到反馈会判定离线 */
    if ((HAL_GetTick() - last_feedback_tick) >= FEEDBACK_PERIOD_MS)
    {
      send_feedback(&current_cmd);
      last_feedback_tick = HAL_GetTick();
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
