/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define FLASH_ADRESS_PASS	0x0800FC00
#define FLASH_ADRESS_PHONE	0x0800F800
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
char rxData[2];
char rxBuffer[200];
int rCount = 0;
int rxindex = 0;
int smsRecFlag = 0;

volatile char responsePhone[14];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void clearBuffer() {
	rCount = 0;
	rxindex = 0;
	int i = 0;
	for (i = 0; i < 200; i++) {
		rxBuffer[i] = '\0';
	}
}
//TODO:SMS GONDERDİKTEN SONRA, ALDIĞI İLK SMS E İŞLEM YAPMIYOR. GONDERDİĞİ IKINCI DEN SONR
void sendSms(char *message, char *number) {

	char str[32];
	strcpy(str, "AT+CMGS=\"");
	strcat(str, number);
	strcat(str, "\"\n");
	HAL_UART_Transmit(&huart1, str, 24, 1000);
	HAL_Delay(1000);

	int smsSize = strlen(message);
	HAL_UART_Transmit(&huart1, message, smsSize, 1000);
	HAL_Delay(1000);
	char foo[] = { 26 }; //Sonlandırma Karakteri 1Ah
	int s;
	s = sizeof(foo);
	HAL_UART_Transmit(&huart1, foo, s, 1000);
	HAL_Delay(1000);


}

void halWriteFlash(uint16_t pass,uint16_t phone1,uint16_t phone2) {
	HAL_FLASH_Unlock();
	FLASH_PageErase(0x0800FC00);

	//0x00000042 in tersleniği 0xFFFFFFBD ile AND işlemine tabi tut. \
	Diğer Bitler Değişmeden CR.PER=0 ve CR.STRT=0 yapılıyor.
	FLASH->CR &= ~0x00000042;

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, 0x0800FC00, pass);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, 0x0800FC02, phone1);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, 0x0800FC04, phone2);

	HAL_FLASH_Lock();
}


uint16_t charPassToInt(char pass[]){

	uint32_t resultPass=atoi(pass);
	return resultPass;
}

void halfWordPhonetoChar(uint16_t p0,uint16_t p1, char *resultPhone){
	uint32_t phone32=p1;
	phone32=phone32<<16;
	phone32=phone32|p0;

	char str[14];
	char temp[14];
	sprintf(str, "%d", phone32);
	for(int i=0;i<14;i++){
		temp[i+4]=str[i];
	}

	temp[3]='5';
	temp[2]='0';
	temp[1]='9';
	temp[0]='+';
	strcpy(resultPhone, temp);

}

uint16_t phoneCharToIntLastSeg(char phoneNumber[]){
		char phones[10];
		for(int i=0;i<10;i++){
			phones[i]=phoneNumber[4+i];
		}
		phones[9]='\0';
		uint32_t pNumber=atoi(phones);
		uint16_t resultPhoneSegMSB=pNumber>>16;
		return resultPhoneSegMSB;
}


uint16_t phoneCharToIntFirstSeg(char phoneNumber[]){
		char phones[10];
		for(int i=0;i<10;i++){
			phones[i]=phoneNumber[4+i];
		}
		phones[9]='\0';
		uint32_t pNumber=atoi(phones);
		uint16_t resultPhoneSegLSB=pNumber&0x0000FFFF;
		return resultPhoneSegLSB;
}



void removeSpecialChar(char *text, char removed_char) {
	char *src, *temp;
	for (src = temp = text; *src != '\0'; src++) {
		*temp = *src;
		if (*temp != removed_char) {
			temp++;
		}
	}
	*temp = '\0';
}

int16_t charToAscii(char *pass) {
	uint8_t sayilar = 0;
	uint8_t karakterler = 0;
	int temp = 0;
	for (karakterler = 0; karakterler < 4; karakterler++) {
		for (sayilar = 0; sayilar < 10; sayilar++) {
			if (pass[karakterler] == sayilar + 48) {
				temp = temp << 4;
				temp = temp | sayilar;
			}
		}
	}
	return temp;
}

void getCommandsFromSms(char *sms, char *resultPassw, char *resultCommand,char *resultSubCmd, char *resultCmdTarget) {

	//TODO Komuttan Başka Mesaj geldiğinde Çkış Yapmalı


	char dizi[200];
	strcpy(dizi, sms);
	char *pass = strtok(dizi, "_");
	char *command = strtok(NULL, "_");
	char *subCommand = strtok(NULL, "_");
	char *commandTarget = strtok(NULL, "_");
	uint8_t leght = strlen(pass);
	char tempPassw[5];

	if (leght > 4) {

		tempPassw[4] = '\0';
		tempPassw[3] = pass[leght - 1];
		tempPassw[2] = pass[leght - 2];
		tempPassw[1] = pass[leght - 3];
		tempPassw[0] = pass[leght - 4];
	}

	removeSpecialChar(tempPassw, '\r');
	removeSpecialChar(tempPassw, '\n');
	strcpy(resultPassw, tempPassw);

	removeSpecialChar(command, '\r');
	removeSpecialChar(command, '\n');
	strcpy(resultCommand, command);

	removeSpecialChar(subCommand, '\r');
	removeSpecialChar(subCommand, '\n');
	strcpy(resultSubCmd, subCommand);

	removeSpecialChar(commandTarget, '\r');
	removeSpecialChar(commandTarget, '\n');
	strcpy(resultCmdTarget, commandTarget);
}

const char* phoneNumber(char *sms) {

	int phoneNumberStart = 0;
	int phoneNumberStop = 0;
	int phoneNumberLeght = 0;
	char *tempSMS = sms;

	uint8_t i = 0;
	while (*tempSMS != '+') {
		tempSMS++;
		phoneNumberStart++;
	}
	tempSMS++;
	while (*tempSMS != '+') {
		tempSMS++;
		phoneNumberStart++;
	}
	phoneNumberStop = 13;
	char *phoneNumberVal = (char*) malloc(phoneNumberStop);
	char dizi[200];
	strcpy(dizi, tempSMS);
	strncpy(phoneNumberVal, dizi, phoneNumberStop);
	phoneNumberVal[phoneNumberStop] = '\0';
	return phoneNumberVal;
}

const char* atCommand(char *sms) {
	int atCommandStart = 0;
	int atCommandStop = 0;
	int atCommandLeght = 0;
	char *tempCommand = sms;
	int lenSms = strlen(sms);
	int i = 0;

	while ((*tempCommand != '+') && (atCommandStart < lenSms)) {
		tempCommand++;
		atCommandStart++;
	}
	tempCommand = sms;
	while ((*tempCommand != ':') && (atCommandStart < lenSms)) {
		tempCommand++;
		atCommandStop++;
	}
	atCommandLeght = atCommandStop - atCommandStart;
	if (atCommandLeght > 0) {
		char *atCommandVal = (char*) malloc(atCommandLeght);

		char dizi[lenSms];
		strcpy(dizi, sms + atCommandStart + 1);
		strncpy(atCommandVal, dizi, atCommandLeght);
		atCommandVal[atCommandLeght] = '\0';
		return atCommandVal;
	} else {
		return NULL;
	}
}

//TODO:rxBuffer i parametre olarak almak gerekebilir
void CMT_Message() {


	volatile char passw[6];//sms den gelen sifre
	volatile char command[6];//sms den gelen komut
	volatile char subCommand[6];//sms den gene alt komut
	volatile char commandTarget[5];//sms den gelen  komut hedefi

	const char *phone = phoneNumber(rxBuffer);//sms in atıldığı telefon numarasını alır

	getCommandsFromSms(rxBuffer, passw, command, subCommand, commandTarget);//sms içindeki passw vb alır

	//uint16_t tempPass = charToAscii(passw);
	//uint32_t tempPhone=charToAscii(phone);

	uint16_t passIntSromSMS=charPassToInt(passw);
	volatile uint16_t resultPhoneSegLSB_FromSMS;
	volatile uint16_t resultPhoneSegMSB_FromSMS;
	resultPhoneSegLSB_FromSMS=phoneCharToIntFirstSeg(phone);
	resultPhoneSegMSB_FromSMS=phoneCharToIntLastSeg(phone);
	//Flashdaki son sektor okunuyor
	uint16_t fPass = *(uint16_t*) 0x0800FC00;
	uint16_t fphoneLSB=*(uint16_t*) 0x0800FC02;//flash telefon numarasının lsb si ///TELEFON NUMARARININ BAŞINDA +905 yok
	uint16_t fphoneMSB=*(uint16_t*) 0x0800FC04;//flash da bulunan telefon numarasının msb si

	if(fPass==65535){fPass=0;}//ilk kez çalıştırılıyorsa şifre 0000 girilmeli. flash daki değer 65535 0xFFFF
	if (passIntSromSMS==fPass) {

		/* SMS den gelen telefon numarası yada daha önceden kaydedilmiş flash dan okunan Telefon Numarasının cevap numarası için kullanılıyor
		 * şu anda kumut atılmış numaradan sms den gelen numaraya dönüş yapılıyor. phone "sms den gelen telefon numarasıdır"
		volatile char responsePhone[14];
		halfWordPhonetoChar(resultPhoneSegLSB_FromSMS,resultPhoneSegMSB_FromSMS, responsePhone);
		*/
		if (!(strcmp(command, "Passw"))) {	//ESKI_Passw_Chg_YENI
			//PASSWORD KOMUT FONKSIYONLARI
			if (!(strcmp(subCommand, "Chg"))) {
				uint16_t passFromCharToInt=charPassToInt(commandTarget);
				halWriteFlash(passFromCharToInt, resultPhoneSegLSB_FromSMS, resultPhoneSegMSB_FromSMS);

				sendSms("Sifre ve Cevap Sms Numarasi Degistirildi",phone);
				//changePass(charToAscii(commandTarget));
			}
		}



		if (!(strcmp(command, "Start\0"))) {
			//START KOMUT FONKSIYONLARI
			if (!(strcmp(subCommand, "Motor\0"))) {
				if (!(strcmp(commandTarget, "001"))) {
					HAL_GPIO_WritePin(DG_OUTPUT_0_GPIO_Port, DG_OUTPUT_0_Pin,
							GPIO_PIN_SET);
					sendSms("OK", phone);

				}
				if (!(strcmp(commandTarget, "002"))) {
					HAL_GPIO_WritePin(DG_OUTPUT_1_GPIO_Port, DG_OUTPUT_1_Pin,
							GPIO_PIN_SET);
					sendSms("OK", phone);

				}

				if (!(strcmp(commandTarget, "003"))) {
					HAL_GPIO_WritePin(DG_OUTPUT_2_GPIO_Port, DG_OUTPUT_2_Pin,
							GPIO_PIN_SET);
					sendSms("OK", phone);

				}

				if (!(strcmp(commandTarget, "004"))) {

					HAL_GPIO_WritePin(DG_OUTPUT_3_GPIO_Port, DG_OUTPUT_3_Pin,
							GPIO_PIN_SET);
					sendSms("OK", phone);

				}
			}
		}
		if (!(strcmp(command, "Stop\0"))) {
			//STOP KOMUT FONKSIYONLARI
			if (!(strcmp(subCommand, "Motor"))) {
				if (!(strcmp(commandTarget, "001"))) {
					HAL_GPIO_WritePin(DG_OUTPUT_0_GPIO_Port, DG_OUTPUT_0_Pin,
							GPIO_PIN_RESET);
					sendSms("OK", phone);

				}
				if (!(strcmp(commandTarget, "002"))) {
					HAL_GPIO_WritePin(DG_OUTPUT_1_GPIO_Port, DG_OUTPUT_1_Pin,
							GPIO_PIN_RESET);
					sendSms("OK", phone);

				}

				if (!(strcmp(commandTarget, "003"))) {
					HAL_GPIO_WritePin(DG_OUTPUT_2_GPIO_Port, DG_OUTPUT_2_Pin,
							GPIO_PIN_RESET);
					sendSms("OK", phone);

				}

				if (!(strcmp(commandTarget, "004"))) {
					HAL_GPIO_WritePin(DG_OUTPUT_3_GPIO_Port, DG_OUTPUT_3_Pin,
							GPIO_PIN_RESET);
					sendSms("OK", phone);

				}
			}
		}

		if (!(strcmp(command, "Check\0"))) {
			//KONTROL KOMUT FONKSIYONLARI
			if (!(strcmp(subCommand, "Input\0"))) {
				if (!(strcmp(commandTarget, "001"))) {
					int t = HAL_GPIO_ReadPin(DG_INPUT_0_GPIO_Port,
					DG_INPUT_0_Pin);
					if (t == 0) {
						sendSms("INPUT 001 RESET", phone);
					}

					if (t == 1) {
						sendSms("INPUT 001 SET", phone);
					}

				}
				if (!(strcmp(commandTarget, "002"))) {
					int t = HAL_GPIO_ReadPin(DG_INPUT_1_GPIO_Port,
					DG_INPUT_0_Pin);

					if (t == 0) {
						sendSms("INPUT 002 RESET", phone);
					}

					if (t == 1) {
						sendSms("INPUT 002 SET", phone);
					}
				}

				if (!(strcmp(commandTarget, "003"))) {
					int t = HAL_GPIO_ReadPin(DG_INPUT_2_GPIO_Port,
					DG_INPUT_2_Pin);
					if (t == 0) {
						sendSms("INPUT 003 RESET", phone);
					}

					if (t == 1) {
						sendSms("INPUT 003 SET", phone);
					}
				}
				if (!(strcmp(commandTarget, "004"))) {
					int t = HAL_GPIO_ReadPin(DG_INPUT_3_GPIO_Port,
					DG_INPUT_3_Pin);
					if (t == 0) {
						sendSms("INPUT 004 RESET", phone);
					}

					if (t == 1) {
						sendSms("INPUT 004 SET", phone);
					}
				}
			}
			if (!(strcmp(subCommand, "Sytem"))) {
				if (!(strcmp(commandTarget, "Res"))) {
					HAL_NVIC_SystemReset();

				}

			}
		}

	}
}

char getTimeFromSim(char *message, char time[]) {

	int i = 0;
	for (i = 0; i < 18; i++) {
		time[i] = message[10 + i];
	}
	time[17] = NULL;

}

void CCLK_Message() {
	char myTime[18];
	getTimeFromSim(rxBuffer, myTime);
}

//TODO: rxBuffer parametre olarak almak gerekir
void readSMS() {
	//int bufferSize = strlen(rxBuffer) - 1;
	//int mesajSize = 21;
	//int bufferSplitSize = bufferSize - mesajSize;

	const char *atCommandFromSms = atCommand(rxBuffer);	//rxBuffer usart interrupt tarafından dolduruluyor

	//TODO: SMS ATTIKTAN SONRA CMGS: mesajı geliyor nedir ?
	if (!(strcmp(atCommandFromSms, "CMT:"))) {	//SMS
		CMT_Message();
	} else if (!(strcmp(atCommandFromSms, "CCLK:"))) {	//ZAMAN BILGISI
		CCLK_Message();
		/*Details:"\r\n+CMGS: 117\r\n\r\n", '\0' <repeats 183 times>
		 * //SMS ATTIKTAN SONRA GELİYOR.
		 */

	} else {
		__asm__("nop");
	}

	clearBuffer();
	HAL_Delay(1000);
}




void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		if (rxData[0] == '\r') {
			rCount++;
		}
		if (rCount == 2) {
			__asm__("nop");
			//VERİ GELDİ KONTROL ET
		}

		if (rCount == 3) {
			__asm__("nop");
			//SMS VERİ GELDİ KONTROL ET
			smsRecFlag = 1;

		}
		rxBuffer[rxindex++] = rxData[0];
		HAL_UART_Receive_IT(&huart1, rxData, 1);
	}
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

	//HAL_Delay(10000);
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */

	//AT+CNMI=2,2,0,0  AT+CNMI=1,2,0,0,0



/*
  	  char netData[20];
  	char netVal;
	while(netVal!="1") {	//SMS

		HAL_UART_Transmit(&huart1, "AT+CREG?\n", 9, 1000);
		HAL_UART_Receive(&huart1, netData, 20, 1000);
		//char *head = strtok(netData, "OK");
		//char *net = strtok(NULL, "\0");
		//netVal=net;
		HAL_Delay(500);
		HAL_GPIO_TogglePin(SYS_ERROR_LED_GPIO_Port, SYS_ERROR_LED_Pin);
	}

*/
  	HAL_UART_Receive_IT(&huart1, rxData, 1);	//USART INT ENABLE.

	HAL_UART_Transmit(&huart1, "ATE0\n", 5, 1000);
	HAL_Delay(1000);

	HAL_UART_Transmit(&huart1, "AT+CMGF=1\n", 10, 1000);	//SMS TEXT MODE
	HAL_Delay(1000);

	HAL_UART_Transmit(&huart1, "AT+CNMI=2,2,0,0\n", 16, 1000);
	HAL_Delay(1000);




	//HAL_UART_Transmit(&huart1, "AT+CNMI=1, 2, 0, 0, 0\n", 22, 1000);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	uint16_t fphoneLSB=*(uint16_t*) 0x0800FC02;
	uint16_t fphoneMSB=*(uint16_t*) 0x0800FC04;

	halfWordPhonetoChar(fphoneLSB,fphoneMSB, responsePhone);
	sendSms("Sistem Calismaya Basladi\0", responsePhone);

	//MX_IWDG_Init();
	while (1) {

		HAL_GPIO_TogglePin(SYS_RUN_LED_GPIO_Port,SYS_RUN_LED_Pin);
		//HAL_IWDG_Refresh(&hiwdg);	//WD RESET 27sec
		HAL_Delay(50);

		if (smsRecFlag) {
			NVIC_DisableIRQ(USART1_IRQn);
			readSMS();	//TODO: SMS DEĞİL MESAJ GELDİ
			smsRecFlag = 0;
			NVIC_EnableIRQ(USART1_IRQn);

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_4;
  hiwdg.Init.Window = 4095;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

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
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SYS_RUN_LED_Pin|SYS_ERROR_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, DG_OUTPUT_3_Pin|DG_OUTPUT_2_Pin|DG_OUTPUT_1_Pin|DG_OUTPUT_0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DG_INPUT_0_Pin DG_INPUT_1_Pin DG_INPUT_2_Pin DG_INPUT_3_Pin */
  GPIO_InitStruct.Pin = DG_INPUT_0_Pin|DG_INPUT_1_Pin|DG_INPUT_2_Pin|DG_INPUT_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SYS_RUN_LED_Pin SYS_ERROR_LED_Pin */
  GPIO_InitStruct.Pin = SYS_RUN_LED_Pin|SYS_ERROR_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DG_OUTPUT_3_Pin DG_OUTPUT_2_Pin DG_OUTPUT_1_Pin DG_OUTPUT_0_Pin */
  GPIO_InitStruct.Pin = DG_OUTPUT_3_Pin|DG_OUTPUT_2_Pin|DG_OUTPUT_1_Pin|DG_OUTPUT_0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

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

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
