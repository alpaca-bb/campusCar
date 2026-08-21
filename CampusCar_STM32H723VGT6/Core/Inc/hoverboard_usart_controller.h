#ifndef HOVERBOARD_USART_CONTROLLER_H
#define HOVERBOARD_USART_CONTROLLER_H

#include "main.h"

#include <stdbool.h>
#include <stdint.h>

/* Current firmware wire protocol. All multi-byte fields are little-endian. */
#define HOVER_START_FRAME        0xABCDU
#define HOVER_CONTROL_FRAME      0xABCEU

#define HOVER_MOTION_FRAME_SIZE  8U
#define HOVER_CONTROL_FRAME_SIZE 8U
#define HOVER_FEEDBACK_FRAME_SIZE 26U

#define HOVER_CMD_RELEASE        1U
#define HOVER_CMD_RESUME         2U
#define HOVER_CMD_ESTOP          3U

#define HOVER_STATE_RUN          0U
#define HOVER_STATE_RELEASE      1U
#define HOVER_STATE_ESTOP        2U

#define HOVER_ACK_COMMAND_MASK   0x007FU
#define HOVER_ACK_ACCEPTED       0x0080U

/* The driver sends feedback about every 10 ms. */
#define HOVER_DEFAULT_MOTION_PERIOD_MS 20U
#define HOVER_DEFAULT_FEEDBACK_TIMEOUT_MS 100U

typedef enum
{
  HOVER_RESULT_OK = 0,
  HOVER_RESULT_INVALID_ARGUMENT,
  HOVER_RESULT_UART_ERROR,
  HOVER_RESULT_ACK_TIMEOUT,
  HOVER_RESULT_REJECTED,
  HOVER_RESULT_UNEXPECTED_STATE
} HOVER_Result_t;

typedef struct
{
  int16_t cmd1;
  int16_t cmd2;
  int16_t speedRightRpm;
  int16_t speedLeftRpm;
  int16_t wheelRightCount;
  int16_t wheelLeftCount;
  int16_t batteryVoltageX100;
  int16_t boardTemperatureX10;
  uint16_t commandLed;
  uint16_t driverState;
  uint16_t controlAck;
  uint32_t receivedTick;
} HOVER_Feedback_t;

typedef struct
{
  UART_HandleTypeDef *uart;
  uint8_t rxByte;
  uint8_t feedbackFrame[HOVER_FEEDBACK_FRAME_SIZE];
  uint8_t feedbackFrameLength;

  volatile HOVER_Feedback_t latestFeedback;
  volatile uint8_t feedbackValid;
  volatile uint32_t feedbackFrameCount;
  volatile uint32_t checksumErrorCount;
  volatile uint32_t uartErrorCount;
} HOVER_Link_t;

void HOVER_LinkInit(HOVER_Link_t *link, UART_HandleTypeDef *uart);
HAL_StatusTypeDef HOVER_StartReceive(HOVER_Link_t *link);

/* Send one normal 8-byte motion frame. steer/speed are in [-1000, 1000]. */
HOVER_Result_t HOVER_SendMotion(HOVER_Link_t *link, int16_t steer, int16_t speed);
HOVER_Result_t HOVER_SendZero(HOVER_Link_t *link);

/* Send 0xABCE and wait for a new matching feedback ACK. */
HOVER_Result_t HOVER_SendControlAndWait(HOVER_Link_t *link,
                                         uint16_t command,
                                         int16_t value,
                                         uint16_t expectedState,
                                         uint32_t timeoutMs,
                                         uint8_t retries);

HOVER_Result_t HOVER_Release(HOVER_Link_t *link, uint32_t timeoutMs);
HOVER_Result_t HOVER_Resume(HOVER_Link_t *link, uint32_t timeoutMs);
HOVER_Result_t HOVER_SetEstop(HOVER_Link_t *link, bool enabled, uint32_t timeoutMs);

bool HOVER_GetFeedback(const HOVER_Link_t *link, HOVER_Feedback_t *feedback);
bool HOVER_IsFeedbackFresh(const HOVER_Link_t *link, uint32_t timeoutMs);
void HOVER_GetStatistics(const HOVER_Link_t *link,
                         uint32_t *feedbackFrames,
                         uint32_t *checksumErrors,
                         uint32_t *uartErrors);
HOVER_Result_t HOVER_GetLastResult(const HOVER_Link_t *link);
const char *HOVER_ResultString(HOVER_Result_t result);

/* Called internally by HAL callbacks; useful only for custom interrupt glue. */
void HOVER_ProcessRxBytes(HOVER_Link_t *link, const uint8_t *data, uint16_t length);

#endif
