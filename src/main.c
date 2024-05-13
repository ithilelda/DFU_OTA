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
    uint32_t boot_ota;
    EEPROM_READ(EEPROM_DATA_ADDR, &boot_ota, sizeof(boot_ota));
    if(!boot_ota) // only the number 0 will boot the app because the flash will be erased to 1's. You have to actively set it to be 0, so 0 means set.
    {
        jumpApp();
    }
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
