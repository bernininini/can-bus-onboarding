/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : CAN bus onboarding - one firmware, three nodes.
  *
  * Every node on the bus runs this same file. The only difference between
  * nodes is NODE_ID, which the Makefile passes in (-DNODE_ID=1, 2 or 3).
  *
  * When you're done, each node will:
  *   - show its NODE_ID on the 8 LEDs at boot                    (Task 1)
  *   - start CAN1                                                (Task 2)
  *   - every HEARTBEAT_PERIOD_MS send a "heartbeat" frame:       (Task 3)
  *        ID   = 0x100 + NODE_ID
  *        data = { NODE_ID, counter }
  *   - receive the other nodes' frames, print them on USART6 and
  *     show their counter in binary on the LEDs                  (Task 4)
  *
  * Your work goes in the four functions marked "TASK". Everything else is
  * already done for you. The guide (docs/CAN-Bus-Onboarding.pdf) explains
  * each task step by step.
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
/* Your tasks */
static void show_on_leds(uint8_t value);         /* Task 1 */
static void can_start(void);                     /* Task 2 (+ Task 4, part A) */
static void send_heartbeat(uint8_t counter);     /* Task 3 */
/* Task 4, part B is HAL_CAN_RxFifo0MsgPendingCallback(), further down */

/* Helpers that are already written for you (not static, so the compiler
 * doesn't warn while your tasks don't use them yet) */
void print_tx(const CAN_TxHeaderTypeDef *header, const uint8_t *data);
void print_rx(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
void rx_push(const CAN_RxHeaderTypeDef *header, const uint8_t *data);
int rx_pop(RxFrame *out);
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

  show_on_leds(NODE_ID);   /* each board shows its own number at boot */
  can_start();

  printf("[node%d] up: sending id=0x%03X every %u ms\r\n",
         NODE_ID, (unsigned)HEARTBEAT_ID, (unsigned)HEARTBEAT_PERIOD_MS);

  uint32_t next_tx = HAL_GetTick() + HEARTBEAT_PERIOD_MS;
  uint8_t counter = 0;
  /* USER CODE END 2 */

  while (1)
  {
    /* USER CODE BEGIN 3 */
    /* Every HEARTBEAT_PERIOD_MS milliseconds, send one heartbeat. */
    if ((int32_t)(HAL_GetTick() - next_tx) >= 0)
    {
      next_tx += HEARTBEAT_PERIOD_MS;
      send_heartbeat(counter++);
    }

    /* Print every frame the receive callback has queued up. */
    RxFrame f;
    while (rx_pop(&f))
    {
      print_rx(&f.header, f.data);
      if (f.header.DLC >= 2)
      {
        show_on_leds(f.data[1]);
      }
    }
    /* USER CODE END 3 */
  }
}

/* USER CODE BEGIN 4 */
/* ========================================================================
 * TASK 1: LEDs (GPIO)                                    guide: "Task 1"
 * ------------------------------------------------------------------------
 * Show `value` in binary on the 8 user LEDs: LED_A = bit 0 ... LED_H = bit 7.
 * Example: value 5 = 0b00000101 -> LED_A and LED_C on, the rest off.
 *
 * Use:   HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET or GPIO_PIN_RESET)
 * Know:  the LEDs are LED_A_Pin..LED_H_Pin on port LED_GPIO_Port (main.h).
 *        They are ACTIVE-LOW: GPIO_PIN_RESET turns an LED ON.
 * ======================================================================== */
static void show_on_leds(uint8_t value)
{
  /* TODO (Task 1) */
  (void)value;
}

/* ========================================================================
 * TASK 2: Start CAN1                                     guide: "Task 2"
 * ------------------------------------------------------------------------
 * MX_CAN1_Init() has already set CAN1 to 1 Mbit/s, but the controller is
 * still offline and throws away every frame. Bring it onto the bus:
 *
 *   1. Set up filter bank 0 to accept every frame into FIFO0:
 *        fill in a CAN_FilterTypeDef, then HAL_CAN_ConfigFilter()
 *   2. Join the bus:  HAL_CAN_Start()
 *
 * Both return HAL_OK on success. If one doesn't, call Error_Handler().
 *
 * (Task 4, part A, adds one more call at the end of this function.)
 * ======================================================================== */
static void can_start(void)
{
  /* TODO (Task 2) */
}

/* ========================================================================
 * TASK 3: Send a frame                                   guide: "Task 3"
 * ------------------------------------------------------------------------
 * Send one heartbeat:  ID = HEARTBEAT_ID,  2 data bytes = { NODE_ID, counter }
 *
 *   1. Fill in a CAN_TxHeaderTypeDef: StdId, IDE, RTR, DLC
 *   2. Put the data bytes in a uint8_t array
 *   3. Queue it:  HAL_CAN_AddTxMessage(&hcan1, &header, data, &mailbox)
 *   4. Show it in the log:  print_tx(&header, data)
 *
 * Extra: if HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) is 0, all 3 transmit
 * mailboxes are full. Print a message and return instead of sending.
 * ======================================================================== */
static void send_heartbeat(uint8_t counter)
{
  /* TODO (Task 3) */
  (void)counter;
}

/* ========================================================================
 * TASK 4: Receive frames                                 guide: "Task 4"
 * ------------------------------------------------------------------------
 * Part A: at the end of can_start(), switch on the "frame received"
 *         interrupt:
 *           HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING)
 *
 * Part B: HAL calls the function below from the CAN1 RX0 interrupt every
 *         time a frame lands in FIFO0. In it:
 *           1. Read the frame out of FIFO0:
 *                HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, data)
 *           2. Hand it to the main loop:  rx_push(&header, data)
 *
 * Do part B before you test part A. Keep this function short, and don't
 * printf here: you're inside an interrupt.
 * ======================================================================== */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  /* TODO (Task 4, part B) */
  (void)hcan;
}

/* ------------------------------------------------------------------------
 * Helpers: already written, you don't need to change these.
 * ------------------------------------------------------------------------ */

/* Print a frame you sent, e.g.  [node1] tx id=0x101 dlc=2 data: 01 00 */
void print_tx(const CAN_TxHeaderTypeDef *header, const uint8_t *data)
{
  printf("[node%d] tx id=0x%03lX dlc=%lu data:", NODE_ID, header->StdId, header->DLC);
  for (uint32_t i = 0; i < header->DLC; i++)
  {
    printf(" %02X", data[i]);
  }
  printf("\r\n");
}

/* Print a frame you received, e.g.  [node2] rx id=0x101 dlc=2 data: 01 00 */
void print_rx(const CAN_RxHeaderTypeDef *header, const uint8_t *data)
{
  printf("[node%d] rx id=0x%03lX dlc=%lu data:", NODE_ID, header->StdId, header->DLC);
  for (uint32_t i = 0; i < header->DLC; i++)
  {
    printf(" %02X", data[i]);
  }
  printf("\r\n");
}

/* Called from the receive interrupt: copy one frame into the queue. */
void rx_push(const CAN_RxHeaderTypeDef *header, const uint8_t *data)
{
  uint32_t next = (rx_head + 1) % RX_QUEUE_LEN;
  if (next == rx_tail)
  {
    rx_dropped++;   /* queue full: this frame is lost */
    return;
  }
  rx_queue[rx_head].header = *header;
  for (uint32_t i = 0; i < 8; i++)
  {
    rx_queue[rx_head].data[i] = data[i];
  }
  rx_head = next;
}

/* Called from the main loop: take one frame out of the queue. Interrupts
 * are switched off for a moment so the CAN interrupt can't change the
 * queue while we read it. Returns 1 if it got a frame, 0 if empty. */
int rx_pop(RxFrame *out)
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
