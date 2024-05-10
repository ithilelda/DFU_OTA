/*
 * OTA_profile.h
 *
 *  Created on: May 7, 2024
 *      Author: huawe
 */

#ifndef SRC_OTA_PROFILE_H_
#define SRC_OTA_PROFILE_H_

#include "config.h"


#define CTRL_POINT_BUFFER_SIZE 8

#define OTAPROFILE_OTA_SERVICE 0x01

/*********************************************************************
 * Service UUIDs.
 */
// OTA service UUID.
// I'm implementing Nordics protocol. So I follow their conventions.
#define OTAPROFILE_OTA_SERV_UUID            0xFE59
// Nordic DFU control point characteristic UUID: 8ec90001-f315-4f60-9fb8-838830daea50
#define OTAPROFILE_OTA_CTRL_POINT_UUID      0x0001
// Nordic DFU packet characteristic UUID: 8ec90002-f315-4f60-9fb8-838830daea50
#define OTAPROFILE_OTA_PACKET_UUID          0x0002
#define CONSTRUCT_CHAR_UUID(a) 0x50,0xea,0xda,0x30,0x88,0x83,0xb8,0x9f,0x60,0x4f,0x15,0xf3,LO_UINT16(a),HI_UINT16(a),0xc9,0x8e

/*********************************************************************
 * Control Point OpCodes.
 */
#define OTA_CTRL_POINT_OPCODE_NUMBER                 7
#define OTA_CTRL_POINT_OPCODE_CREATE                 0x01
#define OTA_CTRL_POINT_OPCODE_SET_PRN                0x02
#define OTA_CTRL_POINT_OPCODE_CALC_CRC               0x03
#define OTA_CTRL_POINT_OPCODE_EXECUTE                0x04
#define OTA_CTRL_POINT_OPCODE_SELECT                 0x06
#define OTA_CTRL_POINT_OPCODE_RSP_CODE               0x60
/*********************************************************************
 * Control Point Response Code.
 */
#define OTA_CTRL_POINT_RSP_INV_CODE                  0x00
#define OTA_CTRL_POINT_RSP_SUCCESS                   0x01
#define OTA_CTRL_POINT_RSP_NOT_SUPPORTED             0x02
#define OTA_CTRL_POINT_RSP_INV_PARAM                 0x03
#define OTA_CTRL_POINT_RSP_INSUFFICIENT_RESOURCES    0x04
#define OTA_CTRL_POINT_RSP_INV_OBJECT                0x05
#define OTA_CTRL_POINT_RSP_UNSUPPORTED_TYPE          0x07
#define OTA_CTRL_POINT_RSP_OP_NOT_PERMITTED          0x08
#define OTA_CTRL_POINT_RSP_OP_FAILED                 0x0A
#define OTA_CTRL_POINT_RSP_EXT_ERROR                 0x0B

/*********************************************************************
 * Types and functions.
 */
typedef void (*OTA_SetupCalcCRCTask_t)(uint16_t connHandle, uint16_t attrHandle, uint32_t offset, uint32_t crc32);
typedef void (*OTA_SetupSelectTask_t)(uint16_t connHandle, uint16_t attrHandle, uint8_t* pValue, uint8_t len);
typedef void (*OTA_SetupRspCodeTask_t)(uint16_t connHandle, uint16_t attrHandle, uint8_t err);
typedef struct
{
    OTA_SetupCalcCRCTask_t setupCalcCRCTask;
    OTA_SetupSelectTask_t setupSelectTask;
    OTA_SetupRspCodeTask_t setupRspCodeTask;

} OTA_CtrlPointRspTasks_t;
bStatus_t OTAProfile_AddService(uint32_t services);
void OTAProfile_RegisterWriteRspTasks(OTA_CtrlPointRspTasks_t* tasks);

#endif /* SRC_OTA_PROFILE_H_ */
