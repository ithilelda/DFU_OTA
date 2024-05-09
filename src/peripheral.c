/*
 * bletester.c
 *
 *  Created on: Apr 5, 2024
 *      Author: huawe
 */


#include "config.h"
#include "peripheral.h"
#include "OTA_profile.h"


// Local variable definitions.
static uint8_t Main_TaskID;
static uint8_t advertData[31] = {
   // Flags; this sets the device to use limited discoverable mode (advertises indefinitely)
   0x02, // length of this data
   GAP_ADTYPE_FLAGS,
   GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

   // short name of the device.
   0x08,
   GAP_ADTYPE_LOCAL_NAME_SHORT,
   'D','F','U','_','O','T','A',

   // service uuid lists. usually, 16-bits uuids are reserved, so we use a 128-bits custom one. It is nicer to indicate here.
   0x11,
   GAP_ADTYPE_128BIT_MORE,
   CONSTRUCT_OTA_UUID(OTAPROFILE_SERVICE_UUID, OTAPROFILE_OTA_SERV_UUID)
};
static uint8_t scanRspData[31] = {
    // complete name
    0x08, // length of this data
    GAP_ADTYPE_LOCAL_NAME_SHORT,
    'D','F','U','_','O','T','A',
    // connection interval range
    0x05, // length of this data
    GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
    LO_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
    HI_UINT16(DEFAULT_DESIRED_MIN_CONN_INTERVAL),
    LO_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),
    HI_UINT16(DEFAULT_DESIRED_MAX_CONN_INTERVAL),

    // Tx power level
    0x02, // length of this data
    GAP_ADTYPE_POWER_LEVEL,
    0 // 0dBm
};
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "DFU_OTA";
static void IAP_GAPStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static gapRolesCBs_t IAP_GAPRoleCBs = {IAP_GAPStateNotificationCB, NULL, NULL};
static gapBondCBs_t IAP_BondMgrCBs = {NULL,NULL};
static uint8_t advertising_enabled = TRUE;
static uint8_t advertising_event_type = GAP_ADTYPE_ADV_IND;
static uint16_t desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
static uint16_t desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;


// Public functions.
/*********************************************************************
 * @fn      BLETester_Init
 *
 * @brief   主要业务初始化。
 *
 * @return  none
 */
void IAP_Init()
{
    GAPRole_PeripheralInit();
    Main_TaskID = TMOS_ProcessEventRegister(Main_Task_ProcessEvent);

    // init GAP to advertise in DEFAULT_ADVERTISING_INTERVAL intervals.
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN, DEFAULT_ADVERTISING_INTERVAL);
    GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX, DEFAULT_ADVERTISING_INTERVAL);

    GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL, sizeof(uint16_t), &desired_min_interval);
    GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL, sizeof(uint16_t), &desired_max_interval);
    GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enabled);
    GAPRole_SetParameter(GAPROLE_ADV_EVENT_TYPE, sizeof(uint8_t), &advertising_event_type);
    GAPRole_SetParameter(GAPROLE_ADVERT_DATA, sizeof(advertData), advertData);
    GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA, sizeof(scanRspData), scanRspData);

    // setting up the GAP GATT services required by the BLE specification. (this is boilerplate that you have to do for every peripheral. Detailed doc on TI's website.)
    GGS_SetParameter(GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName);
    GGS_AddService(GATT_ALL_SERVICES);         // GAP
    GATTServApp_AddService(GATT_ALL_SERVICES); // GATT attributes

    OTAProfile_AddService(GATT_ALL_SERVICES);

    // start the device with callback that does nothing.
    GAPRole_PeripheralStartDevice(Main_TaskID, &IAP_BondMgrCBs, &IAP_GAPRoleCBs);

    // we need to setup a stop trigger after the set up time MAIN_TASK_ADV_LENGTH.
    tmos_start_task(Main_TaskID, MAIN_TASK_SLEEP_EVENT, MAIN_TASK_ADV_LENGTH);
    GPIOB_ResetBits(GPIO_Pin_7);
}

uint16_t Main_Task_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if (events & MAIN_TASK_SLEEP_EVENT)
    {
        // setting up wakeup event.
        tmos_start_task(Main_TaskID, MAIN_TASK_WAKEUP_EVENT, MAIN_TASK_SLEEP_LENGTH);
        GPIOB_SetBits(GPIO_Pin_7);
        // turn off advertising.
        advertising_enabled = FALSE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enabled);
        // clear event bit.
        return events ^ MAIN_TASK_SLEEP_EVENT;
    }
    // setup TMR1 and start counting.
    if (events & MAIN_TASK_COUNT_EVENT)
    {
        // we count the cycles for 500ms.
        tmos_start_task(Main_TaskID, MAIN_TASK_WAKEUP_EVENT, 800);
        // turn off advertising.
        advertising_enabled = FALSE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enabled);
        GPIOB_ResetBits(GPIO_Pin_7); // turn on the switch and start the comparator circuit.
        // clear event bit.
        return events ^ MAIN_TASK_COUNT_EVENT;
    }
    // wakeup and start advertising packet.
    if (events & MAIN_TASK_WAKEUP_EVENT)
    {
        // we need to stop the advertising after the MAIN_TASK_ADV_LENGTH time.
        tmos_start_task(Main_TaskID, MAIN_TASK_SLEEP_EVENT, MAIN_TASK_ADV_LENGTH);
        GPIOB_ResetBits(GPIO_Pin_7);
        // turn on advertising.
        advertising_enabled = TRUE;
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8_t), &advertising_enabled);
        // clear event bit.
        return events ^ MAIN_TASK_WAKEUP_EVENT;
    }

    // fail proof route. Does nothing when the event is unknown.
    return 0;
}

/*********************************************************************
 * @fn      BLETester_GAPStateNotificationCB
 *
 * @brief   Notification from the profile of a state change.
 *
 * @param   newState - new state
 *
 * @return  none
 */
static void IAP_GAPStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    switch(newState)
    {
        case GAPROLE_STARTED:
            break;

        case GAPROLE_ADVERTISING:
            break;

        case GAPROLE_WAITING:
            break;

        case GAPROLE_ERROR:
            break;

        default:
            break;
    }
}
