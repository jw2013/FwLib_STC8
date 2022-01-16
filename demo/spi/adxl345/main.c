// Copyright 2021 IOsetting <iosetting(at)outlook.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/***
 * Demo: ADXL345
 * Board: STC8H8K64U
 *                                8H8K64U
 * GND       -> GND               G
 * VDD(Vcc)  -> VCC               V
 * CS        -> GPIO OUT P35      34
 * INT1      -> GPIO IN  P36 INT2 35
 * INT2      -> GPIO IN  P37 INT3 36
 * SDO       -> MISO     P33      30
 * SDI/SDA   -> MOSI     P34      31
 * SCL       -> SCLK     P32      29
 */

#include "fw_hal.h"
#include "adxl345.h"
#include <stdio.h>

uint8_t buf[6], count_int2 = 0, count_int3 = 0, count_double_tap = 0;

void SPI_Init(void)
{
    // ADXL345, SPI CLK max frequency is 5MHz
    SPI_SetClockPrescaler(SPI_ClockPreScaler_16);
    // Clock is high when idle
    SPI_SetClockPolarity(HAL_State_ON);
    // Data transfer is driven by lower SS pin
    SPI_SetClockPhase(SPI_ClockPhase_TrailingEdge);
    // MSB first
    SPI_SetDataOrder(SPI_DataOrder_MSB);
    // Define the output pins
    SPI_SetPort(SPI_AlterPort_P35_P34_P33_P32);
    // Ignore SS pin, use MSTR to swith between master/slave mode
    SPI_IgnoreSlaveSelect(HAL_State_ON);
    // Master mode
    SPI_SetMasterMode(HAL_State_ON);
    // Start SPI
    SPI_SetEnabled(HAL_State_ON);
}

void GPIO_Init(void)
{
    // Configure GPIO pins before SPI and device
    // MISO(P33), MOSI(P34)
    GPIO_P3_SetMode(GPIO_Pin_3|GPIO_Pin_4, GPIO_Mode_InOut_QBD);
    // SCLK(P32), CS(P35), 
    GPIO_P3_SetMode(GPIO_Pin_2|GPIO_Pin_5, GPIO_Mode_Output_PP);
    // INT2(P36), INT3(P37)
    GPIO_P3_SetMode(GPIO_Pin_6|GPIO_Pin_7, GPIO_Mode_Input_HIP);
}

void INT_Init()
{
    EXTI_Int2_SetIntState(HAL_State_ON);
    EXTI_Int3_SetIntState(HAL_State_ON);
    EXTI_Global_SetIntState(HAL_State_ON);
}

INTERRUPT(Int2_Routine, EXTI_VectInt2)
{
    count_int2++;
    if (ADXL345_IsInterrupt(ADXL345_INT_DOUBLE_TAP) > 0)
    {
        count_double_tap++;
    }
}

INTERRUPT(Int3_Routine, EXTI_VectInt3)
{
    count_int3++;
}

int main(void)
{
    uint8_t intsrc;
    int16_t x, y, z;
    SYS_SetClock();
    GPIO_Init();
    UART1_Config8bitUart(UART1_BaudSource_Timer1, HAL_State_ON, 115200);
    SPI_Init();
    INT_Init();
    ADXL345_Init(
        ADXL345_DATARATE_100_HZ,
        ADXL345_SPI_WIRE_4,
        ADXL345_INT_ACTIVE_LOW,
        ADXL345_DATA_RESOLVE_FULL,
        ADXL345_DATA_ALIGNMENT_RIGHT,
        ADXL345_G_RANGE_8G
    );
    ADXL345_WriteByte(ADXL345_REG_THRESH_TAP, 0x2F);
    ADXL345_WriteByte(ADXL345_REG_DUR, 0x1F);
    ADXL345_WriteByte(ADXL345_REG_LATENT, 0x1F);
    ADXL345_WriteByte(ADXL345_REG_WINDOW, 0x7F);
    ADXL345_EnableTapDetectOnAxes(
        ADXL345_TAP_DETECT_AXIS_X|ADXL345_TAP_DETECT_AXIS_Y|ADXL345_TAP_DETECT_AXIS_Z);
    ADXL345_SetInterrupts(
        ADXL345_INT_DATA_READY|ADXL345_INT_SINGLE_TAP|ADXL345_INT_DOUBLE_TAP);
    ADXL345_RemapInterrupts(ADXL345_INT_DATA_READY);

    while(1)
    {
        buf[0] = ADXL345_ReadByte(ADXL345_REG_DATAX0);
        buf[1] = ADXL345_ReadByte(ADXL345_REG_DATAX1);
        buf[2] = ADXL345_ReadByte(ADXL345_REG_DATAY0);
        buf[3] = ADXL345_ReadByte(ADXL345_REG_DATAY1);
        buf[4] = ADXL345_ReadByte(ADXL345_REG_DATAZ0);
        buf[5] = ADXL345_ReadByte(ADXL345_REG_DATAZ1);
        x = *((int16_t *)&buf[0]);
        y = *((int16_t *)&buf[2]);
        z = *((int16_t *)&buf[4]);
        printf("X:%6d, Y:%6d, Z:%6d, DAT:%3d, TAP:%3d, 2-TAP:%3d\r\n", 
            x, y, z, count_int2, count_int3, count_double_tap);
        SYS_Delay(100);
    }
}
