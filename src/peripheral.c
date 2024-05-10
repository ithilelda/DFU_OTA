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
static BOOL Conn_Established = FALSE;
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
   0x3,
   GAP_ADTYPE_16BIT_MORE,
   LO_UINT16(OTAPROFILE_OTA_SERV_UUID), HI_UINT16(OTAPROFILE_OTA_SERV_UUID)
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
static void OTA_GAPStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static gapRolesCBs_t OTA_GAPRoleCBs = {OTA_GAPStateNotificationCB, NULL, NULL};
static gapBondCBs_t OTA_BondMgrCBs = {NULL,NULL};
static void OTA_CtrlPointSelectTask(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint8_t len);
static OTA_CtrlPointRspTasks_t OTA_RspTasks = {NULL, OTA_CtrlPointSelectTask, NULL};
static attHandleValueNoti_t notification;
static uint16_t writeRsp_connHandle;

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
void OTA_Init()
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
    OTAProfile_RegisterWriteRspTasks(&OTA_RspTasks);

    // start TMOS with the init event.
    tmos_set_event(Main_TaskID, MAIN_TASK_INIT_EVENT);
}

uint16_t Main_Task_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if (events & MAIN_TASK_INIT_EVENT)
    {
        // start the device as a peripheral.
        GAPRole_PeripheralStartDevice(Main_TaskID, &OTA_BondMgrCBs, &OTA_GAPRoleCBs);
        // setup a timeout to reset the device because we are limited discoverable.
        tmos_start_task(Main_TaskID, MAIN_TASK_TIMEOUT_EVENT, MAIN_TASK_ADV_TIMEOUT);
        return events ^ MAIN_TASK_INIT_EVENT;
    }
    if (events & MAIN_TASK_TIMEOUT_EVENT)
    {
        // after timeout occurs and there is no connection, put the device into shutdown, you must disconnect and reconnect power to startup again.
        if (!Conn_Established)
        {
            GPIOB_SetBits(GPIO_Pin_7);
            LowPower_Shutdown(0);
        }
        return events ^ MAIN_TASK_TIMEOUT_EVENT;
    }
    if (events & MAIN_TASK_WRITERSP_EVENT)
    {
        GPIOB_ResetBits(GPIO_Pin_7);
        // when we have a hanging write response waiting to be notified, we dispatch the notification here.
        if(writeRsp_connHandle && GATT_Notification(writeRsp_connHandle, &notification, FALSE) != SUCCESS)
        {
            GPIOB_SetBits(GPIO_Pin_7);
            GATT_bm_free((gattMsg_t *)&notification, ATT_WRITE_RSP);
        }
        return events ^ MAIN_TASK_WRITERSP_EVENT;
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
static void OTA_GAPStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent)
{
    switch(newState)
    {
        case GAPROLE_STARTED:
            break;

        case GAPROLE_ADVERTISING:
            if(pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
            {
                Conn_Established = FALSE;
                GPIOB_SetBits(GPIO_Pin_7);
                LowPower_Shutdown(0);
            }
            else
            {
                GPIOB_ResetBits(GPIO_Pin_7);
            }
            break;

        case GAPROLE_WAITING:
            if(pEvent->gap.opcode == GAP_LINK_TERMINATED_EVENT)
            {
                Conn_Established = FALSE;
                GPIOB_SetBits(GPIO_Pin_7);
                LowPower_Shutdown(0);
            }
            else
            {
                GPIOB_SetBits(GPIO_Pin_7);
            }
            break;

        case GAPROLE_ERROR:
            break;

        case GAPROLE_CONNECTED:
            GPIOB_SetBits(GPIO_Pin_7);
            Conn_Established = TRUE; // once connected, we raise the connected flag.
            break;

        default:
            break;
    }
}

static void OTA_CtrlPointSelectTask(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint8_t len)
{
    GATT_bm_free((gattMsg_t*)pValue, ATT_WRITE_RSP);
    writeRsp_connHandle = connHandle;
    notification.handle = attrHandle;
    notification.len = len;
    notification.pValue = GATT_bm_alloc(connHandle, ATT_WRITE_RSP, notification.len, NULL, 0);
    if (notification.pValue)
    {
        tmos_memcpy(notification.pValue, pValue, len);
        tmos_start_task(Main_TaskID, MAIN_TASK_WRITERSP_EVENT, 80);
    }
}