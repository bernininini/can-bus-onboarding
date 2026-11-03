/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : CAN bus onboarding - one firmware, three nodes.
  *
  * Every node on the bus runs this same file. The only difference between
  * nodes is NODE_ID, which the Makefile passes in (-DNODE_ID=1, 2 or 3).
  *
  * What each node does:
  *   1. Every HEARTBEAT_PERIOD_MS it sends a "heartbeat" frame:
  *        ID   = 0x100 + NODE_ID
  *        data = { NODE_ID, counter }
  *   2. Every frame it receives is printed on USART6 and the counter byte
  *      is shown in binary on the 8 user LEDs (LED_A = bit 0).
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* USER CODE BEGIN PD */
#ifndef NODE_ID
#define NODE_ID 1
#endif

#define HEARTBEAT_ID        (0x100U + NODE_ID)
#define HEARTBEAT_PERIOD_MS (400U + 100U * NODE_ID)   /* 500, 600, 700 ms */
#define RX_QUEUE_LEN        8U
/* USER CODE END PD */

CAN_HandleTypeDef hcan1;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
/* A received frame: the header (ID, length, ...) plus up to 8 data bytes. */
typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[8];
} RxFrame;

/* The CAN interrupt puts frames in here; the main loop takes them out. */
static RxFrame rx_queue[RX_QUEUE_LEN];
static uint32_t rx_head;      /* next slot the interrupt writes */
static uint32_t rx_tail;      /* next slot the main loop reads */
static uint32_t rx_dropped;   /* frames lost because the queue was full */
/* USER CODE END PV */

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_USART6_UART_Init(void);

/* USER CODE BEGIN PFP */
static void CAN_Filter_Config(void);
static void send_heartbeat(uint8_t counter);
static int rx_pop(RxFrame *out);
static void print_frame(const RxFrame *f);
static void show_on_leds(uint8_t value);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
/* printf() ends up here: send the characters out of USART6. */
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart6, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY);
  return len;
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_CAN1_Init();
  MX_USART6_UART_Init();

  /* USER CODE BEGIN 2 */
  setvbuf(stdout, NULL, _IONBF, 0);   /* print immediately, no buffering */

  CAN_Filter_Config();
  if (HAL_CAN_Start(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* Call HAL_CAN_RxFifo0MsgPendingCallback() whenever a frame arrives */
  if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    Error_Handler();
  }

  printf("[node%d] up: sending id=0x%03X every %u ms\r\n",
         NODE_ID, (unsigned)HEARTBEAT_ID, (unsigned)HEARTBEAT_PERIOD_MS);

  uint32_t next_tx = HAL_GetTick() + HEARTBEAT_PERIOD_MS;
  uint8_t counter = 0;
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    if ((int32_t)(HAL_GetTick() - next_tx) >= 0)
    {
      next_tx += HEARTBEAT_PERIOD_MS;
      send_heartbeat(counter++);
    }

    RxFrame f;
    while (rx_pop(&f))
    {
      print_frame(&f);
      if (f.header.DLC >= 2)
      {
        show_on_leds(f.data[1]);
      }
    }
    /* USER CODE END 3 */
  }
}

/* USER CODE BEGIN 4 */
/* Accept every frame into FIFO0. A mask bit of 0 means "don't care", so an
 * all-zero mask matches any ID. */
static void CAN_Filter_Config(void)
{
  CAN_FilterTypeDef filter = {0};
  filter.FilterBank = 0;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = 0x0000;
  filter.FilterIdLow = 0x0000;
  filter.FilterMaskIdHigh = 0x0000;
  filter.FilterMaskIdLow = 0x0000;
  filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  filter.FilterActivation = ENABLE;
  filter.SlaveStartFilterBank = 14;   /* banks 0-13 for CAN1, 14-27 for CAN2 */

  if (HAL_CAN_ConfigFilter(&hcan1, &filter) != HAL_OK)
  {
    Error_Handler();
  }
}

static void send_heartbeat(uint8_t counter)
{
  CAN_TxHeaderTypeDef header = {0};
  header.StdId = HEARTBEAT_ID;     /* 11-bit identifier */
  header.IDE = CAN_ID_STD;         /* standard (11-bit), not extended (29-bit) */
  header.RTR = CAN_RTR_DATA;       /* a data frame, not a remote request */
  header.DLC = 2;                  /* number of data bytes, 0..8 */
  header.TransmitGlobalTime = DISABLE;

  uint8_t data[2] = { NODE_ID, counter };
  uint32_t mailbox;

  /* bxCAN has 3 transmit mailboxes. If nobody acknowledges our frames
   * (e.g. we are alone on the bus) they stay full. */
  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0)
  {
    printf("[node%d] tx skipped: all 3 mailboxes busy\r\n", NODE_ID);
    return;
  }
  if (HAL_CAN_AddTxMessage(&hcan1, &header, data, &mailbox) != HAL_OK)
  {
    printf("[node%d] tx failed\r\n", NODE_ID);
    return;
  }
  printf("[node%d] tx id=0x%03lX dlc=%lu data: %02X %02X\r\n",
         NODE_ID, header.StdId, header.DLC, data[0], data[1]);
}

/* Runs inside the CAN1 RX0 interrupt. Keep it short: copy the frame out of
 * the hardware FIFO into our queue and return. Printing happens in main(). */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  RxFrame *slot = &rx_queue[rx_head];
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &slot->header, slot->data) != HAL_OK)
  {
    return;
  }

  uint32_t next = (rx_head + 1) % RX_QUEUE_LEN;
  if (next == rx_tail)
  {
    rx_dropped++;   /* queue full: this frame is lost */
    return;
  }
  rx_head = next;
}

/* Take one frame out of the queue. Interrupts are switched off for a moment
 * so the CAN interrupt can't change the queue while we read it. */
static int rx_pop(RxFrame *out)
{
  int got = 0;
  __disable_irq();
  if (rx_tail != rx_head)
  {
    *out = rx_queue[rx_tail];
    rx_tail = (rx_tail + 1) % RX_QUEUE_LEN;
    got = 1;
  }
  __enable_irq();
  return got;
}

static void print_frame(const RxFrame *f)
{
  printf("[node%d] rx id=0x%03lX dlc=%lu data:", NODE_ID, f->header.StdId, f->header.DLC);
  for (uint32_t i = 0; i < f->header.DLC; i++)
  {
    printf(" %02X", f->data[i]);
  }
  printf("\r\n");
}

/* LED_A shows bit 0, LED_H shows bit 7. The LEDs are wired active-low:
 * writing 0 (RESET) to the pin turns the LED on. */
static void show_on_leds(uint8_t value)
{
  for (int bit = 0; bit < 8; bit++)
  {
    GPIO_PinState state = ((value >> bit) & 1U) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_A_Pin << bit, state);
  }
}
/* USER CODE END 4 */

/**
  * @brief System Clock Configuration: 12 MHz crystal -> PLL -> 180 MHz.
  *        APB1 = 45 MHz (CAN1 runs from this), APB2 = 90 MHz.
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 180;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  *        Bit rate = 45 MHz / Prescaler / (1 + BS1 + BS2)
  *                 = 45 MHz / 3 / (1 + 11 + 3) = 1 Mbit/s
  *        Sample point = (1 + 11) / 15 = 80 %
  */
static void MX_CAN1_Init(void)
{
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 3;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_11TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART6 Initialization Function: 115200 baud, 8N1.
  */
static void MX_USART6_UART_Init(void)
{
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function: LED_A..LED_H as outputs, all off.
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  const uint16_t led_pins = LED_A_Pin | LED_B_Pin | LED_C_Pin | LED_D_Pin
                          | LED_E_Pin | LED_F_Pin | LED_G_Pin | LED_H_Pin;

  __HAL_RCC_GPIOG_CLK_ENABLE();

  /* Active-low LEDs: SET = off */
  HAL_GPIO_WritePin(LED_GPIO_Port, led_pins, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = led_pins;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
