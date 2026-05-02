#ifndef __MLX90640_H
#define	__MLX90640_H

#include "main.h"
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"



extern float Ta;
extern float emissivity;
//extern uint16_t mlx90640To[768];
//extern uint16_t mlx90640_Zoom10[834];
//extern uint32_t BatteryVal;
//extern uint8_t  BatChrg_Sta;  //电池充电状态，1表示正在充电

void Disp_TempPic(void);
void Disp_Color_Bar(void);
//void Disp_BatPower(void);
//void Disp_test();

#endif
