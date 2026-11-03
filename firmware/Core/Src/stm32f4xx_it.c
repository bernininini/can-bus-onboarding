/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"
#include "stm32f4xx_it.h"

extern CAN_HandleTypeDef hcan1;

void NMI_Handler(void)
{
  while (1)
  {
  }
}

void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void MemManage_Handler(void)
{
  while (1)
  {
  }
}

void BusFault_Handler(void)
{
  while (1)
  {
  }
}

void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

/* Fires every 1 ms; HAL_GetTick() counts these. */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/* A frame landed in CAN1's receive FIFO0. HAL works out why the interrupt
 * fired and calls HAL_CAN_RxFifo0MsgPendingCallback() in main.c. */
void CAN1_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan1);
}
