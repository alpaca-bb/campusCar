#include "hoverboard_usart_controller.h"

#include <string.h>

static HOVER_Link_t *g_rxLink;
static HOVER_Result_t g_lastResult = HOVER_RESULT_OK;

static uint16_t readU16(const uint8_t *data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static int16_t readI16(const uint8_t *data)
{
  return (int16_t)readU16(data);
}

static void writeU16(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)(value & 0xFFU);
  data[1] = (uint8_t)(value >> 8);
}

static uint16_t feedbackChecksum(const uint8_t *frame)
{
  uint16_t checksum = 0U;
  for (uint32_t offset = 0U; offset < HOVER_FEEDBACK_FRAME_SIZE - 2U; offset += 2U) {
    checksum ^= readU16(&frame[offset]);
  }
  return checksum;
}

static bool feedbackFrameIsValid(const uint8_t *frame)
{
  return readU16(&frame[0]) == HOVER_START_FRAME &&
         readU16(&frame[HOVER_FEEDBACK_FRAME_SIZE - 2U]) == feedbackChecksum(frame);
}

static void publishFeedback(HOVER_Link_t *link, const uint8_t *frame)
{
  HOVER_Feedback_t feedback;

  feedback.cmd1 = readI16(&frame[2]);
  feedback.cmd2 = readI16(&frame[4]);
  feedback.speedRightRpm = readI16(&frame[6]);
  feedback.speedLeftRpm = readI16(&frame[8]);
  feedback.wheelRightCount = readI16(&frame[10]);
  feedback.wheelLeftCount = readI16(&frame[12]);
  feedback.batteryVoltageX100 = readI16(&frame[14]);
  feedback.boardTemperatureX10 = readI16(&frame[16]);
  feedback.commandLed = readU16(&frame[18]);
  feedback.driverState = readU16(&frame[20]);
  feedback.controlAck = readU16(&frame[22]);
  feedback.receivedTick = HAL_GetTick();

  __disable_irq();
  link->latestFeedback = feedback;
  link->feedbackValid = 1U;
  link->feedbackFrameCount++;
  __enable_irq();
}

static void processRxByte(HOVER_Link_t *link, uint8_t byte)
{
  if (link->feedbackFrameLength == 0U) {
    if (byte == 0xCDU) {
      link->feedbackFrame[0] = byte;
      link->feedbackFrameLength = 1U;
    }
    return;
  }

  if (link->feedbackFrameLength == 1U && byte != 0xABU) {
    /* Keep a possible new low byte as the next candidate frame start. */
    link->feedbackFrameLength = (byte == 0xCDU) ? 1U : 0U;
    if (link->feedbackFrameLength == 1U) {
      link->feedbackFrame[0] = byte;
    }
    return;
  }

  link->feedbackFrame[link->feedbackFrameLength++] = byte;
  if (link->feedbackFrameLength < HOVER_FEEDBACK_FRAME_SIZE) {
    return;
  }

  if (feedbackFrameIsValid(link->feedbackFrame)) {
    publishFeedback(link, link->feedbackFrame);
  } else {
    link->checksumErrorCount++;
  }
  link->feedbackFrameLength = 0U;
}

void HOVER_LinkInit(HOVER_Link_t *link, UART_HandleTypeDef *uart)
{
  if (link == NULL) {
    return;
  }

  memset(link, 0, sizeof(*link));
  link->uart = uart;
  g_rxLink = link;
  g_lastResult = HOVER_RESULT_OK;
}

HAL_StatusTypeDef HOVER_StartReceive(HOVER_Link_t *link)
{
  if (link == NULL || link->uart == NULL) {
    return HAL_ERROR;
  }

  g_rxLink = link;
  link->feedbackFrameLength = 0U;
  return HAL_UART_Receive_IT(link->uart, &link->rxByte, 1U);
}

static HOVER_Result_t sendFrame(HOVER_Link_t *link, const uint8_t *frame, uint16_t length)
{
  if (link == NULL || link->uart == NULL || frame == NULL) {
    return HOVER_RESULT_INVALID_ARGUMENT;
  }

  if (HAL_UART_Transmit(link->uart, (uint8_t *)frame, length, 50U) != HAL_OK) {
    return HOVER_RESULT_UART_ERROR;
  }
  return HOVER_RESULT_OK;
}

HOVER_Result_t HOVER_SendMotion(HOVER_Link_t *link, int16_t steer, int16_t speed)
{
  uint8_t frame[HOVER_MOTION_FRAME_SIZE];
  uint16_t checksum;

  steer = (int16_t)((steer < -1000) ? -1000 : (steer > 1000) ? 1000 : steer);
  speed = (int16_t)((speed < -1000) ? -1000 : (speed > 1000) ? 1000 : speed);

  writeU16(&frame[0], HOVER_START_FRAME);
  writeU16(&frame[2], (uint16_t)steer);
  writeU16(&frame[4], (uint16_t)speed);
  checksum = (uint16_t)(HOVER_START_FRAME ^ (uint16_t)steer ^ (uint16_t)speed);
  writeU16(&frame[6], checksum);

  g_lastResult = sendFrame(link, frame, sizeof(frame));
  return g_lastResult;
}

HOVER_Result_t HOVER_SendZero(HOVER_Link_t *link)
{
  return HOVER_SendMotion(link, 0, 0);
}

bool HOVER_GetFeedback(const HOVER_Link_t *link, HOVER_Feedback_t *feedback)
{
  if (link == NULL || feedback == NULL || link->feedbackValid == 0U) {
    return false;
  }

  __disable_irq();
  *feedback = link->latestFeedback;
  __enable_irq();
  return true;
}

bool HOVER_IsFeedbackFresh(const HOVER_Link_t *link, uint32_t timeoutMs)
{
  HOVER_Feedback_t feedback;
  if (!HOVER_GetFeedback(link, &feedback)) {
    return false;
  }
  return (HAL_GetTick() - feedback.receivedTick) <= timeoutMs;
}

void HOVER_GetStatistics(const HOVER_Link_t *link,
                         uint32_t *feedbackFrames,
                         uint32_t *checksumErrors,
                         uint32_t *uartErrors)
{
  if (link == NULL) {
    return;
  }

  __disable_irq();
  if (feedbackFrames != NULL) {
    *feedbackFrames = link->feedbackFrameCount;
  }
  if (checksumErrors != NULL) {
    *checksumErrors = link->checksumErrorCount;
  }
  if (uartErrors != NULL) {
    *uartErrors = link->uartErrorCount;
  }
  __enable_irq();
}

HOVER_Result_t HOVER_SendControlAndWait(HOVER_Link_t *link,
                                         uint16_t command,
                                         int16_t value,
                                         uint16_t expectedState,
                                         uint32_t timeoutMs,
                                         uint8_t retries)
{
  uint8_t frame[HOVER_CONTROL_FRAME_SIZE];
  HOVER_Feedback_t feedback;

  if (link == NULL || retries == 0U || command == 0U || command > 0x7FU) {
    return HOVER_RESULT_INVALID_ARGUMENT;
  }

  writeU16(&frame[0], HOVER_CONTROL_FRAME);
  writeU16(&frame[2], command);
  writeU16(&frame[4], (uint16_t)value);
  writeU16(&frame[6], (uint16_t)(HOVER_CONTROL_FRAME ^ command ^ (uint16_t)value));

  for (uint8_t attempt = 0U; attempt < retries; attempt++) {
    uint16_t previousAck = 0U;
    if (HOVER_GetFeedback(link, &feedback)) {
      previousAck = feedback.controlAck;
    }

    HOVER_Result_t sendResult = sendFrame(link, frame, sizeof(frame));
    if (sendResult != HOVER_RESULT_OK) {
      g_lastResult = sendResult;
      continue;
    }

    uint32_t startTick = HAL_GetTick();
    while ((HAL_GetTick() - startTick) < timeoutMs) {
      if (HOVER_GetFeedback(link, &feedback) &&
          feedback.controlAck != previousAck &&
          (feedback.controlAck & HOVER_ACK_COMMAND_MASK) == command) {
        if ((feedback.controlAck & HOVER_ACK_ACCEPTED) == 0U) {
          g_lastResult = HOVER_RESULT_REJECTED;
          return g_lastResult;
        }
        if (feedback.driverState != expectedState) {
          g_lastResult = HOVER_RESULT_UNEXPECTED_STATE;
          return g_lastResult;
        }
        g_lastResult = HOVER_RESULT_OK;
        return g_lastResult;
      }
      HAL_Delay(1U);
    }
    g_lastResult = HOVER_RESULT_ACK_TIMEOUT;
  }

  return g_lastResult;
}

HOVER_Result_t HOVER_Release(HOVER_Link_t *link, uint32_t timeoutMs)
{
  return HOVER_SendControlAndWait(link, HOVER_CMD_RELEASE, 0,
                                   HOVER_STATE_RELEASE, timeoutMs, 3U);
}

HOVER_Result_t HOVER_Resume(HOVER_Link_t *link, uint32_t timeoutMs)
{
  return HOVER_SendControlAndWait(link, HOVER_CMD_RESUME, 0,
                                   HOVER_STATE_RUN, timeoutMs, 3U);
}

HOVER_Result_t HOVER_SetEstop(HOVER_Link_t *link, bool enabled, uint32_t timeoutMs)
{
  return HOVER_SendControlAndWait(link, HOVER_CMD_ESTOP, enabled ? 1 : 0,
                                   enabled ? HOVER_STATE_ESTOP : HOVER_STATE_RUN,
                                   timeoutMs, 3U);
}

HOVER_Result_t HOVER_GetLastResult(const HOVER_Link_t *link)
{
  (void)link;
  return g_lastResult;
}

const char *HOVER_ResultString(HOVER_Result_t result)
{
  switch (result) {
    case HOVER_RESULT_OK: return "OK";
    case HOVER_RESULT_INVALID_ARGUMENT: return "invalid argument";
    case HOVER_RESULT_UART_ERROR: return "UART error";
    case HOVER_RESULT_ACK_TIMEOUT: return "ACK timeout";
    case HOVER_RESULT_REJECTED: return "firmware rejected command";
    case HOVER_RESULT_UNEXPECTED_STATE: return "unexpected driver state";
    default: return "unknown result";
  }
}

void HOVER_ProcessRxBytes(HOVER_Link_t *link, const uint8_t *data, uint16_t length)
{
  if (link == NULL || data == NULL) {
    return;
  }
  for (uint16_t index = 0U; index < length; index++) {
    processRxByte(link, data[index]);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (g_rxLink == NULL || huart != g_rxLink->uart) {
    return;
  }

  processRxByte(g_rxLink, g_rxLink->rxByte);
  (void)HAL_UART_Receive_IT(g_rxLink->uart, &g_rxLink->rxByte, 1U);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (g_rxLink == NULL || huart != g_rxLink->uart) {
    return;
  }

  g_rxLink->uartErrorCount++;
  (void)HAL_UART_Receive_IT(g_rxLink->uart, &g_rxLink->rxByte, 1U);
}
