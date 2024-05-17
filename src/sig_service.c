#include "HAL.h"
#include "sig_service.h"
#include "rng/yarrow.h"
#include "ecc/ed25519.h"


// OTA Service UUID.
static const uint8_t SIG_ServiceUUID[ATT_BT_UUID_SIZE] = {LO_UINT16(SIG_SERV_UUID), HI_UINT16(SIG_SERV_UUID)};
static const gattAttrType_t SIGService = {ATT_BT_UUID_SIZE, SIG_ServiceUUID};

// OTA Characteristics.
static const uint8_t SIG_CharUUID[ATT_UUID_SIZE] = {SIG_CHAR_UUID};
static uint8_t SIG_CharProps = GATT_PROP_READ | GATT_PROP_WRITE_NO_RSP | GATT_PROP_NOTIFY;
static uint8_t SIG_CharValue[ED25519_SIGNATURE_LEN];
static uint8_t SIG_CharUserDesp[13] = "ED25519 Test";
static gattCharCfg_t SIG_CharClientCharCfg[PERIPHERAL_MAX_CONNECTION];


// Profile Attributes Table.
static gattAttribute_t SIGServiceAttrTable[5] = {
    // SIG Service declaration
    {
        {ATT_BT_UUID_SIZE, primaryServiceUUID}, /* type */
        GATT_PERMIT_READ,                       /* permissions */
        0,                                      /* handle */
        (uint8_t *)&SIGService                  /* pValue */
    },

    // Characteristic Declaration
    {
        {ATT_BT_UUID_SIZE, characterUUID},
        GATT_PERMIT_READ,
        0,
        &SIG_CharProps
    },

    // Characteristic Value
    {
        {ATT_UUID_SIZE, SIG_CharUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        SIG_CharValue
    },

    // Characteristic User Description
    {
        {ATT_BT_UUID_SIZE, charUserDescUUID},
        GATT_PERMIT_READ,
        0,
        SIG_CharUserDesp
    },

    // Characteristic CCC
    {
        {ATT_BT_UUID_SIZE, clientCharCfgUUID},
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8_t*)SIG_CharClientCharCfg
    },
};
static bStatus_t SIG_ReadAttrCB(uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue, uint16_t *pLen, uint16_t offset, uint16_t maxLen, uint8_t method)
{
    bStatus_t status = ATT_ERR_ATTR_NOT_FOUND;
    if(pAttr->type.len == ATT_UUID_SIZE)
    {
        if (tmos_memcmp(pAttr->type.uuid, SIG_CharUUID, ATT_UUID_SIZE) == 1)
        {
            *pLen = ED25519_SIGNATURE_LEN;
            tmos_memcpy(pValue, pAttr->pValue, ED25519_SIGNATURE_LEN);
            status = SUCCESS;
        }
    }
    return status;
}
static bStatus_t SIG_WriteAttrCB(uint16_t connHandle, gattAttribute_t *pAttr, uint8_t *pValue, uint16_t len, uint16_t offset, uint8_t method)
{
    bStatus_t status = ATT_ERR_ATTR_NOT_FOUND;
    if(pAttr->type.len == ATT_UUID_SIZE)
    {
        if (tmos_memcmp(pAttr->type.uuid, SIG_CharUUID, ATT_UUID_SIZE) == 1)
        {
            uint8_t pub[ED25519_PUBLIC_KEY_LEN];
            //YarrowContext yarrow;
            //yarrowInit(&yarrow);
            //yarrowFastReseed(&yarrow);
            //ed25519GenerateKeyPair(YARROW_PRNG_ALGO, &yarrow, priv, pub);
            ed25519VerifySignature(pub, pValue, len, NULL, 0, 0, pAttr->pValue);
            attHandleValueNoti_t noti;
            noti.handle = pAttr->handle;
            noti.len = 1;
            noti.pValue = GATT_bm_alloc(connHandle, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
            if(noti.pValue)
            {
                *noti.pValue = 0x00;
                if(GATT_Notification(connHandle, &noti, FALSE) != SUCCESS)
                {
                    GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
                }
            }
            status = SUCCESS;
        }
    }
    else if(pAttr->type.len == ATT_BT_UUID_SIZE)
    {
        uint16_t uuid = BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1]);
        switch(uuid)
        {
            case GATT_CLIENT_CHAR_CFG_UUID:
                if(pAttr->handle == SIGServiceAttrTable[4].handle)
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
static gattServiceCBs_t SIGServiceCBs = {
    SIG_ReadAttrCB,  // Read callback function pointer
    SIG_WriteAttrCB, // Write callback function pointer
    NULL                               // Authorization callback function pointer
};
bStatus_t SIG_AddService()
{
    uint8_t status = FAILURE;
    // init CCC tables. the define name is invalid_handle, but it actually means all handle here. Wch follows TI convention.
    // Register GATT attribute list and CBs with GATT Server App
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, SIG_CharClientCharCfg);
    status = GATTServApp_RegisterService(SIGServiceAttrTable,
                                            GATT_NUM_ATTRS(SIGServiceAttrTable),
                                            GATT_MAX_ENCRYPT_KEY_SIZE,
                                            &SIGServiceCBs);
    return (status);
}