#include "HAL.h"
#include "OTA_profile.h"


// OTA Service UUID.
static const uint8_t OTAProfile_OTAServiceUUID[ATT_BT_UUID_SIZE] = {LO_UINT16(OTAPROFILE_OTA_SERV_UUID), HI_UINT16(OTAPROFILE_OTA_SERV_UUID)};
static const gattAttrType_t OTAService = {ATT_BT_UUID_SIZE, OTAProfile_OTAServiceUUID};

// OTA Characteristics.
static const uint8_t OTAProfile_CtrlPointUUID[ATT_UUID_SIZE] = {CONSTRUCT_CHAR_UUID(OTAPROFILE_OTA_CTRL_POINT_UUID)};
static uint8_t OTAProfile_CtrlPointProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_NOTIFY;
static uint8_t OTAProfile_CtrlPointValue[CTRL_POINT_BUFFER_SIZE];
static uint16_t OTAProfile_CtrlPointValueLen;
static uint8_t OTAProfile_CtrlPointUserDesp[18] = "DFU Control Point";
static gattCharCfg_t OTAProfile_CtrlPointClientCharCfg[PERIPHERAL_MAX_CONNECTION];

static const uint8_t OTAProfile_PacketUUID[ATT_UUID_SIZE] = {CONSTRUCT_CHAR_UUID(OTAPROFILE_OTA_PACKET_UUID)};
static uint8_t OTAProfile_PacketProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP | GATT_PROP_NOTIFY;
static uint8_t OTAProfile_PacketValue[ATT_MAX_MTU_SIZE];
static uint16_t OTAProfile_PacketValueLen;
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
        OTAProfile_CtrlPointValue
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
        OTAProfile_PacketValue
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
static OTA_WriteCharCBs_t* OTAProfile_WriteCharCBs;

// local functions.
static bStatus_t OTAProfile_OTAService_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method)
{
    bStatus_t status = ATT_ERR_ATTR_NOT_FOUND;
    if(pAttr->type.len == ATT_UUID_SIZE)
    {
        // holy crap they implemented memcmp's same-string status code as 1!?? it is so anti-conventional...
        if (tmos_memcmp(pAttr->type.uuid, OTAProfile_CtrlPointUUID, ATT_UUID_SIZE) == 1)
        {
            tmos_memcpy(pValue, pAttr->pValue, OTAProfile_CtrlPointValueLen);
            *pLen = OTAProfile_CtrlPointValueLen;
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
                tmos_memcpy(pAttr->pValue, pValue, len);
                OTAProfile_CtrlPointValueLen = len;
                if(OTAProfile_WriteCharCBs && OTAProfile_WriteCharCBs->ctrlPointCb)
                {
                    OTAProfile_WriteCharCBs->ctrlPointCb(connHandle, pAttr->handle, pValue, len);
                }
                status = SUCCESS;
            }
            else
            {
                status = ATT_ERR_INSUFFICIENT_RESOURCES;
            }
        }
        else if (tmos_memcmp(pAttr->type.uuid, OTAProfile_PacketUUID, ATT_UUID_SIZE) == 1)
        {
            if (len <= ATT_MAX_MTU_SIZE)
            {
                tmos_memcpy(pAttr->pValue, pValue, len);
                OTAProfile_PacketValueLen = len;
                if(OTAProfile_WriteCharCBs && OTAProfile_WriteCharCBs->packetCb)
                {
                    OTAProfile_WriteCharCBs->packetCb(connHandle, pAttr->handle, pValue, len);
                }
                status = SUCCESS;
            }
            else
            {
                status = ATT_ERR_INSUFFICIENT_RESOURCES;
            }
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

void OTAProfile_RegisterWriteCharCBs(OTA_WriteCharCBs_t* cbs)
{
    OTAProfile_WriteCharCBs = cbs;
}


// static variables used only by these two functions.
static uint16_t CtrlPoint_ConnHandle = 0;
static attHandleValueNoti_t CtrlPoint_Noti;

void OTAProfile_SetupCtrlPointRsp(uint16_t connHandle, uint16_t attrHandle, uint8_t opcode, OTA_CtrlPointRsp_t* rsp, OtaRspCode_t rspCode)
{
    CtrlPoint_ConnHandle = connHandle;
    CtrlPoint_Noti.handle = attrHandle;
    // only length varies based on opcode. defaults to 0.
    uint16_t content_len = 0;
    uint8_t* content = (uint8_t*)rsp;
    if (rspCode == OTA_RSP_SUCCESS)
    {
        switch(opcode)
        {
            case OTA_CTRL_POINT_OPCODE_VERSION:
                content_len = sizeof(OTA_CtrlPointRsp_Version_t);
                break;
            case OTA_CTRL_POINT_OPCODE_CRC:
                content_len = sizeof(OTA_CtrlPointRsp_CRC_t);
                break;
            case OTA_CTRL_POINT_OPCODE_SELECT:
                content_len = sizeof(OTA_CtrlPointRsp_Select_t);
                break;
            case OTA_CTRL_POINT_OPCODE_GET_MTU:
                content_len = sizeof(OTA_CtrlPointRsp_MTU_t);
                break;
            case OTA_CTRL_POINT_OPCODE_WRITE:
                // TODO.
                content_len = 0;
                break;
            case OTA_CTRL_POINT_OPCODE_PING:
                content_len = sizeof(OTA_CtrlPointRsp_Ping_t);
                break;
            case OTA_CTRL_POINT_OPCODE_HW_VERSION:
                content_len = sizeof(OTA_CtrlPointRsp_Hardware_t);
                break;
            case OTA_CTRL_POINT_OPCODE_FW_VERSION:
                content_len = sizeof(OTA_CtrlPointRsp_Firmware_t) - 3; // we don't need the paddings.
                content += 3; // skip the 3 paddings
                break;
            default:
                // any other opcode will only return 3 required bytes, no content, so the len is not modified.
                break;
        }
    }
    
    // we can allocate memory and copy here because everyone does the same thing.
    // we need to allocate 3 more bytes because we need to insert 0x60(response opcode), request opcode, and the response code.
    CtrlPoint_Noti.len = content_len + 3;
    CtrlPoint_Noti.pValue = GATT_bm_alloc(connHandle, ATT_HANDLE_VALUE_NOTI, CtrlPoint_Noti.len, NULL, 0);
    if(CtrlPoint_Noti.pValue)
    {
        CtrlPoint_Noti.pValue[0] = OTA_CTRL_POINT_OPCODE_RSP;
        CtrlPoint_Noti.pValue[1] = opcode;
        CtrlPoint_Noti.pValue[2] = rspCode;
        if (content_len)
        {
            tmos_memcpy(CtrlPoint_Noti.pValue+3, content, content_len);
        }
    }
}

bStatus_t OTAProfile_DispatchCtrlPointRsp()
{
    bStatus_t status = bleIncorrectMode;
    if(GATTServApp_ReadCharCfg(CtrlPoint_ConnHandle, OTAProfile_CtrlPointClientCharCfg))
    {
        status = GATT_Notification(CtrlPoint_ConnHandle, &CtrlPoint_Noti, FALSE);
        if(status != SUCCESS)
        {
            GATT_bm_free((gattMsg_t *)&CtrlPoint_Noti, ATT_HANDLE_VALUE_NOTI);
        }
    }
    return status;
}