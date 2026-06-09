/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "TMC5160.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
	STATE_CALIBRATION = 0,
	STATE_RUNNING
}SystemState_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* USER CODE BEGIN PV */
osThreadId_t logicTaskHandle;
osThreadId_t canTaskHandle;
osMessageQueueId_t canQueueHandle;

const osThreadAttr_t logicTask_attributes = { .name = "LogicTask", .stack_size = 512 * 4, .priority = osPriorityNormal };
const osThreadAttr_t canTask_attributes = { .name = "CANTask", .stack_size = 256 * 4, .priority = osPriorityNormal };
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */
void vLogicTask(void *argument);
void vCANTask(void *argument);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  osKernelInitialize();

  /* USER CODE BEGIN RTOS_QUEUES */
  canQueueHandle = osMessageQueueNew(10, sizeof(int32_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  logicTaskHandle = osThreadNew(vLogicTask, NULL, &logicTask_attributes);
  canTaskHandle = osThreadNew(vCANTask, NULL, &canTask_attributes);
  /* USER CODE END RTOS_THREADS */

  osKernelStart();

  while (1)
  {
  }
}

/* USER CODE BEGIN 4 */

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

// --- wrapper dla biblioteki od TMC ---
void tmc5160_readWriteSPI(uint16_t icID, uint8_t *data, size_t dataLength) {
    HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, data, data, dataLength, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin, GPIO_PIN_SET);
}

bool tmc5160_readWriteUART(uint16_t icID, uint8_t *data, size_t writeLength, size_t readLength) {
    return false;
}

TMC5160BusType tmc5160_getBusType(uint16_t icID) {
    return IC_BUS_SPI;
}

uint8_t tmc5160_getNodeAddress(uint16_t icID) {
    return 0;
}
// ----------------------------------------

void vLogicTask(void *argument)
{
    uint32_t adc_raw = 0;
    float real_voltage = 12.0f;

    // twardy reset
    HAL_GPIO_WritePin(GPIOB, PWR_CONVERTER_EN_Pin, GPIO_PIN_RESET); // wylaczenie przetwornicy
    HAL_GPIO_WritePin(GPIOD, SD_MODE_Pin, GPIO_PIN_RESET); // SD_MODE na 0 zeby uklad sluchal SPI
    HAL_GPIO_WritePin(GPIOA, MOTOR_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, SPI_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, DIR_Pin, GPIO_PIN_RESET);

    printf("[LOGIC] oczekiwanie az spadnie ponizej 800mV\r\n");

    // dzielnik napięcia - poczekac aż bedzie około 800mV
    while(real_voltage > 0.8f)
    {
        HAL_ADC_Start(&hadc1);
        if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            adc_raw = HAL_ADC_GetValue(&hadc1);
            real_voltage = (((float)adc_raw * 3.3f) / 4095.0f) * 4.03f;
            printf("[LOGIC] V: %.2f\r\n", real_voltage);
        }
        osDelay(10);
    }

    printf("[LOGIC] napiecie zeszlo. Wlaczam zasilanie\r\n");
    HAL_GPIO_WritePin(GPIOB, PWR_CONVERTER_EN_Pin, GPIO_PIN_SET);
    osDelay(2000);
    HAL_GPIO_WritePin(GPIOA, MOTOR_EN_Pin, GPIO_PIN_RESET);
    osDelay(500);

    // konfiguracja rejestrów
    printf("[LOGIC] konfiguracja rejestrow \r\n");

    tmc5160_writeRegister(0, TMC5160_GSTAT, 0x07);            // czyszczenie smieci z GSTAT
    tmc5160_writeRegister(0, TMC5160_SWMODE, 0x00);           // wylaczenie krancowek
    tmc5160_writeRegister(0, TMC5160_CHOPCONF, 0x000100C3);   // CHOPCONF (SpreadCycle domyslny)
    tmc5160_writeRegister(0, TMC5160_IHOLD_IRUN, 0x00061F0A); // IRUN na 31 (MAX pradu)
    tmc5160_writeRegister(0, TMC5160_TPOWERDOWN, 0x0A);

    // Wylaczamy StealthChop, czyli cichą prace
    tmc5160_writeRegister(0, TMC5160_GCONF, 0x00);

    // agresywna rampa pod klapy
    tmc5160_writeRegister(0, TMC5160_A1, 10000);
    tmc5160_writeRegister(0, TMC5160_V1, 80000);
    tmc5160_writeRegister(0, TMC5160_AMAX, 8000);
    tmc5160_writeRegister(0, TMC5160_VMAX, 300000);
    tmc5160_writeRegister(0, TMC5160_DMAX, 10000);
    tmc5160_writeRegister(0, TMC5160_D1, 15000);
    tmc5160_writeRegister(0, TMC5160_VSTOP, 10);

    tmc5160_writeRegister(0, TMC5160_RAMPMODE, TMC5160_MODE_POSITION); // tryb pozycjonowania
    tmc5160_writeRegister(0, TMC5160_XACTUAL, 0); // XACTUAL na 0

    osDelay(1000);

    // symulacja z CANa
    volatile int32_t target_position = 0;
    int32_t current_target = 0;
    int step = 0;

    while(1)
    {
        // tu bedzie nadpisywac CAN z joysticka, narazie symulacja ruchow
        step++;
        if(step == 1) target_position = 200000;
        if(step == 2) target_position = -200000;
        if(step > 2) step = 0;

        // blyskawiczna aktualizacja rejestru bez czekania
        if(target_position != current_target) {
            printf("[RUNNING] Jazda na: %ld\r\n", target_position);
            tmc5160_writeRegister(0, TMC5160_XTARGET, target_position);
            current_target = target_position;
        }

        // na testy zeby bylo widac ze jedzie (potem wywalic ten delay)
        osDelay(1500);
    }
}

void vCANTask(void *argument)
{
    // tu bedzie logika od odbierania ramek danych w joysticka
    while(1)
    {
        osDelay(100);
    }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  for(;;)
  {
    osDelay(1);
  }
}
/* USER CODE END Header_StartDefaultTask */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */

/**
  * @brief System Clock Configuration
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
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  */
static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin|MOTOR_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, PWR_CONVERTER_EN_Pin|ENC_PRESET_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, ENC_EN_Pin|SPI_EN_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(SD_MODE_GPIO_Port, SD_MODE_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = ENC_I_Pin|ENC_A_Pin|ENC_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TMC_CS_Pin|MOTOR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = PWR_CONVERTER_EN_Pin|ENC_EN_Pin|ENC_PRESET_Pin|SPI_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DIR_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SD_MODE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_MODE_GPIO_Port, &GPIO_InitStruct);
}
