#include "HAL.h"
#include "OTA_profile.h"


// OTA Service UUID.
static const uint8_t OTAProfile_OTAServiceUUID[ATT_UUID_SIZE] = {CONSTRUCT_OTA_UUID(OTAPROFILE_SERVICE_UUID, OTAPROFILE_OTA_SERV_UUID)};
static const gattAttrType_t OTAService = {ATT_UUID_SIZE, OTAProfile_OTAServiceUUID};

// OTA Characteristics.
static const uint8_t OTAProfile_FlashUUID[ATT_UUID_SIZE] = {CONSTRUCT_OTA_UUID(OTAPROFILE_CHARACTER_UUID, OTAPROFILE_OTA_FLASH_UUID)};
static uint8_t OTAProfile_FlashProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8_t OTAProfile_FlashBuf[MAX_ATTR_PACKET_LEN] = "default value";
static uint8_t OTAProfile_FlashUserDesp[11] = "Flash Data";

static const uint8_t OTAProfile_FlashPtrUUID[ATT_UUID_SIZE] = {CONSTRUCT_OTA_UUID(OTAPROFILE_CHARACTER_UUID, OTAPROFILE_OTA_FLASH_PTR_UUID)};
static uint8_t OTAProfile_FlashPtrProps = GATT_PROP_READ | GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8_t OTAProfile_FlashPtr[FLASH_PTR_SIZE];
static uint8_t OTAProfile_FlashPtrUserDesp[14] = "Flash Pointer";
static uint8_t OTAProfile_FlashPtrFormat[7] = {0x06,0x00,0x00,0x27,0x00,0x00,0x00};

// Profile Attributes Table.
static gattAttribute_t OTAServiceAttrTable[8] = {
    // OTA Service declaration
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&OTAService                  /* pValue */
    },

    // Flash Characteristic Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfile_FlashProps
    },

    // Flash Characteristic Value
    {
        {ATT_UUID_SIZE, OTAProfile_FlashUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        OTAProfile_FlashBuf
    },

    // Flash Characteristic User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfile_FlashUserDesp
    },

    // Flash Pointer Characteristic Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &OTAProfile_FlashPtrProps
    },

    // Flash Pointer Characteristic Value
    {
        {ATT_UUID_SIZE, OTAProfile_FlashPtrUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        OTAProfile_FlashPtr
    },

    // Flash Pointer Characteristic User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfile_FlashPtrUserDesp
    },

    // Flash Pointer Characteristic Presentation Format
    {
        {ATT_BT_UUID_SIZE, charFormatUUID},
        GATT_PERMIT_READ,
        0,
        OTAProfile_FlashPtrFormat
    },
};

// local functions.
static bStatus_t OTAProfile_OTAService_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method)
{
    bStatus_t status = FAILURE;
    if(pAttr->type.len == ATT_UUID_SIZE)
    {
        // holy crap they implemented memcmp's same status code as 1!?? it is so anti-conventional...
        if (tmos_memcmp(pAttr->type.uuid, OTAProfile_FlashUUID, ATT_UUID_SIZE) == 1)
        {
            tmos_memcpy(pValue, OTAProfile_FlashBuf, 14);
            *pLen = 14;
            status = SUCCESS;
        }
        else if (tmos_memcmp(pAttr->type.uuid, OTAProfile_FlashPtrUUID, ATT_UUID_SIZE) == 1)
        {
            *pValue = 'a';
            *pLen = 1;
            status = SUCCESS;
        }
        else
        {
            status = ATT_ERR_ATTR_NOT_FOUND;
        }
    }
    return status;

}
static bStatus_t OTAProfile_OTAService_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method)
{
    bStatus_t status = FAILURE;
    if(pAttr->type.len == ATT_UUID_SIZE)
    {
        if (tmos_memcmp(pAttr->type.uuid, OTAProfile_FlashUUID, ATT_UUID_SIZE) == 0)
        {

        }
        else if (tmos_memcmp(pAttr->type.uuid, OTAProfile_FlashPtrUUID, ATT_UUID_SIZE) == 0)
        {

        }
        else
        {
            status = ATT_ERR_ATTR_NOT_FOUND;
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
        // Register GATT attribute list and CBs with GATT Server App
        status = GATTServApp_RegisterService(OTAServiceAttrTable,
                                             GATT_NUM_ATTRS(OTAServiceAttrTable),
                                             GATT_MAX_ENCRYPT_KEY_SIZE,
                                             &OTAServiceCBs);
    }

    return (status);
}
