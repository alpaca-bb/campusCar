/*
 * Application integration example.
 *
 * CubeMX/CubeIDE must generate huart1 and MX_USART1_UART_Init() first.
 * This file deliberately contains no PWM code. The application only sends
 * USART motion/control frames to the current hoverboard firmware.
 */

#include "hoverboard_usart_app_example.h"

extern UART_HandleTypeDef huart1;

static HOVER_Link_t hoverLink;
static int16_t applicationSteer = 0;
static int16_t applicationSpeed = 0;
static uint32_t nextMotionTick;

void HoverboardApp_Init(void)
{
  HOVER_LinkInit(&hoverLink, &huart1);
  (void)HOVER_StartReceive(&hoverLink);

  /* Keep the driver alive with a safe zero command from startup. */
  (void)HOVER_SendZero(&hoverLink);
  nextMotionTick = HAL_GetTick();
}

void HoverboardApp_SetCommand(int16_t steer, int16_t speed)
{
  applicationSteer = steer;
  applicationSpeed = speed;
}

void HoverboardApp_20msTask(void)
{
  uint32_t now = HAL_GetTick();
  if ((int32_t)(now - nextMotionTick) < 0) {
    return;
  }

  nextMotionTick = now + HOVER_DEFAULT_MOTION_PERIOD_MS;

  /* Do not propagate stale non-zero commands after a feedback outage. */
  if (!HOVER_IsFeedbackFresh(&hoverLink,
                             HOVER_DEFAULT_FEEDBACK_TIMEOUT_MS)) {
    (void)HOVER_SendZero(&hoverLink);
    return;
  }

  (void)HOVER_SendMotion(&hoverLink, applicationSteer, applicationSpeed);
}

HOVER_Result_t HoverboardApp_Release(void)
{
  applicationSteer = 0;
  applicationSpeed = 0;
  (void)HOVER_SendZero(&hoverLink);
  return HOVER_Release(&hoverLink, 150U);
}

HOVER_Result_t HoverboardApp_Resume(void)
{
  HOVER_Result_t result;
  applicationSteer = 0;
  applicationSpeed = 0;

  result = HOVER_Resume(&hoverLink, 150U);
  if (result == HOVER_RESULT_OK) {
    /* Firmware requires a fresh motion frame after RESUME. */
    (void)HOVER_SendZero(&hoverLink);
  }
  return result;
}

HOVER_Result_t HoverboardApp_SetEstop(bool enabled)
{
  applicationSteer = 0;
  applicationSpeed = 0;
  (void)HOVER_SendZero(&hoverLink);
  return HOVER_SetEstop(&hoverLink, enabled, 150U);
}

bool HoverboardApp_IsOnline(void)
{
  return HOVER_IsFeedbackFresh(&hoverLink,
                               HOVER_DEFAULT_FEEDBACK_TIMEOUT_MS);
}

bool HoverboardApp_ReadFeedback(HOVER_Feedback_t *feedback)
{
  return HOVER_GetFeedback(&hoverLink, feedback);
}

void HoverboardApp_GetStatistics(uint32_t *feedbackFrames,
                                 uint32_t *checksumErrors,
                                 uint32_t *uartErrors)
{
  HOVER_GetStatistics(&hoverLink, feedbackFrames,
                      checksumErrors, uartErrors);
}
