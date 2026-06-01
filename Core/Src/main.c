/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    uint8_t address;
    uint32_t value;
} SPI_Message_t;

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
osThreadId_t logicTaskHandle, spiTaskHandle;
osMessageQueueId_t spiQueueHandle;
osThreadId_t canTaskHandle;
osMessageQueueId_t canQueueHandle;

volatile int32_t encoder_position = 0;

const osThreadAttr_t logicTask_attributes = { .name = "LogicTask", .stack_size = 256 * 4, .priority = osPriorityNormal };
const osThreadAttr_t spiTask_attributes = { .name = "SPITask", .stack_size = 256 * 4, .priority = osPriorityHigh };
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
void vSPITask(void *argument);
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
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  spiQueueHandle = osMessageQueueNew(16, sizeof(SPI_Message_t), NULL);
  canQueueHandle = osMessageQueueNew(10, sizeof(int32_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  logicTaskHandle = osThreadNew(vLogicTask, NULL, &logicTask_attributes);
  spiTaskHandle = osThreadNew(vSPITask, NULL, &spiTask_attributes);
  canTaskHandle = osThreadNew(vCANTask, NULL, &canTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
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

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
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
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
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
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin|MOTOR_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, PWR_CONVERTER_EN_Pin|ENC_PRESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ENC_EN_Pin|SPI_EN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : ENC_I_Pin ENC_A_Pin ENC_B_Pin */
  GPIO_InitStruct.Pin = ENC_I_Pin|ENC_A_Pin|ENC_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : TMC_CS_Pin MOTOR_EN_Pin */
  GPIO_InitStruct.Pin = TMC_CS_Pin|MOTOR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PWR_CONVERTER_EN_Pin ENC_EN_Pin ENC_PRESET_Pin SPI_EN_Pin */
  GPIO_InitStruct.Pin = PWR_CONVERTER_EN_Pin|ENC_EN_Pin|ENC_PRESET_Pin|SPI_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : DIR_Pin */
  GPIO_InitStruct.Pin = DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DIR_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

void vLogicTask(void *argument)
{
    SPI_Message_t Msg;
    uint32_t adc_raw = 0;
    float real_voltage = 12.0f;

    SystemState_t currentState = STATE_CALIBRATION; //zaczynamy od kalibracji na start

    //Odblokowanie przetwornicy i silnika
    HAL_GPIO_WritePin(GPIOB, PWR_CONVERTER_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, MOTOR_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(GPIOB, SPI_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, DIR_Pin,GPIO_PIN_RESET);


    printf("[LOGIC] Oczekiwanie na 800mV\r\n");
    while(real_voltage > 0.8f)
    {
        HAL_ADC_Start(&hadc1);

        if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
        {
            adc_raw = HAL_ADC_GetValue(&hadc1);
            real_voltage = (((float)adc_raw * 3.3f) / 4095.0f) * 4.03f;
            printf("[LOGIC] V: %.2f\r\n", real_voltage);
        }
        osDelay(100);
    }

    printf("[LOGIC] Napiecie OK/Przetwornica ON, silnik/enkoder ENABLE\r\n");
    HAL_GPIO_WritePin(GPIOB, PWR_CONVERTER_EN_Pin, GPIO_PIN_SET);
    osDelay(2000);
    HAL_GPIO_WritePin(GPIOA, MOTOR_EN_Pin, GPIO_PIN_RESET);
    osDelay(500);

    //obudzenie enkodera
    HAL_GPIO_WritePin(GPIOB, ENC_EN_Pin, GPIO_PIN_SET);

    printf("[LOGIC] Konfiguracja rejestrów\r\n");

        // --- ZABEZPIECZENIA ---
        // Czyszczenie smieci z GSTAT po wlaczeniu zasilania
        Msg.address = 0x81; Msg.value = 0x00000007;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // SW_MODE ==> właczenie sprzetowego stopu przy wykryciu uderzenia (sg_stop = bit 10)
        Msg.address = 0xB4; Msg.value = 0x00000400;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        //TCOOLTHRS: StallGuard działa tylko, gdy silnik kręci się
        // szybciej niż ta wartość (żeby nie wyłączał się przy ruszaniu z miejsca)
        Msg.address = 0x94; Msg.value = 400;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // COOLCONF: StallGuard Threshold = SGT
        // Wartość 0x00050000 (średnia czułość), zmienic jesli bedzie za mała lub za duza
        Msg.address = 0xED; Msg.value = 0x00050000;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // XACTUAL na 0 - zeby nie myslał ze jest u celu juz
        Msg.address = 0xA1; Msg.value = 0x00000000;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // --- ZASILANIE I CHOPPER ---
        // CHOPCONF przeklejony z PDF od TMC
        Msg.address = 0xEC; Msg.value = 0x000100C3;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // IHOLD_IRUN: IRUN na 31 (MAX)
        Msg.address = 0x90; Msg.value = 0x00061F0A;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // TPOWERDOWN
        Msg.address = 0x91; Msg.value = 0x0000000A;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // EN_PWM_MODE (odpala StealthChop - cicha praca)
        Msg.address = 0x80; Msg.value = 0x00000004;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        // TPWM_THRS
        Msg.address = 0x93; Msg.value = 0x000001F4;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        osDelay(1000);

        // --- RAMPA I RUCH ---
        printf("[LOGIC] Konfiguracja rampy \r\n");

        // Parametry rampy przeklejone 1:1 z dokumentacji producenta
        Msg.address = 0xA4; Msg.value = 1000;   osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever); // A1
        Msg.address = 0xA5; Msg.value = 50000;  osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever); // V1
        Msg.address = 0xA6; Msg.value = 500;    osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever); // AMAX
        Msg.address = 0xA7; Msg.value = 200000; osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever); // VMAX
        Msg.address = 0xA8; Msg.value = 700;    osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever); // DMAX
        Msg.address = 0xAA; Msg.value = 1400;   osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever); // D1
        Msg.address = 0xAB; Msg.value = 10;     osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever); // VSTOP

        // RAMPMODE = 0 tryb pozycjonowania
        Msg.address = 0xA0; Msg.value = 0;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        //--- ENKODER ---
        printf("[LOGIC] Uruchomienie zliczania z enkodera \r\n ");

        //ENCMODE - domyslne zliczanie
        Msg.address = 0xB8; Msg.value = 0x00000000;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        //ENC_CONST - mnoznik
        Msg.address = 0xBA; Msg.value = 0x00010000;
        osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);



        while(1)
        {
        	switch(currentState)
        	{
        	case STATE_CALIBRATION:
        		printf("[STATE] Kalibracja silnika \r\n");

        		//Zmiana trybu na Velocity RAMPMODE = 1
        		Msg.address = 0xA0; Msg.value = 1;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		// Podajemy maksymalną prędkość (np. 150 000)
        		Msg.address = 0xA7; Msg.value = 150000;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		osDelay(4000);

        		// Odcinamy prędkość na zero, zeby odblokować układ po zderzeniu
        		Msg.address = 0xA7; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		printf("Wychylenie max ==> jazda na środek \r\n");

        		//XACTUALL: Dajemy tutaj nowy pkt odniesienia
        		Msg.address = 0xA1; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		//Tryb pozycjonowania, RAMPMODE = 0
        		Msg.address = 0xA0; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		//Zwiekszenie prędkości
        		Msg.address = 0xA7; Msg.value = 200000;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		//Jazda na połowe / mnienic ew. na "-"
        		Msg.address = 0xAD; Msg.value = -250000;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		osDelay(4000);

        		printf("[STATE] Koniec kalibracji \r\n");

        		//XACTUALL: Ustawienie obecniej pozycji jako zerowej/startowej
        		Msg.address = 0xA1; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		//X_ENC - reset pozycji enkodera
        		Msg.address = 0xB9; Msg.value = 0; // X_ENC = 0
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		currentState = STATE_RUNNING;
        		break;

        	case STATE_RUNNING:
        		//narazie testowy ping pong mozna by powiedzieć wokol zera XD
        		//potem tutaj bedzie sterowane po CANie z joysticka
        		printf("[RUNNING] Jazda na 50000 \r\n");

        		//XTARGET
        		Msg.address = 0xAD; Msg.value = 50000;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		osDelay(3000);

        		//Odczyt enkodera (podwójny by miec pewnośc ze nie odczytami smieci)
        		Msg.address = 0x39; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);
        		Msg.address = 0x39; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);
        		osDelay(500);

        		printf("[RUNNING] Jazda na -51200 \r\n");

        		Msg.address = 0xAD; Msg.value = -51200;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);

        		osDelay(3000);

        		// Ponowny odczyt enkodera
        		Msg.address = 0x39; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);
        		Msg.address = 0x39; Msg.value = 0;
        		osMessageQueuePut(spiQueueHandle, &Msg, 0, osWaitForever);
        		osDelay(500);

        		break;
        	}
        }
}

void vCANTask(void *argument)
{
	//tu bedzie logika od odbierania ramek danych w joysticka
	while(1)
	{
		osDelay(100);
	}
}

void vSPITask(void *argument) {
    SPI_Message_t ReceivedMsg;
    uint8_t txData[5], rxData[5];
    while(1) {
        if(osMessageQueueGet(spiQueueHandle, &ReceivedMsg, NULL, osWaitForever) == osOK) {

        	//Przygotowanie ramki TX
        	txData[0] = ReceivedMsg.address;
            txData[1] = (ReceivedMsg.value >> 24) & 0xFF;
            txData[2] = (ReceivedMsg.value >> 16) & 0xFF;
            txData[3] = (ReceivedMsg.value >> 8)  & 0xFF;
            txData[4] = ReceivedMsg.value         & 0xFF;

            //Transmisja SPI
            HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin, GPIO_PIN_RESET);
            HAL_SPI_TransmitReceive(&hspi1, txData, rxData, 5, HAL_MAX_DELAY);
            HAL_GPIO_WritePin(GPIOA, TMC_CS_Pin, GPIO_PIN_SET);

            //32 - bitowa wartosc RX
            uint32_t rxValue = ((uint32_t)rxData[1] << 24) |
                    			((uint32_t)rxData[2] << 16) |
								((uint32_t)rxData[3] << 8)  |
								 (uint32_t)rxData[4];

            //LOG ==> TX: Adres -> Wartość | RX: Status | Wartość odebrana
            printf("[SPI] TX: 0x%02X -> 0x%08lX | RX STAT: 0x%02X | RX DATA: 0x%08lX\r\n",
                    txData[0],
					ReceivedMsg.value,
					rxData[0],
					rxValue);

            //LOG dla Enkodera
            if(txData[0] == 0x39)
            {
            	printf("ODCZYT ENKODERA POZYCJA: %ld \r\n", (int32_t)rxValue);
            }
        }
    }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
