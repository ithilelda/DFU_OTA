/*
 * bletester.c
 *
 *  Created on: Apr 5, 2024
 *      Author: huawe
 */


#include "config.h"
#include "crc.h"
#include "peripheral.h"
#include "OTA_service.h"
#include "signature.h"


// function declaration for later reference.
static void OTA_GAPStateNotificationCB(gapRole_States_t newState, gapRoleEvent_t *pEvent);
static void OTA_GAPParamUpdateCB(uint16_t connHandle, uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout);
static void OTA_CtrlPointCB(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint16_t len);
static void OTA_PacketCB(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint16_t len);
static bStatus_t OTA_PreValidateCmdObject(CmdObject_t* obj);

/**************************************************
 * Public APIs.
 */
// I declare variables only before when needed.
static uint32_t BOOTAPP = 0; // constant to write to the EEPROM.
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
   LO_UINT16(OTA_SERV_UUID), HI_UINT16(OTA_SERV_UUID)
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
static uint8_t advertising_enabled = TRUE;
static uint8_t advertising_event_type = GAP_ADTYPE_ADV_IND;
static uint16_t desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
static uint16_t desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
static uint8_t attDeviceName[GAP_DEVICE_NAME_LEN] = "DFU_OTA";
static gapRolesCBs_t OTA_GAPRoleCBs = {OTA_GAPStateNotificationCB, NULL, OTA_GAPParamUpdateCB};
static gapBondCBs_t OTA_BondMgrCBs = {NULL,NULL};
static OTA_WriteCharCBs_t OTA_WriteCharCBs = {OTA_CtrlPointCB, OTA_PacketCB};
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

    OTA_AddService();
    OTA_RegisterWriteCharCBs(&OTA_WriteCharCBs);

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
        OTA_DispatchCtrlPointRsp();
        return events ^ MAIN_TASK_WRITERSP_EVENT;
    }
    if (events & MAIN_TASK_RESET_EVENT)
    {
        SYS_ResetExecute();
        return events ^ MAIN_TASK_RESET_EVENT;
    }
    // fail proof route. Does nothing when the event is unknown.
    return 0;
}

/**************************************************
 * Private functions.
 */
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
            GAPRole_PeripheralConnParamUpdateReq(((gapEstLinkReqEvent_t *)pEvent)->connectionHandle,
                                                DEFAULT_DESIRED_MIN_CONN_INTERVAL,
                                                DEFAULT_DESIRED_MAX_CONN_INTERVAL,
                                                DEFAULT_DESIRED_SLAVE_LATENCY,
                                                DEFAULT_DESIRED_CONN_TIMEOUT,
                                                Main_TaskID);
            break;

        default:
            break;
    }
}

/**
 * @brief 
 * 
 * @param connHandle 
 * @param connInterval 
 * @param connSlaveLatency 
 * @param connTimeout 
 */
static void OTA_GAPParamUpdateCB(uint16_t connHandle, uint16_t connInterval, uint16_t connSlaveLatency, uint16_t connTimeout)
{

}

static uint16_t OTA_Receipt_PRN = 0;
static uint16_t OTA_Receipt_PRN_Counter = 0;
// since we need to write this buffer to flash, it has to be dword aligned, and the size is one page size.
__attribute__((aligned(4))) static uint8_t OTA_ObjectBuffer[EEPROM_PAGE_SIZE];
static uint16_t OTA_ObjectBufferOffset = 0; // this is the offset within the buffer, so that multiple packets can be stored.
static uint8_t OTA_CurrentObject = OTA_CONTROL_POINT_OBJ_TYPE_INVALID; // 0 is invalid object, 1 is command, 2 is data.
static CmdObject_t cmdObj;
static uint32_t OTA_CmdObjectOffset = 0;
static uint32_t OTA_CmdObjectSize = 0;
static uint32_t OTA_CmdObjectCRC = CRC_INITIAL_VALUE;
static uint32_t OTA_DataObjectOffset = 0;
static uint32_t OTA_DataObjectSize = 0;
static uint32_t OTA_DataObjectCRC = CRC_INITIAL_VALUE;
static void OTA_CtrlPointCB(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint16_t len)
{
    OtaRspCode_t rspCode = OTA_RSP_INSUFFICIENT_RESOURCES;
    if(len <= 0) return; // guard.
    uint8_t opcode = pValue[0];
    uint8_t* pContent = pValue+1;
    uint32_t size;
    OTA_CtrlPointRsp_t rsp;
    uint16_t mtu = ATT_GetMTU(connHandle);
    if (mtu >= sizeof(OTA_CtrlPointRsp_t) + 6)
    {
        switch(opcode)
        {
            case OTA_CTRL_POINT_OPCODE_VERSION:
                rsp.version.version = OTA_PROTOCOL_VER;
                rspCode = OTA_RSP_SUCCESS;
                break;
            case OTA_CTRL_POINT_OPCODE_CREATE:
                OTA_CurrentObject = pContent[0];
                tmos_memcpy(&size, pContent+1, sizeof(uint32_t));
                if(size == 0)
                {
                    rspCode = OTA_RSP_INV_PARAM;
                }
                else if(OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_CMD)
                {
                    OTA_CmdObjectSize = size;
                    OTA_ObjectBufferOffset = 0; // when creating a new object, we reset the buffer offset because old data is executed (dumped somewhere else.)
                    rspCode = OTA_RSP_SUCCESS;
                }
                else if(OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_DATA)
                {
                    OTA_DataObjectSize = size;
                    OTA_ObjectBufferOffset = 0;
                    rspCode = OTA_RSP_SUCCESS;
                }
                else
                {
                    rspCode = OTA_RSP_UNSUPPORTED_TYPE;
                }
                break;
            case OTA_CTRL_POINT_OPCODE_SET_RCPT_NOTI:
                tmos_memcpy(&OTA_Receipt_PRN, pContent, sizeof(uint16_t));
                OTA_Receipt_PRN_Counter = 0; // we need to reset the counter everytime we update the PRN.
                rspCode = OTA_RSP_SUCCESS;
                break;
            case OTA_CTRL_POINT_OPCODE_CRC:
                if(OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_CMD)
                {
                    rsp.crc.offset = OTA_CmdObjectOffset;
                    rsp.crc.crc = OTA_CmdObjectCRC;
                    rspCode = OTA_RSP_SUCCESS;
                }
                else if(OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_DATA)
                {
                    rsp.crc.offset = OTA_DataObjectOffset;
                    rsp.crc.crc = OTA_DataObjectCRC;
                    rspCode = OTA_RSP_SUCCESS;
                }
                else
                {
                    rspCode = OTA_RSP_INV_OBJECT;
                }
                
                break;
            case OTA_CTRL_POINT_OPCODE_EXECUTE:
                if (OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_CMD)
                {
                    // when executing the command object, we finalize and validate it.
                    tmos_memcpy(&cmdObj, OTA_ObjectBuffer, sizeof(CmdObject_t));
                    rspCode = OTA_PreValidateCmdObject(&cmdObj);
                }
                else if (OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_DATA)
                {
                    UpdateHash(OTA_ObjectBuffer, OTA_ObjectBufferOffset);
                    // the write to flash operation is executed here.
                    if(FLASH_ROM_WRITE(APPLICATION_START_ADDR+OTA_DataObjectOffset-OTA_ObjectBufferOffset, OTA_ObjectBuffer, OTA_ObjectBufferOffset))
                    {
                        rspCode = OTA_RSP_EXT_ERROR;
                    }
                    else
                    {
                        rspCode = OTA_RSP_SUCCESS;
                    }
                    // do post validation.
                    if(OTA_DataObjectOffset == cmdObj.bin_size)
                    {
                        if(VerifyHash(cmdObj.fw_hash) == SUCCESS)
                        {
                            // raise the boot app flag.
                            EEPROM_WRITE(EEPROM_DATA_ADDR, &BOOTAPP, sizeof(uint32_t));
                            // dispatch a delayed reset.
                            tmos_start_task(Main_TaskID, MAIN_TASK_RESET_EVENT, 800); // half a second later.
                            rspCode = OTA_RSP_SUCCESS;
                        }
                        else
                        {
                            rspCode = OTA_RSP_OP_FAILED;
                        }
                    }
                }
                else
                {
                    rspCode = OTA_RSP_INV_OBJECT;
                }
                break;
            case OTA_CTRL_POINT_OPCODE_SELECT:
                if(pContent[0] == OTA_CONTROL_POINT_OBJ_TYPE_CMD)
                {
                    rsp.select.offset = OTA_CmdObjectOffset;
                    OTA_CmdObjectCRC = CRC_INITIAL_VALUE; // we initialize CRC when we select.
                    rsp.select.crc = OTA_CmdObjectCRC;
                    rsp.select.max_size = EEPROM_PAGE_SIZE; // each object can be at max the length of our object buffer.
                    rspCode = OTA_RSP_SUCCESS;
                }
                else if(pContent[0] == OTA_CONTROL_POINT_OBJ_TYPE_DATA)
                {
                    rsp.select.offset = OTA_DataObjectOffset;
                    OTA_DataObjectCRC = CRC_INITIAL_VALUE;
                    rsp.select.crc = OTA_DataObjectCRC;
                    rsp.select.max_size = EEPROM_PAGE_SIZE;
                    InitHash();
                    // we need to erase the corresponding flash region to prepare for the write.
                    if(FLASH_ROM_ERASE(APPLICATION_START_ADDR, APPLICATION_MAX_SIZE))
                    {
                        rspCode = OTA_RSP_EXT_ERROR;
                    }
                    else
                    {
                        rspCode = OTA_RSP_SUCCESS;
                    }
                }
                else
                {
                    rspCode = OTA_RSP_UNSUPPORTED_TYPE;
                }
                break;
            case OTA_CTRL_POINT_OPCODE_GET_MTU:
                rsp.mtu.size = mtu;
                rspCode = OTA_RSP_SUCCESS;
                break;
            case OTA_CTRL_POINT_OPCODE_WRITE:
                // TODO.
                rspCode = OTA_RSP_SUCCESS;
                break;
            case OTA_CTRL_POINT_OPCODE_PING:
                rsp.ping.id = pContent[0];
                rspCode = OTA_RSP_SUCCESS;
                break;
            case OTA_CTRL_POINT_OPCODE_HW_VERSION:
                rsp.hardware.part = HARDWARE_VERSION;
                rsp.hardware.variant = HARDWARE_VARIANT;
                rsp.hardware.memory.rom_size = CH571_ROM_SIZE;
                rsp.hardware.memory.ram_size = CH57x_RAM_SIZE;
                rsp.hardware.memory.rom_page_size = EEPROM_PAGE_SIZE;
                rspCode = OTA_RSP_SUCCESS;
                break;
            case OTA_CTRL_POINT_OPCODE_FW_VERSION:
                rsp.firmware.type = pContent[0];
                switch(pContent[0])
                {
                    case OTA_FW_TYPE_BLE_LIB:
                    rsp.firmware.version = *VER_LIB;
                    rsp.firmware.addr = LIB_FLASH_BASE_ADDRESSS;
                    rsp.firmware.len = LIB_FLASH_MAX_SIZE;
                    rspCode = OTA_RSP_SUCCESS;
                    break;
                    case OTA_FW_TYPE_APPLICATION:
                    rsp.firmware.version = 0; // TODO.
                    rsp.firmware.addr = APPLICATION_START_ADDR;
                    rsp.firmware.len = APPLICATION_MAX_SIZE;
                    rspCode = OTA_RSP_SUCCESS;
                    break;
                    case OTA_FW_TYPE_BOOTLOADER:
                    rsp.firmware.version = BOOTLOADER_VERSION;
                    rsp.firmware.addr = BOOTLOADER_START_ADDR;
                    rsp.firmware.len = BOOTLOADER_MAX_SIZE;
                    rspCode = OTA_RSP_SUCCESS;
                    break;
                    default:
                    rspCode = OTA_RSP_INV_PARAM;
                    break;
                }
                break;
            case OTA_CTRL_POINT_OPCODE_ABORT:
                // TODO.
                rspCode = OTA_RSP_SUCCESS;
                break;
            default:
                rspCode = OTA_RSP_INV_CODE;
                break;
        }
    }
    
    OTA_SetupCtrlPointRsp(connHandle, attrHandle, opcode, &rsp, rspCode);
    tmos_set_event(Main_TaskID, MAIN_TASK_WRITERSP_EVENT);
}

static void OTA_PacketCB(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint16_t len)
{
    // if we received a command object and we have enough space, we copy the data.
    if(OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_CMD && OTA_CmdObjectOffset + len <= ATT_MAX_MTU_SIZE)
    {
        tmos_memcpy(OTA_ObjectBuffer+OTA_ObjectBufferOffset, pValue, len);
        OTA_ObjectBufferOffset += len;
        OTA_CmdObjectOffset += len;
        // in order to save calculation cycles, we update the crc value while we are receiving the object.
        OTA_CmdObjectCRC = update_CRC32(OTA_CmdObjectCRC, pValue, len);
    }
    else if(OTA_CurrentObject == OTA_CONTROL_POINT_OBJ_TYPE_DATA)
    {
        tmos_memcpy(OTA_ObjectBuffer+OTA_ObjectBufferOffset, pValue, len);
        OTA_ObjectBufferOffset += len;
        OTA_DataObjectOffset += len;
        OTA_DataObjectCRC = update_CRC32(OTA_DataObjectCRC, pValue, len);
    }
}

static bStatus_t OTA_PreValidateCmdObject(CmdObject_t* obj)
{
    bStatus_t result = OTA_RSP_SUCCESS;
    __attribute__((aligned(4))) uint8_t key[SIGNATURE_KEY_LEN];
    __attribute__((aligned(4))) EEPROM_Data_t data;
    EEPROM_READ(SIGNATURE_KEY_ADDR, key, SIGNATURE_KEY_LEN);
    EEPROM_READ(EEPROM_DATA_ADDR, &data, sizeof(EEPROM_Data_t));
    if(VerifySignature((uint8_t*)obj, sizeof(CmdObject_t) - SIGNATURE_LEN, obj->obj_signature, key)) result = OTA_RSP_OP_FAILED;
    else if(obj->lib_version > *VER_LIB) result = OTA_RSP_OP_FAILED;
    else if(obj->hw_version != HARDWARE_VERSION) result = OTA_RSP_OP_FAILED;
    else if(obj->type != OTA_FW_TYPE_BOOTLOADER && obj->type != OTA_FW_TYPE_APPLICATION) result = OTA_RSP_OP_FAILED; // we only support uploading bootloader or app.
    else if(obj->type == OTA_FW_TYPE_BOOTLOADER && (obj->bin_size > BOOTLOADER_MAX_SIZE || (!obj->is_debug && obj->fw_version <= data.bl_version))) result = OTA_RSP_OP_FAILED;
    else if(obj->type == OTA_FW_TYPE_APPLICATION && (obj->bin_size > APPLICATION_MAX_SIZE || (!obj->is_debug && obj->fw_version <= data.app_version))) result = OTA_RSP_OP_FAILED;
    return result;
}