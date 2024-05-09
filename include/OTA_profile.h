/*
 * OTA_profile.h
 *
 *  Created on: May 7, 2024
 *      Author: huawe
 */

#ifndef SRC_OTA_PROFILE_H_
#define SRC_OTA_PROFILE_H_

#include "config.h"


#define MAX_ATTR_PACKET_LEN 247
#define FLASH_PTR_SIZE 2

#define OTAPROFILE_OTA_SERVICE 0x01

/*********************************************************************
 * UUIDs of the profile. I use a custom UUID so it has to be 128 bits.
 * the basic UUID is : XXXXXXXX-7228-4e88-8742-831676c7b629.
 * services and characteristics only change the first 32 bits of the MSB.
 */
// the base part of the 128 bit uuid.
#define OTAPROFILE_UUID_1      0x7228
#define OTAPROFILE_UUID_2      0x4e88
#define OTAPROFILE_UUID_3      0x8742
#define OTAPROFILE_UUID_4_1    0x8316
#define OTAPROFILE_UUID_4_2    0x76c7
#define OTAPROFILE_UUID_4_3    0xb629
// Service UUIDs starts with the higher part as 0x8339.
#define OTAPROFILE_SERVICE_UUID      0x8339
// Characteristic UUIDs starts with the higher part as 0x8340.
#define OTAPROFILE_CHARACTER_UUID    0x8340
// helper macro to construct the OTA UUID easier.
#define CONSTRUCT_OTA_UUID(a, b) LO_UINT16(OTAPROFILE_UUID_4_3), HI_UINT16(OTAPROFILE_UUID_4_3), \
                                 LO_UINT16(OTAPROFILE_UUID_4_2), HI_UINT16(OTAPROFILE_UUID_4_2), \
                                 LO_UINT16(OTAPROFILE_UUID_4_1), HI_UINT16(OTAPROFILE_UUID_4_1), \
                                 LO_UINT16(OTAPROFILE_UUID_3), HI_UINT16(OTAPROFILE_UUID_3), \
                                 LO_UINT16(OTAPROFILE_UUID_2), HI_UINT16(OTAPROFILE_UUID_2), \
                                 LO_UINT16(OTAPROFILE_UUID_1), HI_UINT16(OTAPROFILE_UUID_1), \
                                 LO_UINT16(b), HI_UINT16(b), LO_UINT16(a), HI_UINT16(a)

/*********************************************************************
 * Service UUIDs.
 */
// OTA service UUID.
// full uuid: 8339b3d7-7228-4e88-8742-831676c7b629
#define OTAPROFILE_OTA_SERV_UUID     0xb3d7

// OTA flash characteristic UUID: 8340a237-7228-4e88-8742-831676c7b629
// read returns the required number of bytes starting from the pointer.
// write will write the specified number of bytes into the pointer's address.
#define OTAPROFILE_OTA_FLASH_UUID      0xa237
// OTA flash pointer characteristic UUID: 8340a238-7228-4e88-8742-831676c7b629
// reading return the position of the current flash pointer.
// writing sets the position.
#define OTAPROFILE_OTA_FLASH_PTR_UUID      0xa238
// OTA flash CRC characteristic UUID: 8340a239-7228-4e88-8742-831676c7b629
#define OTAPROFILE_OTA_FLASH_CRC_UUID     0xa239

/*********************************************************************
 * Functions.
 */
bStatus_t OTAProfile_AddService(uint32_t services);

#endif /* SRC_OTA_PROFILE_H_ */
