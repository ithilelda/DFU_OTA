/*
 * OTA_profile.h
 *
 *  Created on: May 7, 2024
 *      Author: huawe
 */

#ifndef OTA_SERVICE_H
#define OTA_SERVICE_H

#include "config.h"


#define CTRL_POINT_BUFFER_SIZE              8
#define OTA_PROTOCOL_VER                    0x00

/*********************************************************************
 * Service UUIDs.
 */
// OTA service UUID.
// I'm implementing Nordics protocol. So I follow their conventions.
#define OTA_SERV_UUID                                0xFE59
// Nordic DFU control point characteristic UUID: 8ec90001-f315-4f60-9fb8-838830daea50
#define OTA_CTRL_POINT_UUID                          0x0001
// Nordic DFU packet characteristic UUID: 8ec90002-f315-4f60-9fb8-838830daea50
#define OTA_PACKET_UUID                              0x0002
#define CONSTRUCT_CHAR_UUID(a) 0x50,0xea,0xda,0x30,0x88,0x83,0xb8,0x9f,0x60,0x4f,0x15,0xf3,LO_UINT16(a),HI_UINT16(a),0xc9,0x8e

/*********************************************************************
 * Control Point OpCodes.
 */
#define OTA_CTRL_POINT_OPCODE_VERSION                0x00
#define OTA_CTRL_POINT_OPCODE_CREATE                 0x01
#define OTA_CTRL_POINT_OPCODE_SET_RCPT_NOTI          0x02
#define OTA_CTRL_POINT_OPCODE_CRC                    0x03
#define OTA_CTRL_POINT_OPCODE_EXECUTE                0x04
#define OTA_CTRL_POINT_OPCODE_SELECT                 0x06
#define OTA_CTRL_POINT_OPCODE_GET_MTU                0x07
#define OTA_CTRL_POINT_OPCODE_WRITE                  0x08
#define OTA_CTRL_POINT_OPCODE_PING                   0x09
#define OTA_CTRL_POINT_OPCODE_HW_VERSION             0x0A
#define OTA_CTRL_POINT_OPCODE_FW_VERSION             0x0B
#define OTA_CTRL_POINT_OPCODE_ABORT                  0x0C
#define OTA_CTRL_POINT_OPCODE_RSP                    0x60
/*********************************************************************
 * Control Point Response Code.
 */
#define OTA_RSP_INV_CODE                             0x00
#define OTA_RSP_SUCCESS                              0x01
#define OTA_RSP_NOT_SUPPORTED                        0x02
#define OTA_RSP_INV_PARAM                            0x03
#define OTA_RSP_INSUFFICIENT_RESOURCES               0x04
#define OTA_RSP_INV_OBJECT                           0x05
#define OTA_RSP_UNSUPPORTED_TYPE                     0x07
#define OTA_RSP_OP_NOT_PERMITTED                     0x08
#define OTA_RSP_OP_FAILED                            0x0A
#define OTA_RSP_EXT_ERROR                            0x0B
/*********************************************************************
 * Control Point Object Type.
 */
#define OTA_CONTROL_POINT_OBJ_TYPE_INVALID           0x00
#define OTA_CONTROL_POINT_OBJ_TYPE_CMD               0x01
#define OTA_CONTROL_POINT_OBJ_TYPE_DATA              0x02
/*********************************************************************
 * Firmware types definition.
 */
#define OTA_FW_TYPE_BLE_LIB                          0x00
#define OTA_FW_TYPE_APPLICATION                      0x01
#define OTA_FW_TYPE_BOOTLOADER                       0x02
#define OTA_FW_TYPE_UNKNOWN                          0xFF

/*********************************************************************
 * Types and functions.
 */

// the control point response types.
typedef uint8_t OtaRspCode_t;
typedef struct
{
    uint32_t rom_size;
    uint32_t ram_size;
    uint32_t rom_page_size;
} OTA_HW_Memory_t;
typedef struct
{
    uint8_t version;
} OTA_CtrlPointRsp_Version_t;
typedef struct
{
    uint32_t offset;
    uint32_t crc;
} OTA_CtrlPointRsp_CRC_t;
typedef struct
{
    uint32_t max_size;
    uint32_t offset;
    uint32_t crc;
} OTA_CtrlPointRsp_Select_t;
typedef struct
{
    uint16_t size;
} OTA_CtrlPointRsp_MTU_t;
typedef struct
{
    uint8_t id;
} OTA_CtrlPointRsp_Ping_t;
typedef struct
{
    uint32_t part;
    uint32_t variant;
    OTA_HW_Memory_t memory;
} OTA_CtrlPointRsp_Hardware_t;
typedef struct
{
    uint8_t padding[3]; // alignment problem.
    uint8_t type;
    uint32_t version;
    uint32_t addr;
    uint32_t len;
} OTA_CtrlPointRsp_Firmware_t;

typedef union
{
    OTA_CtrlPointRsp_Version_t version;
    OTA_CtrlPointRsp_CRC_t crc;
    OTA_CtrlPointRsp_Select_t select;
    OTA_CtrlPointRsp_MTU_t mtu;
    OTA_CtrlPointRsp_Ping_t ping;
    OTA_CtrlPointRsp_Hardware_t hardware;
    OTA_CtrlPointRsp_Firmware_t firmware;
    
} OTA_CtrlPointRsp_t;

// the application callback function types.
typedef void (*OTA_HandleCtrlPointCB)(uint16_t connHandle, uint16_t attrHandle, uint8_t *pValue, uint16_t len);
typedef void (*OTA_HandlePacketCB)(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint16_t len);
typedef struct
{
    OTA_HandleCtrlPointCB ctrlPointCb;
    OTA_HandlePacketCB packetCb;

} OTA_WriteCharCBs_t;

// public apis.
bStatus_t OTA_AddService();
void OTA_RegisterWriteCharCBs(OTA_WriteCharCBs_t* cbs);
void OTA_SetupCtrlPointRsp(uint16_t connHandle, uint16_t attrHandle, uint8_t opcode, OTA_CtrlPointRsp_t* rsp, OtaRspCode_t rspCode);
bStatus_t OTA_DispatchCtrlPointRsp();

#endif /* OTA_SERVICE_H */
