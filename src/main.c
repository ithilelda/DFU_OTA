#include "HAL.h"
#include "peripheral.h"


__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
/*********************************************************************
 * @fn      main
 *
 * @brief   主函数，初始化整个HAL层和BLE固件。比较固定，大部分初始化业务都交给具体的项目同名C文件，循环交给TMOS。
 *
 * @return  none
 */
int main()
{
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_20mA);
    CH57X_BLEInit();
    HAL_Init();
    OTA_Init();
    while(1)
    {
        TMOS_SystemProcess();
    }
}
