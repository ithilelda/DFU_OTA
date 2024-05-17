#ifndef SIG_SERVICE_H
#define SIG_SERVICE_H


#include "config.h"

/*********************************************************************
 * Service UUIDs.
 */
#define SIG_SERV_UUID                                0xFEE1
// Nordic DFU control point characteristic UUID: 5b1c0001-ba8d-4212-bbdc-6fc023caa68e
#define SIG_CHAR_UUID                                0x8e,0xa6,0xca,0x23,0xc0,0x6f,0xdc,0xbb,0x12,0x42,0x8d,0xba,0x01,0x00,0x1c,0x5b

// public apis.
bStatus_t SIG_AddService();

#endif /* SIG_SERVICE_H */
