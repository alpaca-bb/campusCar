#ifndef HOVERBOARD_USART_APP_EXAMPLE_H
#define HOVERBOARD_USART_APP_EXAMPLE_H

#include "hoverboard_usart_controller.h"

void HoverboardApp_Init(void);
void HoverboardApp_SetCommand(int16_t steer, int16_t speed);
void HoverboardApp_20msTask(void);
HOVER_Result_t HoverboardApp_Release(void);
HOVER_Result_t HoverboardApp_Resume(void);
HOVER_Result_t HoverboardApp_SetEstop(bool enabled);
bool HoverboardApp_IsOnline(void);
bool HoverboardApp_ReadFeedback(HOVER_Feedback_t *feedback);
void HoverboardApp_GetStatistics(uint32_t *feedbackFrames,
                                 uint32_t *checksumErrors,
                                 uint32_t *uartErrors);

#endif
