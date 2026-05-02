/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "fatfs.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"
#include "fsmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "ff.h"
#include "w25q64.h"
#include "lcd.h"
#include "MLX90640.h"
#include <MLX90640_API.h>
#include <MLX90640_I2C_Driver.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t save_image_flag = 0;  // 保存图像标志
volatile uint8_t capture_in_progress = 0;  // 屏幕保存期间标志
uint8_t bmp_header[54];  // BMP头缓冲区
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void SaveScreenBMP(void);

extern volatile uint8_t usb_storage_busy;

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
	paramsMLX90640 mlx90640;
	uint8_t x = 0;
    int refreshRate = 0;
    uint8_t iic_addr;
	uint8_t lcd_id[12];
    
	static uint16_t eeMLX90640[832];  
	uint16_t frame[834];
	uint16_t mlx90640_Zoom10[834];  
	uint8_t DisBuf[10*320];
	float mlx90640To[768];
    
	uint16_t statusRegister;
    float Ta,vdd,tr;
	float emissivity=0.95f;
	int status;
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
  MX_ADC1_Init();
  MX_FSMC_Init();
  MX_USB_DEVICE_Init();
  MX_SPI3_Init();
  W25Q64_Init();
  MX_USART6_UART_Init();
  MX_I2C2_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  MLX90640_I2CInit();
  
  lcd_init();                             /* ��ʼ��LCD */
  led_init();                             /* ��ʼ��LED */
  g_point_color = RED;
  MLX90640_SetRefreshRate(0x33, 0x04);  //����֡��
  refreshRate = MLX90640_GetRefreshRate(0x33);
  MLX90640_SetChessMode(0x33);                 //����ģʽ  
  HAL_Delay(50);
  status = MLX90640_DumpEE(0x33, eeMLX90640);  //��ȡ����У��ϵ��
  //if (status != 0) printf("\r\nload system parameters error with code:%d\r\n",status);
  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);  //����У��ϵ��
  //if (status != 0) printf("\r\nParameter extraction failed with error code:%d\r\n",status);
  for(statusRegister=0;statusRegister<3;statusRegister++)
  {
          MLX90640_GetFrameData(0x33, mlx90640_Zoom10);
          Ta = MLX90640_GetTa(mlx90640_Zoom10, &mlx90640);  
        MLX90640_CalculateTo(mlx90640_Zoom10, &mlx90640, emissivity , Ta - 8.0, mlx90640To);
                  
          //Disp_TempPic();
  }
    
  lcd_clear(WHITE);
  lcd_show_string(10, 40, 240, 32, 32, "STM32", RED);
  lcd_set_window(0, 0, 319, 239);
/*-----------------------------------------------------------------*/
  //���Թ����ļ�ϵͳ�����ļ�ϵͳ����ʧ�ܣ������¸�ʽ����
	FRESULT res;                           
	printf("Mounting the FATFS...\n");
	res = f_mount(&USERFatFS,"0:",1);
	if(res == FR_OK)
	{
		printf("Mount_OK\n");
	}
	else
	{
		printf("Mount_Error:%d\n",res);
		BYTE WorkBuffer[4096];
		DWORD cluster_size = 0;
		printf("MKFS-ing...\n");

		res = f_mkfs("0:",FM_FAT,cluster_size,WorkBuffer,FLASH_SECTOR_SIZE);
		if(res == FR_OK)
		{
			printf("MKFS_OK\n");

			res = f_mount(&USERFatFS, "0:", 1);		// ���¹����ļ�ϵͳ
			if (res == FR_OK) 
			{
					printf("MOUNT_OK\n");
			} 
			else 
			{
					printf("Mount_Error:%d\n",res);
			}
		}
		else
		{
			printf("MKFS_ERROR:%d\n",res);
		}
	}
		
//	�ļ���д����
		char FILE_NAME[32] = "test.txt";  // �����ļ���
		char FILE_CONTENT[32] = "Hello, FATFS!";  // д���ļ�������
		FIL file;     // �ļ����
    UINT bytes_written;  // д����ֽ���
    UINT bytes_read;     // ��ȡ���ֽ���
    char read_buffer[100];  // ��ȡ������

    // ���ļ�������ļ������ڣ��򴴽���
    res = f_open(&file, FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK)
    {
        printf("File opened/created successfully.\n");

        // д�����ݵ��ļ�
        res = f_write(&file, FILE_CONTENT, strlen(FILE_CONTENT), &bytes_written);
        if (res == FR_OK)
        {
            printf("Data written successfully. Bytes written: %u\n", bytes_written);
        }
        else
        {
            printf("Write error: %d\n", res);
        }

        // �ر��ļ�
        f_close(&file);
    }
    else
    {
        printf("Failed to open/create file. Error: %d\n", res);
    }

    // �ٴδ��ļ����ж�ȡ
    res = f_open(&file, FILE_NAME, FA_READ);
    if (res == FR_OK)
    {
        printf("File opened for reading successfully.\n");

        // ��ն�ȡ������
        memset(read_buffer, 0, sizeof(read_buffer));

        // ��ȡ�ļ�����
        res = f_read(&file, read_buffer, sizeof(read_buffer) - 1, &bytes_read);
        if (res == FR_OK)
        {
            printf("Data read successfully. Bytes read: %u\n", bytes_read);
            printf("File content: %s\n", read_buffer);
        }
        else
        {
            printf("Read error: %d\n", res);
        }

        // �ر��ļ�
        f_close(&file);
    }
    else
    {
        printf("Failed to open file for reading. Error: %d\n", res);
    }

  
  
/*-----------------------------------------------------------------*/
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (save_image_flag && !capture_in_progress && !usb_storage_busy)
    {
        capture_in_progress = 1;
        save_image_flag = 0;
        printf("Capture triggered, stopping display...\r\n");
        SaveScreenBMP();
        capture_in_progress = 0;
        printf("Capture complete, resuming display.\r\n");
    }

    if (!capture_in_progress && !usb_storage_busy)
    {
        status = MLX90640_GetFrameData(0x33, frame);  // ��ȡһ֡ԭʼ��
        if (status >= 0)
        {
            if (status == 1)
            {
                printf("GetFrameData subpage 1 (Chess Mode)\r\n");
            }
            vdd = MLX90640_GetVdd(frame, &mlx90640);
            Ta = MLX90640_GetTa(frame, &mlx90640);
            tr = Ta - 8.0;
            MLX90640_CalculateTo(frame, &mlx90640, emissivity, tr, mlx90640To);
            MLX90640_BadPixelsCorrection(mlx90640.brokenPixels, mlx90640To, 1, &mlx90640);
            MLX90640_BadPixelsCorrection(mlx90640.outlierPixels, mlx90640To, 1, &mlx90640);
            Disp_TempPic();
        }
        else
        {
            printf("GetFrameData failed: %d\r\n", status);
        }
    }

    LED_TOGGLE();
    HAL_Delay(10);
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
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_2)
    {
        printf("PA2 button pressed, requesting capture...\r\n");
        save_image_flag = 1;
    }
}

void SaveScreenBMP(void)
{
    uint16_t width = lcddev.width;
    uint16_t height = lcddev.height;
    uint32_t line_size = width * 3;
    uint32_t row_pad = (4 - (line_size % 4)) % 4;
    uint32_t image_size = (line_size + row_pad) * height;
    uint32_t file_size = 54 + image_size;

    BITMAPFILEHEADER bmfh;
    BITMAPINFOHEADER bmih;

    bmfh.bfType = 0x4D42;
    bmfh.bfSize = file_size;
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;
    bmfh.bfOffBits = 54;

    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = width;
    bmih.biHeight = height;
    bmih.biPlanes = 1;
    bmih.biBitCount = 24;
    bmih.biCompression = 0;
    bmih.biSizeImage = image_size;
    bmih.biXPelsPerMeter = 2835;
    bmih.biYPelsPerMeter = 2835;
    bmih.biClrUsed = 0;
    bmih.biClrImportant = 0;

    memcpy(bmp_header, &bmfh, sizeof(BITMAPFILEHEADER));
    memcpy(bmp_header + sizeof(BITMAPFILEHEADER), &bmih, sizeof(BITMAPINFOHEADER));

    char filename[32];
    static int file_count = 0;
    sprintf(filename, "0:IMG%03d.BMP", file_count++);

    FIL file;
    FRESULT res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("Failed to open BMP file: %d\r\n", res);
        return;
    }

    UINT bytes_written;
    res = f_write(&file, bmp_header, 54, &bytes_written);
    if (res != FR_OK)
    {
        printf("BMP header write failed: %d\r\n", res);
        f_close(&file);
        return;
    }

    uint8_t bmp_line[480 * 3 + 3];
    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < width; x++)
        {
            uint16_t pixel = lcd_read_point(x, y);
            uint8_t r = ((pixel >> 11) & 0x1F) * 255 / 31;
            uint8_t g = ((pixel >> 5) & 0x3F) * 255 / 63;
            uint8_t b = (pixel & 0x1F) * 255 / 31;
            bmp_line[x * 3 + 0] = b;
            bmp_line[x * 3 + 1] = g;
            bmp_line[x * 3 + 2] = r;
        }
        res = f_write(&file, bmp_line, line_size, &bytes_written);
        if (res != FR_OK)
        {
            printf("BMP line write failed at y=%d: %d\r\n", y, res);
            break;
        }
        if (row_pad)
        {
            static const uint8_t pad[3] = {0, 0, 0};
            res = f_write(&file, pad, row_pad, &bytes_written);
            if (res != FR_OK)
            {
                printf("BMP pad write failed at y=%d: %d\r\n", y, res);
                break;
            }
        }
    }

    if (res == FR_OK)
    {
        f_sync(&file);
        printf("Saved screen image: %s\r\n", filename);
    }
    else
    {
        printf("Failed to save BMP file: %d\r\n", res);
    }
    f_close(&file);
}

/* USER CODE END 4 */

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */

