#include "HAL.h"
#include "OTA_profile.h"


// OTA Service UUID.
static const uint8_t OTAProfile_OTAServiceUUID[ATT_BT_UUID_SIZE] = {LO_UINT16(OTAPROFILE_OTA_SERV_UUID), HI_UINT16(OTAPROFILE_OTA_SERV_UUID)};
static const gattAttrType_t OTAService = {ATT_BT_UUID_SIZE, OTAProfile_OTAServiceUUID};

// OTA Characteristics.
static const uint8_t OTAProfile_CtrlPointUUID[ATT_UUID_SIZE] = {CONSTRUCT_CHAR_UUID(OTAPROFILE_OTA_CTRL_POINT_UUID)};
static uint8_t OTAProfile_CtrlPointProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_NOTIFY;
static uint8_t OTAProfile_CtrlPointValue = 0; // placeholder. It doesn't matter.
static uint8_t OTAProfile_CtrlPointUserDesp[18] = "DFU Control Point";
static gattCharCfg_t OTAProfile_CtrlPointClientCharCfg[PERIPHERAL_MAX_CONNECTION];

static const uint8_t OTAProfile_PacketUUID[ATT_UUID_SIZE] = {CONSTRUCT_CHAR_UUID(OTAPROFILE_OTA_PACKET_UUID)};
static uint8_t OTAProfile_PacketProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP | GATT_PROP_NOTIFY;
static uint8_t OTAProfile_PacketBuf[ATT_MAX_MTU_SIZE];
static uint8_t OTAProfile_PacketUserDesp[11] = "DFU Packet";
static gattCharCfg_t OTAProfile_PacketClientCharCfg[PERIPHERAL_MAX_CONNECTION];

// Profile Attributes Table.
static gattAttribute_t OTAServiceAttrTable[9] = {
    // OTA Service declaration
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&OTAService                  /* pValue */
    },

    // Control Point Characteristic Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfile_CtrlPointProps
    },

    // Control Point Characteristic Value
    {
        {ATT_UUID_SIZE, OTAProfile_CtrlPointUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        &OTAProfile_CtrlPointValue
    },

    // Control Point Characteristic User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfile_CtrlPointUserDesp
    },

    // Control Point Characteristic CCC
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t*)OTAProfile_CtrlPointClientCharCfg
    },

    // Packet Characteristic Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfile_PacketProps
    },

    // Packet Characteristic Value
    {
        {ATT_UUID_SIZE, OTAProfile_PacketUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        OTAProfile_PacketBuf
    },

    // Packet Characteristic User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfile_PacketUserDesp
    },

    // Packet Characteristic CCC
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t*)OTAProfile_PacketClientCharCfg
    },
};

// application callbacks.
static OTA_CtrlPointRspTasks_t* OTAProfile_CtrlPointRspTasks;

// buffers for operation.
static uint8_t OTAProfile_CtrlPoint_Buffer[CTRL_POINT_BUFFER_SIZE];
static uint16_t OTAProfile_CtrlPoint_BufferLen = 0;
static bStatus_t OTAProfile_CtrlPoint_RspCode[OTA_CTRL_POINT_OPCODE_NUMBER];

static bStatus_t OTAService_Handle_CtrlPoint(uint16_t connHandle, uint8_t* content, uint16_t len)
{
    bStatus_t status = ATT_ERR_UNSUPPORTED_REQ;
    uint8_t opcode = content[0];
    uint16_t attrHandle = OTAServiceAttrTable[2].handle;
    switch(opcode)
    {
        case OTA_CTRL_POINT_OPCODE_CREATE:
            status = SUCCESS;
            break;
        case OTA_CTRL_POINT_OPCODE_SET_PRN:
            status = SUCCESS;
            break;
        case OTA_CTRL_POINT_OPCODE_CALC_CRC:
            if(OTAProfile_CtrlPointRspTasks->setupCalcCRCTask)
            {
                OTAProfile_CtrlPointRspTasks->setupCalcCRCTask(connHandle,attrHandle,0,0);
                OTAProfile_CtrlPoint_RspCode[opcode] = OTA_CTRL_POINT_RSP_SUCCESS;
                status = blePending;
            }
            else
            {
                OTAProfile_CtrlPoint_RspCode[opcode] = OTA_CTRL_POINT_RSP_OP_FAILED;
                status = SUCCESS;
            }
            break;
        case OTA_CTRL_POINT_OPCODE_EXECUTE:
            status = SUCCESS;
            break;
        case OTA_CTRL_POINT_OPCODE_SELECT:
            if(OTAProfile_CtrlPointRspTasks->setupSelectTask)
            {
                OTAProfile_CtrlPointRspTasks->setupSelectTask(connHandle,attrHandle,content,len);
                OTAProfile_CtrlPoint_RspCode[opcode] = OTA_CTRL_POINT_RSP_SUCCESS;
                status = blePending;
            }
            else
            {
                OTAProfile_CtrlPoint_RspCode[opcode] = OTA_CTRL_POINT_RSP_OP_FAILED;
                status = SUCCESS;
            }
            break;
        case OTA_CTRL_POINT_OPCODE_RSP_CODE:
            if(OTAProfile_CtrlPointRspTasks->setupRspCodeTask)
            {
                OTAProfile_CtrlPointRspTasks->setupRspCodeTask(connHandle,attrHandle,0);
                OTAProfile_CtrlPoint_RspCode[opcode] = OTA_CTRL_POINT_RSP_SUCCESS;
                status = blePending;
            }
            else
            {
                OTAProfile_CtrlPoint_RspCode[opcode] = OTA_CTRL_POINT_RSP_OP_FAILED;
                status = SUCCESS;
            }
            break;
        default:
            break;
    }
    return status;
}

// local functions.
static bStatus_t OTAProfile_OTAService_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method)
{
    bStatus_t status = ATT_ERR_ATTR_NOT_FOUND;
    if(pAttr->type.len == ATT_UUID_SIZE)
    {
        // holy crap they implemented memcmp's same-string status code as 1!?? it is so anti-conventional...
        if (tmos_memcmp(pAttr->type.uuid, OTAProfile_CtrlPointUUID, ATT_UUID_SIZE) == 1)
        {
            tmos_memcpy(pValue, OTAProfile_CtrlPoint_Buffer, OTAProfile_CtrlPoint_BufferLen);
            *pLen = OTAProfile_CtrlPoint_BufferLen;
            status = SUCCESS;
        }
        else if (tmos_memcmp(pAttr->type.uuid, OTAProfile_PacketUUID, ATT_UUID_SIZE) == 1)
        {
            *pValue = LO_UINT16(maxLen);
            *(pValue + 1) = HI_UINT16(maxLen);
            *pLen = 2;
            status = SUCCESS;
        }
    }
    return status;

}
static bStatus_t OTAProfile_OTAService_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method)
{
    bStatus_t status = ATT_ERR_ATTR_NOT_FOUND;
    if(pAttr->type.len == ATT_UUID_SIZE)
    {
        if (tmos_memcmp(pAttr->type.uuid, OTAProfile_CtrlPointUUID, ATT_UUID_SIZE) == 1)
        {
            if (len <= CTRL_POINT_BUFFER_SIZE)
            {
                tmos_memcpy(OTAProfile_CtrlPoint_Buffer, pValue, len);
                OTAProfile_CtrlPoint_BufferLen = len;
                status = OTAService_Handle_CtrlPoint(connHandle, pValue, len);
            }
            else
            {
                status = ATT_ERR_INSUFFICIENT_RESOURCES;
            }
        }
        else if (tmos_memcmp(pAttr->type.uuid, OTAProfile_PacketUUID, ATT_UUID_SIZE) == 1)
        {

        }
    }
    else if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch(uuid)
        {
            case GATT_CLIENT_CHAR_CFG_UUID:
                if(pAttr->handle == OTAServiceAttrTable[4].handle || pAttr->handle == OTAServiceAttrTable[8].handle)
                {
                    status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len, offset, GATT_CLIENT_CFG_NOTIFY);
                }
                break;
            default:
                break;
        }
    }
    return status;
}
static gattServiceCBs_t OTAServiceCBs = {
    OTAProfile_OTAService_ReadAttrCB,  // Read callback function pointer
    OTAProfile_OTAService_WriteAttrCB, // Write callback function pointer
    NULL                               // Authorization callback function pointer
};

/*********************************************************************
 * @fn      OTAProfile_AddService
 *
 * @brief   OTA Profile初始化
 *
 * @param   services    - 服务控制字
 *
 * @return  初始化的状态
 */
bStatus_t OTAProfile_AddService(uint32_t services)
{
    uint8_t status = FAILURE;

    if(services & OTAPROFILE_OTA_SERVICE)
    {
        // init CCC tables. the define name is invalid_handle, but it actually means all handle here. Wch follows TI convention.
        GATTServApp_InitCharCfg(INVALID_CONNHANDLE, OTAProfile_CtrlPointClientCharCfg);
        GATTServApp_InitCharCfg(INVALID_CONNHANDLE, OTAProfile_PacketClientCharCfg);
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService(OTAServiceAttrTable,
                                             GATT_NUM_ATTRS(OTAServiceAttrTable),
                                             GATT_MAX_ENCRYPT_KEY_SIZE,
                                             &OTAServiceCBs);
    }

    return (status);
}

void OTAProfile_RegisterWriteRspTasks(OTA_CtrlPointRspTasks_t* tasks)
{
    OTAProfile_CtrlPointRspTasks = tasks;
}